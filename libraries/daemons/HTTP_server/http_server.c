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
 *  Implements both HTTP and HTTPS servers
 *
 */

#include <string.h>
#include "http_server.h"
#include "wwd_assert.h"
#include "wiced.h"
#include "wiced_resource.h"
#include "strings.h"
#include "platform_resource.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HTTP_SERVER_EVENT_THREAD_STACK_SIZE  (1500)

#define HTTP_SERVER_THREAD_PRIORITY    (WICED_DEFAULT_LIBRARY_PRIORITY)
#define SSL_LISTEN_PORT                (443)
#define HTTP_LISTEN_PORT               (80)
#define HTTP_SERVER_RECEIVE_TIMEOUT    (2000)

/* HTTP Tokens */
#define GET_TOKEN                      "GET "
#define POST_TOKEN                     "POST "
#define PUT_TOKEN                      "PUT "

#define HTTP_1_1_TOKEN                 " HTTP/1.1"
#define FINAL_CHUNKED_PACKET           "0\r\n\r\n"

/*
 * Request-Line =   Method    SP        Request-URI           SP       HTTP-Version      CRLFCRLF
 *              = <-3 char->  <-1 char->   <-1 char->      <-1 char->  <--8 char-->    <-4char->
 *              = 18
 */
#define MINIMUM_REQUEST_LINE_LENGTH    (18)
#define EVENT_QUEUE_DEPTH              (10)
#define COMPARE_MATCH                  (0)
#define MAX_URL_LENGTH                 (100)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    SOCKET_ERROR_EVENT,
    SOCKET_CONNECT_EVENT,
    SOCKET_DISCONNECT_EVENT,
    SERVER_STOP_EVENT,
} http_server_event_t;

typedef enum
{
    HTTP_HEADER_AND_DATA_FRAME_STATE,
    HTTP_DATA_ONLY_FRAME_STATE,
    HTTP_ERROR_STATE
} wiced_http_packet_state_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_tcp_socket_t* socket;
    http_server_event_t event_type;
} server_event_message_t;

typedef struct
{
    wiced_tcp_socket_t*       socket;
    wiced_http_packet_state_t http_packet_state;
    char                      url[ MAX_URL_LENGTH ];
    uint16_t                  url_length;
    wiced_http_message_body_t message_body_descriptor;
} wiced_http_message_context_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t           http_server_connect_callback         ( wiced_tcp_socket_t* socket, void* arg );
static wiced_result_t           http_server_disconnect_callback      ( wiced_tcp_socket_t* socket, void* arg );
static wiced_result_t           http_server_receive_callback         ( wiced_tcp_socket_t* socket, void* arg );
static wiced_result_t           http_server_deferred_receive_callback( void* arg );
static void                     http_server_event_thread_main        ( uint32_t arg );
static wiced_result_t           http_server_parse_receive_packet     ( wiced_http_server_t* server, wiced_tcp_socket_t* socket, wiced_packet_t* packet );
static wiced_result_t           http_server_process_url_request      ( wiced_tcp_socket_t* socket, const wiced_http_page_t* server_url_list, char * url, int url_len, wiced_http_message_body_t* http_message_body );
static uint16_t                 http_server_remove_escaped_characters( char* output, uint16_t output_length, const char* input, uint16_t input_length );
static wiced_packet_mime_type_t http_server_get_mime_type            ( const char* request_data );
static wiced_result_t           http_server_get_request_type_and_url ( char* request, wiced_http_request_type_t* type, char** url_start, uint16_t* url_length );

/******************************************************
 *                 Static Variables
 ******************************************************/

static const char* const http_mime_array[ MIME_UNSUPPORTED ] =
{
    MIME_TABLE( EXPAND_AS_MIME_TABLE )
};

