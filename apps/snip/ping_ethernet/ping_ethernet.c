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
 * ICMP Ping over Ethernet Application
 *
 */

#include "wiced.h"
#include "ping_ethernet.h"


/******************************************************
 *                      Macros
 ******************************************************/

#define PING_TIMEOUT_MS          2000
#define PING_PERIOD_MS           3000

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

static wiced_result_t send_ping              ( void* arg );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_timed_event_t ping_timed_event;
static wiced_ip_address_t  ping_target_ip;

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_result_t result;

    /* Initialise the device */
    wiced_init( );

    /* Bring up the network on the ethernet interface */
    result = wiced_network_up( WICED_ETHERNET_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );

    if ( result == WICED_SUCCESS )
    {
        uint32_t ipv4;

        /* The ping target is the gateway */
        wiced_ip_get_gateway_address( WICED_ETHERNET_INTERFACE, &ping_target_ip );

        /* Setup a regular ping event and setup the callback to run in the networking worker thread */
        wiced_rtos_register_timed_event( &ping_timed_event, WICED_NETWORKING_WORKER_THREAD, &send_ping, PING_PERIOD_MS, 0 );

        /* Print ping parameters */
        ipv4 = GET_IPV4_ADDRESS(ping_target_ip);
        WPRINT_APP_INFO(("Pinging %u.%u.%u.%u every %ums with a %ums timeout.\n",
                                                 (unsigned int)((ipv4 >> 24) & 0xFF),
                                                 (unsigned int)((ipv4 >> 16) & 0xFF),
                                                 (unsigned int)((ipv4 >>  8) & 0xFF),
                                                 (unsigned int)((ipv4 >>  0) & 0xFF),
                                                 (unsigned int)PING_PERIOD_MS,
                                                 (unsigned int)PING_TIMEOUT_MS));
    }
    else
    {
        WPRINT_APP_INFO(("Unable to bring up network connection\n"));
    }
}

static wiced_result_t send_ping( void* arg )
{
    uint32_t elapsed_ms;
    wiced_result_t status;

    WPRINT_APP_INFO(("Ping about to be sent\n"));

    status = wiced_ping( WICED_ETHERNET_INTERFACE, &ping_target_ip, PING_TIMEOUT_MS, &elapsed_ms );

    if ( status == WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Ping Reply : %lu ms\n", (unsigned long)elapsed_ms ));
    }
    else if ( status == WICED_TIMEOUT )
    {
        WPRINT_APP_INFO(("Ping timeout\n"));
    }
    else
    {
        WPRINT_APP_INFO(("Ping error\n"));
    }

    return WICED_SUCCESS;
}

