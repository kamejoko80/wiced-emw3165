/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "command_console_wps.h"

#include "../wifi/command_console_wifi.h"
#include "command_console.h"
#include "stdlib.h"
#include "wwd_debug.h"
#include "string.h"
#include "wps_host.h"
#include "wiced_wps.h"
#include "wiced_management.h"
#include "wwd_crypto.h"
#include "wiced_framework.h"
#include "wps_host_interface.h"
#include "wiced_time.h"
#include "besl_host.h"
#include "besl_host_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_CREDENTIAL_COUNT   5
#define MAX_SSID_LEN 32
#define MAX_PASSPHRASE_LEN 64
#define JOIN_ATTEMPT_TIMEOUT   60000

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

static wiced_result_t internal_start_registrar( wiced_wps_mode_t mode, const wiced_wps_device_detail_t* details, char* password, wiced_wps_credential_t* credentials, uint16_t credential_count );
static wiced_result_t run_wps(besl_wps_mode_t mode, char* password, wiced_wps_credential_t* credentials, uint16_t credential_count);

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

static const wiced_wps_device_detail_t registrar_details =
{
    .device_name                      = "Wiced",
    .manufacturer                     = "Broadcom",
    .model_name                       = "BCM943362",
    .model_number                     = "Wiced",
    .serial_number                    = "12345670",
    .device_category                  = PRIMARY_DEVICE_NETWORK_INFRASTRUCTURE,
    .sub_category                     = 1,
    .config_methods                   = WPS_CONFIG_LABEL | WPS_CONFIG_PUSH_BUTTON | WPS_CONFIG_VIRTUAL_PUSH_BUTTON | WPS_CONFIG_VIRTUAL_DISPLAY_PIN,
    .authentication_type_flags        = WPS_OPEN_AUTHENTICATION | WPS_WPA_PSK_AUTHENTICATION | WPS_WPA2_PSK_AUTHENTICATION,
    .encryption_type_flags            = WPS_NO_ENCRYPTION | WPS_MIXED_ENCRYPTION,
    .add_config_methods_to_probe_resp = 1,
};

static wps_agent_t* workspace = NULL;
static wps_ap_t*    ap_array;
static uint16_t     ap_array_size;


/******************************************************
 *               Function Definitions
 ******************************************************/