static const char* const http_status_codes[ ] =
{
    [HTTP_200_TYPE] = HTTP_HEADER_200,
    [HTTP_204_TYPE] = HTTP_HEADER_204,
    [HTTP_207_TYPE] = HTTP_HEADER_207,
    [HTTP_301_TYPE] = HTTP_HEADER_301,
    [HTTP_400_TYPE] = HTTP_HEADER_400,
    [HTTP_403_TYPE] = HTTP_HEADER_403,
    [HTTP_404_TYPE] = HTTP_HEADER_404,
    [HTTP_405_TYPE] = HTTP_HEADER_405,
    [HTTP_406_TYPE] = HTTP_HEADER_406,
    [HTTP_412_TYPE] = HTTP_HEADER_412,
    [HTTP_415_TYPE] = HTTP_HEADER_406,
    [HTTP_429_TYPE] = HTTP_HEADER_429,
    [HTTP_444_TYPE] = HTTP_HEADER_444,
    [HTTP_470_TYPE] = HTTP_HEADER_470,
    [HTTP_500_TYPE] = HTTP_HEADER_500,
    [HTTP_504_TYPE] = HTTP_HEADER_504
};

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_http_server_start( wiced_http_server_t* server, uint16_t port, uint16_t max_sockets, const wiced_http_page_t* page_database, wiced_interface_t interface, uint32_t url_processor_stack_size)
{
    memset( server, 0, sizeof( *server ) );

    /* Store the inputs database */
    server->page_database = page_database;

    wiced_http_server_deregister_callbacks( server );

    /* Initialise the socket state for all sockets */
    WICED_VERIFY( wiced_tcp_server_start( &server->tcp_server, interface, port, max_sockets, http_server_connect_callback, http_server_receive_callback, http_server_disconnect_callback, (void*) server ) );

    WICED_VERIFY( wiced_rtos_create_worker_thread( &server->worker_thread, WICED_APPLICATION_PRIORITY, url_processor_stack_size, EVENT_QUEUE_DEPTH ) );

    WICED_VERIFY( wiced_rtos_init_queue( &server->event_queue, NULL, sizeof(server_event_message_t), EVENT_QUEUE_DEPTH ) );

    /* Create server thread and return */
    return wiced_rtos_create_thread( &server->event_thread, HTTP_SERVER_THREAD_PRIORITY, "HTTPserver", http_server_event_thread_main, HTTP_SERVER_EVENT_THREAD_STACK_SIZE, server );
}

