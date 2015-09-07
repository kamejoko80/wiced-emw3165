/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wiced_defaults.h"
#include "wiced_tcpip.h"
#include "wiced_rtos.h"
#include "wiced_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define OTA_HTTP_REDIRECT(url) "HTTP/1.0 301\r\nLocation: " url "\r\n\r\n"

#define OTA_HTTP_404 \
    "HTTP/1.0 404 Not Found\r\n" \
    "Content-Type: text/html\r\n\r\n" \
    "<!doctype html>\n" \
    "<html><head><title>404 - WICED Web Server</title></head><body>\n" \
    "<h1>Address not found on WICED Web Server</h1>\n" \
    "<p><a href=\"/\">Return to home page</a></p>\n" \
    "</body>\n</html>\n"

#define OTA_START_OF_HTTP_PAGE_DATABASE(name) \
    const ota_http_page_t name[] = {

#define OTA_ROOT_HTTP_PAGE_REDIRECT(url) \
    { "/", "", OTA_RAW_STATIC_URL_CONTENT, .ota_url_content.ota_static_data  = {OTA_HTTP_REDIRECT(url), sizeof(OTA_HTTP_REDIRECT(url))-1 } }

#define OTA_END_OF_HTTP_PAGE_DATABASE() \
    {0,0,0, .ota_url_content.ota_static_data  = {NULL, 0 } } \
    }

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef struct
{
    uint8_t* data;
    uint16_t size;
} http_body_chunk_t;

typedef struct
{
    wiced_packet_t*           request_packets[3];
    http_body_chunk_t         body_chunks[3];
    uint16_t                  current_packet_index;
    uint8_t*                  request_type;
    uint16_t                  request_type_length;
    uint8_t*                  header_ptr;
    uint16_t                  header_size;
    uint8_t*                  protocol_type_ptr;
    uint16_t                  protocol_type_length;
    uint8_t*                  url_ptr;
    uint16_t                  url_length;
    uint8_t*                  query_ptr;
    uint16_t                  query_length;
    uint16_t                  content_length;
} ota_http_request_message_t;


/******************************************************
 *                    Structures
 ******************************************************/

typedef int (*ota_http_request_processor_t)( ota_http_request_message_t* , wiced_tcp_stream_t* stream, void* arg );

typedef struct
{
    const char* const ota_url;
    const char* const ota_mime_type;
    enum
    {
        OTA_STATIC_URL_CONTENT,
        OTA_DYNAMIC_URL_CONTENT,
        OTA_RESOURCE_URL_CONTENT,
        OTA_RAW_STATIC_URL_CONTENT,
        OTA_RAW_DYNAMIC_URL_CONTENT,
        OTA_RAW_RESOURCE_URL_CONTENT
    } ota_url_content_type;
    union
    {
        struct
        {
            const ota_http_request_processor_t generator;
            void*                 arg;
        } ota_dynamic_data;
        struct
        {
            const void* ptr;
            uint32_t length;
        } ota_static_data;
        const resource_hnd_t* ota_resource_data;
    } ota_url_content;
} ota_http_page_t;


typedef enum
{
    READING_HEADER,
    READING_BODY
} ota_server_connection_state_t;

typedef struct
{
    wiced_tcp_socket_t              socket;
    ota_server_connection_state_t   state;
    volatile wiced_bool_t           quit;
    wiced_thread_t                  thread;
    const ota_http_page_t*          page_database;
    ota_http_request_message_t      request;
    wiced_bool_t                    reboot_required;
} ota_server_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t ota_server_start       ( ota_server_t* server, uint16_t port, const ota_http_page_t* page_database, wiced_interface_t interface );
wiced_result_t ota_server_stop        ( ota_server_t* server );

#ifdef __cplusplus
} /* extern "C" */
#endif
