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
 * BCM43909 vector table
 */
#include "platform_isr.h"
#include "wwd_rtos_isr.h"

/******************************************************
 *                      Macros
 ******************************************************/

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

extern void _tx_timer_interrupt( void ); /* Defined within ThreadX */

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

WWD_RTOS_DEFINE_ISR( platform_tick_isr )
{
    if ( platform_tick_irq_handler( ) )
    {
        _tx_timer_interrupt( );
    }
}

WWD_RTOS_MAP_ISR( platform_tick_isr, Timer_ISR )
