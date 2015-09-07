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
 *  MCU powersave implementation
 */

#include "platform_peripheral.h"
#include "platform_appscr4.h"
#include "platform_config.h"
#include "wwd_assert.h"
#include "wwd_rtos.h"
#include "cr4.h"

#include "typedefs.h"
#include "wiced_osl.h"
#include "sbchipc.h"

#include "wiced_deep_sleep.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define POWERSAVE_MOVING_AVERAGE_PARAM 128
#if PLATFORM_TICK_STATS
#define POWERSAVE_MOVING_AVERAGE_CALC( var, add ) \
    do \
    { \
        (var) *= POWERSAVE_MOVING_AVERAGE_PARAM - 1; \
        (var) += (add); \
        (var) /= POWERSAVE_MOVING_AVERAGE_PARAM; \
    } \
    while ( 0 )
#else
#define POWERSAVE_MOVING_AVERAGE_CALC( var, add )
#endif /* PLATFORM_TICK_STATS */

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

static wiced_bool_t          powersave_is_warmboot          = WICED_FALSE;

#ifndef WICED_DISABLE_MCU_POWERSAVE

static int                   powersave_disable_counter      = 1; /* init state - disabled */

#if PLATFORM_TICK_STATS
/* Statistic variables */
float                        powersave_stat_avg_sleep_ticks = 0;
float                        powersave_stat_avg_run_ticks   = 0;
float                        powersave_stat_avg_load        = 0;
float                        powersave_stat_avg_ticks_req   = 0;
float                        powersave_stat_avg_ticks_adj   = 0;
uint32_t                     powersave_stat_call_number     = 0;
#endif /* PLATFORM_TICK_STATS */

#endif /* !WICED_DISABLE_MCU_POWERSAVE */

#if PLATFORM_WLAN_POWERSAVE
static int                   powersave_wlan_res_counter     = 0;
static host_semaphore_type_t powersave_wlan_res_event;
#endif /* PLATFORM_WLAN_POWERSAVE */

/******************************************************
 *               Function Definitions
 ******************************************************/

inline static wiced_bool_t
platform_mcu_powersave_permission( void )
{
#ifdef WICED_DISABLE_MCU_POWERSAVE
    return WICED_FALSE;
#else
    return ( powersave_disable_counter == 0 ) ? WICED_TRUE : WICED_FALSE;
#endif
}

static void
platform_mcu_powersave_fire_event( void )
{
    const wiced_bool_t            powersave = platform_mcu_powersave_permission();
    const platform_tick_command_t command   = powersave ? PLATFORM_TICK_COMMAND_POWERSAVE_BEGIN : PLATFORM_TICK_COMMAND_POWERSAVE_END;

    platform_tick_execute_command( command );
}

static platform_result_t
platform_mcu_powersave_add_disable_counter( int add )
{
#ifdef WICED_DISABLE_MCU_POWERSAVE
    return PLATFORM_FEATURE_DISABLED;
#else
    uint32_t flags;

    WICED_SAVE_INTERRUPTS( flags );

    powersave_disable_counter += add;
    wiced_assert("unbalanced powersave calls", powersave_disable_counter >= 0);

    platform_mcu_powersave_fire_event();

    WICED_RESTORE_INTERRUPTS( flags );

    return PLATFORM_SUCCESS;
#endif /* WICED_DISABLE_MCU_POWERSAVE */
}

