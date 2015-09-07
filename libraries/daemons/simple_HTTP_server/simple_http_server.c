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
#include "simple_http_server.h"
#include "wwd_assert.h"
#include "wiced.h"
#include "wiced_resource.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define HTTP_SERVER_THREAD_PRIORITY (WICED_DEFAULT_LIBRARY_PRIORITY)
#if (defined(WPRINT_ENABLE_WEBSERVER) && (defined(WPRINT_ENABLE_DEBUG) || defined(WPRINT_ENABLE_ERROR)))
#define HTTP_SERVER_STACK_SIZE      (5000) /* printf requires 4K of stack */
#else
#define HTTP_SERVER_STACK_SIZE      (10000)
#endif

#define SSL_LISTEN_PORT              (443)
#define HTTP_SERVER_RECEIVE_TIMEOUT (2000)

static const char ok_header[] =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: ";

static const char crlfcrlf[] ="\r\n\r\n";

#ifndef USE_404

static const char not_found_header[] = "HTTP/1.0 301\r\nLocation: /\r\n\r\n";

#else /* ifndef USE_404 */
static const char not_found_header[] =
    "HTTP/1.0 404 Not Found\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<!doctype html>\n"
    "<html><head><title>404 - WICED Web Server</title></head><body>\n"
    "<h1>Address not found on WICED Web Server</h1>\n"
    "<p><a href=\"/\">Return to home page</a></p>\n"
    "</body>\n</html>\n";

#endif /* ifndef USE_404 */

#define NO_CACHE_HEADER "\r\n" \
                        "Expires: Mon, 26 Jul 1997 05:00:00 GMT\r\n" \
                        "Cache-Control: no-store, no-cache, must-revalidate, max-age=0, post-check=0, pre-check=0\r\n" \
                        "Pragma: no-cache"

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

static wiced_result_t http_server_process_packet(const wiced_http_page_t* page_database, wiced_tcp_socket_t* socket, wiced_packet_t* packet);
static wiced_result_t process_url_request( wiced_tcp_socket_t* socket, const wiced_http_page_t* server_url_list, char * url, int url_len );
static void http_server_thread_main(uint32_t arg);
static uint16_t escaped_string_copy(char* output, uint16_t output_length, const char* input, uint16_t input_length);

/******************************************************
 *                 Static Variables
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_simple_http_server_start( wiced_simple_http_server_t* server, uint16_t port, const wiced_http_page_t* page_database, wiced_interface_t interface )
{
    memset( server, 0, sizeof( *server ) );

    /* Create the TCP socket */
    WICED_VERIFY(wiced_tcp_create_socket( &server->socket, interface ));
    if (wiced_tcp_listen( (wiced_tcp_socket_t*) &server->socket, port ) != WICED_SUCCESS)
    {
        wiced_tcp_delete_socket(&server->socket);
        return WICED_ERROR;
    }

    server->page_database = page_database;
    return wiced_rtos_create_thread(&server->thread, HTTP_SERVER_THREAD_PRIORITY, "HTTPserver", http_server_thread_main, HTTP_SERVER_STACK_SIZE, server);
}

wiced_result_t wiced_simple_http_server_stop (wiced_simple_http_server_t* server)
{
    server->quit = WICED_TRUE;
    if ( wiced_rtos_is_current_thread( &server->thread ) != WICED_SUCCESS )
    {
        wiced_rtos_thread_force_awake( &server->thread );
        wiced_rtos_thread_join( &server->thread );
        wiced_rtos_delete_thread( &server->thread );
    }
    return WICED_SUCCESS;
}


static void http_server_thread_main(uint32_t arg)
{
    wiced_simple_http_server_t* server = (wiced_simple_http_server_t*) arg;

    while ( server->quit != WICED_TRUE )
    {
        wiced_packet_t* temp_packet = NULL;

        /* Wait for a connection */
        wiced_result_t result = wiced_tcp_accept( &server->socket );
        if ( result == WICED_SUCCESS )
        {
            /* Receive the query */
            if (wiced_tcp_receive( &server->socket, &temp_packet, WICED_WAIT_FOREVER ) == WICED_SUCCESS)
            {
                /* Process the request */
                http_server_process_packet( server->page_database, &server->socket, temp_packet );
                /* Cleanup */
                wiced_packet_delete( temp_packet );
            }

            wiced_tcp_disconnect( &server->socket );
        }
    }

    wiced_tcp_delete_socket( &server->socket );
    WICED_END_OF_CURRENT_THREAD( );
}


