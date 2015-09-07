/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * BCM43909 CPU initialization and RTOS ticks
 */

#include "platform_isr.h"
#include "platform_isr_interface.h"
#include "platform_appscr4.h"
#include "platform_config.h"
#include "platform_mcu_peripheral.h"

#include "cr4.h"

#include "wwd_assert.h"
#include "wwd_rtos.h"

#include "wiced_deep_sleep.h"

#include "typedefs.h"
#include "wiced_osl.h"
#include "sbchipc.h"

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef CPU_CLOCK_HZ_EMUL_SLOW_COEFF
#define CPU_CLOCK_HZ_EMUL_SLOW_COEFF            1
#endif

#define CPU_CLOCK_HZ_DEFINE(clock_hz)           ( (clock_hz) / CPU_CLOCK_HZ_EMUL_SLOW_COEFF )
#define CPU_CLOCK_HZ_160MHZ                     CPU_CLOCK_HZ_DEFINE( 160000000 )
#define CPU_CLOCK_HZ_320MHZ                     CPU_CLOCK_HZ_DEFINE( 320000000 )
#define CPU_CLOCK_HZ_DEFAULT                    CPU_CLOCK_HZ_160MHZ

#define CYCLES_PER_TICK(clock_hz)               ( ( ( (clock_hz) + ( SYSTICK_FREQUENCY / 2 ) ) / SYSTICK_FREQUENCY ) )
#define CYCLES_PMU_PER_TICK                     CYCLES_PER_TICK( osl_ilp_clock() )
#define CYCLES_CPU_PER_TICK                     CYCLES_PER_TICK( platform_cpu_clock_get_freq() )
#define CYCLES_TINY_PER_TICK                    CYCLES_PER_TICK( CPU_CLOCK_HZ )
#define CYCLES_PMU_INIT_PER_TICK                CYCLES_PER_TICK( ILP_CLOCK )
#define CYCLES_CPU_INIT_PER_TICK                CYCLES_PER_TICK( CPU_CLOCK_HZ_DEFAULT )

#define PLATFORM_CPU_CLOCK_SLEEPING             PLATFORM_CPU_CLOCK_SLEEPING_NO_REQUEST
#define PLATFORM_CPU_CLOCK_NOT_SLEEPING         PLATFORM_CPU_CLOCK_NOT_SLEEPING_HT_REQUEST

#define TICK_CPU_MAX_COUNTER                    0xFFFFFFFF
#define TICK_PMU_MAX_COUNTER                    0x00FFFFFF

#define TICKS_MAX(max_counter, cycles_per_tick) MIN( (max_counter) / (cycles_per_tick) - 1, 0xFFFFFFFF / (cycles_per_tick) - 2 )

#ifndef PLATFORM_TICK_CPU_TICKLESS_THRESH
#define PLATFORM_TICK_CPU_TICKLESS_THRESH       1
#endif

#ifndef PLATFORM_TICK_PMU_TICKLESS_THRESH
#define PLATFORM_TICK_PMU_TICKLESS_THRESH       5
#endif

#define PLATFORM_PMU_CLKREQ_GROUP_SEL           1

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    PLATFORM_CPU_CLOCK_NOT_SLEEPING_ALP_REQUEST               = 0x0,
    PLATFORM_CPU_CLOCK_NOT_SLEEPING_HT_REQUEST                = 0x1,
    PLATFORM_CPU_CLOCK_NOT_SLEEPING_ALP_REQUEST_HT_AVAILABLE  = 0x2,
    PLATFORM_CPU_CLOCK_NOT_SLEEPING_HT_AVAILABLE              = 0x3,
} platform_cpu_clock_not_sleeping_t;

typedef enum
{
    PLATFORM_CPU_CLOCK_SLEEPING_NO_REQUEST   = 0x0,
    PLATFORM_CPU_CLOCK_SLEEPING_ILP_REQUEST  = 0x1,
    PLATFORM_CPU_CLOCK_SLEEPING_ALP_REQUEST  = 0x2,
    PLATFORM_CPU_CLOCK_SLEEPING_HT_AVAILABLE = 0x3,
} platform_cpu_clock_sleeping_t;

typedef enum
{
    PLATFORM_CPU_CLOCK_SOURCE_BACKPLANE = 0x0, /* CPU runs on backplane clock */
    PLATFORM_CPU_CLOCK_SOURCE_ARM       = 0x4  /* CPU runs on own ARM clock */
} platform_cpu_clock_source_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct platform_tick_timer
{
    void         ( *init_func                )( struct platform_tick_timer* timer );
    uint32_t     ( *freerunning_current_func )( struct platform_tick_timer* timer );
    void         ( *start_func               )( struct platform_tick_timer* timer, uint32_t cycles_till_fire );
    void         ( *reset_func               )( struct platform_tick_timer* timer, uint32_t cycles_till_fire ); /* expect to be called from Timer ISR only when Timer is fired and not ticking anymore */
    void         ( *restart_func             )( struct platform_tick_timer* timer, uint32_t cycles_till_fire );
    void         ( *stop_func                )( struct platform_tick_timer* timer );
    wiced_bool_t ( *sleep_permission_func    )( struct platform_tick_timer* timer, wiced_bool_t powersave_permission );
    void         ( *sleep_invoke_func        )( struct platform_tick_timer* timer, platform_tick_sleep_idle_func idle_func );

    uint32_t     cycles_per_tick;
    uint32_t     cycles_max;
    uint32_t     tickless_thresh;
    uint32_t     freerunning_stamp;
    uint32_t     cycles_till_fire;
    wiced_bool_t started;
} platform_tick_timer_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

