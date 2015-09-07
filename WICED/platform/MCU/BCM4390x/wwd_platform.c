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
#include "wwd_constants.h"
#include "wwd_assert.h"
#include "platform_config.h"
#include "wiced_platform.h"
#include "platform/wwd_platform_interface.h"
#include "platform_appscr4.h"
#include "cr4.h"


/******************************************************
 *                      Macros
 ******************************************************/
#ifdef __GNUC__
#define TRIGGER_BREAKPOINT() __asm__("bkpt")
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define TRIGGER_BREAKPOINT() __asm("bkpt 0")
#endif

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

wwd_result_t host_platform_init( void )
{

    return WWD_SUCCESS;
}

wwd_result_t host_platform_deinit( void )
{
    return WWD_SUCCESS;
}

uint32_t host_platform_get_cycle_count( void )
{
    return cr4_get_cycle_counter( );
}

wwd_result_t host_platform_init_wlan_powersave_clock( void )
{
    return WWD_SUCCESS;
}

wwd_result_t host_platform_deinit_wlan_powersave_clock( void )
{
    return WWD_SUCCESS;
}

wiced_bool_t host_platform_is_in_interrupt_context( void )
{
    return WICED_FALSE;
}

void host_platform_reset_wifi( wiced_bool_t reset_asserted )
{
    UNUSED_PARAMETER( reset_asserted );
}