platform_result_t
platform_mcu_powersave_init( void )
{
    platform_result_t res = PLATFORM_FEATURE_DISABLED;

#ifndef WICED_DISABLE_MCU_POWERSAVE
    /* Define resource mask used to wake up application domain. */
    PLATFORM_PMU->res_req_mask1 = PMU_RES_APPS_UP_MASK;

    /* For deep sleep current measuring. */
    platform_pmu_regulatorcontrol( PMU_REGULATOR_LPLDO1_REG,
                                   PMU_REGULATOR_LPLDO1_MASK,
                                   PMU_REGULATOR_LPLDO1_1_0_V );

    /* Force app always-on memory on. */
    platform_pmu_chipcontrol( PMU_CHIPCONTROL_APP_VDDM_POWER_FORCE_REG,
                              PMU_CHIPCONTROL_APP_VDDM_POWER_FORCE_MASK,
                              PMU_CHIPCONTROL_APP_VDDM_POWER_FORCE_EN );

    /*
     * Set deep-sleep flag.
     * It is reserved for software.
     * Does not trigger any hardware reaction.
     * Used by software during warm boot to know whether it should go normal boot path or warm boot.
     */
    platform_gci_chipcontrol( GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_REG,
                              GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_MASK,
                              GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_SET );

    /* Clear status fields which may affect warm-boot and which are cleared by power-on-reset only. */
    PLATFORM_APPSCR4->core_status.raw = PLATFORM_APPSCR4->core_status.raw;

    res = PLATFORM_SUCCESS;
#endif /* !WICED_DISABLE_MCU_POWERSAVE */

#if PLATFORM_WLAN_POWERSAVE
    host_rtos_init_semaphore( &powersave_wlan_res_event );
    PLATFORM_PMU->res_event1 = PMU_RES_WLAN_UP_MASK;
#endif /* PLATFORM_WLAN_POWERSAVE */

    platform_mcu_powersave_fire_event();

    return res;
}

void
platform_mcu_powersave_set_mode( platform_mcu_powersave_mode_t mode )
{
#ifdef WICED_DISABLE_MCU_POWERSAVE
    UNUSED_PARAMETER( mode );
#else
    uint32_t mask = PLATFORM_PMU->max_res_mask;

    switch ( mode )
    {
        case MCU_POWERSAVE_DEEP_SLEEP:
            mask = PMU_RES_DEEP_SLEEP_MASK;
            break;

        case MCU_POWERSAVE_SLEEP:
            mask = PMU_RES_SLEEP_MASK;
            break;

        default:
            wiced_assert( "Bad mode", 0 );
            break;
    }

    PLATFORM_PMU->min_res_mask = mask;
#endif /* WICED_DISABLE_MCU_POWERSAVE */
}

void
platform_mcu_powersave_warmboot_init( void )
{
    appscr4_core_status_reg_t status = { .raw =  PLATFORM_APPSCR4->core_status.raw };

    powersave_is_warmboot = WICED_FALSE;

    if ( status.bits.s_error_log || status.bits.s_bp_reset_log )
    {
        /* Board was resetted. */
    }
    else if ( ( platform_gci_chipcontrol( GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_REG,
                                          0x0,
                                          0x0) & GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_MASK ) != GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_SET )
    {
        /* Previously booted software does not want us to run warm reboot sequence. */
    }
    else
    {
        powersave_is_warmboot = WICED_TRUE;
    }
}

wiced_bool_t
platform_mcu_powersave_is_warmboot( void )
{
    return powersave_is_warmboot;
}

platform_result_t
platform_mcu_powersave_disable( void )
{
    return platform_mcu_powersave_add_disable_counter( 1 );
}

platform_result_t
platform_mcu_powersave_enable( void )
{
    return platform_mcu_powersave_add_disable_counter( -1 );
}

/******************************************************
 *               RTOS Powersave Hooks
 ******************************************************/

#ifndef WICED_DISABLE_MCU_POWERSAVE

inline static uint32_t
platform_mcu_appscr4_stamp( void )
{
    return PLATFORM_APPSCR4->cycle_cnt;
}

inline static uint32_t
platform_mcu_pmu_stamp( void )
{
    uint32_t time = PLATFORM_PMU->pmutimer;

    if ( time != PLATFORM_PMU->pmutimer )
    {
        time = PLATFORM_PMU->pmutimer;
    }

    return CPU_CLOCK_HZ / osl_ilp_clock() * time;
}