#if !PLATFORM_TICK_TINY
static void platform_tick_init( void );
#endif /* !PLATFORM_TICK_TINY */

/******************************************************
 *               Variables Definitions
 ******************************************************/

#if !PLATFORM_TICK_TINY
static platform_tick_timer_t* platform_tick_current_timer  = NULL;
#endif /* !PLATFORM_TICK_TINY */

uint32_t                      platform_cpu_clock_hz        = CPU_CLOCK_HZ_DEFAULT;

static uint32_t               platform_pmu_cycles_per_tick = CYCLES_PMU_INIT_PER_TICK;
static uint32_t               WICED_DEEP_SLEEP_SAVED_VAR( platform_pmu_last_sleep_stamp );
static uint32_t               platform_pmu_last_deep_sleep_stamp;

/******************************************************
 *               Clock Function Definitions
 ******************************************************/

static void
platform_cpu_clock_publish( uint32_t clock )
{
    platform_cpu_clock_hz = clock;

#if !PLATFORM_TICK_TINY
    platform_tick_init();
#endif /* !PLATFORM_TICK_TINY */
}

static void
platform_cpu_wait_ht( void )
{
    uint32_t timeout;
    for ( timeout = 100000000; timeout != 0; timeout-- )
    {
        if ( PLATFORM_CLOCKSTATUS_REG(PLATFORM_APPSCR4_REGBASE(0x0))->bits.bp_on_ht != 0 )
        {
            break;
        }
    }
    wiced_assert( "timed out", timeout != 0 );
}

static platform_cpu_clock_source_t
platform_cpu_core_init( void )
{
    appscr4_core_ctrl_reg_t core_ctrl;

    /* Initialize core control register. */

    core_ctrl.raw = PLATFORM_APPSCR4->core_ctrl.raw;

    core_ctrl.bits.force_clock_source     = 0;
    core_ctrl.bits.wfi_clk_stop           = 1;
    core_ctrl.bits.s_error_int_en         = 1;
    core_ctrl.bits.pclk_dbg_stop          = 0;
    core_ctrl.bits.sleeping_clk_req       = PLATFORM_CPU_CLOCK_SLEEPING;
    core_ctrl.bits.not_sleeping_clk_req_0 = PLATFORM_CPU_CLOCK_NOT_SLEEPING & 0x1;
    core_ctrl.bits.not_sleeping_clk_req_1 = (PLATFORM_CPU_CLOCK_NOT_SLEEPING >> 1) & 0x1;

    PLATFORM_APPSCR4->core_ctrl.raw = core_ctrl.raw;

    /* Initialize ARM clock */
    if ( core_ctrl.bits.clock_source != PLATFORM_CPU_CLOCK_SOURCE_ARM )
    {
        platform_gci_chipcontrol( GCI_CHIPCONTROL_APPS_CPU_FREQ_REG,
                                  GCI_CHIPCONTROL_APPS_CPU_FREQ_MASK,
                                  GCI_CHIPCONTROL_APPS_CPU_FREQ_320);
    }

    /* Core is configured to request HT clock. Let's wait backplane actually run on HT. */
    platform_cpu_wait_ht();

    return core_ctrl.bits.clock_source;
}

static void
platform_cpu_clock_source( platform_cpu_clock_source_t clock_source )
{
    /*
     * Source change actually happen when WFI instruction is executed and _only_ if WFI
     * actually wait for interrups. I.e. if interrupt was pending before WFI, then no
     * switch happen; interrupt must fire after WFI executed.
     *
     * When switching into 2:1 mode (PLATFORM_CPU_CLOCK_SOURCE_ARM) SW must ensure that the HT clock is
     * available and backplane runs on HT.
     * All clock values are valid provided there is always a pair of clocks; one at 2x and one at 1x.
     * Apps CPU needs to downshift to 1:1 (PLATFORM_CPU_CLOCK_SOURCE_BACKPLANE) before going to ALP.
     */

    const uint32_t          clockstable_mask = IRQN2MASK( ClockStable_ExtIRQn );
    uint32_t                irq_mask;
    uint32_t                int_mask;
    appscr4_core_ctrl_reg_t core_ctrl;

    /*
     * XXX
     * This is to avoid possible hardware bug where backplane and ARM clocks may be not synchronized.
     * So when switch to ARM clock, let's also make backplane clock run from same source.
     */
    if ( clock_source == PLATFORM_CPU_CLOCK_SOURCE_ARM )
    {
        platform_gci_chipcontrol( GCI_CHIPCONTROL_BP_CLK_FROM_ARMCR4_CLK_REG,
                                  0x0,
                                  GCI_CHIPCONTROL_BP_CLK_FROM_ARMCR4_CLK_MASK );
    }

    /* Save masks and disable all interrupts */
    irq_mask = PLATFORM_APPSCR4->irq_mask;
    int_mask = PLATFORM_APPSCR4->int_mask;
    PLATFORM_APPSCR4->irq_mask = 0;
    PLATFORM_APPSCR4->int_mask = 0;

    /* Tell hw which source want */
    core_ctrl.raw = PLATFORM_APPSCR4->core_ctrl.raw;
    core_ctrl.bits.clock_source = clock_source;
    PLATFORM_APPSCR4->core_ctrl.raw = core_ctrl.raw;

    /*
     * Ack, unmask ClockStable interrupt and call WFI.
     * We should not have pending interrupts at the moment, so calling should
     * trigger clock switch. At the end of switch CPU will be woken up
     * by ClockStable interrupt.
     */
    PLATFORM_APPSCR4->fiqirq_status = clockstable_mask;
    PLATFORM_APPSCR4->irq_mask = clockstable_mask;
    cpu_wait_for_interrupt();
    wiced_assert("no clockstable irq", PLATFORM_APPSCR4->fiqirq_status & clockstable_mask );
    PLATFORM_APPSCR4->fiqirq_status = clockstable_mask;

    /* Restore interrupt masks */
    PLATFORM_APPSCR4->int_mask = int_mask;
    PLATFORM_APPSCR4->irq_mask = irq_mask;
}