int join_wps( int argc, char* argv[] )
{
    int a;
    char* ip = NULL;
    char* netmask = NULL;
    char* gateway = NULL;
    wiced_result_t result = WICED_ERROR;
    wiced_wps_credential_t credential[MAX_CREDENTIAL_COUNT];
    char pin_string[9];

    memset( credential, 0, MAX_CREDENTIAL_COUNT*sizeof( wiced_wps_credential_t ) );

    if (wwd_wifi_is_ready_to_transceive(WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        wiced_network_down( WWD_STA_INTERFACE );
    }

    if ( workspace != NULL )
    {
        besl_wps_deinit( workspace );
        besl_host_free( workspace );
        workspace = NULL;
    }

    if ( strcmp( argv[1], "pin" ) == 0 )
    {
        /* PIN mode*/
        if ( argc == 6 ) /* PIN is supplied */
        {
            ip      = argv[3];
            netmask = argv[4];
            gateway = argv[5];
        }
        else if ( argc == 5 ) /* PIN is auto-generated */
        {
            ip      = argv[2];
            netmask = argv[3];
            gateway = argv[4];
        }

        if ( argc == 3 || argc == 6)
        {
            if ( is_digit_str(argv[2]) && ( ( strlen( argv[2] ) == 4 ) || ( strlen( argv[2] ) == 8 ) ) )
            {
                if ( strlen( argv[2] ) == 8 )
                {
                    if ( !besl_wps_validate_pin_checksum( argv[2] ) )
                    {
                        WPRINT_APP_INFO(("Invalid PIN checksum\n"));
                        return ( ERR_CMD_OK );
                    }
                }
                WPRINT_APP_INFO(("Starting Enrollee in PIN mode\n"));
                result = wiced_wps_enrollee(WICED_WPS_PIN_MODE, &enrollee_details, argv[2], credential, MAX_CREDENTIAL_COUNT);
            }
            else
            {
                WPRINT_APP_INFO(("PIN must be 4 or 8 digits\n"));
                return ( ERR_CMD_OK );
            }
        }
        else if ( argc == 2 || argc == 5)
        {
            besl_wps_generate_pin( pin_string );
            WPRINT_APP_INFO(("Starting Enrollee in PIN mode\n"));
            WPRINT_APP_INFO(("Enter this PIN in the other device: %s\n", pin_string));
            result = wiced_wps_enrollee(WICED_WPS_PIN_MODE, &enrollee_details, pin_string, credential, MAX_CREDENTIAL_COUNT);
        }
        else
        {
            return ERR_UNKNOWN;
        }
    }
    else if ( strcmp( argv[1], "pbc" ) == 0 )
    {
        /* Push Button mode */
        if ( argc == 5 )
        {
            ip      = argv[2];
            netmask = argv[3];
            gateway = argv[4];
        }
        if (argc == 2 || argc == 5)
        {
            WPRINT_APP_INFO(("Starting Enrollee in PBC mode\n"));
            char pbc_password[9] = "00000000";
            result = wiced_wps_enrollee(WICED_WPS_PBC_MODE, &enrollee_details, pbc_password, credential, MAX_CREDENTIAL_COUNT);

            switch ( result )
            {
                case WICED_SUCCESS:
                    WPRINT_APP_INFO(("WPS Successful\n"));
                    break;

                case WICED_BESL_PBC_OVERLAP:
                    WPRINT_APP_INFO(("PBC overlap detected - wait and try again\n"));
                    break;

                case WICED_BESL_ERROR_RECEIVED_WEP_CREDENTIALS:
                    WPRINT_APP_INFO(("Error - Registrar provided WEP credentials (not supported by WPS 2.0)\n"));
                    break;

                default:
                    /* WPS failed. Abort */
                   WPRINT_APP_INFO(("WPS failed\n"));
                   break;
            }
        }
        else
        {
            return ERR_UNKNOWN;
        }
    }
    else
    {
        return ERR_UNKNOWN;
    }

    /* Check if WPS was successful */
    if ( credential[0].ssid.length != 0 )
    {
        for (a=0; credential[a].ssid.length != 0; ++a)
        {
            WPRINT_APP_INFO(("SSID: %s\n",credential[a].ssid.value));
            WPRINT_APP_INFO(("Security: "));
            switch ( credential[a].security )
            {
                case WICED_SECURITY_OPEN:
                    WPRINT_APP_INFO(("Open\n"));
                    break;
                case WICED_SECURITY_WEP_PSK:
                    WPRINT_APP_INFO(("WEP PSK\n"));
                    break;
                case WICED_SECURITY_WPA_TKIP_PSK:
                case WICED_SECURITY_WPA_AES_PSK:
                    WPRINT_APP_INFO(("WPA PSK\n"));
                    break;
                case WICED_SECURITY_WPA2_AES_PSK:
                case WICED_SECURITY_WPA2_TKIP_PSK:
                case WICED_SECURITY_WPA2_MIXED_PSK:
                    WPRINT_APP_INFO(("WPA2 PSK\n"));
                    break;
                default:
                    break;
            }
            if ( credential[a].security != WICED_SECURITY_OPEN )
            {
                WPRINT_APP_INFO(("Network key: "));
                if ( credential[a].passphrase_length != 64 )
                {
                    WPRINT_APP_INFO(("%s\n", credential[a].passphrase));
                }
                else
                {
                    WPRINT_APP_INFO(("%.64s\n", credential[a].passphrase));
                }
            }
        }


        /* Join AP */
        int ret;
        wiced_wps_credential_t* cred;
        a = 0;
        uint32_t start_time = host_rtos_get_time( );
        do
        {
            if (( host_rtos_get_time( ) - start_time ) > JOIN_ATTEMPT_TIMEOUT)
            {
                return ERR_UNKNOWN;
            }
            cred = &credential[a];
            WPRINT_APP_INFO(("Joining : %s\n", cred->ssid.value));
            ret = wifi_join( (char*)cred->ssid.value, cred->security, (uint8_t*) cred->passphrase, cred->passphrase_length, ip, netmask, gateway );
            if (ret != ERR_CMD_OK)
            {
                WPRINT_APP_INFO(("Failed to join  : %s   .. retrying\n", cred->ssid.value));
                ++a;
                if (credential[a].ssid.length == 0)
                {
                    a = 0;
                }
            }
        }
        while (ret != ERR_CMD_OK);
        WPRINT_APP_INFO(("Successfully joined : %s\n", cred->ssid.value));
    }

    return ERR_CMD_OK;
}


/*!
 ******************************************************************************
 * Starts a WPS Registrar as specified by the provided arguments
 *
 * @return  0 for success, otherwise error
 */

int start_registrar( int argc, char* argv[] )
{
    static wiced_wps_credential_t credential;
    static char pin[10];
    platform_dct_wifi_config_t* dct_wifi_config;

    wiced_result_t result = WICED_ERROR;

    if ( wwd_wifi_is_ready_to_transceive( WICED_AP_INTERFACE ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Use start_ap command to bring up AP interface first\n"));
        return ERR_CMD_OK;
    }

    memset(&credential, 0, sizeof(wiced_wps_credential_t));

    /* Read config to get internal AP settings */
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );

    /* Copy config into the credential to be used by WPS */
    credential.ssid.length = dct_wifi_config->soft_ap_settings.SSID.length;
    memcpy((char*)&credential.ssid.value, (char*)dct_wifi_config->soft_ap_settings.SSID.value, credential.ssid.length);
    credential.security = dct_wifi_config->soft_ap_settings.security;
    credential.passphrase_length = dct_wifi_config->soft_ap_settings.security_key_length;
    memcpy(credential.passphrase, dct_wifi_config->soft_ap_settings.security_key, credential.passphrase_length);

    wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_FALSE );

    /* PIN mode */
    if ( ( strcmp( argv[1], "pin" ) == 0 ) && argc >= 3 )
    {
        memset(pin, 0, sizeof(pin));
        strncpy( pin, argv[2], ( sizeof(pin) - 1 ));
        if ( argc == 4 ) /* Then PIN may be in the form nnnn nnnn */
        {
            if ( ( strlen( argv[2] ) == 4 ) && ( strlen( argv[3] ) == 4 ) )
            {
                strncat( pin, argv[3], 4 );
            }
            else
            {
                WPRINT_APP_INFO(("Invalid PIN format\n"));
                return ( ERR_CMD_OK );
            }
            argc--;
        }
        if ( argc == 3 )
        {
            if ( strlen( pin ) == 9 ) /* Then PIN may be in the form nnnn-nnnn */
            {
                dehyphenate_pin( pin );
            }
            if ( is_digit_str(pin) && ( ( strlen( pin ) == 4 ) || ( strlen( pin ) == 8 ) ) )
            {
                if ( strlen( pin ) == 8 )
                {
                    if ( !besl_wps_validate_pin_checksum( pin ) )
                    {
                        WPRINT_APP_INFO(("Invalid PIN checksum\n"));
                        return ( ERR_CMD_OK );
                    }
                }
                WPRINT_APP_INFO(("Starting Registrar in PIN mode\n"));
                result = internal_start_registrar(WICED_WPS_PIN_MODE, &registrar_details, pin, &credential, 1);
            }
            else
            {
                WPRINT_APP_INFO(("PIN must be 4 or 8 digits\n"));
                return ( ERR_CMD_OK );
            }
        }
        else
        {
            return ERR_UNKNOWN;
        }
    }
    else if ( strcmp( argv[1], "pbc" ) == 0 )
    {
        /* Push Button mode */
        if (argc == 2)
        {
            WPRINT_APP_INFO(("Starting registrar in PBC mode\n"));
            result = internal_start_registrar(WICED_WPS_PBC_MODE, &registrar_details, "00000000", &credential, 1);

            switch ( result )
            {
                case WICED_SUCCESS:
                    /* WPRINT_APP_INFO(("Registrar starting\n")); */
                    break;

                case WICED_BESL_PBC_OVERLAP:
                    WPRINT_APP_INFO(("PBC overlap detected\n"));
                    break;

                default:
                    /* WPS failed. Abort */
                   WPRINT_APP_INFO(("WPS failed\n"));
                   break;
            }
        }
        else
        {
            return ERR_UNKNOWN;
        }
    }
    else
    {
        return ERR_UNKNOWN;
    }

    return ERR_CMD_OK;
}