static void
platform_mcu_suspend( void )
{
    wiced_bool_t permission = platform_mcu_powersave_permission();

    WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS( permission, WICED_DEEP_SLEEP_EVENT_ENTER );

#if PLATFORM_TICK_STATS

    static uint32_t begin_stamp_appscr4, end_stamp_appscr4;
#if PLATFORM_TICK_PMU
    uint32_t begin_stamp_pmu = platform_mcu_pmu_stamp();
#endif
    uint32_t sleep_ticks;
    float total_ticks;

    begin_stamp_appscr4 = platform_mcu_appscr4_stamp();

    if ( end_stamp_appscr4 != 0 )
    {
        POWERSAVE_MOVING_AVERAGE_CALC( powersave_stat_avg_run_ticks, begin_stamp_appscr4 - end_stamp_appscr4 );
    }

    cpu_wait_for_interrupt();

    end_stamp_appscr4 = platform_mcu_appscr4_stamp();

#if PLATFORM_TICK_PMU
    sleep_ticks = platform_mcu_pmu_stamp() - begin_stamp_pmu;
#else
    sleep_ticks = end_stamp_appscr4 - begin_stamp_appscr4;
#endif
    POWERSAVE_MOVING_AVERAGE_CALC( powersave_stat_avg_sleep_ticks, sleep_ticks );

    total_ticks             = powersave_stat_avg_run_ticks + powersave_stat_avg_sleep_ticks;
    powersave_stat_avg_load = ( total_ticks > 0.0 ) ? ( powersave_stat_avg_run_ticks / total_ticks ) : 0.0;

    powersave_stat_call_number++;

#else

    cpu_wait_for_interrupt();

#endif /* PLATFORM_TICK_STATS */

    WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS( permission, WICED_DEEP_SLEEP_EVENT_CANCEL );
}

#endif /* !WICED_DISABLE_MCU_POWERSAVE */

/* Expect to be called with interrupts disabled */
void platform_idle_hook( void )
{
#ifndef WICED_DISABLE_MCU_POWERSAVE
    (void)platform_tick_sleep( platform_mcu_suspend, 0, WICED_TRUE );
#endif
}

/* Expect to be called with interrupts disabled */
uint32_t platform_power_down_hook( uint32_t ticks )
{
#ifdef WICED_DISABLE_MCU_POWERSAVE
    return 0;
#else
    uint32_t ret = 0;

    if ( platform_mcu_powersave_permission() )
    {
        ret = platform_tick_sleep( platform_mcu_suspend, ticks, WICED_TRUE );

        POWERSAVE_MOVING_AVERAGE_CALC( powersave_stat_avg_ticks_req, ticks );
        POWERSAVE_MOVING_AVERAGE_CALC( powersave_stat_avg_ticks_adj, ret );
    }

    return ret;
#endif /* WICED_DISABLE_MCU_POWERSAVE */
}

/* Expect to be called with interrupts disabled */
int platform_power_down_permission( void )
{
#ifdef WICED_DISABLE_MCU_POWERSAVE
    return 0;
#else
    wiced_bool_t permission = platform_mcu_powersave_permission();

    if ( !permission )
    {
        (void)platform_tick_sleep( platform_mcu_suspend, 0, WICED_FALSE );
    }

    return permission;
#endif /* WICED_DISABLE_MCU_POWERSAVE */
}

#if PLATFORM_WLAN_POWERSAVE

static void platform_wlan_powersave_pmu_timer_slow_write( uint32_t val )
{
    while ( PLATFORM_PMU->pmustatus & PST_SLOW_WR_PENDING );

    PLATFORM_PMU->res_req_timer1.raw = val;

    while ( PLATFORM_PMU->pmustatus & PST_SLOW_WR_PENDING );
}

