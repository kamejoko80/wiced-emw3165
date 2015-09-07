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
 * Ethernet Testing for Console Application
 *
 */

#include "command_console_ethernet.h"

#include "command_console.h"
#include "wiced.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define PING_TIMEOUT_MS          2000
#define PING_PERIOD_MS           3000

/******************************************************
 *                    Constants
 ******************************************************/
#define PING_RCV_TIMEO ( 1000 ) /* ping receive timeout - in milliseconds */
#define PING_MAX_PAYLOAD_SIZE ( 10000 ) /* ping max size */
#define ENABLE_LONG_PING /* Long ping enabled by default */

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

static wiced_result_t send_ping             (); //( void* arg );

/******************************************************
 *               Variable Definitions
 ******************************************************/
static const uint8_t const long_ping_payload[PING_MAX_PAYLOAD_SIZE] = { 0 };

//static wiced_timed_event_t ping_timed_event;
static wiced_ip_address_t  ping_target_ip;

/******************************************************
 *               Function Definitions
 ******************************************************/
int network_suspend( int argc, char* argv[] )
{
    /* Suspend network timers */
    if( wiced_network_suspend() == WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Suspended network\n"));
        return ERR_CMD_OK;
    }
    else
    {
        WPRINT_APP_INFO(("Suspending network failed\n"));
        return ERR_UNKNOWN;
    }
}

int network_resume( int argc, char* argv[] )
{
    /* Resume network timers */
    if( wiced_network_resume() == WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Resumed network\n"));
        return ERR_CMD_OK;
    }
    else
    {
        WPRINT_APP_INFO(("Resuming network failed\n"));
        return ERR_UNKNOWN;
    }
}

int ethernet_up( int argc, char* argv[] )
{
    wiced_result_t result;
    wiced_ip_setting_t static_ip_settings;

    /* Device already initialized */

    if (argc > 4)
    {
        return ERR_TOO_MANY_ARGS;
    }

    if (argc > 1 && argc != 4)
    {
        return ERR_INSUFFICENT_ARGS;
    }

    /* Bring up the network on the Ethernet interface */
    /* If static IP */
    if( argc == 4 )
    {
        str_to_ip( argv[1], &static_ip_settings.ip_address );
        str_to_ip( argv[2], &static_ip_settings.netmask );
        str_to_ip( argv[3], &static_ip_settings.gateway );

        result = wiced_network_up( WICED_ETHERNET_INTERFACE, WICED_USE_STATIC_IP, &static_ip_settings );
    }
    /* DHCP */
    else
    {
        result = wiced_network_up( WICED_ETHERNET_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    }

    if( result == WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Bring up on Ethernet was SUCCESSFUL\n"));
        return ERR_CMD_OK;
    }
    else
    {
        WPRINT_APP_INFO(("Unable to bring up network connection\n"));
        return ERR_UNKNOWN;
    }
}

int ethernet_down( int argc, char* argv[] )
{
    wiced_result_t result;
    WPRINT_PLATFORM_DEBUG(("ethernet down request\n"));

    if( wiced_network_is_up( WICED_ETHERNET_INTERFACE ) == WICED_FALSE )
    {
        WPRINT_APP_INFO(("Ethernet interface not up\n"));
        return ERR_UNKNOWN;
    }

    result = wiced_network_down( WICED_ETHERNET_INTERFACE );

    if( result == WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Bringing down Ethernet connection was SUCCESSFUL\n"));
        return ERR_CMD_OK;
    }
    else
    {
        WPRINT_APP_INFO(("Unable to bring down Ethernet connection\n"));
        return ERR_UNKNOWN;
    }
}


int ethernet_ping( int argc, char *argv[] )
{

    if ( argc == 1 )
    {
        return ERR_INSUFFICENT_ARGS;
    }

    int i        = 0;
    int len      = 100;
    int num      = 1;
    int interval = 1000;
    wiced_bool_t continuous = WICED_FALSE;
    int temp[4];


    ping_target_ip.version = WICED_IPV4;
    sscanf( argv[1], "%d.%d.%d.%d", &temp[0], &temp[1], &temp[2], &temp[3] );
    ping_target_ip.ip.v4 = temp[0] << 24 | temp[1] << 16 | temp[2] << 8 | temp[3];


    for ( i = 2; i < argc; i++ )
    {
        switch (argv[i][1])
        {
            case 'i':
                interval = atoi(argv[i+1]);
                if ( interval < 0 )
                {
                    WPRINT_APP_INFO(("min interval 0\n\r"));
                    return ERR_CMD_OK;
                }
                WPRINT_APP_INFO(("interval: %d milliseconds\n\r", interval));
                i++;
                break;

            case 'l':
                len = atoi(argv[i+1]);
                if ( ( len > PING_MAX_PAYLOAD_SIZE ) || ( len < 0 ) )
                {
                    WPRINT_APP_INFO(("max ping length: %d, min: 0\n\r", PING_MAX_PAYLOAD_SIZE));
                    return ERR_CMD_OK;
                }
                WPRINT_APP_INFO(("length: %d\n\r", len));
                i++;
                break;

            case 'n':
                num = atoi(argv[i+1]);
                if ( num < 1 )
                {
                    WPRINT_APP_INFO(("min number of packets 1\n\r"));
                    return ERR_CMD_OK;
                }
                WPRINT_APP_INFO(("number : %d\n\r", num));
                i++;
                break;

            case 't':
                continuous = WICED_TRUE;
                WPRINT_APP_INFO(("continuous...\n\r"));
                break;

            default:
                WPRINT_APP_INFO(("Not supported, ignoring: %s\n\r", argv[i]));
            break;
        }
    }

    while ( ( num > 0 ) || ( continuous == WICED_TRUE ) )
    {
        send_ping();

        num--;
        if ( ( num > 0 ) || ( continuous == WICED_TRUE ) )
        {
            wiced_rtos_delay_milliseconds( interval ); // This is simple and should probably wait for a residual period
        }
    }

    return ERR_CMD_OK;
}

static wiced_result_t send_ping()
{
    uint32_t elapsed_ms;
    wiced_result_t status;

    WPRINT_APP_INFO(("Ping about to be sent\n"));
    WPRINT_APP_INFO ( ("PING %u.%u.%u.%u\r\n", (unsigned char) ( ( ping_target_ip.ip.v4 >> 24 ) & 0xff ),
                                                 (unsigned char) ( ( ping_target_ip.ip.v4 >> 16 ) & 0xff ),
                                                 (unsigned char) ( ( ping_target_ip.ip.v4 >>  8 ) & 0xff ),
                                                 (unsigned char) ( ( ping_target_ip.ip.v4 >>  0 ) & 0xff ) ));

    status = wiced_ping( WICED_ETHERNET_INTERFACE, &ping_target_ip, PING_TIMEOUT_MS, &elapsed_ms );

    if ( status == WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Ping Reply : %lu ms\n", (unsigned long)elapsed_ms ));
    }
    else if ( status == WICED_TIMEOUT )
    {
        WPRINT_APP_INFO(("Ping timeout\n"));
    }
    else if (status == WICED_NOTUP)
    {
        WPRINT_APP_INFO(("Interface not up\n"));
    }
    else
    {
        WPRINT_APP_INFO(("Ping error\n"));
    }


    return WICED_SUCCESS;
}