wiced_result_t wiced_simple_https_server_stop (wiced_simple_https_server_t* server)
{
    server->quit = WICED_TRUE;
    if ( wiced_rtos_is_current_thread( &server->thread ) != WICED_SUCCESS )
    {
        wiced_rtos_thread_force_awake( &server->thread );
        wiced_rtos_thread_join( &server->thread );
        wiced_rtos_delete_thread( &server->thread );
    }
    return WICED_SUCCESS;
}

wiced_result_t wiced_simple_https_server_start(wiced_simple_https_server_t* server, uint16_t port, const wiced_http_page_t* page_database, const char* server_cert, const char* server_key, wiced_interface_t interface )
{
    /* Create the TLS socket */
    if (wiced_tcp_create_socket( &server->socket, interface ) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    if ( wiced_tcp_listen( &server->socket, port ) != WICED_SUCCESS)
    {
        wiced_tcp_delete_socket(&server->socket);
        return WICED_ERROR;
    }

    /* Load our security data */
//#ifdef USE_SELF_SIGNED_TLS_CERT
//    if (server_cert == NULL || server_key == NULL )
//    {
//        wiced_tls_init_advanced_context(&server->tls_context, brcm_server_certificate, brcm_server_rsa_key );
//    }
//    else
//#endif
    {
        wiced_tls_init_advanced_context(&server->tls_context, server_cert, server_key );
    }

    wiced_tcp_enable_tls(&server->socket, &server->tls_context);
    server->page_database = page_database;
    return wiced_rtos_create_thread(&server->thread, HTTP_SERVER_THREAD_PRIORITY, "HTTPserver", http_server_thread_main, HTTP_SERVER_STACK_SIZE, server);
}


static wiced_result_t http_server_process_packet(const wiced_http_page_t* page_database, wiced_tcp_socket_t* socket, wiced_packet_t* packet)
{
    char* request_string;
    uint16_t request_length;
    char* start_of_url;
    char* end_of_url;
    wiced_result_t result = WICED_ERROR;
    uint16_t orig_url_length;
    uint16_t new_url_length;
    uint16_t available_data_length;

    if (packet == NULL)
    {
        return WICED_ERROR;
    }

    wiced_packet_get_data( packet, 0, (uint8_t**) &request_string, &request_length, &available_data_length );

    /* Verify we have enough data to start processing */
    if (request_length < 5)  /* TODO : MAGIC NUMBER */
    {
        return WICED_ERROR;
    }

    /* Check that this is a GET request */
    if ( strstr( request_string, "GET " ) != 0 )
    {
        request_string[3] = '\x00';
        start_of_url = &request_string[4];
    }
    else if ( strstr( request_string, "POST " ) != 0 )
    {
        request_string[4] = '\x00';
        start_of_url = &request_string[5];
    }
    else
    {
        result = WICED_ERROR;
        goto cleanup;
    }

    end_of_url = start_of_url;
    /* Find the end of the request path ( space, newline, null, or end of packet ) */
    while ( ( *end_of_url != ' ' ) && ( *end_of_url != '\n' ) && ( *end_of_url != '\x00' ) )
    {
        ++end_of_url;

        /* Check if we've run out of data */
        if ( end_of_url > &request_string[request_length] )
        {
            /* Can't find end of URL, perhaps only part of the request*/
            return WICED_ERROR;
        }
    }

    /* Set the end of url character to a newline  */
    *end_of_url = '\n';

    orig_url_length = (uint16_t)( end_of_url - start_of_url );
    new_url_length = escaped_string_copy( start_of_url, orig_url_length, start_of_url, orig_url_length );
    /* Check if the url has "shrunk" due to escaped character replacement */
    if (new_url_length != orig_url_length)
    {
        /* Set the "extra" characters to null  */
        memset( start_of_url + new_url_length, '\x00', (size_t) ( orig_url_length - new_url_length ) );
    }
    process_url_request( (wiced_tcp_socket_t*)socket, page_database, start_of_url, new_url_length );

    result = WICED_SUCCESS;

cleanup:
    return result;
}

static wiced_result_t process_url_request( wiced_tcp_socket_t* socket, const wiced_http_page_t* server_url_list, char * url, int url_len )
{
    /* Search the url to find the question mark if there is one */
    char * params = url;
    int i = 0;
    int params_len = url_len;
    wiced_tcp_stream_t stream;
    wiced_bool_t found = WICED_FALSE;

    while ( ( *params != '?' ) && ( params_len > 0 ) )
    {
        params++;
        params_len--;
    }

    /* terminate the path part of the string with a null - will replace the question mark */
    *params = '\x00';

    /* increment the pointer to the parameter query part of the url to skip over the null which was just written */
    params++;

    WPRINT_WEBSERVER_DEBUG(("Processing request for: %s\n", url));

    /* Init the tcp stream */
    wiced_tcp_stream_init(&stream, socket);

    /* Search URL list to determine if request matches one of our pages */
    while ( server_url_list[i].url != NULL )
    {
        /* Compare request to a path */
        if ( 0 == strcmp( server_url_list[i].url, url ) )
        {
            /* Matching path found */
            found = WICED_TRUE;

            /* Call the content handler function to write the page content into the packet and adjust the write pointers */
            switch (server_url_list[i].url_content_type)
            {
                case WICED_DYNAMIC_URL_CONTENT:
                    wiced_simple_http_write_reply_header(&stream, server_url_list[i].mime_type, WICED_TRUE);
                    /* Fall through */
                case WICED_RAW_DYNAMIC_URL_CONTENT:
                    server_url_list[i].url_content.dynamic_data.generator( params, &stream, server_url_list[i].url_content.dynamic_data.arg );
                    wiced_tcp_stream_flush(&stream);
                    break;

                case WICED_STATIC_URL_CONTENT:
                    wiced_simple_http_write_reply_header(&stream, server_url_list[i].mime_type, WICED_FALSE);
                    /* Fall through */
                case WICED_RAW_STATIC_URL_CONTENT:
                    wiced_tcp_stream_write(&stream, server_url_list[i].url_content.static_data.ptr, server_url_list[i].url_content.static_data.length);
                    wiced_tcp_stream_flush(&stream);
                    break;
                case WICED_RESOURCE_URL_CONTENT:
                    wiced_simple_http_write_reply_header(&stream, server_url_list[i].mime_type, WICED_FALSE);
                    /* Fall through */
                case WICED_RAW_RESOURCE_URL_CONTENT:
                    wiced_tcp_stream_write_resource( &stream, server_url_list[i].url_content.resource_data );
                    wiced_tcp_stream_flush( &stream );
                    break;
                default:
                    wiced_assert("Unknown entry in URL list", 0 != 0 );
                    break;

            }
            break;
        }
        i++;
    }

    /* Check if page was not found */
    if ( found == WICED_FALSE )
    {
        /* Send back 404 */
        wiced_tcp_stream_write(&stream, not_found_header, sizeof(not_found_header)-1);
        wiced_tcp_stream_flush(&stream);
    }

    wiced_assert( "Page Serve finished with data still in stream", stream.tx_packet == NULL );

    return WICED_SUCCESS;
}

wiced_result_t wiced_simple_http_write_reply_header(wiced_tcp_stream_t* stream, const char* mime_type, wiced_bool_t nocache )
{
    /* Copy HTTP OK header into packet and adjust the write pointers */
    wiced_tcp_stream_write( stream, ok_header, sizeof( ok_header ) - 1 );

    /* Add Mime type */
    wiced_tcp_stream_write( stream, mime_type, strlen( mime_type ) );

    /* Output No-Cache headers if required */
    if ( nocache == WICED_TRUE )
    {
        wiced_tcp_stream_write( stream, NO_CACHE_HEADER, sizeof( NO_CACHE_HEADER ) - 1 );
    }

    /* Add double carriage return, line feed */
    wiced_tcp_stream_write( stream, crlfcrlf, sizeof( crlfcrlf ) - 1 );

    return WICED_SUCCESS;
}

static uint16_t escaped_string_copy( char* output, uint16_t output_length, const char* input, uint16_t input_length )
{
    uint16_t bytes_copied;
    int a;
    for (bytes_copied = 0; (input_length > 0) && (bytes_copied != output_length); ++bytes_copied)
    {
        if (*input == '%')
        {
            if (*(input+1) == '%')
            {
                ++input;
                *output = '%';
            }
            else
            {
                *output = 0;
                for (a=0; a < 2; ++a)
                {
                    *output = (char) ( *output << 4 );
                    ++input;
                    if (*input >= '0' && *input <= '9')
                    {
                        *output = (char) ( *output + *input - '0' );
                    }
                    else if (*input >= 'a' && *input <= 'f')
                    {
                        *output = (char) ( *output + *input - 'a' + 10 );
                    }
                    else if (*input >= 'A' && *input <= 'F')
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
