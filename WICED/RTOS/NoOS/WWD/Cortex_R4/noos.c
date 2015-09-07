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
 *  NoOS functions specific to the Cortex-R4 Platform
 *
 */

#include <stdint.h>
#include "wwd_rtos.h"
#include "wwd_rtos_isr.h"
#include "platform_assert.h"

volatile uint32_t noos_total_time = 0;

#define CYCLES_PER_SYSTICK  ( ( CPU_CLOCK_HZ / SYSTICK_FREQUENCY ) - 1 )

void NoOS_setup_timing( void )
{
    platform_tick_start( );

    WICED_ENABLE_INTERRUPTS( );
}

void NoOS_stop_timing( void )
{
    WICED_DISABLE_INTERRUPTS();

    platform_tick_stop( );
}

/* NoOS SysTick handler */
WWD_RTOS_DEFINE_ISR( platform_tick_isr )
{
    if ( platform_tick_irq_handler( ) )
    {
        noos_total_time++;
    }
}

WWD_RTOS_MAP_ISR( platform_tick_isr, Timer_ISR )