static platform_cpu_clock_source_t
platform_cpu_clock_freq_to_source( platform_cpu_clock_frequency_t freq )
{
    switch ( freq )
    {
        default:
            wiced_assert( "unsupported frequency", 0 );
        case PLATFORM_CPU_CLOCK_FREQUENCY_160_MHZ:
            return PLATFORM_CPU_CLOCK_SOURCE_BACKPLANE;

        case PLATFORM_CPU_CLOCK_FREQUENCY_320_MHZ:
            return PLATFORM_CPU_CLOCK_SOURCE_ARM;
    }
}

static uint32_t
platform_cpu_clock_get_freq( void )
{
    platform_cpu_clock_source_t clock_source = PLATFORM_APPSCR4->core_ctrl.bits.clock_source;
    uint32_t ret;

    switch ( clock_source )
    {
        case PLATFORM_CPU_CLOCK_SOURCE_ARM:
            if ( ( platform_gci_chipcontrol( GCI_CHIPCONTROL_APPS_CPU_FREQ_REG, 0x0, 0x0 ) &
                   GCI_CHIPCONTROL_APPS_CPU_FREQ_MASK ) == GCI_CHIPCONTROL_APPS_CPU_FREQ_320 )
            {
                ret = CPU_CLOCK_HZ_320MHZ;
            }
            else
            {
                ret = CPU_CLOCK_HZ_160MHZ;
            }
            break;

        default:
            wiced_assert( "unknown ratio", 0 );
        case PLATFORM_CPU_CLOCK_SOURCE_BACKPLANE:
            ret = CPU_CLOCK_HZ_160MHZ;
            break;
    }

    return ret;
}

void
platform_cpu_clock_init( platform_cpu_clock_frequency_t freq )
{
    /*
     * This function to be called during system initialization, before RTOS ticks
     * start to run. If there is need to change CPU frequency later, then it can be done
     * using following sequence:
     *    platform_tick_stop();
     *    platform_cpu_clock_init(...);
     *    platform_tick_start();
     * This is to ensure ticks and cycles per ticks are updated correctly.
     */

    platform_cpu_clock_source_t clock_source = platform_cpu_clock_freq_to_source( freq );
    uint32_t flags;

    WICED_SAVE_INTERRUPTS( flags );

    if ( platform_cpu_core_init() != clock_source )
    {
        platform_cpu_clock_source( clock_source );
    }

    platform_cpu_clock_publish( platform_cpu_clock_get_freq() );

    WICED_RESTORE_INTERRUPTS( flags );
}

void
platform_lpo_clock_init( void )
{
    uint32_t cpu_freq = platform_cpu_clock_get_freq();

#if PLATFORM_LPO_CLOCK_EXT
    osl_set_ext_lpoclk( cpu_freq );
#else
    osl_set_int_lpoclk( cpu_freq );
#endif /* PLATFORM_LPO_CLK_EXT */

    platform_cpu_clock_publish( cpu_freq );
}

/******************************************************
 *               PMU Timer Function Definitions
 ******************************************************/

static wiced_bool_t
platform_tick_pmu_slow_write_pending( void )
{
    return (PLATFORM_PMU->pmustatus & PST_SLOW_WR_PENDING) ? WICED_TRUE : WICED_FALSE;
}

static void
platform_tick_pmu_slow_write_barrier( void )
{
    /*
     * Ensure that previous write to slow (ILP) register is completed.
     * Write placed in intermediate buffer and return immediately.
     * Then it takes few slow ILP clocks to move write from intermediate
     * buffer to actual register. If subsequent write happen before previous
     * moved to actual register, then second write may take long time and also
     * occupy bus. This busy loop try to ensure that previous write is completed.
     * It can be used to avoid long writes, and also as synchronization barrier
     * to ensure that previous register update is completed.
     */
    while ( platform_tick_pmu_slow_write_pending() );
}

