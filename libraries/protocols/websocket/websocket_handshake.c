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
 */

#include "websocket_handshake.h"
#include "wwd_crypto.h"
#include <base64.h>
#include "wiced_security.h"
#include <ctype.h>

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define STRING_LENGTH_GUID                               36
#define GUID                                             "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" /* Globally Unique Identifier for Websocket Protocol defined in RFC6455 */
#define HTTP_RESPONSE_STATUS_TOKEN                       "HTTP/1.1 101"
#define HTPP_UPGRADE_CASE_SENSITIVE_TOKEN                "Upgrade: "
#define HTPP_UPGRADE_CASE_INSENSITIVE_TOKEN              "WEBSOCKET"
#define HTTP_CONNECTION_CASE_SENSITIVE_TOKEN             "Connection: "
#define HTTP_CONNECTION_CASE_INSENSITIVE_TOKEN           "UPGRADE"
#define HTTP_WEBSOCKET_ACCEPT_CASE_SENSITIVE_TOKEN       "Sec-WebSocket-Accept: "
#define HTTP_WEBSOCKET_SUB_PROTOCOL_CASE_SENSITIVE_TOKEN "Sec-WebSocket-Protocol: "
#define CR_LN                                            "\r\n"

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

static wiced_result_t wiced_generate_client_websocket_key( uint8_t* websocket_key_base64_ptr);
static wiced_result_t wiced_generate_server_websocket_key( uint8_t* server_websocket_key,uint8_t* websocket_key_from_client, uint32_t websocket_key_length);
static wiced_result_t wiced_verify_server_handshake(char* server_response);

/* case insensitive string compare */
static char *stristr(const char *string, const char *pattern);

/******************************************************
 *               Variable Definitions
 ******************************************************/

static uint8_t websocket_key_base64[CLIENT_WEBSOCKET_BASE64_KEY_LENGTH];
static char    sub_protocol_supported[SUB_PROTOCOL_STRING_LENGTH];