static wiced_result_t internal_start_registrar( wiced_wps_mode_t mode, const wiced_wps_device_detail_t* details, char* password, wiced_wps_credential_t* credentials, uint16_t credential_count )
{
    wiced_result_t result;

    if ( workspace == NULL )
    {
        workspace = besl_host_calloc("wps", 1, sizeof(wps_agent_t));
        if ( workspace == NULL )
        {
            WPRINT_APP_INFO(("No memory for registrar agent\n"));
            return WICED_OUT_OF_HEAP_SPACE;
        }

        besl_wps_init( workspace, (besl_wps_device_detail_t*) details, WPS_REGISTRAR_AGENT, WICED_AP_INTERFACE );
    }
    else
    {
        if (workspace->wps_result == WPS_COMPLETE ||
            workspace->wps_result == WPS_PBC_OVERLAP ||
            workspace->wps_result == WPS_TIMEOUT ||
            workspace->wps_result == WPS_ABORTED ||
            workspace->wps_result == WPS_SUCCESS)
        {
            besl_wps_deinit( workspace );
            besl_wps_init( workspace, (besl_wps_device_detail_t*) details, WPS_REGISTRAR_AGENT, WICED_AP_INTERFACE );
        }
        else if (workspace->wps_result != WPS_NOT_STARTED )
        {
            if ( ( workspace->wps_mode == WPS_PBC_MODE ) && ( mode == WICED_WPS_PBC_MODE ) )
            {
                WPRINT_APP_INFO(("Restarting 2 minute window\n"));
                besl_wps_restart( workspace );
            }
            else
            {
                WPRINT_APP_INFO(("WPS already running\n"));
            }
            return WWD_SUCCESS;
        }
    }

    result = besl_wps_start( workspace, mode, password, (besl_wps_credential_t*) credentials, credential_count );

    if ( result == WICED_BESL_PBC_OVERLAP )
    {
        WPRINT_APP_INFO(("WPS fail - PBC overlap\n"));
        return result;
    }
    else if ( result != WICED_SUCCESS )
    {
        besl_wps_deinit( workspace );
        free( workspace );
        workspace = NULL;
        return result;
    }

    return WWD_SUCCESS;
}