static void
platform_tick_pmu_slow_write( volatile uint32_t* reg, uint32_t val )
{
    platform_tick_pmu_slow_write_barrier();
    *reg = val;
}

static void
platform_tick_pmu_slow_write_sync( volatile uint32_t* reg, uint32_t val )
{
    platform_tick_pmu_slow_write( reg, val );
    platform_tick_pmu_slow_write_barrier();
}

static uint32_t
platform_tick_pmu_get_freerunning_stamp( void )
{
    uint32_t time = PLATFORM_PMU->pmutimer;

    if ( time != PLATFORM_PMU->pmutimer )
    {
        time = PLATFORM_PMU->pmutimer;
    }

    return time;
}

#if PLATFORM_TICK_PMU

static pmu_res_req_timer_t
platform_tick_pmu_timer_read( void )
{
    pmu_res_req_timer_t ret = PLATFORM_PMU->res_req_timer1;

    if ( ret.raw != PLATFORM_PMU->res_req_timer1.raw )
    {
        ret = PLATFORM_PMU->res_req_timer1;
    }

    return ret;
}

static pmu_intstatus_t
platform_tick_pmu_intstatus_init( void )
{
    pmu_intstatus_t status;

    status.raw                      = 0;
    status.bits.rsrc_req_timer_int1 = 1;

    return status;
}

static pmu_res_req_timer_t
platform_tick_pmu_timer_init( uint32_t ticks )
{
    pmu_res_req_timer_t timer;

    timer.raw                   = 0x0;
    timer.bits.time             = ( ticks & TICK_PMU_MAX_COUNTER );
    timer.bits.int_enable       = 1;
    timer.bits.force_ht_request = 1;
    timer.bits.clkreq_group_sel = PLATFORM_PMU_CLKREQ_GROUP_SEL;

    return timer;
}

static wiced_bool_t
platform_tick_pmu_idle( void )
{
    /*
     * Synchronous write to PMU timer is slow.
     * This function return true if PMU timer is surely idle.
     * It may be used to avoid unnecessary stopping.
     */

    if ( platform_tick_pmu_slow_write_pending() )
    {
        return WICED_FALSE;
    }

    if ( platform_tick_pmu_timer_read().bits.time != 0 )
    {
        return WICED_FALSE;
    }

    return WICED_TRUE;
}

static void
platform_tick_pmu_init( platform_tick_timer_t* timer )
{
    timer->cycles_per_tick = CYCLES_PMU_PER_TICK;
    timer->cycles_max      = TICKS_MAX( TICK_PMU_MAX_COUNTER, timer->cycles_per_tick );
}

static uint32_t
platform_tick_pmu_freerunning_current( platform_tick_timer_t* timer )
{
    UNUSED_PARAMETER( timer );
    return platform_tick_pmu_get_freerunning_stamp();
}

static void
platform_tick_pmu_start( platform_tick_timer_t* timer, uint32_t cycles_till_fire )
{
    uint32_t mask = platform_tick_pmu_intstatus_init().raw;

    PLATFORM_PMU->pmuintstatus.raw = mask;
    platform_tick_pmu_slow_write( &PLATFORM_PMU->res_req_timer1.raw, platform_tick_pmu_timer_init( cycles_till_fire ).raw );

    platform_common_chipcontrol( &PLATFORM_PMU->pmuintmask1.raw, 0x0, mask );

    timer->started = WICED_TRUE;
}

static void
platform_tick_pmu_reset( platform_tick_timer_t* timer, uint32_t cycles_till_fire )
{
    UNUSED_PARAMETER( timer );

    PLATFORM_PMU->pmuintstatus.raw = platform_tick_pmu_intstatus_init().raw;
    platform_tick_pmu_slow_write( &PLATFORM_PMU->res_req_timer1.raw, platform_tick_pmu_timer_init( cycles_till_fire ).raw );
}

static void
platform_tick_pmu_restart( platform_tick_timer_t* timer, uint32_t cycles_till_fire )
{
    UNUSED_PARAMETER( timer );

    if ( !platform_tick_pmu_idle() )
    {
        platform_tick_pmu_slow_write_sync( &PLATFORM_PMU->res_req_timer1.raw, platform_tick_pmu_timer_init( 0x0 ).raw );
    }
    PLATFORM_PMU->pmuintstatus.raw = platform_tick_pmu_intstatus_init().raw;
    platform_tick_pmu_slow_write( &PLATFORM_PMU->res_req_timer1.raw, platform_tick_pmu_timer_init( cycles_till_fire ).raw );
}

static void
platform_tick_pmu_stop( platform_tick_timer_t* timer )
{
    uint32_t mask = platform_tick_pmu_intstatus_init().raw;

    timer->started = WICED_FALSE;

    platform_common_chipcontrol( &PLATFORM_PMU->pmuintmask1.raw, mask, 0x0 );

    platform_tick_pmu_slow_write_sync( &PLATFORM_PMU->res_req_timer1.raw, 0x0 );
    PLATFORM_PMU->pmuintstatus.raw = mask;
}

