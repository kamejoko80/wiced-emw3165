/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "command_console_wifi.h"

#include "wwd_wlioctl.h"
#include "wwd_wifi.h"
#include "string.h"
#include "wwd_debug.h"
#include "command_console.h"
#include "wwd_assert.h"
#include "wwd_network.h"
#include "stdlib.h"
#include "wwd_management.h"
#include "internal/wwd_sdpcm.h"
#include "internal/wwd_internal.h"
#include "network/wwd_buffer_interface.h"
#include "wiced_management.h"
#include "dhcp_server.h"
#include "wwd_crypto.h"
#include "wiced.h"
#include "wiced_security.h"
#include "internal/wiced_internal_api.h"
#include "besl_host.h"
#include "besl_host_interface.h"
#include "certificate.h"
#include "wiced_tls.h"
#include "wiced_utilities.h"

#ifdef COMMAND_CONSOLE_WPS_ENABLED
#include "command_console_wps.h"
#include "wiced_wps.h"
#include "wwd_events.h"
#include "wps_common.h"
#endif

#ifdef COMMAND_CONSOLE_P2P_ENABLED
#include "p2p_structures.h"
#include "wiced_p2p.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define CHECK_IOCTL_BUFFER( buff )  if ( buff == NULL ) {  wiced_assert("Allocation failed\n", 0 == 1); return ERR_UNKNOWN; }

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_SSID_LEN         ( 32 )
#define MAX_PASSPHRASE_LEN   ( 64 )
#define A_SHA_DIGEST_LEN     ( 20 )
#define DOT11_PMK_LEN        ( 32 )

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t scan_result_handler( wiced_scan_handler_result_t* malloced_scan_result );
static int wifi_join_specific(char* ssid, wiced_security_t auth_type, uint8_t* security_key, uint16_t key_length, char* bssid, char* channel, char* ip, char* netmask, char* gateway);
static void ac_params_print( const wiced_edcf_ac_param_t *acp, const int *priority );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static char last_joined_ssid[32] = "";
char last_started_ssid[32] = "";
static char last_soft_ap_passphrase[MAX_PASSPHRASE_LEN+1] = "";
static int record_count;
static wiced_semaphore_t scan_semaphore;
static const wiced_ip_setting_t ap_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};

#ifdef COMMAND_CONSOLE_P2P_ENABLED
extern p2p_workspace_t p2p_workspace;
#endif

static wiced_tls_session_t tls_session = { 0 };

/******************************************************
 *               Function Definitions
 ******************************************************/

/*!
 ******************************************************************************
 * Joins an access point specified by the provided arguments
 *
 * @return  0 for success, otherwise error
 */

int join( int argc, char* argv[] )
{
    char* ssid = argv[1];
    wiced_security_t auth_type = str_to_authtype(argv[2]);
    uint8_t* security_key;
    uint8_t key_length;
    uint8_t wep_key_buffer[64] = { 0 };

    if (argc > 7)
    {
        return ERR_TOO_MANY_ARGS;
    }

    if (argc > 4 && argc != 7)
    {
        return ERR_INSUFFICENT_ARGS;
    }

    if ( auth_type == WICED_SECURITY_UNKNOWN )
    {
        WPRINT_APP_INFO(( "Error: Invalid security type\n" ));
        return ERR_UNKNOWN;
    }

    if ( auth_type == WICED_SECURITY_WEP_PSK )
    {
        int a;
        wiced_wep_key_t* temp_wep_key = (wiced_wep_key_t*)wep_key_buffer;
        char temp_string[3];
        temp_string[2] = 0;
        key_length = strlen(argv[3])/2;

        /* Setup WEP key 0 */
        temp_wep_key[0].index = 0;
        temp_wep_key[0].length = key_length;
        for (a = 0; a < temp_wep_key[0].length; ++a)
        {
            uint32_t tmp_val;
            memcpy(temp_string, &argv[3][a*2], 2);
            string_to_unsigned( temp_string, 2, &tmp_val, 1 );
            temp_wep_key[0].data[a] = (uint8_t) tmp_val;
        }

        /* Setup WEP keys 1 to 3 */
        memcpy(wep_key_buffer + 1*(2 + key_length), temp_wep_key, (2 + key_length));
        memcpy(wep_key_buffer + 2*(2 + key_length), temp_wep_key, (2 + key_length));
        memcpy(wep_key_buffer + 3*(2 + key_length), temp_wep_key, (2 + key_length));
        wep_key_buffer[1*(2 + key_length)] = 1;
        wep_key_buffer[2*(2 + key_length)] = 2;
        wep_key_buffer[3*(2 + key_length)] = 3;

        security_key = wep_key_buffer;
        key_length = 4*(2 + key_length);
    }
    else if ( ( auth_type != WICED_SECURITY_OPEN ) && ( argc < 4 ) )
    {
        WPRINT_APP_INFO(("Error: Missing security key\n" ));
        return ERR_UNKNOWN;
    }
    else
    {
        security_key = (uint8_t*)argv[3];
        key_length = strlen((char*)security_key);
    }

    if ( argc == 7 )
    {
        return wifi_join( ssid, auth_type, (uint8_t*) security_key, key_length, argv[4], argv[5], argv[6]);
    }
    else
    {
        return wifi_join( ssid, auth_type, (uint8_t*) security_key, key_length, NULL, NULL, NULL );
    }
}


/*!
 ******************************************************************************
 * Joins an access point using enterprise security
 *
 * @return  0 for success, otherwise error
 */

int join_ent( int argc, char* argv[] )
{
    char* ssid = argv[1];
    supplicant_workspace_t supplicant_workspace;
    wiced_security_t auth_type;
    char eap_identity[] = "wifi-user@wifilabs.local";
    wiced_tls_advanced_context_t context;

    if ( argc != 3 )
    {
        return ERR_INSUFFICENT_ARGS;
    }

    auth_type = str_to_enterprise_authtype(argv[2]);
    if ( auth_type == WICED_SECURITY_UNKNOWN )
    {
        WPRINT_APP_INFO(("Unknown security type\n" ));
        return ERR_CMD_OK;
    }


    wiced_tls_init_advanced_context( &context, WIFI_USER_CERTIFICATE_STRING, WIFI_USER_PRIVATE_KEY_STRING );

    if ( tls_session.length > 0 )
    {
        memcpy( &context.session, &tls_session, sizeof(wiced_tls_session_t) );
    }
    else
    {
        memset( &context.session, 0, sizeof(wiced_tls_session_t) );
    }

    wiced_tls_init_root_ca_certificates( WIFI_ROOT_CERTIFICATE_STRING );

    if ( besl_supplicant_init( &supplicant_workspace, EAP_TYPE_TLS, WWD_STA_INTERFACE ) == BESL_SUCCESS )
    {
        wiced_supplicant_enable_tls( &supplicant_workspace, &context );
        besl_supplicant_set_identity( &supplicant_workspace, eap_identity, strlen( eap_identity ) );
        if ( besl_supplicant_start( &supplicant_workspace ) == BESL_SUCCESS )
        {
            if ( wifi_join( ssid, auth_type, NULL, 0, NULL, NULL, NULL ) == ERR_CMD_OK )
            {
                memcpy( &tls_session, &context.session, sizeof(wiced_tls_session_t) );
            }
        }
    }
    else
    {
        WPRINT_APP_INFO( ("Unable to initialize supplicant\n" ) );
    }

    wiced_tls_deinit_context( (wiced_tls_simple_context_t*)&context );
    wiced_tls_deinit_root_ca_certificates();

    besl_supplicant_deinit( &supplicant_workspace );

    return ERR_CMD_OK;
}