int stop_registrar( int argc, char* argv[] )
{
    if ( workspace == NULL )
    {
        WPRINT_APP_INFO(("WPS Registrar not yet initialized\n"));
    }
    else if (workspace->wps_result != WPS_NOT_STARTED )
    {
        if ( besl_wps_abort( workspace ) == BESL_SUCCESS )
        {
            WPRINT_APP_INFO(("WPS Registrar stopped\n"));
        }
    }
    else
    {
        WPRINT_APP_INFO(("WPS Registrar not running\n"));
    }
    return ERR_CMD_OK;
}


void dehyphenate_pin(char* str )
{
    int i;

    for ( i = 4; i < 9; i++ )
    {
        str[i] = str[i+1];
    }
}

int force_alignment( int argc, char* argv[] )
{
    volatile uint32_t* configuration_control_register = (uint32_t*)0xE000ED14;
    *configuration_control_register |= (1 << 3);
    return ERR_CMD_OK;
}

wiced_result_t enable_ap_registrar_events( void )
{
    wiced_result_t result;

    if ( workspace == NULL )
    {
        workspace = besl_host_calloc("wps", 1, sizeof(wps_agent_t));
        if ( workspace == NULL )
        {
            WPRINT_APP_INFO(("Error calloc wps\n"));
            stop_ap(0, NULL);
            return WICED_OUT_OF_HEAP_SPACE;
        }
    }

    if ( ( result = besl_wps_init( workspace, (besl_wps_device_detail_t*) &registrar_details, WPS_REGISTRAR_AGENT, WWD_AP_INTERFACE ) ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Error besl init %u\n", (unsigned int)result));
        stop_ap(0, NULL);
        return result;
    }
    if ( ( result = besl_wps_management_set_event_handler( workspace, WICED_TRUE ) ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Error besl setting event handler %u\n", (unsigned int)result));
        stop_ap(0, NULL);
        return result;
    }
    return WICED_SUCCESS;
}

void disable_ap_registrar_events( void )
{
    if (workspace != NULL)
    {
        besl_wps_abort( workspace );
        besl_wps_management_set_event_handler( workspace, WICED_FALSE );
        besl_wps_deinit( workspace );
        free( workspace );
        workspace = NULL;
    }
}

extern wps_ap_t* wps_host_retrieve_ap( void* workspace );
int scan_wps( int argc, char* argv[] )
{
    if ( workspace == NULL )
    {
        workspace = besl_host_calloc("wps", 1, sizeof(wps_agent_t));
        if ( workspace == NULL )
        {
            WPRINT_APP_INFO(("No memory for enrollee agent\n"));
            return ERR_UNKNOWN;
        }

        besl_wps_init( workspace, (besl_wps_device_detail_t*) &enrollee_details, WPS_ENROLLEE_AGENT, WICED_STA_INTERFACE );
    }

    if ( besl_wps_scan( workspace, &ap_array, &ap_array_size, WICED_STA_INTERFACE ) == BESL_SUCCESS )
    {
        int a;
        for ( a = 0; a < ap_array_size; ++a )
        {
            WPRINT_APP_INFO(( "%u: %.*s\n", a, ap_array[a].scan_result.SSID.length, ap_array[a].scan_result.SSID.value ));
        }
    }

    while ( wps_host_retrieve_ap( workspace->wps_host_workspace ) != NULL )
    {
    }

    return ERR_CMD_OK;
}

int join_wps_specific( int argc, char* argv[] )
{
    int a;
    char* ip = NULL;
    char* netmask = NULL;
    char* gateway = NULL;
    wiced_result_t result = WICED_ERROR;
    wiced_wps_credential_t credential[MAX_CREDENTIAL_COUNT];
    char pin_string[9];

    scan_wps( 0, 0 );

    WPRINT_APP_INFO(( "Select an ap:\n" ));

    uint8_t selected_ap;
    do
    {
        selected_ap = (1 + getchar( ) - '1')%10;
    } while (selected_ap > ap_array_size);

    besl_wps_set_directed_wps_target( workspace, &ap_array[selected_ap], 0xFFFFFFFF);

    memset( credential, 0, MAX_CREDENTIAL_COUNT*sizeof( wiced_wps_credential_t ) );

    if (wwd_wifi_is_ready_to_transceive(WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        wiced_network_down( WICED_STA_INTERFACE );
    }

    /* PIN mode*/
    if ( argc == 5 ) // PIN is supplied
    {
        ip      = argv[2];
        netmask = argv[3];
        gateway = argv[4];
    }
    else if ( argc == 4 ) // PIN is auto-generated
    {
        ip      = argv[1];
        netmask = argv[2];
        gateway = argv[3];
    }

    if ( argc == 2 || argc == 5)
    {
        if ( is_digit_str(argv[1]) && ( ( strlen( argv[1] ) == 4 ) || ( strlen( argv[1] ) == 8 ) ) )
        {
            if ( strlen( argv[1] ) == 8 )
            {
                if ( !besl_wps_validate_pin_checksum( argv[1] ) )
                {
                    WPRINT_APP_INFO(("Invalid PIN checksum\n"));
                    return ( ERR_CMD_OK );
                }
            }
            WPRINT_APP_INFO(("Starting Enrollee in PIN mode\n"));
            result = run_wps(WICED_WPS_PIN_MODE, argv[1], credential, MAX_CREDENTIAL_COUNT);
        }
        else
        {
            WPRINT_APP_INFO(("PIN must be 4 or 8 digits\n"));
            return ( ERR_CMD_OK );
        }
    }
    else if ( argc == 1 || argc == 4)
    {
        besl_wps_generate_pin( pin_string );
        WPRINT_APP_INFO(("Enter this PIN in the other device: %s\n", pin_string));
        if ( besl_wps_validate_pin_checksum( pin_string ) )
        {
            WPRINT_APP_INFO(("Starting Enrollee in PIN mode\n"));
            result = run_wps(WICED_WPS_PIN_MODE, pin_string, credential, MAX_CREDENTIAL_COUNT);
        }
        else
        {
            WPRINT_APP_INFO(("Invalid PIN checksum, try again\n"));
        }
    }
    else
    {
        return ERR_UNKNOWN;
    }

    /* Check if WPS was successful */
    if ( ( result != WICED_ERROR ) && ( credential[0].ssid.length != 0 ) )
    {
        for (a=0; credential[a].ssid.length != 0; ++a)
        {
            WPRINT_APP_INFO(("SSID: %s\n",credential[a].ssid.value));
            WPRINT_APP_INFO(("Security: "));
            switch ( credential[a].security )
            {
                case WICED_SECURITY_OPEN:
                    WPRINT_APP_INFO(("Open\n"));
                    break;
                case WICED_SECURITY_WEP_PSK:
                    WPRINT_APP_INFO(("WEP PSK\n"));
                    break;
                case WICED_SECURITY_WPA_TKIP_PSK:
                case WICED_SECURITY_WPA_AES_PSK:
                    WPRINT_APP_INFO(("WPA PSK\n"));
                    break;
                case WICED_SECURITY_WPA2_AES_PSK:
                case WICED_SECURITY_WPA2_TKIP_PSK:
                case WICED_SECURITY_WPA2_MIXED_PSK:
                    WPRINT_APP_INFO(("WPA2 PSK\n"));
                    break;
                default:
                    break;
            }
            if ( credential[a].security != WICED_SECURITY_OPEN )
            {
                WPRINT_APP_INFO(("Network key: "));
                if ( credential[a].passphrase_length != 64 )
                {
                    WPRINT_APP_INFO(("%s\n", credential[a].passphrase));
                }
                else
                {
                    WPRINT_APP_INFO(("%.64s\n", credential[a].passphrase));
                }
            }
        }


        /* Join AP */
        int ret;
        wiced_wps_credential_t* cred;
        a = 0;
        uint32_t start_time = host_rtos_get_time( );
        do
        {
            if (( host_rtos_get_time( ) - start_time ) > JOIN_ATTEMPT_TIMEOUT)
            {
                return ERR_UNKNOWN;
            }
            cred = &credential[a];
            WPRINT_APP_INFO(("Joining : %s\n", cred->ssid.value));
            ret = wifi_join( (char*)cred->ssid.value, cred->security, (uint8_t*) cred->passphrase, cred->passphrase_length, ip, netmask, gateway );
            if (ret != ERR_CMD_OK)
            {
                WPRINT_APP_INFO(("Failed to join  : %s   .. retrying\n", cred->ssid.value));
                ++a;
                if (credential[a].ssid.length == 0)
                {
                    a = 0;
                }
            }
        }
        while (ret != ERR_CMD_OK);
        WPRINT_APP_INFO(("Successfully joined : %s\n", cred->ssid.value));
    }

    return ERR_CMD_OK;
}

static wiced_result_t run_wps(besl_wps_mode_t mode, char* password, wiced_wps_credential_t* credentials, uint16_t credential_count)
{
    wiced_result_t result;

    if ( workspace == NULL )
    {
        WPRINT_APP_INFO(("Must run wps_scan before attempting a directed join\n"));
        return WICED_ERROR;
    }

    result = besl_wps_start( workspace, mode, password, (besl_wps_credential_t*) credentials, credential_count );
    wiced_rtos_delay_milliseconds(10); /* Delay required to allow the WPS thread to run */
    if ( result == WICED_SUCCESS )
    {
        besl_wps_wait_till_complete( workspace );
        result = besl_wps_get_result( workspace );
    }

    besl_wps_deinit( workspace );
    besl_host_free(workspace);
    workspace = NULL;
    return result;
}