static wiced_bool_t
platform_tick_pmu_sleep_permission( platform_tick_timer_t* timer, wiced_bool_t powersave_permission )
{
    /*
     * In this mode we potentially can put overall system into sleep when execute WFI.
     * Powersave disabling completely switch off sleeping.
     */

    UNUSED_PARAMETER( timer );

    return powersave_permission;
}

static void
platform_tick_pmu_sleep_invoke( platform_tick_timer_t* timer, platform_tick_sleep_idle_func idle_func )
{
    /*
     * When CPU runs on ARM clock, it also request HT clock.
     * So to drop HT clock we must to switch back to backplane clock.
     */

    const wiced_bool_t switch_to_bp = ( PLATFORM_APPSCR4->core_ctrl.bits.clock_source == PLATFORM_CPU_CLOCK_SOURCE_ARM );

    UNUSED_PARAMETER( timer );

    if ( switch_to_bp )
    {
        platform_cpu_clock_source( PLATFORM_CPU_CLOCK_SOURCE_BACKPLANE );
    }

    idle_func();

    if ( switch_to_bp )
    {
        platform_cpu_clock_source( PLATFORM_CPU_CLOCK_SOURCE_ARM );
    }
}

static platform_tick_timer_t platform_tick_pmu_timer =
{
    .init_func                = platform_tick_pmu_init,
    .freerunning_current_func = platform_tick_pmu_freerunning_current,
    .start_func               = platform_tick_pmu_start,
    .reset_func               = platform_tick_pmu_reset,
    .restart_func             = platform_tick_pmu_restart,
    .stop_func                = platform_tick_pmu_stop,
    .sleep_permission_func    = platform_tick_pmu_sleep_permission,
    .sleep_invoke_func        = platform_tick_pmu_sleep_invoke,
    .cycles_per_tick          = CYCLES_PMU_INIT_PER_TICK,
    .cycles_max               = TICKS_MAX( TICK_PMU_MAX_COUNTER, CYCLES_PMU_INIT_PER_TICK ),
    .tickless_thresh          = PLATFORM_TICK_PMU_TICKLESS_THRESH,
    .freerunning_stamp        = 0,
    .cycles_till_fire         = 0,
    .started                  = WICED_FALSE
};

#endif /* PLATFORM_TICK_PMU */

/******************************************************
 *               CPU Timer Function Definitions
 ******************************************************/

static inline void
platform_tick_cpu_write_sync( volatile uint32_t* reg, uint32_t val )
{
    *reg = val;
    (void)*reg; /* read back to ensure write completed */
}

static inline void
platform_tick_cpu_start_base( uint32_t cycles_per_tick )
{
    PLATFORM_APPSCR4->int_status = IRQN2MASK( Timer_IRQn );
    PLATFORM_APPSCR4->int_timer  = cycles_per_tick;

    platform_irq_enable_int( Timer_IRQn );

    PLATFORM_CLOCKSTATUS_REG(PLATFORM_APPSCR4_REGBASE(0x0))->bits.force_ht_request = 1; /* keep HT running while CPU is sleeping */
}

static inline void
platform_tick_cpu_reset_base( uint32_t cycles_till_fire )
{
    PLATFORM_APPSCR4->int_status = IRQN2MASK( Timer_IRQn );
    PLATFORM_APPSCR4->int_timer  = cycles_till_fire;
}

static inline void
platform_tick_cpu_stop_base( void )
{
    PLATFORM_CLOCKSTATUS_REG(PLATFORM_APPSCR4_REGBASE(0x0))->bits.force_ht_request = 0; /* no need to keep anymore */

    platform_irq_disable_int( Timer_IRQn );

    platform_tick_cpu_write_sync( &PLATFORM_APPSCR4->int_timer, 0 );
    PLATFORM_APPSCR4->int_status = IRQN2MASK( Timer_IRQn );
}

#if PLATFORM_TICK_CPU

static void
platform_tick_cpu_init( platform_tick_timer_t* timer )
{
    timer->cycles_per_tick = CYCLES_CPU_PER_TICK;
    timer->cycles_max      = TICKS_MAX( TICK_CPU_MAX_COUNTER, timer->cycles_per_tick );
}

static uint32_t
platform_tick_cpu_freerunning_current( platform_tick_timer_t* timer )
{
    UNUSED_PARAMETER( timer );

    return PLATFORM_APPSCR4->cycle_cnt;
}

static void
platform_tick_cpu_start( platform_tick_timer_t* timer, uint32_t cycles_till_fire )
{
    platform_tick_cpu_start_base( cycles_till_fire );

    timer->started = WICED_TRUE;
}

static void
platform_tick_cpu_reset( platform_tick_timer_t* timer, uint32_t cycles_till_fire )
{
    UNUSED_PARAMETER( timer );

    platform_tick_cpu_reset_base( cycles_till_fire );
}

