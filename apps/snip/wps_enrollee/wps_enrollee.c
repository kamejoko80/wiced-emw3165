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
 * WPS Enrollee Application
 *
 * This application snippet demonstrates how to use the WPS Enrollee
 *
 * Features demonstrated
 *  - WPS Enrollee
 *
 * Application Instructions
 *   1. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *   2. Using a WPS capable Wi-Fi Access Point, press the WPS button on
 *      the AP to start a WPS setup session
 *   3. Connection progress is printed to the console
 *
 * The WPS Enrollee runs for up to 2 minutes before either successfully
 * connecting to the AP or timing out.
 *
 */

#include "wiced.h"
#include "wiced_wps.h"
#include "wps_host_interface.h"

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

static const wiced_wps_device_detail_t enrollee_details =
{
    .device_name               = PLATFORM,
    .manufacturer              = "Broadcom",
    .model_name                = PLATFORM,
    .model_number              = "1.0",
    .serial_number             = "1408248",
    .device_category           = WICED_WPS_DEVICE_COMPUTER,
    .sub_category              = 7,
    .config_methods            = WPS_CONFIG_LABEL | WPS_CONFIG_VIRTUAL_PUSH_BUTTON | WPS_CONFIG_VIRTUAL_DISPLAY_PIN,
    .authentication_type_flags = WPS_OPEN_AUTHENTICATION | WPS_WPA_PSK_AUTHENTICATION | WPS_WPA2_PSK_AUTHENTICATION,
    .encryption_type_flags     = WPS_NO_ENCRYPTION | WPS_MIXED_ENCRYPTION,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_wps_credential_t     credential[CONFIG_AP_LIST_SIZE];
    platform_dct_wifi_config_t* wifi_config;
    wiced_result_t result = WICED_SUCCESS;

    memset( &credential, 0, sizeof( credential ) );

    wiced_init();

    WPRINT_APP_INFO(("Starting WPS Enrollee in PBC mode. Press the WPS button on your AP now.\r\n"));

    result = wiced_wps_enrollee( WICED_WPS_PBC_MODE, &enrollee_details, "00000000", credential, CONFIG_AP_LIST_SIZE );
    if (WICED_SUCCESS == result)
    {
        uint32_t i;

        WPRINT_APP_INFO( ("WPS enrollment was successful\r\n") );

        /* Copy Wi-Fi credentials obtained from WPS to the Wi-Fi config section in the DCT */
        wiced_dct_read_lock( (void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof( platform_dct_wifi_config_t ) );

        for ( i = 0; i < CONFIG_AP_LIST_SIZE; i++ )
        {
            memcpy( (void *)&wifi_config->stored_ap_list[ i ].details.SSID, &credential[ i ].ssid, sizeof(wiced_ssid_t) );
            memcpy( wifi_config->stored_ap_list[ i ].security_key, &credential[ i ].passphrase, credential[ i ].passphrase_length );

            wifi_config->stored_ap_list[i].details.security    = credential[i].security;
            wifi_config->stored_ap_list[i].security_key_length = credential[i].passphrase_length;
        }

        wiced_dct_write ( (const void*)wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof (platform_dct_wifi_config_t) );

        wiced_dct_read_unlock( (void*)wifi_config, WICED_TRUE );

        /* AP credentials have been stored in the DCT, now join the AP */
        wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    }
    else
    {
        WPRINT_APP_INFO( ("WPS enrollment was not successful\r\n") );
    }
}
