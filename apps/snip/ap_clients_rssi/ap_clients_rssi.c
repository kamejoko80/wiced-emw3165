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
 * softAP Clients Application
 *
 * This application snippet demonstrates how to retrieve a list
 * of Wi-Fi clients associated to the WICED softAP and obtain the
 * RSSI (signal strength) for each client.
 *
 * Features demonstrated
 *  - Wi-Fi softAP mode and related APIs
 *
 * Application Instructions
 *   Connect a PC terminal to the serial port of the WICED Eval board,
 *   then build and download the application as described in the WICED
 *   Quick Start Guide
 *
 *   Using a Wi-Fi client such as an iOS or Android device, connect to
 *   the softAP with credentials : WICED Soft AP / 12345678
 *
 *   One per second, connection details of the client are printed to
 *   the serial port. The details of any additional clients are also
 *   printed if additional clients join.
 *
 */

#include "wiced.h"
#include "resources.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_SOFT_AP_CLIENTS  (5)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    int         count;
    wiced_mac_t mac_list[MAX_SOFT_AP_CLIENTS];
} client_info_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

static const wiced_ip_setting_t ap_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start(void)
{
    int32_t         rssi = 0;
    int             client_number = 0;
    wiced_result_t  result;
    client_info_t   client_info;

    client_info.count = MAX_SOFT_AP_CLIENTS;

    /* Initialise Wiced system */
    wiced_init();

    /* Bring up the softAP interface  */
    wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &ap_ip_settings );

    while ( 1 )
    {
        /* Our client_info can only fit information about no more than 5 clients (4 if the STA interface is also used)*/
        client_info.count = MAX_SOFT_AP_CLIENTS;

        /* Get the list of the stations connected to Wiced soft AP */
        result = wiced_wifi_get_associated_client_list( &client_info, sizeof( client_info ) );
        if ( result != WICED_SUCCESS )
        {
            WPRINT_APP_INFO( ("Error. Can't get the list of clients\r\n") );
            wiced_rtos_delay_milliseconds( 1000 );
            continue;
        }

        if ( client_info.count == 0 )
        {
            WPRINT_APP_INFO( ("Clients connected 0..\r\n") );
        }
        else
        {
            WPRINT_APP_INFO( ("Clients connected %d..\r\n", client_info.count) );
            for ( client_number = 0; client_number < client_info.count; client_number++ )
            {
                /* Get the RSSI of every client currently connected to the soft AP */
                result = wiced_wifi_get_ap_client_rssi( &rssi, &client_info.mac_list[ client_number ] );
                if ( result != WICED_SUCCESS )
                {
                    WPRINT_APP_INFO( ("Error. Can't get rssi of the client\r\n") );
                }
                else
                {
                    WPRINT_APP_INFO( ("------------------------------------\r\n") );
                    WPRINT_APP_INFO( ("| %d | %02x:%02x:%02x:%02x:%02x:%02x | %3lddBm   |\r\n",
                        client_number,
                        client_info.mac_list[client_number].octet[0], client_info.mac_list[client_number].octet[1], client_info.mac_list[client_number].octet[2], client_info.mac_list[client_number].octet[3], client_info.mac_list[client_number].octet[4], client_info.mac_list[client_number].octet[5],
                        rssi ) );
                }
            }
            WPRINT_APP_INFO( ("------------------------------------\r\n") );
        }

        /* Sleep for a second and do it again */
        WPRINT_APP_INFO( ("Waiting for an update...\r\n\r\n") );
        wiced_rtos_delay_milliseconds( 1000 );
    }
}