static void
platform_tick_cpu_restart( platform_tick_timer_t* timer, uint32_t cycles_till_fire )
{
    UNUSED_PARAMETER( timer );

    platform_tick_cpu_write_sync( &PLATFORM_APPSCR4->int_timer, 0 );
    PLATFORM_APPSCR4->int_status = IRQN2MASK( Timer_IRQn );
    PLATFORM_APPSCR4->int_timer  = cycles_till_fire;
}

static void
platform_tick_cpu_stop( platform_tick_timer_t* timer )
{
    timer->started = WICED_FALSE;

    platform_tick_cpu_stop_base();
}

static wiced_bool_t
platform_tick_cpu_sleep_permission( platform_tick_timer_t* timer, wiced_bool_t powersave_permission )
{
    /*
     * Even if powersaving disabled it is safe to call WFI instruction and reduce current draw when idle.
     * In this mode we do not put overall system in sleep, just stop CPU till interrupt come.
     *
     * Powersave disabling just disables tick-less mode and so increase timer accuracy.
     *
     */

    UNUSED_PARAMETER( timer );
    UNUSED_PARAMETER( powersave_permission );

    return WICED_TRUE;
}

static void
platform_tick_cpu_sleep_invoke( platform_tick_timer_t* timer, platform_tick_sleep_idle_func idle_func )
{
    UNUSED_PARAMETER( timer );

    /*
     * When CPU timer used for ticks we always keep HT clock running.
     * No need to drop request to HT clock.
     */

    idle_func();
}

static platform_tick_timer_t platform_tick_cpu_timer =
{
    .init_func                = platform_tick_cpu_init,
    .freerunning_current_func = platform_tick_cpu_freerunning_current,
    .start_func               = platform_tick_cpu_start,
    .reset_func               = platform_tick_cpu_reset,
    .restart_func             = platform_tick_cpu_restart,
    .stop_func                = platform_tick_cpu_stop,
    .sleep_permission_func    = platform_tick_cpu_sleep_permission,
    .sleep_invoke_func        = platform_tick_cpu_sleep_invoke,
    .cycles_per_tick          = CYCLES_CPU_INIT_PER_TICK,
    .cycles_max               = TICKS_MAX( TICK_CPU_MAX_COUNTER, CYCLES_CPU_INIT_PER_TICK ),
    .tickless_thresh          = PLATFORM_TICK_CPU_TICKLESS_THRESH,
    .freerunning_stamp        = 0,
    .cycles_till_fire         = 0,
    .started                  = WICED_FALSE
};

#endif /* PLATFORM_TICK_CPU */

/******************************************************
 *               Timer API Function Definitions
 ******************************************************/

#if PLATFORM_TICK_TINY

static uint32_t
platform_tick_cycles_per_tick( void )
{
    return CYCLES_TINY_PER_TICK;
}

void
platform_tick_start( void )
{
    platform_tick_cpu_start_base( platform_tick_cycles_per_tick() );
}

void
platform_tick_stop( void )
{
    platform_tick_cpu_stop_base();
}

wiced_bool_t
platform_tick_irq_handler( void )
{
    platform_tick_cpu_reset_base( platform_tick_cycles_per_tick() );
    return WICED_TRUE;
}

#else

static wiced_bool_t
platform_tick_any_interrupt_pending( void )
{
    return ( ( PLATFORM_APPSCR4->irq_mask & PLATFORM_APPSCR4->fiqirq_status ) != 0 );
}

static void
platform_tick_init( void )
{
    platform_pmu_cycles_per_tick = CYCLES_PMU_PER_TICK;

#if PLATFORM_TICK_CPU
    platform_tick_cpu_timer.init_func( &platform_tick_cpu_timer );
#endif /* PLATFORM_TICK_CPU */

#if PLATFORM_TICK_PMU
    platform_tick_pmu_timer.init_func( &platform_tick_pmu_timer );
#endif /* PLATFORM_TICK_PMU */

    if ( platform_tick_current_timer == NULL )
    {
#if PLATFORM_TICK_CPU
        /* Use CPU clock as first choice as handling incur less overhead and results are more precise. */
        platform_tick_current_timer = &platform_tick_cpu_timer;
#elif PLATFORM_TICK_PMU
        platform_tick_current_timer = &platform_tick_pmu_timer;
#endif /* PLATFORM_TICK_CPU */
    }
}

#if PLATFORM_TICK_CPU && PLATFORM_TICK_PMU

static void
platform_tick_switch_timer( platform_tick_timer_t* timer )
{
    platform_tick_timer_t* timer_curr = platform_tick_current_timer;

    wiced_assert( "timers must be non-NULL", timer && timer_curr );

    if ( ( timer != timer_curr ) && timer_curr->started )
    {
        const float    convert_coeff = (float)timer->cycles_per_tick / (float)timer_curr->cycles_per_tick;
        const uint32_t restart_curr  = timer_curr->cycles_till_fire;
        const uint32_t passed_curr   = timer_curr->freerunning_current_func( timer_curr ) - timer_curr->freerunning_stamp;
        const uint32_t restart_new   = MIN( MAX( ( uint32_t )( ( float )restart_curr * convert_coeff + 0.5f ), 1 ), timer->cycles_per_tick );
        const uint32_t passed_new    = MIN( ( uint32_t )( ( float )passed_curr * convert_coeff + 0.5f ), restart_new - 1 );

        timer->freerunning_stamp = timer->freerunning_current_func( timer ) - passed_new;
        timer->cycles_till_fire  = restart_new;

        timer_curr->stop_func( timer_curr );

        timer->start_func( timer, restart_new - passed_new );
    }

    platform_tick_current_timer = timer;
}

