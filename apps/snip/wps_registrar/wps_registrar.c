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
 * WPS Registrar Application
 *
 * This application snippet demonstrates how to use the
 * WPS Registrar on an existing softAP interface.
 *
 * Features demonstrated
 *  - Wi-Fi softAP mode
 *  - WPS Registrar
 *
 * Application Instructions
 *   1. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *   2. Using a WPS capable device (Windows PC, Android device),
 *      search for the WICED softAP "WPS_REGISTRAR_EXAMPLE" using
 *      the Wi-Fi setup method for the device
 *   3. Attempt to connect to the WICED softAP using WPS
 *   4. Connection progress is printed to the console
 *
 *   The application starts a softAP, and then immediately enables
 *   the WPS registrar which runs for 2 minutes before timing out.
 *   If your WPS capable device does not connect within the 2 minute
 *   window, reset the WICED eval board and try again.
 *
 *   Results are printed to the terminal.
 *
 * Notes
 *   1. A Windows client detects that WPS is enabled on the softAP
 *      and automatically attempts to connect using PBC mode
 *      Windows does not support PIN mode.
 *   2. Apple iOS devices such as iPhones DO NOT support WPS.
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

static const wiced_wps_device_detail_t registrar_details =
{
    .device_name                      = PLATFORM,
    .manufacturer                     = "Broadcom",
    .model_name                       = PLATFORM,
    .model_number                     = "1.0",
    .serial_number                    = "1408248",
    .device_category                  = WICED_WPS_DEVICE_NETWORK_INFRASTRUCTURE,
    .sub_category                     = 1,
    .config_methods                   = WPS_CONFIG_PUSH_BUTTON | WPS_CONFIG_VIRTUAL_PUSH_BUTTON | WPS_CONFIG_VIRTUAL_DISPLAY_PIN,
    .authentication_type_flags        = WPS_OPEN_AUTHENTICATION | WPS_WPA_PSK_AUTHENTICATION | WPS_WPA2_PSK_AUTHENTICATION,
    .encryption_type_flags            = WPS_NO_ENCRYPTION | WPS_MIXED_ENCRYPTION,
    .add_config_methods_to_probe_resp = 1,
};

static const wiced_ip_setting_t device_init_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS(192, 168, 10,  1) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS(255, 255, 255, 0) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS(192, 168, 10,  1) ),
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_wps_credential_t   ap_info;
    wiced_config_soft_ap_t*  softap_info;

    wiced_init();

    WPRINT_APP_INFO( ("Starting access point\r\n") );
    wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &device_init_ip_settings );

    /* Extract the settings from the WICED softAP to pass to the WPS registrar */
    wiced_dct_read_lock( (void**) &softap_info, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, OFFSETOF(platform_dct_wifi_config_t, soft_ap_settings), sizeof(wiced_config_soft_ap_t) );
    memcpy( ap_info.passphrase, softap_info->security_key, sizeof(ap_info.passphrase) );
    memcpy( &ap_info.ssid,      &softap_info->SSID,        sizeof(wiced_ssid_t) );
    ap_info.passphrase_length = softap_info->security_key_length;
    ap_info.security          = softap_info->security;
    wiced_dct_read_unlock( softap_info, WICED_FALSE );

    WPRINT_APP_INFO( ("Starting WPS Registrar in PBC mode\r\n") );

    if ( wiced_wps_registrar( WICED_WPS_PBC_MODE, &registrar_details, "00000000", &ap_info, 1 ) == WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("WPS enrollment was successful\r\n") );
    }
    else
    {
        WPRINT_APP_INFO( ("WPS enrollment was not successful\r\n") );
    }
}