wiced_result_t wiced_http_server_stop( wiced_http_server_t* server )
{
    server_event_message_t current_event;

    current_event.event_type = SERVER_STOP_EVENT;
    current_event.socket     = 0;

    wiced_rtos_push_to_queue( &server->event_queue, &current_event, WICED_NO_WAIT );

    if ( wiced_rtos_is_current_thread( &server->event_thread ) != WICED_SUCCESS )
    {
        wiced_rtos_thread_force_awake( &server->event_thread );
        wiced_rtos_thread_join( &server->event_thread );
        wiced_rtos_delete_thread( &server->event_thread );
        wiced_rtos_delete_worker_thread( &server->worker_thread );
        wiced_tcp_server_stop( &server->tcp_server );
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_https_server_start( wiced_https_server_t* server, uint16_t port, uint16_t max_sockets, const wiced_http_page_t* page_database, const char* server_cert, const char* server_key, wiced_interface_t interface, uint32_t url_processor_stack_size )
{
    memset( server, 0, sizeof( *server ) );

    /* store the inputs database */
    server->http_server.page_database = page_database;

    /* Initialise the socket state for all sockets */
    WICED_VERIFY( wiced_tcp_server_start( &server->http_server.tcp_server, interface, port, max_sockets, http_server_connect_callback, http_server_receive_callback, http_server_disconnect_callback, (void*) server ) );

    wiced_tcp_server_add_tls( &server->http_server.tcp_server, &server->tls_context, server_cert, server_key );

    wiced_http_server_deregister_callbacks( &server->http_server );

    WICED_VERIFY( wiced_rtos_create_worker_thread( &server->http_server.worker_thread, WICED_APPLICATION_PRIORITY, url_processor_stack_size, EVENT_QUEUE_DEPTH ) );

    WICED_VERIFY( wiced_rtos_init_queue( &server->http_server.event_queue, NULL, sizeof(server_event_message_t), EVENT_QUEUE_DEPTH ) );

    /* Create server thread and return */
    return wiced_rtos_create_thread( &server->http_server.event_thread, HTTP_SERVER_THREAD_PRIORITY, "HTTPSserver", http_server_event_thread_main, HTTP_SERVER_EVENT_THREAD_STACK_SIZE, &server->http_server );

}

wiced_result_t wiced_https_server_stop( wiced_https_server_t* server )
{
    return wiced_http_server_stop( &server->http_server );
}

wiced_result_t wiced_http_server_register_callbacks( wiced_http_server_t* server, http_server_callback_t receive_callback )
{
    wiced_assert( "bad arg", ( server != NULL ) );

    server->receive_callback = receive_callback;

    return WICED_SUCCESS;
}

wiced_result_t wiced_http_server_deregister_callbacks( wiced_http_server_t* server )
{
    wiced_assert( "bad arg", ( server != NULL ) );

    server->receive_callback = NULL;

    return WICED_SUCCESS;
}

wiced_result_t wiced_http_server_queue_disconnect_request( wiced_http_server_t* server, wiced_tcp_socket_t* socket_to_disconnect )
{
    server_event_message_t current_event;

    current_event.event_type = SOCKET_DISCONNECT_EVENT;
    current_event.socket     = socket_to_disconnect;

    return wiced_rtos_push_to_queue( &server->event_queue, &current_event, WICED_NO_WAIT );
}

wiced_result_t wiced_http_response_stream_enable_chunked_transfer( wiced_http_response_stream_t* stream )
{
    stream->chunked_transfer_enabled = WICED_TRUE;
    return WICED_SUCCESS;
}

wiced_result_t wiced_http_response_stream_disable_chunked_transfer( wiced_http_response_stream_t* stream )
{
    stream->chunked_transfer_enabled = WICED_FALSE;
    return WICED_SUCCESS;
}

wiced_result_t wiced_http_response_stream_write_header( wiced_http_response_stream_t* stream, http_status_codes_t status_code, uint32_t content_length, http_cache_t cache_type, wiced_packet_mime_type_t mime_type )
{
    char data_length_string[ 15 ];

    wiced_assert( "bad arg", ( stream != NULL ) );

    memset( data_length_string, 0x0, sizeof( data_length_string ) );

    /*HTTP/1.1 <status code>\r\n*/
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, http_status_codes[ status_code ], strlen( http_status_codes[ status_code ] ) ) );
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, sizeof( CRLF )-1 ) );

    /* Content-Type: xx/yy\r\n */
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_CONTENT_TYPE, strlen( HTTP_HEADER_CONTENT_TYPE ) ) );
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, http_mime_array[ mime_type ], strlen( http_mime_array[ mime_type ] ) ) );
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );

    if ( cache_type == HTTP_CACHE_DISABLED )
    {
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, NO_CACHE_HEADER, strlen( NO_CACHE_HEADER ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }

    if ( status_code == HTTP_444_TYPE )
    {
        /* Connection: close */
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_CLOSE, strlen( HTTP_HEADER_CLOSE ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }
    else
    {
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_KEEP_ALIVE, strlen( HTTP_HEADER_KEEP_ALIVE ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }

    if ( stream->chunked_transfer_enabled == WICED_TRUE )
    {
        /* Chunked transfer encoding */
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_CHUNKED, strlen( HTTP_HEADER_CHUNKED ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }
    else if ( content_length != 0 )
    {
        /* Content-Length: xx\r\n */
        sprintf( data_length_string, "%lu", (long) content_length );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, HTTP_HEADER_CONTENT_LENGTH, strlen( HTTP_HEADER_CONTENT_LENGTH ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, data_length_string, strlen( data_length_string ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );
    }

    /* Closing sequence */
    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, strlen( CRLF ) ) );

    return WICED_SUCCESS;
}

wiced_result_t wiced_http_response_stream_write( wiced_http_response_stream_t* stream, const void* data, uint32_t length )
{
    if ( length == 0 )
    {
        return WICED_BADARG;
    }

    if ( stream->chunked_transfer_enabled == WICED_TRUE )
    {
        char data_length_string[10];
        memset( data_length_string, 0x0, sizeof( data_length_string ) );
        sprintf( data_length_string, "%lx", (long unsigned int)length );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, data_length_string, strlen( data_length_string ) ) );
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, sizeof( CRLF ) - 1 ) );
    }

    WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, data, length ) );

    if ( stream->chunked_transfer_enabled == WICED_TRUE )
    {
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, CRLF, sizeof( CRLF ) - 1 ) );
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_http_response_stream_write_resource( wiced_http_response_stream_t* stream, const resource_hnd_t* res_id )
{
    wiced_result_t result;
    const void*    data;
    uint32_t       res_size;
    uint32_t       pos = 0;

    do
    {
        resource_result_t resource_result = resource_get_readonly_buffer ( res_id, pos, 0x7fffffff, &res_size, &data );
        if ( resource_result != RESOURCE_SUCCESS )
        {
            return result;
        }

        result = wiced_http_response_stream_write( stream, data, (uint16_t) res_size );
        resource_free_readonly_buffer( res_id, data );
        if ( result != WICED_SUCCESS )
        {
            return result;
        }
        pos += res_size;
    } while ( res_size > 0 );

    return result;
}