void platform_tick_execute_command( platform_tick_command_t command )
{
    static wiced_bool_t powersave_enabled  = WICED_FALSE;
    static wiced_bool_t pmu_timer_released = WICED_FALSE;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS( flags );

    switch ( command )
    {
        case PLATFORM_TICK_COMMAND_POWERSAVE_BEGIN:
            powersave_enabled = WICED_TRUE;
            break;

        case PLATFORM_TICK_COMMAND_POWERSAVE_END:
            powersave_enabled = WICED_FALSE;
            break;

        case PLATFORM_TICK_COMMAND_RELEASE_PMU_TIMER_BEGIN:
            pmu_timer_released = WICED_TRUE;
            break;

        case PLATFORM_TICK_COMMAND_RELEASE_PMU_TIMER_END:
            pmu_timer_released = WICED_FALSE;
            break;

        default:
            wiced_assert( "unknown event", 0 );
            break;
    }

    if ( powersave_enabled && !pmu_timer_released )
    {
        platform_tick_switch_timer( &platform_tick_pmu_timer );
    }
    else
    {
        platform_tick_switch_timer( &platform_tick_cpu_timer );
    }

    WICED_RESTORE_INTERRUPTS( flags );
}

#define PLATFORM_TICK_EXECUTE_COMMAND_IMPLEMENTED

#endif /* PLATFORM_TICK_CPU && PLATFORM_TICK_PMU */

static void
platform_tick_restart( platform_tick_timer_t* timer, uint32_t cycles_till_fire )
{
    timer->freerunning_stamp = timer->freerunning_current_func( timer );
    timer->cycles_till_fire  = cycles_till_fire;

    timer->restart_func( timer, cycles_till_fire );
}

void
platform_tick_start( void )
{
    platform_tick_timer_t* timer = platform_tick_current_timer;

    wiced_assert( "no current timer set", timer );

    timer->freerunning_stamp = timer->freerunning_current_func( timer );
    timer->cycles_till_fire  = timer->cycles_per_tick;

    timer->start_func( timer, timer->cycles_per_tick );
}

void
platform_tick_stop( void )
{
    platform_tick_timer_t* timer = platform_tick_current_timer;

    wiced_assert( "no current timer set", timer );

    timer->stop_func( timer );
}

static void
platform_tick_reset( platform_tick_timer_t* timer )
{
    const uint32_t         current          = timer->freerunning_current_func( timer );
    const uint32_t         passed           = current - timer->freerunning_stamp;
    const uint32_t         next_sched       = timer->cycles_till_fire + timer->cycles_per_tick;
    const uint32_t         cycles_till_fire = ( passed < next_sched ) ? ( next_sched - passed ) : 1;

    timer->freerunning_stamp = current;
    timer->cycles_till_fire  = cycles_till_fire;

    timer->reset_func( timer, cycles_till_fire );
}

wiced_bool_t
platform_tick_irq_handler( void )
{
    wiced_bool_t res = WICED_FALSE;

#if PLATFORM_TICK_PMU
    if ( PLATFORM_PMU->pmuintstatus.bits.rsrc_req_timer_int1 && PLATFORM_PMU->pmuintmask1.bits.rsrc_req_timer_int1 )
    {
        wiced_assert( "not current timer", platform_tick_current_timer == &platform_tick_pmu_timer );
        platform_tick_reset( &platform_tick_pmu_timer );
        res = WICED_TRUE;
    }
#endif /* PLATFORM_TICK_PMU */

#if PLATFORM_TICK_CPU
    if ( PLATFORM_APPSCR4->int_status & PLATFORM_APPSCR4->int_mask & IRQN2MASK( Timer_IRQn ) )
    {
        wiced_assert( "not current timer", platform_tick_current_timer == &platform_tick_cpu_timer );
        platform_tick_reset( &platform_tick_cpu_timer );
        res = WICED_TRUE;
    }
#endif /* PLATFORM_TICK_CPU */

#if PLATFORM_WLAN_POWERSAVE
    if ( PLATFORM_PMU->pmuintstatus.bits.rsrc_event_int1 && PLATFORM_PMU->pmuintmask1.bits.rsrc_event_int1 )
    {
        platform_wlan_powersave_res_event();
    }
#endif /* PLATFORM_WLAN_POWERSAVE */

    return res;
}