int wifi_join(char* ssid, wiced_security_t auth_type, uint8_t* key, uint16_t key_length, char* ip, char* netmask, char* gateway)
{
    wiced_network_config_t network_config;
    wiced_ip_setting_t* ip_settings = NULL;
    wiced_ip_setting_t static_ip_settings;
    platform_dct_wifi_config_t* dct_wifi_config;

    if (wwd_wifi_is_ready_to_transceive(WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }

    /* Read config */
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );

    /* Modify config */
    dct_wifi_config->stored_ap_list[0].details.SSID.length = strlen( ssid );
    strncpy((char*)dct_wifi_config->stored_ap_list[0].details.SSID.value, ssid, MAX_SSID_LEN);
    dct_wifi_config->stored_ap_list[0].details.security = auth_type;
    if ( ( auth_type & ENTERPRISE_ENABLED ) == 0 )
    {
        /* Save credentials for non-enterprise AP */
        memcpy((char*)dct_wifi_config->stored_ap_list[0].security_key, (char*)key, MAX_PASSPHRASE_LEN);
        dct_wifi_config->stored_ap_list[0].security_key_length = key_length;
    }

    /* Write config */
    wiced_dct_write( (const void*) dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );

    /* Tell the network stack to setup it's interface */
    if (ip == NULL )
    {
        network_config = WICED_USE_EXTERNAL_DHCP_SERVER;
    }
    else
    {
        network_config = WICED_USE_STATIC_IP;
        str_to_ip( ip,      &static_ip_settings.ip_address );
        str_to_ip( netmask, &static_ip_settings.netmask );
        str_to_ip( gateway, &static_ip_settings.gateway );
        ip_settings = &static_ip_settings;
    }

    if ( wiced_network_up( WICED_STA_INTERFACE, network_config, ip_settings ) != WICED_SUCCESS )
    {
        if ( auth_type == WICED_SECURITY_WEP_PSK ) /* Now try shared instead of open authentication */
        {
            dct_wifi_config->stored_ap_list[0].details.security = WICED_SECURITY_WEP_SHARED;
            wiced_dct_write( (const void*) dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
            WPRINT_APP_INFO(("WEP with open authentication failed, trying WEP with shared authentication...\n"));

            if ( wiced_network_up( WICED_STA_INTERFACE, network_config, ip_settings ) != WICED_SUCCESS ) /* Restore old value */
            {
                WPRINT_APP_INFO(("Trying shared wep\n"));
                dct_wifi_config->stored_ap_list[0].details.security = WICED_SECURITY_WEP_PSK;
                wiced_dct_write( (const void*) dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
            }
            else
            {
                wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );
                return ERR_CMD_OK;
            }
        }

        wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );
        return ERR_UNKNOWN;
    }

    strncpy(last_joined_ssid, ssid, MAX_SSID_LEN);

    wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Joins a specific access point using the provided arguments
 *
 * @return  0 for success, otherwise error
 */

int join_specific( int argc, char* argv[] )
{
    char* ssid = argv[1];
    wiced_security_t auth_type = str_to_authtype(argv[4]);
    uint8_t* security_key;
    uint8_t key_length;

    if (argc > 9)
    {
        return ERR_TOO_MANY_ARGS;
    }

    if (argc > 6 && argc != 9)
    {
        return ERR_INSUFFICENT_ARGS;
    }

    if ( auth_type == WICED_SECURITY_UNKNOWN )
    {
        WPRINT_APP_INFO(( "Error: Invalid security type\n" ));
        return ERR_UNKNOWN;
    }

    if ( auth_type == WICED_SECURITY_WEP_PSK )
    {
        int a;
        uint8_t          wep_key_buffer[64];
        wiced_wep_key_t* temp_wep_key = (wiced_wep_key_t*)wep_key_buffer;
        char temp_string[3];
        temp_string[2] = 0;
        key_length = strlen(argv[5])/2;

        /* Setup WEP key 0 */
        temp_wep_key[0].index = 0;
        temp_wep_key[0].length = key_length;
        for (a = 0; a < temp_wep_key[0].length; ++a)
        {
            uint32_t tmp_val;
            memcpy(temp_string, &argv[5][a*2], 2);
            string_to_unsigned( temp_string, 2, &tmp_val, 1 );
            temp_wep_key[0].data[a] = (uint8_t) tmp_val;
        }

        /* Setup WEP keys 1 to 3 */
        memcpy(wep_key_buffer + 1*(2 + key_length), temp_wep_key, (2 + key_length));
        memcpy(wep_key_buffer + 2*(2 + key_length), temp_wep_key, (2 + key_length));
        memcpy(wep_key_buffer + 3*(2 + key_length), temp_wep_key, (2 + key_length));
        wep_key_buffer[1*(2 + key_length)] = 1;
        wep_key_buffer[2*(2 + key_length)] = 2;
        wep_key_buffer[3*(2 + key_length)] = 3;

        security_key = wep_key_buffer;
        key_length = 4*(2 + key_length);
    }
    else if ( ( auth_type != WICED_SECURITY_OPEN ) && ( argc < 4 ) )
    {
        WPRINT_APP_INFO(("Error: Missing security key\n" ));
        return ERR_UNKNOWN;
    }
    else
    {
        security_key = (uint8_t*)argv[5];
        key_length = strlen((char*)security_key);
    }

    if ( argc == 9 )
    {
        return wifi_join_specific( ssid, auth_type, (uint8_t*) security_key, key_length, argv[2], argv[3], argv[6], argv[7], argv[8]);
    }
    else
    {
        return wifi_join_specific( ssid, auth_type, (uint8_t*) security_key, key_length, argv[2], argv[3], NULL, NULL, NULL );
    }
}

static int wifi_join_specific(char* ssid, wiced_security_t auth_type, uint8_t* security_key, uint16_t key_length, char* bssid, char* channel, char* ip, char* netmask, char* gateway)
{
    wiced_network_config_t network_config;
    wiced_ip_setting_t static_ip_settings;
    wiced_scan_result_t ap;

    if (wwd_wifi_is_ready_to_transceive(WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }

    memset( &ap, 0, sizeof( ap ) );
    ap.SSID.length = strlen( ssid );
    memcpy( ap.SSID.value, ssid, ap.SSID.length );
    str_to_mac( bssid, &ap.BSSID );
    ap.channel = atoi( channel );
    ap.security = auth_type;
    ap.band = WICED_802_11_BAND_2_4GHZ;
    ap.bss_type = WICED_BSS_TYPE_INFRASTRUCTURE;

    if ( !( NULL_MAC(ap.BSSID.octet) ) && wwd_wifi_join_specific( &ap, security_key, key_length, NULL, WWD_STA_INTERFACE ) == WWD_SUCCESS )
    {

        /* Tell the network stack to setup it's interface */
        if (ip == NULL )
        {
            network_config = WICED_USE_EXTERNAL_DHCP_SERVER;
        }
        else
        {
            network_config = WICED_USE_STATIC_IP;
            str_to_ip( ip,      &static_ip_settings.ip_address );
            str_to_ip( netmask, &static_ip_settings.netmask );
            str_to_ip( gateway, &static_ip_settings.gateway );
        }

        if ( wiced_ip_up( WICED_STA_INTERFACE, network_config, &static_ip_settings ) == WICED_SUCCESS )
        {
            strncpy(last_joined_ssid, ssid, MAX_SSID_LEN);
            return ERR_CMD_OK;
        }
    }

    return ERR_UNKNOWN;
}

/**
 *  Scan result callback
 *  Called whenever a scan result is available
 *
 *  @param result_ptr : pointer to pointer for location where result is stored. The inner pointer
 *                      can be updated to cause the next result to be put in a new location.
 *  @param user_data : unused
 */
static wiced_result_t scan_result_handler( wiced_scan_handler_result_t* malloced_scan_result )
{
    if ( malloced_scan_result != NULL )
    {
        malloc_transfer_to_curr_thread( malloced_scan_result );

        if ( malloced_scan_result->status == WICED_SCAN_INCOMPLETE )
        {
            wiced_scan_result_t* record = &malloced_scan_result->ap_details;

            wiced_assert( "error", ( record->bss_type == WICED_BSS_TYPE_INFRASTRUCTURE ) || ( record->bss_type == WICED_BSS_TYPE_ADHOC ) );

            WPRINT_APP_INFO( ( "%3d ", record_count ) );
            print_scan_result( record );

            ++record_count;
        }
        else
        {
            wiced_rtos_set_semaphore(&scan_semaphore);
        }

        free( malloced_scan_result );
        malloced_scan_result = NULL;
    }

    return WWD_SUCCESS;
}

/*!
 ******************************************************************************
 * Scans for access points and prints out results
 *
 * @return  0 for success, otherwise error
 */

int scan( int argc, char* argv[] )
{
    record_count = 0;
    WPRINT_APP_INFO( ( "Waiting for scan results...\n" ) );

    WPRINT_APP_INFO( ("  # Type  BSSID              RSSI Rate Chan  Security               SSID\n" ) );
    WPRINT_APP_INFO( ("----------------------------------------------------------------------------------------------\n" ) );

    /* Initialise the semaphore that will tell us when the scan is complete */
    wiced_rtos_init_semaphore(&scan_semaphore);

    wiced_wifi_scan_networks(scan_result_handler, NULL );

    /* Wait until scan is complete */
    wiced_rtos_get_semaphore(&scan_semaphore, WICED_WAIT_FOREVER);

    wiced_rtos_deinit_semaphore(&scan_semaphore);

    /* Done! */
    WPRINT_APP_INFO( ( "\nEnd of scan results\n" ) );

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Starts a soft AP as specified by the provided arguments
 *
 * @return  0 for success, otherwise error
 */

int start_ap( int argc, char* argv[] )
{
    char* ssid = argv[1];
    wiced_security_t auth_type = str_to_authtype(argv[2]);
    char* security_key = argv[3];
    char  temp_security_key[64];
    uint8_t channel = atoi(argv[4]);
    wiced_result_t result;
    uint8_t pmk[DOT11_PMK_LEN + 8]; /* PMK storage must be 40 octets in length for use in various functions */
    platform_dct_wifi_config_t* dct_wifi_config;
    uint8_t  key_length = 0;

    if ( wwd_wifi_is_ready_to_transceive( WICED_AP_INTERFACE ) == WWD_SUCCESS )
    {
        WPRINT_APP_INFO(( "Error: AP already started\n" ));
        return ERR_UNKNOWN;
    }

    if ( ( auth_type != WICED_SECURITY_WPA2_AES_PSK ) &&
         ( auth_type != WICED_SECURITY_OPEN ) &&
         ( auth_type != WICED_SECURITY_WPA2_MIXED_PSK ) &&
         ( auth_type != WICED_SECURITY_WEP_PSK ) &&
         ( auth_type != WICED_SECURITY_WEP_SHARED ) )
    {
        WPRINT_APP_INFO(( "Error: Invalid security type\n" ));
        return ERR_UNKNOWN;
    }

    if ( auth_type == WICED_SECURITY_OPEN )
    {
        char c = 0;

        WPRINT_APP_INFO(( "Open without any encryption [y or n]?\n" ));
        while (1)
        {
            c = getchar();
            if ( c == 'y' )
            {
                break;
            }
            if ( c == 'n' )
            {
                return ERR_CMD_OK;
            }
            WPRINT_APP_INFO(( "y or n\n" ));
        }
    }

    if ( argc == 6 )
    {
        if ( memcmp( argv[5], "wps", sizeof("wps") ) != 0 )
        {
            return ERR_UNKNOWN;
        }
    }

    key_length = strlen(security_key);

    /* Read config */
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );

    strncpy(last_soft_ap_passphrase, security_key, MAX_PASSPHRASE_LEN+1);

    /* Modify config */
    if ( key_length < MAX_PASSPHRASE_LEN)
    {
        if ( auth_type == WICED_SECURITY_WEP_PSK || auth_type == WICED_SECURITY_WEP_SHARED )
        {
#ifdef WICED_WIFI_SOFT_AP_WEP_SUPPORT_ENABLED
            /* Format WEP security key */
            format_wep_keys( temp_security_key, security_key, &key_length, WEP_KEY_TYPE );
#endif
        }
        else
        {
            memset(pmk, 0, sizeof(pmk));
            if ( besl_802_11_generate_pmk( security_key, (unsigned char *) ssid, strlen( ssid ), (unsigned char*) pmk ) == BESL_SUCCESS )
            {
                key_length = MAX_PASSPHRASE_LEN;
                besl_host_hex_bytes_to_chars( temp_security_key, pmk, DOT11_PMK_LEN );
            }
        }
        dct_wifi_config->soft_ap_settings.security_key_length = key_length;
        memcpy( dct_wifi_config->soft_ap_settings.security_key, temp_security_key, MAX_PASSPHRASE_LEN );
    }
    else
    {
        dct_wifi_config->soft_ap_settings.security_key_length = MAX_PASSPHRASE_LEN;
        strncpy(dct_wifi_config->soft_ap_settings.security_key, security_key, MAX_PASSPHRASE_LEN);
    }

    if (strlen(ssid) > MAX_SSID_LEN)
    {
        WPRINT_APP_INFO(( "Error: SSID longer than 32 characters\n" ));
        return ERR_UNKNOWN;
    }

    if (strlen(ssid) > MAX_SSID_LEN)
    {
        WPRINT_APP_INFO(( "Error: SSID longer than 32 characters\n" ));
        return ERR_UNKNOWN;
    }

    strncpy((char*)dct_wifi_config->soft_ap_settings.SSID.value, ssid, MAX_SSID_LEN);
    dct_wifi_config->soft_ap_settings.security = auth_type;
    dct_wifi_config->soft_ap_settings.channel = channel;
    dct_wifi_config->soft_ap_settings.SSID.length = strlen( ssid );

    /* Write config */
    wiced_dct_write( (const void*)dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof( platform_dct_wifi_config_t));
    wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );

    if ( ( result = wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &ap_ip_settings ) ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Error starting AP %u\n", (unsigned int)result));
        return result;
    }
#ifdef COMMAND_CONSOLE_WPS_ENABLED
    if ( ( argc == 6 ) && ( memcmp( argv[5], "wps", sizeof("wps") ) == 0 ) )
    {
        result = enable_ap_registrar_events();
        if ( result != WICED_SUCCESS )
        {
            return result;
        }
    }
#endif
    strncpy(last_started_ssid, ssid, MAX_SSID_LEN);
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Stops a running soft AP
 *
 * @return  0 for success, otherwise error
 */

int stop_ap( int argc, char* argv[] )
{
    if (wwd_wifi_is_ready_to_transceive(WICED_AP_INTERFACE) != WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }

#ifdef COMMAND_CONSOLE_WPS_ENABLED
    disable_ap_registrar_events();
#endif

    deauth_all_associated_client_stas(WWD_DOT11_RC_UNSPECIFIED, WICED_AP_INTERFACE);

    return wiced_network_down( WICED_AP_INTERFACE );
}

wiced_result_t deauth_all_associated_client_stas(wwd_dot11_reason_code_t reason, wwd_interface_t interface )
{
    uint8_t* buffer = NULL;
    wiced_maclist_t * clients = NULL;
    const wiced_mac_t * current;
    wiced_result_t      result;
    wl_bss_info_t ap_info;
    wiced_security_t sec;
    uint32_t max_clients = 0;
    size_t size = 0;

    result = wwd_wifi_get_max_associations( &max_clients );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to get max number of associated clients\n") );
        max_clients = 5;
    }

    size = ( sizeof(uint32_t) + (max_clients * sizeof(wiced_mac_t)));
    buffer = calloc(1, size);

    if (buffer == NULL)
    {
        WPRINT_APP_INFO(("Unable to allocate memory for associated clients list\n"));
        return WICED_ERROR;
    }
    clients = (wiced_maclist_t*)buffer;
    clients->count = max_clients;
    memset(&ap_info, 0, sizeof(wl_bss_info_t));

    result = wwd_wifi_get_associated_client_list(clients, size);
    if ( result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Failed to get client list\n"));
        free( buffer );
        return result;
    }

    current = &clients->mac_list[0];
    wwd_wifi_get_ap_info( &ap_info, &sec );

    while ((clients->count > 0) && (!(NULL_MAC(current->octet))))
    {
        if (memcmp(current->octet, &(ap_info.BSSID), sizeof(wiced_mac_t) ) != 0)
        {
            WPRINT_APP_INFO(("Deauthenticating STA MAC: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", current->octet[0], current->octet[1], current->octet[2], current->octet[3], current->octet[4], current->octet[5]));
            result = wwd_wifi_deauth_sta(current, reason, interface );
            if ( result  != WICED_SUCCESS)
            {
                WPRINT_APP_INFO(("Failed to deauth client\n"));
            }
        }

        --clients->count;
        ++current;
    }

    free( buffer );

    return WWD_SUCCESS;
}

int get_associated_sta_list( int argc, char* argv[] )
{
    uint8_t* buffer = NULL;
    wiced_maclist_t * clients = NULL;
    const wiced_mac_t * current;
    wiced_result_t      result;
    wl_bss_info_t ap_info;
    wiced_security_t sec;
    uint32_t max_associations = 0;
    size_t size = 0;
    int32_t rssi = 0;

    if ((result = wwd_wifi_get_max_associations( &max_associations )) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Failed to get max number of associations\n"));
        max_associations = 5;
    }
    else
    {
        WPRINT_APP_INFO(("Max number of associations: %u\n", (unsigned int)max_associations));
    }

    size = (sizeof(uint32_t) + (max_associations * sizeof(wiced_mac_t)));
    buffer = calloc(1, size);

    if (buffer == NULL)
    {
        WPRINT_APP_INFO(("Unable to allocate memory for associations list\n"));
        return WICED_ERROR;
    }
    clients = (wiced_maclist_t*)buffer;
    wwd_wifi_get_associated_client_list(clients, size);

    memset(&ap_info, 0, sizeof(wl_bss_info_t));
    if (wwd_wifi_is_ready_to_transceive(WICED_STA_INTERFACE) == WWD_SUCCESS)
    {
        wwd_wifi_get_ap_info( &ap_info, &sec );
        if (clients->count == 0 )
        {
            clients->count = 1;
            memcpy(&clients->mac_list[0], &ap_info.BSSID, sizeof(wl_ether_addr_t));
        }
    }

    WPRINT_APP_INFO(("Current number of associated STAs: %u\n", (unsigned int)clients->count));
    current = &clients->mac_list[0];
    WPRINT_APP_INFO(("\n"));
    while ((clients->count > 0) && (!(NULL_MAC(current->octet))))
    {
        WPRINT_APP_INFO(("%02x:%02x:%02x:%02x:%02x:%02x ",
                            current->octet[0],
                            current->octet[1],
                            current->octet[2],
                            current->octet[3],
                            current->octet[4],
                            current->octet[5]));
        if (memcmp(current->octet, &(ap_info.BSSID), sizeof(wiced_mac_t)) != 0)
        {
            wwd_wifi_get_ap_client_rssi(&rssi, (wiced_mac_t*)&current->octet[0]);
            WPRINT_APP_INFO(("%3lddBm  Client\n", rssi));
        }
        else
        {
            result = wwd_wifi_get_rssi( &rssi );
            WPRINT_APP_INFO(("%3lddBm  AP\n", rssi));
        }
        --clients->count;
        ++current;
    }
    WPRINT_APP_INFO(("\n"));
    besl_host_free( buffer );
    return ERR_CMD_OK;
}

int test_ap( int argc, char* argv[] )
{
    int i;
    int iterations;

    if (  argc < 6 )
    {
        return ERR_UNKNOWN;
    }
    iterations = atoi(argv[argc - 1]);
    WPRINT_APP_INFO(("Iterations: %d\n", iterations));
    for (i = 0; i < iterations; i++ )
    {
        WPRINT_APP_INFO(( "Iteration %d\n", i));
        start_ap( argc-1, argv );
        stop_ap( 0, NULL );
    }
    wiced_mac_t mac;
    if ( wwd_wifi_get_mac_address( &mac, WWD_STA_INTERFACE ) == WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Test Pass (MAC address is: %02X:%02X:%02X:%02X:%02X:%02X)\n", mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]));
    }
    else
    {
        WPRINT_APP_INFO(("Test Fail\n"));
    }
    return ERR_CMD_OK;
}

int test_join( int argc, char* argv[] )
{
    int i;
    int iterations;
    uint32_t join_fails = 0, leave_fails = 0;

    if (  argc < 5 )
    {
        return ERR_UNKNOWN;
    }
    iterations = atoi(argv[argc - 1]);

    for (i = 0; i < iterations; i++ )
    {
        WPRINT_APP_INFO(( "%d ", i));
        if ( join( argc-1, argv ) != ERR_CMD_OK)
        {
            ++join_fails;
        }
        if ( leave( 0, NULL ) != ERR_CMD_OK )
        {
            ++leave_fails;
        }
    }

    WPRINT_APP_INFO(("Join failures: %u\n", (unsigned int)join_fails));
    WPRINT_APP_INFO(("Leave failures: %u\n", (unsigned int)leave_fails));

    return ERR_CMD_OK;
}

int test_join_specific( int argc, char* argv[] )
{
    int i;
    int iterations;
    uint32_t join_fails = 0, leave_fails = 0;

    if (  argc < 5 )
    {
        return ERR_UNKNOWN;
    }
    iterations = atoi(argv[argc - 1]);

    for (i = 0; i < iterations; i++ )
    {
        WPRINT_APP_INFO(( "%d ", i));
        if ( join_specific( argc-1, argv ) != ERR_CMD_OK)
        {
            ++join_fails;
        }
        if ( leave( 0, NULL ) != ERR_CMD_OK )
        {
            ++leave_fails;
        }
    }

    WPRINT_APP_INFO(("Join specific failures: %u\n", (unsigned int)join_fails));
    WPRINT_APP_INFO(("Leave failures: %u\n", (unsigned int)leave_fails));

    return ERR_CMD_OK;
}

int test_credentials( int argc, char* argv[] )
{
    wwd_result_t result;
    wiced_scan_result_t ap;

    memset(&ap, 0, sizeof(ap));

    ap.SSID.length = strlen(argv[1]);
    memcpy(ap.SSID.value, argv[1], ap.SSID.length);
    str_to_mac(argv[2], &ap.BSSID);
    ap.channel = atoi(argv[3]);
    ap.security = str_to_authtype(argv[4]);
    result = wwd_wifi_test_credentials(&ap, (uint8_t*)argv[5], strlen(argv[5]));

    if ( result == WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Credentials are good\n"));
    }
    else
    {
        WPRINT_APP_INFO(("Credentials are bad\n"));
    }

    return ERR_CMD_OK;
}

int get_soft_ap_credentials( int argc, char* argv[] )
{
    wiced_security_t sec;
    platform_dct_wifi_config_t* dct_wifi_config;

    if ( wwd_wifi_is_ready_to_transceive( WICED_AP_INTERFACE ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Use start_ap command to bring up AP interface first\n"));
        return ERR_CMD_OK;
    }

    /* Read config to get internal AP settings */
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
    WPRINT_APP_INFO(("SSID : %s\n", (char*)dct_wifi_config->soft_ap_settings.SSID.value));
    sec = dct_wifi_config->soft_ap_settings.security;
    WPRINT_APP_INFO( ( "Security : %s\n", ( sec == WICED_SECURITY_OPEN )           ? "Open" :
                                            ( sec == WICED_SECURITY_WEP_PSK )        ? "WEP" :
                                            ( sec == WICED_SECURITY_WPA_TKIP_PSK )   ? "WPA TKIP" :
                                            ( sec == WICED_SECURITY_WPA_AES_PSK )    ? "WPA AES" :
                                            ( sec == WICED_SECURITY_WPA2_AES_PSK )   ? "WPA2 AES" :
                                            ( sec == WICED_SECURITY_WPA2_TKIP_PSK )  ? "WPA2 TKIP" :
                                            ( sec == WICED_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
                                            "Unknown" ) );
    WPRINT_APP_INFO(("Passphrase : %s\n", last_soft_ap_passphrase));

    wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_FALSE );

    return ERR_CMD_OK;
}

int get_pmk( int argc, char* argv[] )
{
    char pmk[64];

    if ( wwd_wifi_get_pmk( argv[1], strlen( argv[1] ), pmk ) == WWD_SUCCESS )
    {
        WPRINT_APP_INFO( ("%s\n", pmk) );
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int get_counters( int argc, char* argv[] )
{
    UNUSED_PARAMETER( argc );
    UNUSED_PARAMETER( argv );
    wwd_interface_t interface = WWD_STA_INTERFACE;
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wiced_counters_t* data;

    wwd_sdpcm_get_iovar_buffer(&buffer, sizeof(*data), IOVAR_STR_COUNTERS);

    if (wwd_sdpcm_send_iovar(SDPCM_GET, buffer, &response, interface) == WWD_SUCCESS)
    {
        data = (wiced_counters_t*)host_buffer_get_current_piece_data_pointer(response);
        WPRINT_APP_INFO(("Received frames with failed PLCP parity check (PHY layer) = %u\n",(unsigned int)data->rxbadplcp));
        WPRINT_APP_INFO(("Received frames with good PLCP header                     = %u\n",(unsigned int)data->rxstrt));
        WPRINT_APP_INFO(("Received frames with failed CRC check (MAC layer)         = %u\n",(unsigned int)data->rxbadfcs));
        WPRINT_APP_INFO(("Received frames with good PLCP header and good CRC        = %u\n",(unsigned int)(data->rxstrt - data->rxbadfcs)));
        host_buffer_release(response, WWD_NETWORK_RX);
    }
    else
    {
        return ERR_UNKNOWN;
    }
    return ERR_CMD_OK;
}

int get_ap_info( int argc, char* argv[] )
{
    wl_bss_info_t ap_info;
    wiced_security_t sec;

    if ( wwd_wifi_get_ap_info( &ap_info, &sec ) == WWD_SUCCESS )
    {
        WPRINT_APP_INFO( ("SSID  : %s\n", (char*)ap_info.SSID ) );
        WPRINT_APP_INFO( ("BSSID : %02X:%02X:%02X:%02X:%02X:%02X\n", ap_info.BSSID.octet[0], ap_info.BSSID.octet[1], ap_info.BSSID.octet[2], ap_info.BSSID.octet[3], ap_info.BSSID.octet[4], ap_info.BSSID.octet[5]) );
        WPRINT_APP_INFO( ("RSSI  : %d\n", ap_info.RSSI) );
        WPRINT_APP_INFO( ("SNR   : %d\n", ap_info.SNR) );
        WPRINT_APP_INFO( ("Noise : %d\n", ap_info.phy_noise) );
        WPRINT_APP_INFO( ("Beacon period : %u\n", ap_info.beacon_period) );
        WPRINT_APP_INFO( ( "Security : %s\n", ( sec == WICED_SECURITY_OPEN )           ? "Open" :
                                                ( sec == WICED_SECURITY_WEP_PSK )        ? "WEP" :
                                                ( sec == WICED_SECURITY_WPA_TKIP_PSK )   ? "WPA TKIP" :
                                                ( sec == WICED_SECURITY_WPA_AES_PSK )    ? "WPA AES" :
                                                ( sec == WICED_SECURITY_WPA2_AES_PSK )   ? "WPA2 AES" :
                                                ( sec == WICED_SECURITY_WPA2_TKIP_PSK )  ? "WPA2 TKIP" :
                                                ( sec == WICED_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
                                                "Unknown" ) );
    }
    else
    {
        return ERR_UNKNOWN;
    }
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Leaves an associated access point
 *
 * @return  0 for success, otherwise error
 */

int leave( int argc, char* argv[] )
{
    return wiced_network_down( WWD_STA_INTERFACE );
}

/*!
 ******************************************************************************
 * Prints the device MAC address
 *
 * @return  0 for success, otherwise error
 */

int get_mac_addr( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wiced_mac_t mac;
    wwd_interface_t interface = WWD_STA_INTERFACE;

    memset(&mac, 0, sizeof( wiced_mac_t));

    CHECK_IOCTL_BUFFER( wwd_sdpcm_get_iovar_buffer( &buffer, sizeof(wiced_mac_t), IOVAR_STR_CUR_ETHERADDR ) );

    if (argc == 2 && argv[1][0] == '1')
    {
        interface = WWD_AP_INTERFACE;
    }

    if ( wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, interface ) == WWD_SUCCESS )
    {
        memcpy( mac.octet, host_buffer_get_current_piece_data_pointer( response ), sizeof(wiced_mac_t) );
        host_buffer_release( response, WWD_NETWORK_RX );
    }
    WPRINT_APP_INFO(("MAC address is: %02X:%02X:%02X:%02X:%02X:%02X\n", mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]));
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Enables or disables power save mode as specified by the arguments
 *
 * @return  0 for success, otherwise error
 */

int wifi_powersave( int argc, char* argv[] )
{
    int a = atoi( argv[1] );

    switch( a )
    {
        case 0:
        {
            if ( wwd_wifi_disable_powersave( ) != WWD_SUCCESS )
            {
                WPRINT_APP_INFO( ("Failed to disable Wi-Fi powersave\n") );
            }
            break;
        }

        case 1:
        {
            if ( wwd_wifi_enable_powersave( ) != WWD_SUCCESS )
            {
                WPRINT_APP_INFO( ("Failed to enable Wi-Fi powersave\n") );
            }
            break;
        }

        case 2:
        {
            uint8_t return_to_sleep_delay_ms = (uint8_t) atoi( argv[ 2 ] );

            if ( wwd_wifi_enable_powersave_with_throughput( return_to_sleep_delay_ms ) != WWD_SUCCESS )
            {
                WPRINT_APP_INFO( ("Failed to enable Wi-Fi powersave with throughput\n") );
            }
            break;
        }

        default:
            return ERR_UNKNOWN_CMD;

    }

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Enables or disables power save mode as specified by the arguments
 *
 * @return  0 for success, otherwise error
 */

int set_wifi_powersave_mode( int argc, char* argv[] )
{
    int a = atoi( argv[1] );

    switch( a )
    {
        case 0:
        {
            if ( wiced_wifi_disable_powersave( ) != WICED_SUCCESS )
            {
                WPRINT_APP_INFO( ("Failed to disable Wi-Fi powersave\n") );
            }
            break;
        }

        case 1:
        {
            if ( wiced_wifi_enable_powersave( ) != WICED_SUCCESS )
            {
                WPRINT_APP_INFO( ("Failed to enable Wi-Fi powersave\n") );
            }
            break;
        }

        case 2:
        {
            uint8_t return_to_sleep_delay_ms = (uint8_t) atoi( argv[ 2 ] );

            if ( wiced_wifi_enable_powersave_with_throughput( return_to_sleep_delay_ms ) != WICED_SUCCESS )
            {
                WPRINT_APP_INFO( ("Failed to enable Wi-Fi powersave with throughput\n") );
            }
            break;
        }

        default:
            return ERR_UNKNOWN_CMD;

    }

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Sets the STA listen interval as specified by the arguments
 *
 * @return  0 for success, otherwise error
 */

int set_listen_interval( int argc, char* argv[] )
{
    int listen_interval;
    int time_unit;

    if ( argc < 3 )
    {
        return ERR_UNKNOWN_CMD;
    }

    /* No bounds checking as console app user cannot know beacon interval or DTIM interval without a sniffer */
    listen_interval = atoi( argv[1] );

    time_unit = atoi( argv[2] );
    if ( ( time_unit != WICED_LISTEN_INTERVAL_TIME_UNIT_BEACON ) && ( time_unit != WICED_LISTEN_INTERVAL_TIME_UNIT_DTIM ) )
    {
        WPRINT_APP_INFO( ("0 for units in Beacon Intervals, 1 for units in DTIM intervals\n") );
        return ERR_UNKNOWN_CMD;
    }

    if ( wiced_wifi_set_listen_interval( (uint8_t)listen_interval, (uint8_t)time_unit ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to set listen interval\n") );
    }

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Sets the transmit power as specified in arguments in dBm
 *
 * @return  0 for success, otherwise error
 */

int set_tx_power( int argc, char* argv[] )
{
    int dbm = atoi( argv[1] );
    return (wwd_wifi_set_tx_power(dbm) != WWD_SUCCESS );
}

/*!
 ******************************************************************************
 * Gets the current transmit power in dBm
 *
 * @return  0 for success, otherwise error
 */

int get_tx_power( int argc, char* argv[] )
{
    wwd_result_t result;
    uint8_t dbm;

    if ( WWD_SUCCESS != ( result = wwd_wifi_get_tx_power( &dbm ) ) )
    {
        return result;
    }

    WPRINT_APP_INFO(("Transmit Power : %ddBm\n", dbm ));

    return result;
}

/*!
 ******************************************************************************
 * Prints the latest RSSI value
 *
 * @return  0 for success, otherwise error
 */

int get_rssi( int argc, char* argv[] )
{
    int32_t rssi;
    wwd_wifi_get_rssi( &rssi );
    WPRINT_APP_INFO(("RSSI is %d\n", (int)rssi));
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Prints the latest PHY Noise value
 *
 * @return  0 for success, otherwise error
 */

int get_noise( int argc, char* argv[] )
{
    int32_t noise;
    wwd_wifi_get_noise( &noise );
    WPRINT_APP_INFO(("NOISE is %d\r\n", (int)noise));
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Returns the status of the Wi-Fi interface
 *
 * @return  0 for success, otherwise error
 */

int status( int argc, char* argv[] )
{
    wiced_mac_t       mac;
    wiced_interface_t interface;

    wiced_wifi_get_mac_address( &mac );
    WPRINT_APP_INFO(("WICED Version  : " WICED_VERSION "\n"));
    WPRINT_APP_INFO(("Platform       : " PLATFORM "\n"));
    WPRINT_APP_INFO(("MAC Address    : %02X:%02X:%02X:%02X:%02X:%02X\n\n", mac.octet[0],mac.octet[1],mac.octet[2],mac.octet[3],mac.octet[4],mac.octet[5]));

    for ( interface = 0; interface < WICED_INTERFACE_MAX; interface++ )
    {
        if ( wiced_network_is_ip_up( interface ) )
        {
            switch ( interface )
            {
                case WICED_STA_INTERFACE:
                {
                    wwd_wifi_get_mac_address( &mac, WWD_STA_INTERFACE );
                    WPRINT_APP_INFO( ( "STA Interface\n"));
                    WPRINT_APP_INFO( ( "   MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n", mac.octet[0],mac.octet[1],mac.octet[2],mac.octet[3],mac.octet[4],mac.octet[5]));
                    WPRINT_APP_INFO( ( "   SSID        : %s\n", last_joined_ssid ) );
                    break;
                }

                case WICED_AP_INTERFACE:
                {
                    wwd_wifi_get_mac_address( &mac, WWD_AP_INTERFACE );
                    WPRINT_APP_INFO( ( "AP Interface\n"));
                    WPRINT_APP_INFO( ( "   MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n", mac.octet[0],mac.octet[1],mac.octet[2],mac.octet[3],mac.octet[4],mac.octet[5]));
                    WPRINT_APP_INFO( ( "   SSID        : %s\n", last_started_ssid ) );
                    break;
                }

#ifdef COMMAND_CONSOLE_P2P_ENABLED
                case WICED_P2P_INTERFACE:
                {
                    char group_owner[]  = "Group Owner";
                    char group_client[] = "Group Client";
                    char* role          = group_owner;

                    wwd_wifi_get_mac_address( &mac, WWD_P2P_INTERFACE );
                    if ( besl_p2p_group_owner_is_up( ) == WICED_FALSE )
                    {
                        role = group_client;
                    }
                    WPRINT_APP_INFO( ( "P2P Interface\n"));
                    WPRINT_APP_INFO( ( "   Role        : %s\n", role ) );
                    WPRINT_APP_INFO( ( "   MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n", mac.octet[0],mac.octet[1],mac.octet[2],mac.octet[3],mac.octet[4],mac.octet[5]));
                    WPRINT_APP_INFO( ( "   SSID        : %s\n", p2p_workspace.group_candidate.ssid ) );
                    break;
                }
#endif

#ifdef WICED_USE_ETHERNET_INTERFACE
                case WICED_ETHERNET_INTERFACE:
                {
                    WPRINT_APP_INFO( ( "Ethernet Interface\r\n"));
                    /* XXX need to read the MAC address and print it out */
                    break;
                }
#endif
                default:
                    break;
            }
            network_print_status( interface );
        }
        else
        {
            switch ( interface )
            {
                case WICED_STA_INTERFACE:
                    WPRINT_APP_INFO( ( "STA Interface  : Down\n") );
                    break;

                case WICED_AP_INTERFACE:
                    WPRINT_APP_INFO( ( "AP Interface   : Down\n") );
                    break;

#ifdef COMMAND_CONSOLE_P2P_ENABLED
                case WICED_P2P_INTERFACE:
                    WPRINT_APP_INFO( ( "P2P Interface  : Down\n") );
                    break;
#endif

#ifdef WICED_USE_ETHERNET_INTERFACE
                case WICED_ETHERNET_INTERFACE:
                    WPRINT_APP_INFO( ( "Ethernet Interface  : Down\n") );
                    break;
#endif

                default:
                    break;
            }
        }
    }

    return ERR_CMD_OK;
}


int antenna( int argc, char* argv[] )
{
    uint32_t value;
    string_to_unsigned( argv[1], strlen(argv[1]), &value, 0);
    if ( ( value == WICED_ANTENNA_1 ) || ( value == WICED_ANTENNA_2 ) || ( value == WICED_ANTENNA_AUTO ) )
    {
        if ( wwd_wifi_select_antenna( (wiced_antenna_t) value ) == WWD_SUCCESS )
        {
            return ERR_CMD_OK;
        }
    }
    return ERR_UNKNOWN;
}

int ant_sel( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t value;
    string_to_unsigned( argv[1], strlen(argv[1]), &value, 0);
    wlc_antselcfg_t* sel = (wlc_antselcfg_t*)wwd_sdpcm_get_iovar_buffer(&buffer, sizeof(wlc_antselcfg_t), "nphy_antsel");
    CHECK_IOCTL_BUFFER( sel );
    sel->ant_config[0] = value;
    sel->ant_config[1] = value;
    sel->ant_config[2] = value;
    sel->ant_config[3] = value;
    sel->num_antcfg = 0;
    if (wwd_sdpcm_send_iovar(SDPCM_SET, buffer, NULL, WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int antdiv( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t* data = wwd_sdpcm_get_ioctl_buffer(&buffer, sizeof(uint32_t));
    CHECK_IOCTL_BUFFER( data );
    uint32_t tmp_val;
    string_to_unsigned( argv[1], strlen(argv[1]), &tmp_val, 0 );
    *data = tmp_val;
    if (wwd_sdpcm_send_ioctl(SDPCM_SET, WLC_SET_ANTDIV, buffer, NULL, WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int txant( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t* data = wwd_sdpcm_get_ioctl_buffer(&buffer, sizeof(uint32_t));
    CHECK_IOCTL_BUFFER( data );
    uint32_t tmp_val;
    string_to_unsigned( argv[1], strlen(argv[1]), &tmp_val, 0 );
    *data = tmp_val;
    if (wwd_sdpcm_send_ioctl(SDPCM_SET, WLC_SET_TXANT, buffer, NULL, WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int ucantdiv( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t* data = wwd_sdpcm_get_ioctl_buffer(&buffer, sizeof(uint32_t));
    CHECK_IOCTL_BUFFER( data );
    uint32_t tmp_val;
    string_to_unsigned( argv[1], strlen(argv[1]), &tmp_val, 0 );
    *data = tmp_val;
    if (wwd_sdpcm_send_ioctl(SDPCM_SET, WLC_SET_UCANTDIV, buffer, NULL, WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int get_country( int argc, char* argv[] )
{
    /* Get country information and print the abbreviation */
    wl_country_t cspec;
    wiced_buffer_t buffer;
    wiced_buffer_t response;

    wl_country_t* temp = (wl_country_t*)wwd_sdpcm_get_iovar_buffer( &buffer, sizeof( wl_country_t ), "country" );
    CHECK_IOCTL_BUFFER( temp );
    memset( temp, 0, sizeof(wl_country_t) );
    wwd_result_t result = wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, WWD_STA_INTERFACE );

    if (result == WWD_SUCCESS)
    {
        memcpy( (char *)&cspec, (char *)host_buffer_get_current_piece_data_pointer( response ), sizeof(wl_country_t) );
        host_buffer_release(response, WWD_NETWORK_RX);
        char* c = (char*)&(cspec.country_abbrev);
        WPRINT_APP_INFO(( "Country is %s\n", c ));

    }
    else
    {
        WPRINT_APP_INFO(("country iovar not supported, trying ioctl\n"));
        temp = (wl_country_t*) wwd_sdpcm_get_ioctl_buffer( &response, sizeof(wl_country_t) );
        CHECK_IOCTL_BUFFER( temp );
        memset( temp, 0, sizeof( wl_country_t ) );
        result = wwd_sdpcm_send_ioctl( SDPCM_GET, WLC_GET_COUNTRY, buffer, &response, WWD_STA_INTERFACE );
        if ( result == WWD_SUCCESS )
        {
            memcpy( (char *)&cspec, (char *)host_buffer_get_current_piece_data_pointer( response ), sizeof(wl_country_t) );
            host_buffer_release(response, WWD_NETWORK_RX);
        }
    }

    return result;
}

int get_rate (int argc, char* argv[])
{
    uint32_t rate;
    wwd_wifi_get_rate(WWD_STA_INTERFACE, &rate);

    if (rate == 0)
        WPRINT_APP_INFO(("auto"));
    else if (rate > 1000) /* this indicates that units are kbps */
        WPRINT_APP_INFO(("%u Kbps", (unsigned int)rate));
    else
        WPRINT_APP_INFO(("%u%s Mbps", ((unsigned int)rate / 2), ((unsigned int)rate & 1) ? ".5" : ""));

    WPRINT_APP_INFO(("\n"));

    return ERR_CMD_OK;
}

int set_legacy_rate (int argc, char* argv[])
{
    uint32_t rate;

    rate = (uint32_t)(2 * atof( argv[1] ));

    if (WWD_SUCCESS != wwd_wifi_set_legacy_rate(WWD_STA_INTERFACE, rate))
        WPRINT_APP_INFO(("Invalid legacy rate rate specification\n"));

    return ERR_CMD_OK;
}

int disable_11n (int argc, char* argv[])
{
    if (WWD_SUCCESS != wwd_wifi_set_11n_support(WWD_STA_INTERFACE, WICED_11N_SUPPORT_DISABLED))
        WPRINT_APP_INFO(("Cannot disable 11n mode\n"));

    return ERR_CMD_OK;
}

int enable_11n (int argc, char* argv[])
{
    if (WWD_SUCCESS != wwd_wifi_set_11n_support(WWD_STA_INTERFACE, WICED_11N_SUPPORT_ENABLED))
        WPRINT_APP_INFO(("Cannot enable 11n mode\n"));

    return ERR_CMD_OK;
}

int set_mcs_rate (int argc, char* argv[])
{
    int32_t mcs;
    wiced_bool_t mcsonly = WICED_FALSE;

    mcs = (int32_t)(atoi( argv[1] ));
    if (argc == 3)
        mcsonly = (wiced_bool_t)(atoi( argv[2]));

    if (WWD_SUCCESS != wwd_wifi_set_mcs_rate(WWD_STA_INTERFACE, mcs, mcsonly))
        WPRINT_APP_INFO(("Invalid MCS rate specification\n"));

    return ERR_CMD_OK;
}

int set_data_rate( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t*          data;

    data = wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, "bg_rate" );
    CHECK_IOCTL_BUFFER( data );

    /* Set data to 2 * <rate> as legacy rate unit is in 0.5Mbps */
    *data = (uint32_t)(2 * atof( argv[1] ));

    if ( wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_STA_INTERFACE ) != WWD_SUCCESS )
    {
        return ERR_UNKNOWN;
    }

    return ERR_CMD_OK;
}

int get_data_rate( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    uint32_t*          data;

    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, "bg_rate" );
    CHECK_IOCTL_BUFFER( data );

    if ( wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, WWD_STA_INTERFACE ) != WWD_SUCCESS )
    {
        return ERR_UNKNOWN;
    }

    data = (uint32_t*) host_buffer_get_current_piece_data_pointer( response );

    /* 5.5 Mbps */
    if ( *data == 11 )
    {
        WPRINT_APP_INFO(( "data rate: 5.5 Mbps\n\r" ));
    }
    else
    {
        WPRINT_APP_INFO(( "data rate: %d Mbps\n\r", (int)(*data / 2) ));

    }

    host_buffer_release( response, WWD_NETWORK_RX );
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Interface to the wwd_wifi_get_random() function. Prints result
 *
 * @return  0 for success, otherwise error
 */
int get_random( int argc, char* argv[] )
{
    uint8_t random[64];
    if ( wwd_wifi_get_random( random, 64 ) == WWD_SUCCESS )
    {
        int a;
        WPRINT_APP_INFO(("Random data is 0x"));
        for (a=0; a<64; ++a)
        {
            WPRINT_APP_INFO(("%.2x", random[a]));
        }
        WPRINT_APP_INFO(("\n"));
        return ERR_CMD_OK;
    }

    return ERR_UNKNOWN;
}

/*!
 ******************************************************************************
 * Get the access categories being used in STA mode
 *
 * @return  0 for success
 */
int get_access_category_parameters_sta( int argc, char* argv[] )
{
    edcf_acparam_t ac_params[AC_COUNT];
    int ac_priority[AC_COUNT];

    if ( wwd_wifi_get_acparams_sta( ac_params ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(( "Error reading EDCF AC Parameters\n"));
    }

    wwd_wifi_prioritize_acparams( ac_params, ac_priority ); // Re-prioritize access categories to match AP configuration
    ac_params_print( ac_params, ac_priority );

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Read WLAN console buffer and dump wlan console log
 *
 * @return  0 for success, otherwise error
 */
int readconsole( int argc, char* argv[] )
{
    const unsigned buffer_size = 200;
    int result = ERR_UNKNOWN;

    char* buffer = malloc_named( "console", buffer_size );
    if ( buffer == NULL )
    {
        return result;
    }

    if ( wwd_wifi_read_wlan_log( buffer, buffer_size ) == WWD_SUCCESS )
    {
        result = ERR_CMD_OK;
    }

    free( buffer );

    return result;
}

void dump_bytes(const uint8_t* bptr, uint32_t len)
{
    int i = 0;

    for (i = 0; i < len; )
    {
        if ((i & 0x0f) == 0)
        {
            WPRINT_APP_INFO( ( "\n" ) );
        }
        else if ((i & 0x07) == 0)
        {
            WPRINT_APP_INFO( (" ") );
        }
        WPRINT_APP_INFO( ( "%02x ", bptr[i++] ) );
    }
    WPRINT_APP_INFO( ( "\n" ) );
}

void ac_params_print( const wiced_edcf_ac_param_t *acp, const int *priority )
{
    int aci;
    int acm, aifsn, ecwmin, ecwmax, txop;
    static const char ac_names[AC_COUNT][6] = {"AC_BE", "AC_BK", "AC_VI", "AC_VO"};

    if ( acp != NULL )
    {
        for (aci = 0; aci < AC_COUNT; aci++, acp++)
        {
            if (((acp->ACI & EDCF_ACI_MASK) >> EDCF_ACI_SHIFT) != aci)
            {
                WPRINT_APP_INFO(("Warning: AC params out of order\n"));
            }
            acm = (acp->ACI & EDCF_ACM_MASK) ? 1 : 0;
            aifsn = acp->ACI & EDCF_AIFSN_MASK;
            ecwmin = acp->ECW & EDCF_ECWMIN_MASK;
            ecwmax = (acp->ECW & EDCF_ECWMAX_MASK) >> EDCF_ECWMAX_SHIFT;
            txop = (uint16_t)acp->TXOP;
            WPRINT_APP_INFO(("%s: raw: ACI 0x%x ECW 0x%x TXOP 0x%x\n", ac_names[aci], acp->ACI, acp->ECW, txop));
            WPRINT_APP_INFO(("       dec: aci %d acm %d aifsn %d " "ecwmin %d ecwmax %d txop 0x%x\n", aci, acm, aifsn, ecwmin, ecwmax, txop) );
                /* CWmin = 2^(ECWmin) - 1 */
                /* CWmax = 2^(ECWmax) - 1 */
                /* TXOP = number of 32 us units */
            WPRINT_APP_INFO(("       eff: CWmin %d CWmax %d TXop %dusec\n", EDCF_ECW2CW(ecwmin), EDCF_ECW2CW(ecwmax), EDCF_TXOP2USEC(txop)));
        }
    }

    if ( priority != NULL )
    {
        for (aci = 0; aci < AC_COUNT; aci++, priority++)
        {
            WPRINT_APP_INFO(("%s: ACI %d Priority %d\n", ac_names[aci], aci, *priority));
        }
    }
}


/*!
 ******************************************************************************
 * Convert a security authentication type string to a wiced_security_t.
 *
 * @param[in] arg  The string containing the value.
 *
 * @return    The value represented by the string.
 */
wiced_security_t str_to_authtype( char* arg )
{
    if ( strcmp( arg, "open" ) == 0 )
    {
        return WICED_SECURITY_OPEN;
    }
    else if ( strcmp( arg, "wep" ) == 0 )
    {
        return WICED_SECURITY_WEP_PSK;
    }
    else if ( strcmp( arg, "wep_shared" ) == 0 )
    {
        return WICED_SECURITY_WEP_SHARED;
    }
    else if ( strcmp( arg, "wpa2_tkip" ) == 0 )
    {
        return WICED_SECURITY_WPA2_TKIP_PSK;
    }
    else if ( strcmp( arg, "wpa2_aes" ) == 0 )
    {
        return WICED_SECURITY_WPA2_AES_PSK;
    }
    else if ( strcmp( arg, "wpa2" ) == 0 )
    {
        return WICED_SECURITY_WPA2_MIXED_PSK;
    }
    else if ( strcmp( arg, "wpa_aes" ) == 0 )
    {
        return WICED_SECURITY_WPA_AES_PSK;
    }
    else if ( strcmp( arg, "wpa_tkip" ) == 0 )
    {
        return WICED_SECURITY_WPA_TKIP_PSK;
    }
    else
    {
        printf ("Bad auth type: '%s'\r\n", arg);
        return WICED_SECURITY_UNKNOWN;
    }
}


/*!
 ******************************************************************************
 * Convert a security authentication type string to an enterprise wiced_security_t.
 *
 * @param[in] arg  The string containing the value.
 *
 * @return    The value represented by the string.
 */
wiced_security_t str_to_enterprise_authtype( char* arg )
{
    if ( strcmp( arg, "wpa2_tkip" ) == 0 )
    {
        return WICED_SECURITY_WPA2_TKIP_ENT;
    }
    else if ( strcmp( arg, "wpa2" ) == 0 )
    {
        return WICED_SECURITY_WPA2_MIXED_ENT;
    }
    else if ( strcmp( arg, "wpa" ) == 0 )
    {
        return WICED_SECURITY_WPA_MIXED_ENT;
    }
    else if ( strcmp( arg, "wpa_tkip" ) == 0 )
    {
        return WICED_SECURITY_WPA_TKIP_ENT;
    }
    else
    {
        printf ("Bad auth type: '%s'\r\n", arg);
        return WICED_SECURITY_UNKNOWN;
    }
}


/*!
 ******************************************************************************
 * Convert a MAC string (xx:xx:xx:xx:xx) to a wiced_mac_t.
 *
 * @param[in] arg  The string containing the value.
 *
 * @return    The value represented by the string.
 */
void str_to_mac( char* arg, wiced_mac_t* mac )
{
    char* start = arg;
    char* end;
    int a = 0;
    do
    {
        uint32_t tmp_val;
        end = strchr( start, ':' );
        if ( end != NULL )
        {
            *end = '\0';
        }
        string_to_unsigned( start, 2, &tmp_val, 1);
        mac->octet[a] = (uint8_t) tmp_val;
        if ( end != NULL )
        {
            *end = ':';
            start = end + 1;
        }
        ++a;
    } while ( a < 6 && end != NULL );
}