wiced_result_t wiced_http_response_stream_flush( wiced_http_response_stream_t* stream )
{
    wiced_assert( "bad arg", ( stream != NULL ) );

    if ( stream->chunked_transfer_enabled == WICED_TRUE )
    {
        /* Send final chunked frame */
        WICED_VERIFY( wiced_tcp_stream_write( &stream->tcp_stream, FINAL_CHUNKED_PACKET, sizeof( FINAL_CHUNKED_PACKET ) - 1 ) );
    }

    return wiced_tcp_stream_flush( &stream->tcp_stream );
}

wiced_result_t wiced_http_response_stream_init( wiced_http_response_stream_t* stream, wiced_tcp_socket_t* socket )
{
    memset( stream, 0, sizeof( wiced_http_response_stream_t ) );

    stream->chunked_transfer_enabled = WICED_FALSE;

    return wiced_tcp_stream_init( &stream->tcp_stream, socket );
}

wiced_result_t wiced_http_response_stream_deinit( wiced_http_response_stream_t* stream )
{
    return wiced_tcp_stream_deinit( &stream->tcp_stream );
}

static wiced_result_t http_server_connect_callback( wiced_tcp_socket_t* socket, void* arg )
{
    server_event_message_t current_event;

    current_event.event_type = SOCKET_CONNECT_EVENT;
    current_event.socket     = socket;
    return wiced_rtos_push_to_queue( &((wiced_http_server_t*)arg)->event_queue, &current_event, WICED_NO_WAIT );
}

static wiced_result_t http_server_disconnect_callback( wiced_tcp_socket_t* socket, void* arg )
{
    return wiced_http_server_queue_disconnect_request( (wiced_http_server_t*)arg, socket );
}

static wiced_result_t http_server_receive_callback( wiced_tcp_socket_t* socket, void* arg )
{
    return wiced_rtos_send_asynchronous_event( &((wiced_http_server_t*)arg)->worker_thread, http_server_deferred_receive_callback, (void*)socket );
}