static void platform_wlan_powersave_res_ack_event( void )
{
    pmu_intstatus_t status;

    status.raw                  = 0;
    status.bits.rsrc_event_int1 = 1;

    PLATFORM_PMU->pmuintstatus.raw = status.raw;
}

static void platform_wlan_powersave_res_mask_event( wiced_bool_t enable )
{
    pmu_intstatus_t mask;

    mask.raw                  = 0;
    mask.bits.rsrc_event_int1 = 1;

    if ( enable )
    {
        platform_common_chipcontrol( &PLATFORM_PMU->pmuintmask1.raw, 0x0, mask.raw );
    }
    else
    {
        platform_common_chipcontrol( &PLATFORM_PMU->pmuintmask1.raw, mask.raw, 0x0 );
    }
}

static void platform_wlan_powersave_res_wait_event( void )
{
    while ( ( PLATFORM_PMU->res_state & PMU_RES_WLAN_UP_MASK ) != PMU_RES_WLAN_UP_MASK )
    {
        if ( ( ( PLATFORM_PMU->res_state | PMU_RES_WLAN_FAST_UP_MASK ) & PMU_RES_WLAN_UP_MASK ) == PMU_RES_WLAN_UP_MASK )
        {
            /* Nearly done. Switch to polling. */
            continue;
        }

        platform_wlan_powersave_res_mask_event( WICED_TRUE );

        host_rtos_get_semaphore( &powersave_wlan_res_event, NEVER_TIMEOUT, WICED_TRUE );

        platform_wlan_powersave_res_mask_event( WICED_FALSE );
    }
}

wiced_bool_t platform_wlan_powersave_res_up( void )
{
    wiced_bool_t res_up = WICED_FALSE;

    powersave_wlan_res_counter++;

    wiced_assert( "wrapped around zero", powersave_wlan_res_counter > 0 );

    if ( powersave_wlan_res_counter == 1 )
    {
        res_up = WICED_TRUE;
    }

    if ( res_up )
    {
        pmu_res_req_timer_t timer;

        timer.raw             = 0;
        timer.bits.req_active = 1;

        platform_tick_execute_command( PLATFORM_TICK_COMMAND_RELEASE_PMU_TIMER_BEGIN );

        PLATFORM_PMU->res_req_mask1 = PMU_RES_WLAN_UP_MASK;

        platform_wlan_powersave_pmu_timer_slow_write( timer.raw );

        platform_wlan_powersave_res_wait_event();
    }

    return res_up;
}

wiced_bool_t platform_wlan_powersave_res_down( wiced_bool_t(*check_ready)(void), wiced_bool_t force )
{
    wiced_bool_t res_down = WICED_FALSE;

    if ( !force )
    {
        wiced_assert( "unbalanced call", powersave_wlan_res_counter > 0 );
        powersave_wlan_res_counter--;
        if ( powersave_wlan_res_counter == 0 )
        {
            res_down = WICED_TRUE;
        }
    }
    else if ( powersave_wlan_res_counter != 0 )
    {
        powersave_wlan_res_counter = 0;
        res_down                   = WICED_TRUE;
    }

    if ( res_down )
    {
        if ( check_ready )
        {
            while ( !check_ready() );
        }

        platform_wlan_powersave_pmu_timer_slow_write( 0x0 );

        PLATFORM_PMU->res_req_mask1 = PMU_RES_APPS_UP_MASK;

        platform_tick_execute_command( PLATFORM_TICK_COMMAND_RELEASE_PMU_TIMER_END );
    }

    return res_down;
}

void platform_wlan_powersave_res_event( void )
{
    uint32_t event_mask = PLATFORM_PMU->res_event1;

    platform_wlan_powersave_res_ack_event();

    if ( ( PLATFORM_PMU->res_state & event_mask ) == event_mask )
    {
        host_rtos_set_semaphore( &powersave_wlan_res_event, WICED_TRUE );
    }
}

#endif /* PLATFORM_WLAN_POWERSAVE */
