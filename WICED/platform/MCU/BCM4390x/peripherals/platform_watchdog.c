/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "platform_peripheral.h"
#include "platform_appscr4.h"
#include "platform_assert.h"
#include "platform_peripheral.h"

#include "typedefs.h"
#include "sbchipc.h"

#include "wiced_defaults.h"
#include "wiced_osl.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/


#define WATCHDOG_TIMEOUT_MULTIPLIER     osl_ilp_clock()

#ifdef APPLICATION_WATCHDOG_TIMEOUT_SECONDS
#define WATCHDOG_TIMEOUT                (APPLICATION_WATCHDOG_TIMEOUT_SECONDS * WATCHDOG_TIMEOUT_MULTIPLIER)
#else
#define WATCHDOG_TIMEOUT                (MAX_WATCHDOG_TIMEOUT_SECONDS * WATCHDOG_TIMEOUT_MULTIPLIER)
#endif /* APPLICATION_WATCHDOG_TIMEOUT_SECONDS */


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

static void
platform_watchdog_set( uint32_t timeout )
{
    PLATFORM_PMU->pmuwatchdog = timeout;
}

platform_result_t platform_watchdog_init( void )
{
    return platform_watchdog_kick();
}

platform_result_t platform_watchdog_kick( void )
{
#ifndef WICED_DISABLE_WATCHDOG
    platform_watchdog_set( WATCHDOG_TIMEOUT );
    return PLATFORM_SUCCESS;
#else
    return PLATFORM_FEATURE_DISABLED;
#endif
}

wiced_bool_t platform_watchdog_check_last_reset( void )
{
    if ( PLATFORM_PMU->pmustatus & PST_WDRESET )
    {
        /* Clear the flag and return */
        PLATFORM_PMU->pmustatus = PST_WDRESET;
        return WICED_TRUE;
    }

    return WICED_FALSE;
}

void platform_mcu_reset( void )
{
    WICED_DISABLE_INTERRUPTS();

    /* Set watchdog to reset system on next tick */
    platform_watchdog_set( 1 );

    /* Loop forever */
    while (1)
    {
    }
}