static wiced_result_t http_server_parse_receive_packet( wiced_http_server_t* server, wiced_tcp_socket_t* socket, wiced_packet_t* packet )
{
    wiced_result_t result                        = WICED_SUCCESS;
    wiced_bool_t   disconnect_current_connection = WICED_FALSE;
    char*          start_of_url                  = NULL; /* Suppress compiler warning */
    uint16_t       url_length                    = 0;    /* Suppress compiler warning */
    char*          request_string;
    uint16_t       request_length;
    uint16_t       new_url_length;
    uint16_t       available_data_length;
    char*          message_data_length_string;
    char*          mime;

    wiced_http_message_body_t http_message_body =
    {
        .data                         = NULL,
        .message_data_length          = 0,
        .total_message_data_remaining = 0,
        .chunked_transfer             = WICED_FALSE,
        .mime_type                    = MIME_UNSUPPORTED,
        .request_type                 = REQUEST_UNDEFINED
    };

    if ( packet == NULL )
    {
        return WICED_ERROR;
    }

    /* Get URL request string from the receive packet */
    result = wiced_packet_get_data( packet, 0, (uint8_t**)&request_string, &request_length, &available_data_length );
    if ( result != WICED_SUCCESS )
    {
        disconnect_current_connection = WICED_TRUE;
        goto exit;
    }

    /* If application registers a receive callback, call the callback before further processing */
    if ( server->receive_callback != NULL )
    {
        result = server->receive_callback( socket, (uint8_t**)&request_string, &request_length );
        if ( result != WICED_SUCCESS )
        {
            disconnect_current_connection = WICED_TRUE;
            goto exit;
        }
    }

    /* Verify we have enough data to start processing */
    if ( request_length < MINIMUM_REQUEST_LINE_LENGTH )
    {
        result = WICED_ERROR;
        goto exit;
    }

    /* Check if this is a close request */
    if ( strstr( request_string, HTTP_HEADER_CLOSE ) != NULL )
    {
        disconnect_current_connection = WICED_TRUE;
    }

    /* First extract the URL from the packet */
    result = http_server_get_request_type_and_url( request_string, &http_message_body.request_type, &start_of_url, &url_length );
    if ( result == WICED_ERROR )
    {
        goto exit;
    }

    /* Remove escape strings from URL */
    new_url_length = http_server_remove_escaped_characters( start_of_url, url_length, start_of_url, url_length );

    /* Now extract packet payload info such as data, data length, data type and message length */
    http_message_body.data = (uint8_t*) strstr( request_string, CRLF_CRLF );

    /* This indicates start of data/end of header was not found, so exit */
    if ( http_message_body.data == NULL )
    {
        result = WICED_ERROR;
        goto exit;
    }
    else
    {
        /* Payload starts just after the header */
        http_message_body.data += strlen( CRLF_CRLF );
    }

    mime = strstr( request_string, HTTP_HEADER_CONTENT_TYPE );
    if ( ( mime != NULL ) && ( mime < (char*) http_message_body.data ) )
    {
        mime += strlen( HTTP_HEADER_CONTENT_TYPE );
        http_message_body.mime_type = http_server_get_mime_type( mime );
    }
    else
    {
        http_message_body.mime_type = MIME_TYPE_ALL;
    }

    if ( strstr( request_string, HTTP_HEADER_CHUNKED ) )
    {
        /* Indicate the format of this frame is chunked. Its up to the application to reassemble */
        http_message_body.chunked_transfer = WICED_TRUE;
        http_message_body.message_data_length = (uint16_t) ( strstr( (char*) http_message_body.data, FINAL_CHUNKED_PACKET ) - (char*) http_message_body.data );
    }
    else
    {
        message_data_length_string = strstr( request_string, HTTP_HEADER_CONTENT_LENGTH );

        if ( ( message_data_length_string != NULL ) && ( message_data_length_string < (char*) http_message_body.data ) )
        {
            http_message_body.message_data_length = (uint16_t) ( (uint8_t*) ( request_string + request_length ) - http_message_body.data );

            message_data_length_string += ( sizeof( HTTP_HEADER_CONTENT_LENGTH ) - 1 );

            http_message_body.total_message_data_remaining = (uint16_t) ( strtol( message_data_length_string, NULL, 10 ) - http_message_body.message_data_length );
        }
        else
        {
            http_message_body.message_data_length = 0;
        }
    }

    result = http_server_process_url_request( (wiced_tcp_socket_t*) socket, server->page_database, start_of_url, new_url_length, &http_message_body );

    exit:
    if ( disconnect_current_connection == WICED_TRUE )
    {
        wiced_http_server_queue_disconnect_request( server, socket );
    }

    return result;
}

