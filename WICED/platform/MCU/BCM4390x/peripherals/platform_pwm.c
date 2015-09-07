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
 *
 */
#include <stdint.h>
#include "typedefs.h"

#include "osl.h"
#include "hndsoc.h"

#include "platform_peripheral.h"
#include "platform_appscr4.h"
#include "platform_toolchain.h"

#include "wwd_assert.h"
#include "wwd_rtos.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define PLATFORM_CC_BASE        PLATFORM_CHIPCOMMON_REGBASE(0x0)
#define PLATFORM_CC_CLOCKSTATUS PLATFORM_CLOCKSTATUS_REG(PLATFORM_CC_BASE)

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    BCM43909_PWM_CLOCK_ALP,
    BCM43909_PWM_CLOCK_BACKPLANE
} platform_pwm_clock_t;

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

/******************************************************
 *               Function Definitions
 ******************************************************/

static host_semaphore_type_t*
pwm_init_sem( void )
{
    static wiced_bool_t done = WICED_FALSE;
    static host_semaphore_type_t sem;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS( flags );

    if ( done != WICED_TRUE )
    {
        host_rtos_init_semaphore( &sem );
        host_rtos_set_semaphore ( &sem, WICED_FALSE );
        done = WICED_TRUE;
    }

    WICED_RESTORE_INTERRUPTS( flags );

    return &sem;
}

static wiced_bool_t
pwm_present( void )
{
    return (PLATFORM_CHIPCOMMON->clock_control.capabilities_extension.bits.pulse_width_modulation_present != 0);
}

static volatile pwm_channel_t*
pwm_get_channel( platform_pin_function_t func )
{
    if ( pwm_present() != WICED_TRUE )
    {
        return NULL;
    }

    switch (func)
    {
        case PIN_FUNCTION_PWM0:
            return &PLATFORM_CHIPCOMMON->pwm.pwm_channels[0];

        case PIN_FUNCTION_PWM1:
            return &PLATFORM_CHIPCOMMON->pwm.pwm_channels[1];

        case PIN_FUNCTION_PWM2:
            return &PLATFORM_CHIPCOMMON->pwm.pwm_channels[2];

        case PIN_FUNCTION_PWM3:
            return &PLATFORM_CHIPCOMMON->pwm.pwm_channels[3];

        case PIN_FUNCTION_PWM4:
            return &PLATFORM_CHIPCOMMON->pwm.pwm_channels[4];

        case PIN_FUNCTION_PWM5:
            return &PLATFORM_CHIPCOMMON->pwm.pwm_channels[5];

        default:
            wiced_assert("wrong func", 0);
            return NULL;
    }
}

static void
pwm_set_clock_src( platform_pwm_clock_t clock_src )
{
    switch ( clock_src )
    {
        case BCM43909_PWM_CLOCK_ALP:
            platform_pmu_chipcontrol( PMU_CHIPCONTROL_PWM_CLK_SLOW_REG,
                                      0x0,
                                      PMU_CHIPCONTROL_PWM_CLK_SLOW_MASK );
            break;

        case BCM43909_PWM_CLOCK_BACKPLANE:
            platform_pmu_chipcontrol( PMU_CHIPCONTROL_PWM_CLK_SLOW_REG,
                                      PMU_CHIPCONTROL_PWM_CLK_SLOW_MASK,
                                      0x0 );
            break;
    }
}

static platform_pwm_clock_t
pwm_get_clock_src( void )
{
    if ( (platform_pmu_chipcontrol( PMU_CHIPCONTROL_PWM_CLK_SLOW_REG, 0x0, 0x0 ) & PMU_CHIPCONTROL_PWM_CLK_SLOW_MASK ) == 0 )
    {
        return BCM43909_PWM_CLOCK_BACKPLANE;
    }
    else
    {
        return BCM43909_PWM_CLOCK_ALP;
    }
}

static wiced_bool_t
pwm_set_clock( platform_clock_t clock )
{
    if ( clock == CLOCK_HT )
    {
        /* Force backplane clock to HT and then switch to backplane clock. */

        uint32_t timeout = 100000000;
        clock_control_status_t control;
        uint32_t flags;

        WICED_SAVE_INTERRUPTS( flags );

        control.raw = PLATFORM_CC_CLOCKSTATUS->raw;
        control.bits.force_ht_request = 1;
        PLATFORM_CC_CLOCKSTATUS->raw = control.raw;

        WICED_RESTORE_INTERRUPTS( flags );

        while ( ( PLATFORM_CC_CLOCKSTATUS->bits.bp_on_ht == 0 ) && ( timeout != 0 ))
        {
            timeout--;
        }

        pwm_set_clock_src( BCM43909_PWM_CLOCK_BACKPLANE );
    }
    else if ( clock == CLOCK_ALP )
    {
        pwm_set_clock_src( BCM43909_PWM_CLOCK_ALP );
    }
    else if ( clock == CLOCK_BACKPLANE )
    {
        pwm_set_clock_src( BCM43909_PWM_CLOCK_BACKPLANE );
    }
    else
    {
        wiced_assert("wrong clock", 0);
        return WICED_FALSE;
    }

    /* Enable PWM clock. */
    platform_pmu_chipcontrol( PMU_CHIPCONTROL_PWM_CLK_EN_REG,
                              0x0,
                              PMU_CHIPCONTROL_PWM_CLK_EN_MASK );
    OSL_DELAY( 10 );

    return WICED_TRUE;
}

