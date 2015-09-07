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
 * NFC token config server snippet application
 *
 * This application demonstrates how to use NFC to write access point credentials
 * onto an NFC tag for easy client association.
 *
 * This is an implementation of the Wi-Fi Simple Configuration (WSC) NFC
 * Configuration Token process as described in section 10.2.2 of the
 * Wi-Fi Simple Configuration Technical Specification from the perspective
 * of an access point.
 *
 * Features demonstrated
 *  - Creation of the WSC Configuration Token and writing to an NFC tag
 *
 * Application Instructions
 *   1. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *   2. Tap the NFC tag to the NFC capable WICED platform.
 *      Status of the transaction will be printed out the serial port.
 *   3. Tap the newly written NFC tag on an NFC enabled client to transfer
 *      access point credentials.
 */

#include "wiced.h"
#include "wiced_nfc_api.h"
#include "wps_constants.h"
#include "wps_structures.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define NFC_WRITE_TIMEOUT       ( 60 * SECONDS )
#define MAX_CONFIG_TOKEN_SIZE   ( 160 )

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

static wiced_result_t create_nfc_config_token( uint8_t token_buffer[ MAX_CONFIG_TOKEN_SIZE ], uint16_t* token_buffer_length, const wiced_config_soft_ap_t* credential );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_nfc_workspace_t nfc_workspace;

static const wiced_ip_setting_t device_init_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS(192, 168, 10,  1) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS(255, 255, 255, 0) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS(192, 168, 10,  1) ),
};

static const wiced_mac_t broadcast_address                  = { .octet = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
static const uint8_t     vendor_ext_network_key_shareable[] = { 0x00, 0x37, 0x2A, 0x02, 0x01, 0x01 };
static const uint8_t     vendor_ext_version[]               = { 0x00, 0x37, 0x2A, 0x00, 0x01, 0x20 };

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_config_soft_ap_t* softap_info;
    wiced_result_t          result;
    wiced_nfc_tag_msg_t     nfc_config_token_message;
    uint8_t                 config_token_buffer[ MAX_CONFIG_TOKEN_SIZE ];
    uint16_t                config_token_length;

    /* Initialize WICED */
    wiced_init();

    /* Bring up the soft AP interface */
    wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &device_init_ip_settings );

    /* Read the soft AP settings from the DCT and prepare the config token data */
    wiced_dct_read_lock( (void**) &softap_info, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, OFFSETOF(platform_dct_wifi_config_t, soft_ap_settings), sizeof(wiced_config_soft_ap_t) );

    create_nfc_config_token( config_token_buffer, &config_token_length, softap_info );

    wiced_dct_read_unlock( softap_info, WICED_FALSE );

    /* Initialize NFC.
     * Note: This can take up to 8 seconds to complete
     */
    WPRINT_APP_INFO( ("\nInitializing NFC...\n") );
    if ( wiced_nfc_init( &nfc_workspace ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("NFC error\n") );
        return;
    }

    /* Prepare config token message */
    nfc_config_token_message.buffer        = config_token_buffer;
    nfc_config_token_message.buffer_length = config_token_length;
    nfc_config_token_message.type          = WICED_NFC_TAG_TYPE_WSC;

    /* Write the config token to a tag */
    do
    {
        WPRINT_APP_INFO( ( "\nWaiting for tag...\n" ) );
        result = wiced_nfc_write_tag( &nfc_config_token_message, NFC_WRITE_TIMEOUT );

        if ( result == WICED_SUCCESS )
        {
            WPRINT_APP_INFO( ( "Config token written successfully\n" ) );

            /* Delay for a short time to allow tag to be removed */
            wiced_rtos_delay_milliseconds( 1000 );
        }
        else if ( result == WICED_TIMEOUT )
        {
            WPRINT_APP_INFO( ( "Tag not detected\n" ) );
        }
        else
        {
            WPRINT_APP_INFO( ( "Tag write error\n" ) );
        }
    } while ( result != WICED_SUCCESS );
}

static wiced_result_t create_nfc_config_token( uint8_t token_buffer[ MAX_CONFIG_TOKEN_SIZE ], uint16_t* token_buffer_length, const wiced_config_soft_ap_t* credential )
{
    uint8_t*       config_token_buffer = token_buffer;
    uint16_t       authentication_type;
    uint16_t       encryption_type;
    uint16_t       config_length;
    tlv16_data_t*  credential_header;
    uint16_t       network_index     = 1;

    memset( config_token_buffer, 0, MAX_CONFIG_TOKEN_SIZE );

    authentication_type = htobe16( WPS_WPA2_PSK_AUTHENTICATION );
    encryption_type     = htobe16( WPS_AES_ENCRYPTION  );

    /* Make a copy of the start of the buffer as the credential TLV header */
    credential_header = ( tlv16_data_t* )config_token_buffer;

    config_token_buffer = tlv_write_header( config_token_buffer, WPS_ID_CREDENTIAL, 0 );
    config_token_buffer = tlv_write_value ( config_token_buffer, WPS_ID_NW_INDEX,   1,                                        &network_index,                     TLV_UINT8 );
    config_token_buffer = tlv_write_value ( config_token_buffer, WPS_ID_SSID,       credential->SSID.length,                  credential->SSID.value,             TLV_UINT8_PTR );
    config_token_buffer = tlv_write_value ( config_token_buffer, WPS_ID_AUTH_TYPE,  WPS_ID_AUTH_TYPE_S,                       &authentication_type,               TLV_UINT8_PTR );
    config_token_buffer = tlv_write_value ( config_token_buffer, WPS_ID_ENCR_TYPE,  WPS_ID_ENCR_TYPE_S,                       &encryption_type,                   TLV_UINT8_PTR );
    config_token_buffer = tlv_write_value ( config_token_buffer, WPS_ID_NW_KEY,     credential->security_key_length,          credential->security_key,           TLV_UINT8_PTR );
    config_token_buffer = tlv_write_value ( config_token_buffer, WPS_ID_MAC_ADDR,   sizeof(wiced_mac_t),                      &broadcast_address,                 TLV_UINT8_PTR );
    config_token_buffer = tlv_write_value ( config_token_buffer, WPS_ID_VENDOR_EXT, sizeof(vendor_ext_network_key_shareable), &vendor_ext_network_key_shareable,  TLV_UINT8_PTR );

    /* Finish off the credential TLV */
    config_length             = ( config_token_buffer - credential_header->data );
    credential_header->length = htobe16( config_length );

    *token_buffer_length = sizeof(tlv16_header_t) + config_length + sizeof(tlv16_header_t) + sizeof(vendor_ext_t);

    if ( *token_buffer_length > MAX_CONFIG_TOKEN_SIZE )
    {
        WPRINT_APP_ERROR( ("Config token overflow\n") );
        return WICED_ERROR;
    }

    /* Write the vendor extension version TLV */
    tlv_write_value( config_token_buffer, WPS_ID_VENDOR_EXT, sizeof( vendor_ext_version ), &vendor_ext_version, TLV_UINT8_PTR );

    return WICED_SUCCESS;
}