static wiced_result_t http_server_process_url_request( wiced_tcp_socket_t* socket, const wiced_http_page_t* server_url_list, char* url, int url_len, wiced_http_message_body_t* http_message_body )
{
    wiced_packet_mime_type_t server_mime          = MIME_TYPE_ALL;
    char*                    url_query_parameters = url;
    int                      query_length         = url_len;
    int                      i                    = 0;
    http_status_codes_t      status_code;
    wiced_http_response_stream_t      stream;

    url[ url_len ] = '\x00';

    while ( ( *url_query_parameters != '?' ) && ( query_length > 0 ) && ( *url_query_parameters != '\0' ) )
    {
        url_query_parameters++;
        query_length--;
    }

    if ( query_length != 0 )
    {
        url_len = url_len - query_length;
    }
    else
    {
        url_query_parameters = url;
    }

    WPRINT_WEBSERVER_DEBUG( ("Processing request for: %s\n", url) );

    /* Init the HTTP stream */
    wiced_http_response_stream_init( &stream, socket );

    status_code = HTTP_404_TYPE;

    /* Search URL list to determine if request matches one of our pages, and break out when found */
    while ( server_url_list[ i ].url != NULL )
    {
        /* For raw dynamic URL, callback may need to decode the full path further so only compare up to the length of the base path */
        uint32_t compare_length = ( server_url_list[i].url_content_type == WICED_RAW_DYNAMIC_URL_CONTENT ) ? strlen( server_url_list[ i ].url ) : (uint32_t)url_len;

        if ( ( strncasecmp( server_url_list[ i ].url, url, compare_length ) == COMPARE_MATCH ) )
        {
            server_mime = http_server_get_mime_type( server_url_list[ i ].mime_type );

            if ( ( server_mime == http_message_body->mime_type ) || ( http_message_body->mime_type == MIME_TYPE_ALL ) )
            {
                status_code = HTTP_200_TYPE;
                break;
            }
        }
        i++;
    }

    if ( status_code == HTTP_200_TYPE )
    {

        /* Call the content handler function to write the page content into the packet and adjust the write pointers */
        switch ( server_url_list[ i ].url_content_type )
        {
            case WICED_DYNAMIC_URL_CONTENT:
                /* message packaging into stream is done through wiced_http_write_dynamic_response_frame API called from callback */
                *url_query_parameters = '\x00';
                url_query_parameters++;

                wiced_http_response_stream_enable_chunked_transfer( &stream );
                wiced_http_response_stream_write_header( &stream, status_code, CHUNKED_CONTENT_LENGTH, HTTP_CACHE_DISABLED, server_mime );
                server_url_list[ i ].url_content.dynamic_data.generator( url_query_parameters, &stream, server_url_list[ i ].url_content.dynamic_data.arg, http_message_body );
                break;

            case WICED_RAW_DYNAMIC_URL_CONTENT:
                server_url_list[ i ].url_content.dynamic_data.generator( url, &stream, server_url_list[ i ].url_content.dynamic_data.arg, http_message_body );
                break;

            case WICED_STATIC_URL_CONTENT:
                wiced_http_response_stream_write_header( &stream, status_code, server_url_list[ i ].url_content.static_data.length, HTTP_CACHE_ENABLED, server_mime );
                wiced_http_response_stream_write( &stream, server_url_list[ i ].url_content.static_data.ptr, server_url_list[ i ].url_content.static_data.length );
                break;

            case WICED_RAW_STATIC_URL_CONTENT: /* This is just a Location header */
                wiced_http_response_stream_write( &stream, HTTP_HEADER_301, strlen( HTTP_HEADER_301 ) );
                wiced_http_response_stream_write( &stream, CRLF, strlen( CRLF ) );
                wiced_http_response_stream_write( &stream, HTTP_HEADER_LOCATION, strlen( HTTP_HEADER_LOCATION ) );
                wiced_http_response_stream_write( &stream, server_url_list[ i ].url_content.static_data.ptr, server_url_list[ i ].url_content.static_data.length );
                wiced_http_response_stream_write( &stream, CRLF, strlen( CRLF ) );
                wiced_http_response_stream_write( &stream, HTTP_HEADER_CONTENT_LENGTH, strlen( HTTP_HEADER_CONTENT_LENGTH ) );
                wiced_http_response_stream_write( &stream, "0", 1 );
                wiced_http_response_stream_write( &stream, CRLF_CRLF, strlen( CRLF_CRLF ) );
                break;

            case WICED_RESOURCE_URL_CONTENT:
                /* Fall through */
            case WICED_RAW_RESOURCE_URL_CONTENT:
                wiced_http_response_stream_enable_chunked_transfer( &stream );
                wiced_http_response_stream_write_header( &stream, status_code, CHUNKED_CONTENT_LENGTH, HTTP_CACHE_DISABLED, server_mime );
                wiced_http_response_stream_write_resource( &stream, server_url_list[ i ].url_content.resource_data );
                break;

            default:
                wiced_assert("Unknown entry in URL list", 0 != 0 );
                break;
        }
    }

    WICED_VERIFY( wiced_http_response_stream_flush( &stream ) );

    wiced_http_response_stream_disable_chunked_transfer( &stream );

    if ( status_code >= HTTP_400_TYPE )
    {
        wiced_http_response_stream_write_header( &stream, status_code, NO_CONTENT_LENGTH, HTTP_CACHE_DISABLED, MIME_TYPE_TEXT_HTML );
        WICED_VERIFY( wiced_http_response_stream_flush( &stream ) );
    }

    wiced_http_response_stream_deinit( &stream );

    wiced_assert( "Page Serve finished with data still in stream", stream.tcp_stream.tx_packet == NULL );

    return WICED_SUCCESS;
}

