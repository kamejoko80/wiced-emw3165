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
 * NFC token config client snippet application
 *
 * This application demonstrates how to use NFC to read access point credentials
 * from an NFC tag for easy association.
 *
 * This is an implementation of the Wi-Fi Simple Configuration (WSC) NFC
 * Configuration Token process as described in section 10.2.2 of the
 * Wi-Fi Simple Configuration Technical Specification from the perspective
 * of a client.
 *
 * Features demonstrated
 *  - Reading the WSC Configuration Token from an NFC tag and storing the
 *    access point credentials into the DCT
 *
 * Application Instructions
 *   1. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *   2. Prepare an NFC tag with configuration token data.
 *      The config_token_server application can be used to do this.
 *   3. Tap the NFC tag to the NFC capable WICED device
 *      Note: Connection progress is printed to the console
 */

#include "wiced.h"
#include "wps_host_interface.h"
#include "wiced_nfc_api.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define NFC_READ_TIMEOUT               ( 60 * SECONDS )
#define NFC_DATA_LENGTH                ( 250 )
#define NDEF_RECORD_HEADER             ( 0xD2 )
#define NDEF_WSC_TYPE_STRING           "application/vnd.wfa.wsc"

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)

typedef struct
{
    uint8_t header;
    uint8_t wsc_type_length;
    uint8_t _padding;
    char    wsc_type[sizeof( NDEF_WSC_TYPE_STRING ) - 1];
    uint8_t attribute_data[1];
} nfc_ndef_record_t;

typedef struct
{
    uint16_t id;
    uint16_t length;
    uint8_t  data[1];
} nfc_ndef_wsc_attribute_t;

typedef struct
{
    uint16_t id;
    uint16_t length;
} nfc_ndef_wsc_attribute_header_t;

#pragma pack()

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t process_nfc_ndef_record( const uint8_t* buffer, wiced_wps_credential_t* credential );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_nfc_workspace_t nfc_workspace;

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_wps_credential_t      credential[1];
    platform_dct_wifi_config_t* wifi_config;
    wiced_result_t              result;
    uint32_t                    i;
    uint8_t                     buffer[NFC_DATA_LENGTH];
    uint32_t                    buffer_length = NFC_DATA_LENGTH;

    memset( &credential, 0, sizeof( credential ) );

    /* Initialize WICED */
    wiced_init( );

    /* Initialize NFC.
     * Note: This can take up to 8 seconds to complete
     */
    WPRINT_APP_INFO(("\nInitializing NFC...\n"));
    if ( wiced_nfc_init( &nfc_workspace ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("NFC error\n") );
        return;
    }

    WPRINT_APP_INFO( ( "Waiting for tag...\n" ) );

    do
    {
        /* Read NFC tag */
        result = wiced_nfc_read_tag( buffer, &buffer_length, NFC_READ_TIMEOUT );
        if (result == WICED_SUCCESS)
        {
            /* Process the config token */
            result = process_nfc_ndef_record( buffer, credential );
            if ( result != WICED_SUCCESS )
            {
                WPRINT_APP_INFO( ("Processing config token failed\n") );
            }
        }
        else if ( result == WICED_TIMEOUT )
        {
            WPRINT_APP_INFO( ( "Tag not detected\n" ) );
        }
    } while ( result != WICED_SUCCESS );

    WPRINT_APP_INFO( ("Config token read successfully\n") );

    /* Write Wi-Fi credentials obtained into the DCT */
    wiced_dct_read_lock( (void**) &wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof( platform_dct_wifi_config_t ) );
    for ( i = 0; i < 1; i++ )
    {
        memcpy( (void *) &wifi_config->stored_ap_list[ i ].details.SSID, &credential[ i ].ssid, sizeof(wiced_ssid_t) );
        memcpy( wifi_config->stored_ap_list[ i ].security_key, &credential[ i ].passphrase, credential[ i ].passphrase_length );

        wifi_config->stored_ap_list[ i ].details.security = credential[ i ].security;
        wifi_config->stored_ap_list[ i ].security_key_length = credential[ i ].passphrase_length;
    }
    wiced_dct_write( (const void*) wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
    wiced_dct_read_unlock( (void*) wifi_config, WICED_TRUE );

    /* Join with new credentials */
    result = wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to connect to access point\n") );
        return;
    }

    WPRINT_APP_INFO( ("Successfully connected to access point\n") );
}

