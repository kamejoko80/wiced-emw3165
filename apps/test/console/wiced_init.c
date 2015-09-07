/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "command_console_ping.h"
#include "command_console_wps.h"
#include "command_console.h"
#include "wiced_management.h"
#include "command_console_wifi.h"
#include "command_console_mallinfo.h"
#include "console_iperf.h"
#include "command_console_thread.h"
#include "command_console_platform.h"

#ifdef CONSOLE_INCLUDE_P2P
#include "command_console_p2p.h"
#endif

#ifdef CONSOLE_INCLUDE_ETHERNET
#include "command_console_ethernet.h"
#endif

#ifdef CONSOLE_ENABLE_WL
#include "console_wl.h"
#endif

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

#define MAX_LINE_LENGTH  (128)
#define MAX_HISTORY_LENGTH (20)

static char line_buffer[MAX_LINE_LENGTH];
static char history_buffer_storage[MAX_LINE_LENGTH * MAX_HISTORY_LENGTH];

static const command_t commands[] =
{
    WIFI_COMMANDS
    IPERF_COMMANDS
    MALLINFO_COMMANDS
    PING_COMMANDS
    PLATFORM_COMMANDS
    THREAD_COMMANDS
    WPS_COMMANDS
#ifdef CONSOLE_INCLUDE_P2P
    P2P_COMMANDS
#endif
#ifdef CONSOLE_INCLUDE_ETHERNET
    ETHERNET_COMMANDS
#endif
#ifdef CONSOLE_ENABLE_WL
    WL_COMMANDS
#endif
    CMD_TABLE_END
};



/**
 *  @param thread_input : Unused parameter - required to match thread prototype
 */
void application_start( void )
{
    /* Initialise the device */
    wiced_init( );

    printf( "Console app\n" );

    /* Run the main application function */
    command_console_init( STDIO_UART, MAX_LINE_LENGTH, line_buffer, MAX_HISTORY_LENGTH, history_buffer_storage, " " );
    console_add_cmd_table( commands );
}

