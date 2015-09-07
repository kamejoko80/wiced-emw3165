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
#include "wiced_defaults.h"
#include "wdt.h"
#include "sam4s.h"

#ifndef WICED_DISABLE_WATCHDOG

/******************************************************
 *                      Macros
 ******************************************************/

#define WATCHDOG_SEC_TO_USEC(sec)    ( sec * 1000000 )
#define RSTC_RESET_TYPE(SR)          (( SR & RSTC_SR_RSTTYP_Msk ) >> RSTC_SR_RSTTYP_Pos )

/******************************************************
 *                    Constants
 ******************************************************/

#define WDT_SCLK_HZ             ( 32768 )   /* SAM4S Slow Clk rate in Hz */

#ifdef APPLICATION_WATCHDOG_TIMEOUT_SECONDS
#if APPLICATION_WATCHDOG_TIMEOUT_SECONDS > 16   /* SAM4S supports Wdog timeout upto 16 secs */
#undef APPLICATION_WATCHDOG_TIMEOUT_SECONDS
#define APPLICATION_WATCHDOG_TIMEOUT_SECONDS    ( 16 )
#warning APPLICATION_WATCHDOG_TIMEOUT_SECONDS reduced to 16 seconds
#endif
#define WATCHDOG_TIMEOUT                ( APPLICATION_WATCHDOG_TIMEOUT_SECONDS )
#else
#define WATCHDOG_TIMEOUT                ( MAX_WATCHDOG_TIMEOUT_SECONDS )
#endif /* APPLICATION_WATCHDOG_TIMEOUT_SECONDS */

/* RSTC Reset types */
#define RSTC_RESET_TYPE_GEN    (0x0)
#define RSTC_RESET_TYPE_BKP    (0x1)
#define RSTC_RESET_TYPE_WDT    (0x2)
#define RSTC_RESET_TYPE_SOFT   (0x3)
#define RSTC_RESET_TYPE_USR    (0x4)

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *             WICED Function Definitions
 ******************************************************/

/******************************************************
 *             SAM4S Function Definitions
 ******************************************************/

platform_result_t platform_watchdog_init( void )
{
    uint32_t counter_value;

    counter_value = wdt_get_timeout_value( WATCHDOG_SEC_TO_USEC( WATCHDOG_TIMEOUT ), WDT_SCLK_HZ );
    if ( counter_value != WDT_INVALID_ARGUMENT )
    {
        /* Cause watchdog to generate both an interrupt and reset at underflow */
        wdt_init( WDT, WDT_MR_WDFIEN | WDT_MR_WDRSTEN, counter_value, counter_value );
    }
    return PLATFORM_SUCCESS;
}

platform_result_t platform_watchdog_kick( void )
{
    wdt_restart( WDT );
    return PLATFORM_SUCCESS;
}

wiced_bool_t platform_watchdog_check_last_reset( void )
{
    if ( RSTC_RESET_TYPE( RSTC->RSTC_SR ) == RSTC_RESET_TYPE_WDT )
    {
        return WICED_TRUE;
    }

    return WICED_FALSE;
}

#else
platform_result_t platform_watchdog_init( void )
{
    wdt_disable( WDT );
    return WICED_SUCCESS;
}

platform_result_t platform_watchdog_kick( void )
{
    return WICED_SUCCESS;
}

wiced_bool_t platform_watchdog_check_last_reset( void )
{
    return WICED_FALSE;
}
#endif /* ifndef WICED_DISABLE_WATCHDOG */
