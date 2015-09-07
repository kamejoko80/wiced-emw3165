/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include <stdint.h>
#include "platform_toolchain.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                    Callback types
 ******************************************************/

typedef enum
{
    WICED_DEEP_SLEEP_EVENT_ENTER,
    WICED_DEEP_SLEEP_EVENT_CANCEL,
    WICED_DEEP_SLEEP_EVENT_LEAVE
} wiced_deep_sleep_event_type_t;

typedef void( *wiced_deep_sleep_event_handler_t )( wiced_deep_sleep_event_type_t event );

/******************************************************
 *                 Platform definitions
 ******************************************************/

#ifdef PLATFORM_DEEP_SLEEP
#define PLATFORM_DEEP_SLEEP_HEADER_INCLUDED
#include "platform_deep_sleep.h"
#endif /* PLATFORM_DEEP_SLEEP */

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef WICED_DEEP_SLEEP_SAVED_VAR
#define WICED_DEEP_SLEEP_SAVED_VAR( var )                                               var
#endif

#ifndef WICED_DEEP_SLEEP_EVENT_HANDLER
#define WICED_DEEP_SLEEP_EVENT_HANDLER( func_name ) \
    static void UNUSED func_name( wiced_deep_sleep_event_type_t event )
#endif

#ifndef WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS
#define WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS( cond, event )
#endif

#ifndef WICED_DEEP_SLEEP_IS_WARMBOOT
#define WICED_DEEP_SLEEP_IS_WARMBOOT( )                                                 0
#endif

#ifndef WICED_DEEP_SLEEP_IS_ENABLED
#define WICED_DEEP_SLEEP_IS_ENABLED( )                                                  0
#endif

#ifndef WICED_DEEP_SLEEP_IS_WARMBOOT_HANDLE
#define WICED_DEEP_SLEEP_IS_WARMBOOT_HANDLE( )                                          ( WICED_DEEP_SLEEP_IS_ENABLED( ) && WICED_DEEP_SLEEP_IS_WARMBOOT( ) )
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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
