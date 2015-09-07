/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "wiced.h"
#include "ota_server.h"
#include "ota_server_web_page.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define OTA_SERVER_PORT (80)

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
static int process_upgrade_chunk( ota_http_request_message_t* request, wiced_tcp_stream_t* stream, void* arg );

/******************************************************
 *               Variables Definitions
 ******************************************************/
ota_server_t ota_server;
OTA_START_OF_HTTP_PAGE_DATABASE(ota_server_page_database)
        OTA_ROOT_HTTP_PAGE_REDIRECT("/wiced_ota_server/upgrade.html"),
        { "/wiced_ota_server/upgrade.html",        "text/html",  OTA_RESOURCE_URL_CONTENT,   .ota_url_content.ota_resource_data  = &resource_upgrade_web_page, },
        { "/wiced_ota_server/mystyle.css",         "text/css",   OTA_RESOURCE_URL_CONTENT,   .ota_url_content.ota_resource_data  = &resource_upgrade_page_stylesheet, },
        { "/wiced_ota_server/upgrade_chunk.html",  "text/html",  OTA_DYNAMIC_URL_CONTENT,    .ota_url_content.ota_dynamic_data   = {process_upgrade_chunk, 0 } },
OTA_END_OF_HTTP_PAGE_DATABASE();


/******************************************************
 *               Function Definitions
 ******************************************************/
wiced_result_t wiced_ota_server_start( wiced_interface_t interface )
{
    wiced_result_t result = WICED_SUCCESS;

    result = ota_server_start( &ota_server, OTA_SERVER_PORT, &ota_server_page_database[0], interface );
    wiced_assert("Failed to start ota server", result == WICED_SUCCESS);

    return result;
}

wiced_result_t wiced_ota_server_stop ( void )
{
    wiced_result_t result = WICED_SUCCESS;
    result =  ota_server_stop(&ota_server);
    wiced_assert("Failed to stop ota server", result == WICED_SUCCESS);

    return result;
}


static int process_upgrade_chunk( ota_http_request_message_t* request, wiced_tcp_stream_t* stream, void* arg )
{
    int              i           = 0;
    uint32_t         offset      = 0;
    uint32_t         file_size   = 0;
    static uint32_t  chunk_count = 0;
    static uint32_t  expected_offset = 0;
    uint32_t         received_offset = 0;
    char             offset_string[13];
    static wiced_app_t app;


    offset    = atoi( strstr((char*)request->query_ptr, "offset=") + strlen("offset=") );
    file_size = atoi(strstr((char*)request->query_ptr, "filesize=") + strlen("filesize=") );
    received_offset = offset;

    if( expected_offset != offset )
    {
        memset(offset_string, 0x00, sizeof(offset_string));
        sprintf(offset_string, "%lu", expected_offset);
        wiced_tcp_stream_write( stream, offset_string, strlen(offset_string));
        return 0;
    }


    if (offset == 0)
    {
        uint32_t    current_size;
        if (wiced_framework_app_open( DCT_APP0_INDEX, &app ) != WICED_SUCCESS)
        {
            return -1;
        }
        if (wiced_framework_app_get_size( &app, &current_size ) != WICED_SUCCESS)
        {
            return -1;
        }
        if (wiced_framework_app_set_size( &app, file_size) != WICED_SUCCESS)
        {
            return -1;
        }
        if (wiced_framework_app_get_size( &app, &current_size ) != WICED_SUCCESS)
        {
            return -1;
        }
        if ( current_size < file_size )
        {
            printf("Error setting application size!!\n");
            return -1;
        }
    }

    chunk_count++;

    for( i = 0; i < 3; i++ )
    {
        if( request->body_chunks[i].size != 0 )
        {
            printf("Writing chunk %lu of size %d from offset %lu \r\n", chunk_count, request->body_chunks[i].size, offset);
            wiced_framework_app_write_chunk( &app, request->body_chunks[i].data, request->body_chunks[i].size );
            offset += request->body_chunks[i].size;
        }
    }

    if( offset == file_size )
    {
        printf("Uploaded file size = %lu\r\n", offset);
        wiced_framework_app_close( &app );
        wiced_framework_set_boot ( DCT_APP0_INDEX, PLATFORM_DEFAULT_LOAD );
        printf("Restarting.. \r\n");
        ota_server.reboot_required = WICED_TRUE;
        ota_server.quit = WICED_TRUE;
        chunk_count = 0;
        expected_offset = 0;

        /* Finally we will send to the server the size of the file, so it knows that we are done and no more */
        /* chunks are needed to be sent */
        memset(offset_string, 0x00, sizeof(offset_string));
        sprintf(offset_string, "%lu", file_size);
        wiced_tcp_stream_write( stream, offset_string, strlen(offset_string));
    }
    else
    {
        expected_offset = received_offset + 1024;
        memset(offset_string, 0x00, sizeof(offset_string));
        sprintf(offset_string, "%lu", expected_offset);
        wiced_tcp_stream_write( stream, offset_string, strlen(offset_string));
    }

    return 0;
}
