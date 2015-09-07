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
 * Thread Monitor Application
 *
 * This application demonstrates how to use the WICED
 * System Monitor API to monitor the operation of the
 * application thread.
 *
 * Features demonstrated
 *  - WICED System Monitor
 *
 * Application Instructions
 *   Connect a PC terminal to the serial port of the WICED Eval board,
 *   then build and download the application as described in the WICED
 *   Quick Start Guide
 *
 *   After the download completes, the app:
 *    - Registers a thread monitor with the WICED system monitor
 *    - Loops 10 times, each time notifying the system monitor that
 *      the thread is working normally
 *    - An unexpected delay is then simulated
 *    - A short time AFTER the unexpected delay occurs, the
 *      watchdog bites and the WICED eval board reboots
 *
 *  The watchdog timeout period is set in the thread_monitor.mk makefile
 *  using the global variable : APPLICATION_WATCHDOG_TIMEOUT_SECONDS
 *
 */

#include "wiced.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAXIMUM_ALLOWED_INTERVAL_BETWEEN_MONITOR_UPDATES (1000*MILLISECONDS)
#define EXPECTED_WORK_TIME                               (MAXIMUM_ALLOWED_INTERVAL_BETWEEN_MONITOR_UPDATES - (100*MILLISECONDS))
#define UNEXPECTED_DELAY                                 (200*MILLISECONDS)


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

static void do_work( void );
static void update_my_thread_monitor( uint32_t count );

/******************************************************
 *               Variable Definitions
 ******************************************************/

wiced_system_monitor_t my_thread_monitor;

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    uint32_t a = 0;

    /* Initialise the device and WICED framework */
    wiced_init( );

    /* Register a thread monitor to keep a watchful eye on my thread processing time */
    WPRINT_APP_INFO( ( "\r\nRegistering my thread monitor\n\n" ) );
    wiced_register_system_monitor( &my_thread_monitor, MAXIMUM_ALLOWED_INTERVAL_BETWEEN_MONITOR_UPDATES );

    /* Do some work and then update my thread monitor.
     * This demonstrates normal behaviour for the thread.
     */
    while (1)
    {
        do_work();
        update_my_thread_monitor( a++ );

        if (a == 10)
        {
            /* Add an unexpected delay. This is abnormal behaviour for the thread */
            wiced_rtos_delay_milliseconds( UNEXPECTED_DELAY );
            WPRINT_APP_INFO( ( "Uh oh, I'm about to watchdog because an unexpected delay occurred!\n\n" ) );
        }
    }
}


static void do_work( void )
{
    WPRINT_APP_INFO( ( "Do some work\n" ) );
    wiced_rtos_delay_milliseconds( EXPECTED_WORK_TIME );  /* 'work' is simply a waste of time for this demo! */
}


static void update_my_thread_monitor( uint32_t count )
{
    WPRINT_APP_INFO( ( "Updating monitor: %d\n\n", (int)count ) );
    wiced_update_system_monitor( &my_thread_monitor, MAXIMUM_ALLOWED_INTERVAL_BETWEEN_MONITOR_UPDATES );
}