static uint16_t http_server_remove_escaped_characters( char* output, uint16_t output_length, const char* input, uint16_t input_length )
{
    uint16_t bytes_copied;
    int a;

    for ( bytes_copied = 0; ( input_length > 0 ) && ( bytes_copied != output_length ); ++bytes_copied )
    {
        if ( *input == '%' )
        {
            if ( *( input + 1 ) == '%' )
            {
                ++input;
                *output = '%';
            }
            else
            {
                *output = 0;
                for ( a = 0; a < 2; ++a )
                {
                    *output = (char) ( *output << 4 );
                    ++input;
                    if ( *input >= '0' && *input <= '9' )
                    {
                        *output = (char) ( *output + *input - '0' );
                    }
                    else if ( *input >= 'a' && *input <= 'f' )
                    {
                        *output = (char) ( *output + *input - 'a' + 10 );
                    }
                    else if ( *input >= 'A' && *input <= 'F' )
                    {
                        *output = (char) ( *output + *input - 'A' + 10 );
                    }
                }
                input_length = (uint16_t) ( input_length - 3 );
            }
        }
        else
        {
            *output = *input;
            --input_length;
        }
        ++output;
        ++input;
    }

    return bytes_copied;
}

static wiced_packet_mime_type_t http_server_get_mime_type( const char* request_data )
{
    wiced_packet_mime_type_t mime_type = MIME_TYPE_TLV;

    if ( request_data != NULL )
    {
        while ( mime_type < MIME_TYPE_ALL )
        {
            if ( strncmp( request_data, http_mime_array[ mime_type ], strlen( http_mime_array[ mime_type ] ) ) == COMPARE_MATCH )
            {
                break;
            }
            mime_type++;
        }
    }
    else
    {
        /* If MIME not specified, assumed all supported (according to rfc2616)*/
        mime_type = MIME_TYPE_ALL;
    }
    return mime_type;
}