static wiced_bool_t
pwm_set_clock_once( void )
{
    static wiced_bool_t done   = WICED_FALSE;
    host_semaphore_type_t* sem = pwm_init_sem();
    wiced_bool_t ret;

    host_rtos_get_semaphore( sem, NEVER_TIMEOUT, WICED_FALSE );

    if ( done != WICED_TRUE )
    {
        osl_core_enable( CC_CORE_ID ); /* PWM is in chipcommon. Enable core before try to access. */
        done = pwm_set_clock( platform_pwm_getclock() );
    }

    ret = done;

    host_rtos_set_semaphore( sem, WICED_FALSE );

    return ret;
}

static uint32_t
pwm_get_clock_freq( void )
{
    if ( ( platform_pmu_chipcontrol( PMU_CHIPCONTROL_PWM_CLK_EN_REG, 0x0, 0x0 ) & PMU_CHIPCONTROL_PWM_CLK_EN_MASK ) == 0 )
    {
        /* PWM clock is not enabled. */
        return 0;
    }

    switch ( pwm_get_clock_src() )
    {
        case BCM43909_PWM_CLOCK_ALP:
            return osl_alp_clock();

        case BCM43909_PWM_CLOCK_BACKPLANE:
            return osl_backplane_clock( PLATFORM_CC_BASE );

        default:
            wiced_assert("wrong clock src", 0);
            return 0;
    }
}

static platform_result_t
pwm_init( const platform_pwm_t* pwm, uint32_t frequency, float duty_cycle )
{
    platform_result_t       ret;
    volatile pwm_channel_t* chan;
    uint32_t                high_period;
    uint32_t                low_period;
    uint32_t                total_period;
    uint32_t                clock_freq;
    pwm_channel_cycle_cnt_t cycle_cnt;

    chan = pwm_get_channel( pwm->func );
    if ( chan == NULL )
    {
        return PLATFORM_BADARG;
    }

    if ( pwm_set_clock_once() != WICED_TRUE )
    {
        return PLATFORM_ERROR;
    }

    clock_freq = pwm_get_clock_freq();
    if ( clock_freq == 0 )
    {
        return PLATFORM_ERROR;
    }

    total_period =  (clock_freq + frequency / 2) / frequency;
    high_period  =  MIN(total_period, (total_period * duty_cycle / 100.0f + 0.5f));
    low_period   =  total_period - high_period;
    if ( ( high_period > 0xFFFF ) || ( low_period > 0xFFFF ) || ( high_period == 0x0 ) || ( low_period == 0x0 ) )
    {
        return PLATFORM_BADARG;
    }

    ret = platform_pin_function_init( pwm->pin, pwm->func, PIN_FUNCTION_CONFIG_UNKNOWN );
    if ( ret != PLATFORM_SUCCESS )
    {
        return ret;
    }

    /* Configure dead time. */
    chan->dead_time.raw             = pwm->dead_time.raw;

    /* Configure cycle timings. */
    cycle_cnt.bits.low_cycle_time   = low_period;
    cycle_cnt.bits.high_cycle_time  = high_period;
    chan->cycle_cnt.raw             = cycle_cnt.raw;

    return PLATFORM_SUCCESS;
}

static platform_result_t
pwm_start( const platform_pwm_t* pwm )
{
    volatile pwm_channel_t* chan = pwm_get_channel( pwm->func );
    pwm_channel_ctrl_t      ctrl;

    if ( chan == NULL )
    {
        return PLATFORM_BADARG;
    }

    ctrl.raw                 = 0;
    ctrl.bits.enable         = 1;
    ctrl.bits.update         = pwm->update_all ? 0 : 1;
    ctrl.bits.update_all     = pwm->update_all ? 1 : 0;
    ctrl.bits.is_single_shot = pwm->is_single_shot ? 1 : 0;
    ctrl.bits.invert         = pwm->invert ? 1 : 0;
    ctrl.bits.alignment      = pwm->alignment;

    chan->ctrl.raw           = ctrl.raw;

    return PLATFORM_SUCCESS;
}

static platform_result_t
pwm_stop( const platform_pwm_t* pwm )
{
    volatile pwm_channel_t* chan = pwm_get_channel( pwm->func );

    if ( chan == NULL )
    {
        return PLATFORM_BADARG;
    }

    chan->ctrl.raw = 0;

    return PLATFORM_SUCCESS;
}

platform_result_t platform_pwm_init( const platform_pwm_t* pwm, uint32_t frequency, float duty_cycle )
{
    platform_result_t ret;

    wiced_assert( "bad argument", pwm != NULL );

    platform_mcu_powersave_disable();

    ret = pwm_init( pwm, frequency, duty_cycle );

    platform_mcu_powersave_enable();

    return ret;
}

platform_result_t platform_pwm_start( const platform_pwm_t* pwm )
{
    platform_result_t ret;

    wiced_assert( "bad argument", pwm != NULL );

    platform_mcu_powersave_disable();

    ret = pwm_start( pwm );

    platform_mcu_powersave_enable();

    return ret;
}

platform_result_t platform_pwm_stop( const platform_pwm_t* pwm )
{
    platform_result_t ret;

    wiced_assert( "bad argument", pwm != NULL );

    platform_mcu_powersave_disable();

    ret = pwm_stop( pwm );

    platform_mcu_powersave_enable();

    return ret;
}

WEAK platform_clock_t platform_pwm_getclock( void )
{
    return CLOCK_ALP;
}