uint32_t
platform_tick_sleep( platform_tick_sleep_idle_func idle_func, uint32_t ticks, wiced_bool_t powersave_permission )
{
    /* Function expect to be called with interrupts disabled. */

    platform_tick_timer_t* timer           = platform_tick_current_timer;
    const wiced_bool_t     sleep           = timer->sleep_permission_func( timer, powersave_permission );
    const wiced_bool_t     tickless        = ( ticks >= timer->tickless_thresh ) && sleep && powersave_permission;
    const uint32_t         cycles_per_tick = timer->cycles_per_tick;
    uint32_t               ret             = 0;

    /* If any interrupts pending better to return now to have more accurate timings. */
    if ( platform_tick_any_interrupt_pending() )
    {
        return 0;
    }

    /* Reconfigure timer if tickless mode. */
    if ( tickless )
    {
        const uint32_t old_restart = timer->cycles_till_fire;
        const uint32_t requested   = cycles_per_tick * MIN( ticks, timer->cycles_max );
        const uint32_t passed      = timer->freerunning_current_func( timer ) - timer->freerunning_stamp;
        uint32_t       new_restart;

        if ( requested + old_restart > passed )
        {
            new_restart = requested + old_restart - passed;
        }
        else
        {
            new_restart = cycles_per_tick - ( ( passed - requested - old_restart ) % cycles_per_tick );
        }

        if ( passed >= old_restart )
        {
            ret = 1 + (passed - old_restart) / cycles_per_tick;
        }

        platform_tick_restart( timer, new_restart );
    }

    /* Here CPU is going to enter sleep mode. */
    if ( sleep )
    {
        platform_pmu_last_sleep_stamp = platform_tick_pmu_get_freerunning_stamp();
        timer->sleep_invoke_func( timer, idle_func );
    }

    /* Reconfigure timer back. */
    if ( tickless )
    {
        const uint32_t old_restart = timer->cycles_till_fire;
        const uint32_t passed      = timer->freerunning_current_func( timer ) - timer->freerunning_stamp;
        uint32_t       new_restart;

        ret += ( old_restart + cycles_per_tick - 1 ) / cycles_per_tick;
        if ( passed <= old_restart )
        {
            const uint32_t remain  = old_restart - passed;
            ret                   -= ( remain + cycles_per_tick - 1 ) / cycles_per_tick;
            new_restart            = remain % cycles_per_tick;
        }
        else
        {
            const uint32_t wrapped        = passed - old_restart;
            const uint32_t wrapped_ticks  = wrapped / cycles_per_tick;
            const uint32_t wrapped_rem    = wrapped - wrapped_ticks * cycles_per_tick;
            ret                          += wrapped_ticks;
            new_restart                   = cycles_per_tick - wrapped_rem;
        }

        platform_tick_restart( timer, new_restart ? new_restart : cycles_per_tick );
    }

    return ret;
}

#endif /* PLATFORM_TICK_TINY */

#ifndef PLATFORM_TICK_EXECUTE_COMMAND_IMPLEMENTED

void platform_tick_execute_command( platform_tick_command_t command )
{
    UNUSED_PARAMETER( command );
}

#endif /* PLATFORM_TICK_EXECUTE_COMMAND_IMPLEMENTED */

void
platform_tick_irq_init( void )
{
    platform_pmu_last_deep_sleep_stamp = platform_pmu_last_sleep_stamp;

    if ( WICED_DEEP_SLEEP_IS_WARMBOOT( ) )
    {
        /* Cleanup interrupts state which can survived warm reboot. */
        platform_tick_pmu_slow_write_sync( &PLATFORM_PMU->res_req_timer1.raw, 0x0 );
        PLATFORM_PMU->pmuintmask1.raw  = 0x0;
        PLATFORM_PMU->pmuintstatus.raw = PLATFORM_PMU->pmuintstatus.raw;
    }
    else
    {
        pmu_intctrl_t intctrl;

        /* Make sure that when PMU interrupt triggered it is requested correct domain clocks. */
        intctrl.raw                   = 0x0;
        intctrl.bits.clkreq_group_sel = PLATFORM_PMU_CLKREQ_GROUP_SEL;
        PLATFORM_PMU->pmuintctrl1.raw = intctrl.raw;
    }

    /*
     * Let's have APPSCR4 timer and PMU interrupts (timer1 and event1) share same OOB line and be delivered to same Timer_ExtIRQn interrupt status bit.
     *
     * First, let's make APPSCR4 timer output irq requests to same bus line as PMU timer1.
     */
    platform_irq_remap_source( PLATFORM_APPSCR4_MASTER_WRAPPER_REGBASE(0x0),
                               OOB_APPSCR4_TIMER_IRQ_NUM,
                               OOB_AOUT_PMU_INTR1 );
    /* Second, let's route this bus line to Timer_ExtIRQn bit. */
    platform_irq_remap_sink( OOB_AOUT_PMU_INTR1, Timer_ExtIRQn );
    /* Third, enable this line. */
    platform_irq_enable_irq( Timer_ExtIRQn );
}

void platform_tick_number_since_last_sleep( uint32_t *since_last_sleep, uint32_t *since_last_deep_sleep )
{
    uint32_t now = platform_tick_pmu_get_freerunning_stamp();

    if ( since_last_sleep )
    {
        *since_last_sleep = ( now - platform_pmu_last_sleep_stamp ) / platform_pmu_cycles_per_tick;
    }

    if ( since_last_deep_sleep )
    {
        *since_last_deep_sleep = ( now - platform_pmu_last_deep_sleep_stamp ) / platform_pmu_cycles_per_tick;
    }
}