static wiced_result_t http_server_get_request_type_and_url( char* request, wiced_http_request_type_t* type, char** url_start, uint16_t* url_length )
{
    char* end_of_url;

    end_of_url = strstr( request, HTTP_1_1_TOKEN );
    if ( end_of_url == NULL )
    {
        return WICED_ERROR;
    }

    if ( memcmp( request, GET_TOKEN, sizeof( GET_TOKEN ) - 1 ) == COMPARE_MATCH )
    {
        /* Get type  */
        *type = WICED_HTTP_GET_REQUEST;
        *url_start = request + sizeof( GET_TOKEN ) - 1;
        *url_length = (uint16_t) ( end_of_url - *url_start );
    }
    else if ( memcmp( request, POST_TOKEN, sizeof( POST_TOKEN ) - 1 ) == COMPARE_MATCH )
    {
        *type = WICED_HTTP_POST_REQUEST;
        *url_start = request + sizeof( POST_TOKEN ) - 1;
        *url_length = (uint16_t) ( end_of_url - *url_start );
    }
    else if ( memcmp( request, PUT_TOKEN, sizeof( PUT_TOKEN ) - 1 ) == COMPARE_MATCH )
    {
        *type = WICED_HTTP_PUT_REQUEST;
        *url_start = request + sizeof( PUT_TOKEN ) - 1;
        *url_length = (uint16_t) ( end_of_url - *url_start );
    }
    else
    {
        *type = REQUEST_UNDEFINED;
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

static void http_server_event_thread_main( uint32_t arg )
{
    wiced_http_server_t*   server = (wiced_http_server_t*) arg;
    server_event_message_t current_event;
    wiced_result_t         result;

    while ( server->quit != WICED_TRUE )
    {
        result = wiced_rtos_pop_from_queue( &server->event_queue, &current_event, WICED_NEVER_TIMEOUT );

        if ( result != WICED_SUCCESS )
        {
            current_event.socket     = NULL;
            current_event.event_type = SOCKET_ERROR_EVENT;
        }

        switch ( current_event.event_type )
        {
            case SOCKET_CONNECT_EVENT:
            {
                wiced_tcp_server_accept( &server->tcp_server, current_event.socket );
                break;
            }
            case SOCKET_DISCONNECT_EVENT:
            {
                wiced_tcp_server_disconnect_socket( &server->tcp_server, current_event.socket );
                break;
            }
            case SERVER_STOP_EVENT:
            {
                server->quit = WICED_TRUE;
                break;
            }
            case SOCKET_ERROR_EVENT: /* Fall through */
            default:
            {
                break;
            }
        }
    }

    wiced_tcp_server_stop( &server->tcp_server );
    wiced_rtos_deinit_queue( &server->event_queue );

    WICED_END_OF_CURRENT_THREAD( );
}

static wiced_result_t http_server_deferred_receive_callback( void* arg )
{
    wiced_tcp_socket_t*  socket = (wiced_tcp_socket_t*)arg;
    wiced_http_server_t* server = (wiced_http_server_t*)socket->arg;
    wiced_packet_t*      packet = NULL;

    wiced_tcp_receive( socket, &packet, WICED_NO_WAIT );
    if ( packet != NULL )
    {
        http_server_parse_receive_packet( server, socket, packet );
        wiced_packet_delete( packet );
    }
    return WICED_SUCCESS;
}
