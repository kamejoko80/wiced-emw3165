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
 * iperf Application
 *
 */

#include "wiced.h"
#include "iperf.h"
#include "command_console.h"

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
 *               Variable Definitions
 ******************************************************/
#define MAX_LINE_LENGTH  (128)
#define MAX_HISTORY_LENGTH (20)

static char line_buffer[MAX_LINE_LENGTH];
static char history_buffer_storage[MAX_LINE_LENGTH * MAX_HISTORY_LENGTH];

static const command_t iperf_commands[] = {
    { (char*) "iperf",  iperf,        0, NULL, NULL, NULL, "Run iperf --help for usage."},
    CMD_TABLE_END
};


/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    /* Initialise the device */
    wiced_init( );

    /* Bring up the network on the STA interface */
    wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );

    printf( "iPerf app\n" );

    /* Add iperf command to console */
    command_console_init( STDIO_UART, MAX_LINE_LENGTH, line_buffer, MAX_HISTORY_LENGTH, history_buffer_storage, " " );
    console_add_cmd_table( iperf_commands );
}