static wiced_result_t process_nfc_ndef_record( const uint8_t* buffer, wiced_wps_credential_t* credential )
{
    uint8_t*                  iter;
    nfc_ndef_wsc_attribute_t* attribute;
    uint16_t                  authentication_type = 0;
    uint16_t                  encryption_type     = 0;
    nfc_ndef_record_t*        record              = (nfc_ndef_record_t*)buffer;

    if ( buffer == NULL )
    {
        return WICED_ERROR;
    }

    /* Verify the record is NDEF */
    if ( record->header != NDEF_RECORD_HEADER )
    {
        WPRINT_APP_INFO(( "Read error: No Wi-Fi NDEF REC found\n" ));
        return WICED_ERROR;
    }

    /* Verify the WSC type length */
    if ( record->wsc_type_length != sizeof( NDEF_WSC_TYPE_STRING ) - 1 )
    {
        WPRINT_APP_INFO( ( "Read error: WSC type length incorrect\n" ) );
        return WICED_ERROR;
    }

    /* Verify the WSC type */
    if ( memcmp( &record->wsc_type, NDEF_WSC_TYPE_STRING, sizeof( record->wsc_type ) ) != 0 )
    {
        WPRINT_APP_INFO( ( "Read error: WSC type incorrect\n" ) );
        return WICED_ERROR;
    }

    /* Move iterator to the attribute data */
    iter = record->attribute_data;

    attribute = (nfc_ndef_wsc_attribute_t*)iter;

    while ( htons(attribute->id) != WPS_ID_VENDOR_EXT )
    {
        switch ( htons(attribute->id) )
        {
            case WPS_ID_SSID:
                credential->ssid.length = htons(attribute->length);
                memcpy( credential->ssid.value, attribute->data, credential->ssid.length );
                break;

            case WPS_ID_AUTH_TYPE:
                authentication_type = attribute->data[ 1 ] | ( attribute->data[ 0 ] << 8 );
                break;

            case WPS_ID_NW_KEY:
                credential->passphrase_length = htons(attribute->length);
                memcpy( credential->passphrase, attribute->data, credential->passphrase_length );
                break;

            case WPS_ID_ENCR_TYPE:
                encryption_type = attribute->data[ 1 ] | ( attribute->data[ 0 ] << 8 );
                break;

            case WPS_ID_CREDENTIAL:
                /* Only skip the header to process the attributes inside */
                iter     += sizeof(nfc_ndef_wsc_attribute_header_t);
                attribute = (nfc_ndef_wsc_attribute_t*)iter;
                continue;

            default:
                break;
        }

        /* Skip to the next attribute */
        iter     += sizeof(nfc_ndef_wsc_attribute_header_t) + htons(attribute->length);
        attribute = (nfc_ndef_wsc_attribute_t*)iter;
    }

    switch ( authentication_type )
    {
        case WPS_OPEN_AUTHENTICATION:
            credential->security = ( encryption_type == WPS_NO_ENCRYPTION ) ? WICED_SECURITY_OPEN : WICED_SECURITY_WEP_PSK;
            break;
        case WPS_WPA_PSK_AUTHENTICATION:
            credential->security = ( encryption_type == WPS_TKIP_ENCRYPTION ) ? WICED_SECURITY_WPA_TKIP_PSK : WICED_SECURITY_WPA_AES_PSK;
            break;
        case WPS_SHARED_AUTHENTICATION:
            credential->security = WICED_SECURITY_WEP_PSK;
            break;
        case WPS_WPA2_PSK_AUTHENTICATION:
            credential->security = ( encryption_type == WPS_TKIP_ENCRYPTION ) ? WICED_SECURITY_WPA2_TKIP_PSK : WICED_SECURITY_WPA2_AES_PSK;
            break;
    }

    return WICED_SUCCESS;
}
