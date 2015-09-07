/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <stdint.h>
#include "platform_isr.h"
#include "platform_isr_interface.h"
#include "platform_assert.h"
#include "platform_peripheral.h"

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

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

PLATFORM_DEFINE_ISR( Unhandled_ISR )
{
#ifdef DEBUG
    WICED_TRIGGER_BREAKPOINT( );

    while( 1 )
    {
    }
#else /* DEBUG */
    platform_mcu_reset( );
#endif /* DEBUG */
}

/******************************************************
 *          Default IRQ Handler Declarations
 ******************************************************/
PLATFORM_SET_DEFAULT_ISR( ChipCommon_ISR   , Unhandled_ISR )
PLATFORM_SET_DEFAULT_ISR( Timer_ISR        , Unhandled_ISR )
PLATFORM_SET_DEFAULT_ISR( Sw0_ISR          , Unhandled_ISR )
PLATFORM_SET_DEFAULT_ISR( Sw1_ISR          , Unhandled_ISR )
PLATFORM_SET_DEFAULT_ISR( GMAC_ISR         , Unhandled_ISR )
PLATFORM_SET_DEFAULT_ISR( Serror_ISR       , Unhandled_ISR )
PLATFORM_SET_DEFAULT_ISR( I2S0_ISR         , Unhandled_ISR )
PLATFORM_SET_DEFAULT_ISR( I2S1_ISR         , Unhandled_ISR )