/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_result_t wiced_generate_client_websocket_key( uint8_t* websocket_key_base64_ptr)
{
    wiced_result_t result;
    uint8_t        client_websocket_key[ WEBSOCKET_KEY_LENGTH ];

    result = wwd_wifi_get_random( client_websocket_key, WEBSOCKET_KEY_LENGTH );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Now we base64 encode the 16 byte key, as required by the RFC6455 specification */
    if ( base64_encode( client_websocket_key, WEBSOCKET_KEY_LENGTH, websocket_key_base64_ptr, CLIENT_WEBSOCKET_BASE64_KEY_LENGTH, BASE64_STANDARD ) == 0 )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

static wiced_result_t wiced_verify_server_handshake(char* server_response)
{
    char*   received_server_key_start_pointer;
    char*   received_server_key_end_pointer;
    char*   start_of_match;
    uint8_t server_websocket_key[SERVER_WEBSOCKET_BASE64_SHA1_KEY_LENGTH];

    if ( strncmp( server_response, HTTP_RESPONSE_STATUS_TOKEN, strlen( HTTP_RESPONSE_STATUS_TOKEN ) ) != 0 )
    {
        return WICED_ERROR;
    }

    memset( sub_protocol_supported, 0x0, sizeof( sub_protocol_supported ) );

    /* Move to the http upgrade token */
    start_of_match = strstr( server_response, HTPP_UPGRADE_CASE_SENSITIVE_TOKEN );

    /* If response does not contain the upgrade token, it is invalid, so exit with error */
    if ( start_of_match == NULL )
    {
        return WICED_ERROR;
    }

    /* Move to the http upgrade token to point to its value*/
    start_of_match += strlen( HTPP_UPGRADE_CASE_SENSITIVE_TOKEN );

    /* According to the RFC6455, the header is case sensitive but the field is case insensitive*/
    if ( stristr( start_of_match, HTPP_UPGRADE_CASE_INSENSITIVE_TOKEN ) == 0 )
    {
        return WICED_ERROR;
    }

    start_of_match = strstr( server_response, HTTP_CONNECTION_CASE_SENSITIVE_TOKEN );

    /* If response does not contain the Connection token, it is invalid, so exit with error */
    if ( start_of_match == NULL )
    {
        return WICED_ERROR;
    }

    /* Move to the ccurrent pointer past onnection token to point to its value*/
    start_of_match += strlen( HTTP_CONNECTION_CASE_SENSITIVE_TOKEN );

    if ( stristr( start_of_match, HTTP_CONNECTION_CASE_INSENSITIVE_TOKEN ) == 0 )
    {
        return WICED_ERROR;
    }

    /* check for any subprotocols*/
    start_of_match = strstr( server_response, HTTP_WEBSOCKET_SUB_PROTOCOL_CASE_SENSITIVE_TOKEN );
    if ( start_of_match != NULL )
    {
        strcpy( sub_protocol_supported, start_of_match );
    }

    received_server_key_start_pointer = strstr( server_response, HTTP_WEBSOCKET_ACCEPT_CASE_SENSITIVE_TOKEN );

    if ( received_server_key_start_pointer == NULL )
    {
        return WICED_ERROR;
    }

    received_server_key_start_pointer += strlen( HTTP_WEBSOCKET_ACCEPT_CASE_SENSITIVE_TOKEN );

    received_server_key_end_pointer = strstr( received_server_key_start_pointer, CR_LN );

    if ( ( received_server_key_end_pointer == NULL ) || ( received_server_key_end_pointer <= received_server_key_start_pointer ) )
    {
        return WICED_ERROR;
    }

    /* generate key that server sent in the handshake, and validate they are the same*/
    wiced_generate_server_websocket_key( server_websocket_key, websocket_key_base64, strlen( (char*) websocket_key_base64 ) );

    if ( strncmp( (char*) server_websocket_key, received_server_key_start_pointer, (uint8_t) ( received_server_key_end_pointer - received_server_key_start_pointer ) ) == 0 )
    {
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}

static wiced_result_t wiced_generate_server_websocket_key( uint8_t* server_websocket_key, uint8_t* websocket_key_from_client, uint32_t websocket_key_length)
{
    wiced_result_t result = WICED_SUCCESS;
    uint8_t        websocket_concatenated_key[ CLIENT_WEBSOCKET_BASE64_KEY_LENGTH + STRING_LENGTH_GUID ] = { 0x0 };
    uint8_t        websocket_key_sha1[ SHA_LENGTH ];

    /* concatenate client key with GUI*/
    strncpy( (char*) websocket_concatenated_key, (char*) websocket_key_from_client, websocket_key_length );
    strncat( (char*) websocket_concatenated_key, GUID, strlen( GUID ) );

    /* Create SHA1 of the key contatination above*/
    sha1( websocket_concatenated_key, (int32_t) strlen( (char*) websocket_concatenated_key ), websocket_key_sha1 );

    /* Base64 Encode the SHA1 value*/
    if ( base64_encode( websocket_key_sha1, SHA_LENGTH, server_websocket_key, SERVER_WEBSOCKET_BASE64_SHA1_KEY_LENGTH, BASE64_STANDARD ) == 0 )
    {
        return WICED_ERROR;
    }
    return result;
}

wiced_result_t wiced_establish_websocket_handshake( wiced_websocket_t* websocket, const wiced_websocket_handshake_fields_t* websocket_header_fields )
{
    wiced_tcp_stream_t stream;
    wiced_packet_t*    tcp_reply_packet;
    uint16_t           tcp_data_available;
    uint16_t           total_received_bytes;
    uint8_t*           received_handshake;

    /* Initialise the tcp stream*/
    wiced_tcp_stream_init( &stream, &websocket->socket );

    /* generate a unique websocket key to send to server as part of initial handshake*/
    WICED_VERIFY( wiced_generate_client_websocket_key( websocket_key_base64 ) );

    /* build the handshaking headers*/

    /* < GET /uri HTTP/1.1 >*/
    wiced_tcp_stream_write( &stream, "GET ", (uint16_t) strlen( "GET " ) );
    wiced_tcp_stream_write( &stream, websocket_header_fields->request_uri, (uint16_t) strlen( websocket_header_fields->request_uri ) );
    wiced_tcp_stream_write( &stream, " HTTP/1.1\r\n", (uint16_t) strlen( " HTTP/1.1\r\n" ) );

    /* < Host: ip1.ip2.ip3.ip4 >*/
    wiced_tcp_stream_write( &stream, "Host: ", (uint16_t) strlen( "Host: " ) );
    wiced_tcp_stream_write( &stream, websocket_header_fields->host, (uint16_t) strlen( websocket_header_fields->host ) );
    wiced_tcp_stream_write( &stream, "\r\n", (uint16_t) strlen( "\r\n" ) );

    /* < Upgrade: websocket>*/
    wiced_tcp_stream_write( &stream, "Upgrade: websocket\r\n", (uint16_t) strlen( "Upgrade: websocket\r\n" ) );

    /* < Connection: Upgrade >*/
    wiced_tcp_stream_write( &stream, "Connection: Upgrade\r\n", (uint16_t) strlen( "Connection: Upgrade\r\n" ) );

    /* < Sec-WebSocket-Key: random_base4_value >*/
    wiced_tcp_stream_write( &stream, "Sec-WebSocket-Key: ", (uint16_t) strlen( "Sec-WebSocket-Key: " ) );
    wiced_tcp_stream_write( &stream, websocket_key_base64, (uint16_t) strlen( (char*) websocket_key_base64 ) );
    wiced_tcp_stream_write( &stream, "\r\n", (uint16_t) strlen( "\r\n" ) );

    /* < Origin: ip1.ip2.ip3.ip4 >*/
    wiced_tcp_stream_write( &stream, "Origin: ", (uint16_t) strlen( "Origin: " ) );
    wiced_tcp_stream_write( &stream, websocket_header_fields->origin, (uint16_t) strlen( websocket_header_fields->origin ) );
    wiced_tcp_stream_write( &stream, "\r\n", (uint16_t) strlen( "\r\n" ) );

    /* The sec_websocket_protocol is optional, so check if it has been added, include in header if required*/
    if ( websocket_header_fields->sec_websocket_protocol != NULL )
    {
        /* < Sec-WebSocket-Protocol: server_understood_protocol >*/
        wiced_tcp_stream_write( &stream, "Sec-WebSocket-Protocol: ", (uint16_t) strlen( "Sec-WebSocket-Protocol: " ) );
        wiced_tcp_stream_write( &stream, websocket_header_fields->sec_websocket_protocol, (uint16_t) strlen( websocket_header_fields->sec_websocket_protocol ) );
        wiced_tcp_stream_write( &stream, "\r\n", (uint16_t) strlen( "\r\n" ) );
    }

    /* < Sec-WebSocket-Version: 13 >*/
    wiced_tcp_stream_write( &stream, "Sec-WebSocket-Version: 13\r\n\r\n", strlen( "Sec-WebSocket-Version: 13\r\n\r\n" ) );

    /* send the handshake to server*/
    wiced_tcp_stream_flush( &stream );
    wiced_tcp_stream_deinit( &stream );

    WICED_VERIFY( wiced_tcp_receive( &websocket->socket, &tcp_reply_packet, WICED_WAIT_FOREVER ) );

    wiced_packet_get_data( tcp_reply_packet, 0, (uint8_t**) &received_handshake, &total_received_bytes, &tcp_data_available );

    WICED_VERIFY( wiced_verify_server_handshake( (char* )received_handshake ) );

    return WICED_SUCCESS;
}

wiced_result_t wiced_get_websocket_subprotocol( const char* subprotocol )
{
    if ( sub_protocol_supported[ 0 ] == 0x0 )
    {
        return WICED_ERROR;
    }

    strncpy( (char*) subprotocol, sub_protocol_supported, SUB_PROTOCOL_STRING_LENGTH - 1 );
    return WICED_SUCCESS;
}

static char* stristr( const char* string, const char* pattern )
{
    char* pptr  = (char *) pattern;
    char* start = (char *) string;
    uint  slen  = strlen( string );
    uint  plen  = strlen( pattern );
    char* sptr;

    for ( ; slen >= plen; start++, slen-- )
    {
        /* find start of pattern in string */
        while ( toupper( (int) *start ) != toupper( (int) *pattern ) )
        {
            start++;
            slen--;

            /* if pattern longer than string */
            if ( slen < plen )
            {
                return NULL;
            }
        }

        sptr = start;
        pptr = (char *) pattern;

        while ( toupper( (int) *sptr ) == toupper( (int) *pptr ) )
        {
            sptr++;
            pptr++;

            /* if end of pattern then pattern was found */
            if ( '\0' == *pptr )
            {
                return start;
            }
        }
    }
    return NULL;
}
