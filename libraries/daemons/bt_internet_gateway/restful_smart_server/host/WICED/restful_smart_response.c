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
#include "smartbridge.h"
#include "restful_smart_server.h"
#include "restful_smart_response.h"
#include "restful_smart_ble.h"
#include "restful_smart_constants.h"
#include "http_server.h"
#include "wiced_bt_dev.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define LTK_MASK  0x08
#define EDIV_MASK 0x04
#define IRK_MASK  0x02
#define CSRK_MASK 0x01

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

/******************************************************
 *               Variables Definitions
 ******************************************************/

const char slash_gatt_slash_nodes_slash      [] = "/gatt/nodes/";
const char slash_management_slash_nodes_slash[] = "/management/nodes/";
const char slash_gap_slash_nodes_slash       [] = "/gap/nodes/";
const char slash_services_slash              [] = "/services/";
const char slash_characteristics_slash       [] = "/characteristics/";
const char slash_descriptors_slash           [] = "/descriptors/";
const char json_data_self_start              [] = "\"self\"       : { \"href\" = \"http://";
const char json_data_self_end                [] = "\"},\r\n";
const char json_data_bdaddr                  [] = "\"bdaddr\"     : \"";
const char json_data_handle                  [] = "\"handle\"     : \"";
const char json_data_uuid                    [] = "\"uuid\"       : \"";
const char json_data_primary                 [] = "\"primary\"    : \"";
const char json_data_properties              [] = "\"properties\" : \"";
const char json_data_value                   [] = "\"value\"      : \"";
const char json_data_end                     [] = "\",\r\n";
const char json_data_end2                    [] = ",\r\n";
const char json_data_end3                    [] = "\"\r\n";
const char json_data_end4                    [] = "\"\r\n}\r\n";
const char json_node_array_start             [] = "{\r\n\"nodes\" : [\r\n";
const char json_service_array_start          [] = "{\r\n\"services\" : [\r\n";
const char json_characteristic_array_start   [] = "{\r\n\"characteristics\" : [\r\n";
const char json_descriptor_array_start       [] = "{\r\n\"descriptor\" : [\r\n";
const char json_array_end                    [] = "]\r\n},\r\n";
const char json_object_start                 [] = "{\r\n";
const char json_object_end                   [] = "},\r\n";
const char json_data_bdaddrtype              [] = "\"bdaddrType\" : \"";
const char json_data_ltk                     [] = "\"LTK\"        : ";
const char json_data_ediv                    [] = "\"EDIV\"       : ";
const char json_data_irk                     [] = "\"IRK\"        : ";
const char json_data_csrk                    [] = "\"CSRK\"       : ";
const char json_data_rssi                    [] = "\"rssi\"       : ";
const char json_data_name                    [] = "\"name\"       : \"";
const char json_adv_array_start              [] = "\"AD\"         : [\r\n";
const char json_adv_type                     [] = "\"ADType\"     : ";
const char json_adv_value                    [] = "\"ADValue\"    : \"";
const char json_data_true                    [] = "true";
const char json_data_false                   [] = "false";
const char json_data_pairing_status_code     [] = "pairingStatusCode:  ";
const char json_data_pairing_status          [] = "pairingStatus:      \"";
const char json_data_pairing_id              [] = "pairingID:          \"0x";
const char json_data_display                 [] = "display:            \"";
const char json_data_bdaddra                 [] = "bdaddra:            \"";
const char json_data_ra                      [] = "ra:                 \"0x";
const char json_data_ca                      [] = "ca:                 \"0x";

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t restful_gateway_write_status_code( wiced_http_response_stream_t* stream, bt_rest_gateway_status_t status )
{
    /*HTTP/1.1 <status code>\r\n*/
//    WICED_VERIFY( wiced_http_response_stream_write( stream, restful_smart_http_status_table[ status ], strlen( restful_smart_http_status_table[ status ] ) ) );
//    WICED_VERIFY( wiced_http_response_stream_write( stream, CRLF, sizeof( CRLF )-1 ) );
//    return WICED_SUCCESS;

    return wiced_http_response_stream_write_header( stream, restful_smart_http_status_table[status], CHUNKED_CONTENT_LENGTH, HTTP_CACHE_DISABLED, MIME_TYPE_JSON );
}

wiced_result_t restful_gateway_write_node( wiced_http_response_stream_t* stream, const char* node_handle, const char* bdaddr )
{
    wiced_ip_address_t address;
    char               address_string[16];

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start, sizeof( json_object_start ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/gatt/nodes/<node1>"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,         sizeof( json_data_self_start         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,               strlen( address_string               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_gatt_slash_nodes_slash, sizeof( slash_gatt_slash_nodes_slash ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node_handle,                  strlen( node_handle                  )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,           sizeof( json_data_self_end           ) - 1 ) );

    /* "handle" : "<node1>", */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_handle, sizeof( json_data_handle ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node_handle,      strlen( node_handle      )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,    sizeof( json_data_end    ) - 1 ) );

    /* "bdaddr" : "<bdaddr1>" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_bdaddr, sizeof( json_data_bdaddr ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, bdaddr,           strlen( bdaddr           )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,    sizeof( json_data_end    ) - 1 ) );

    /* },\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end, sizeof( json_object_end ) - 1 ) );

    return WICED_SUCCESS;

}

wiced_result_t restful_gateway_write_service( wiced_http_response_stream_t* stream, const char* node, const char* service, uint16_t handle, const char* uuid, wiced_bool_t is_primary_service )
{
    wiced_ip_address_t address;
    char               address_string[16];
    char               handle_string[5];
    char*              is_primary_string;

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start, sizeof( json_object_start ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/gatt/nodes/<node1>/services/<service1>"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,         sizeof( json_data_self_start         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,               strlen( address_string               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_gatt_slash_nodes_slash, sizeof( slash_gatt_slash_nodes_slash ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node,                         strlen( node                         )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_services_slash,         sizeof( slash_services_slash         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, service,                      strlen( service                      )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,           sizeof( json_data_self_end           ) - 1 ) );

    /* "handle" : "<service1>",\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_handle, sizeof( json_data_handle ) - 1 ) );
    unsigned_to_hex_string( (uint32_t)handle, handle_string, 4, 4 );
    WICED_VERIFY( wiced_http_response_stream_write( stream, handle_string,    strlen( handle_string    )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,    sizeof( json_data_end    ) - 1 ) );

    /* "uuid" : "<uuid1>,\r\n" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_uuid, sizeof( json_data_uuid ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, uuid,           strlen( uuid           )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,  sizeof( json_data_end  ) - 1 ) );

    /* "primary" : "<true/false>,\r\n" */
    is_primary_string = ( is_primary_service == WICED_TRUE ) ? "true" : "false";
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_primary, sizeof( json_data_primary ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, is_primary_string, strlen( is_primary_string )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,     sizeof( json_data_end     ) - 1 ) );

    /* },\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end, sizeof( json_object_end ) - 1 ) );

    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_characteristic( wiced_http_response_stream_t* stream, const char* node, const char* characteristic, uint16_t handle, const char* uuid, uint8_t properties )
{
    wiced_ip_address_t address;
    char               address_string[16];
    char               handle_string[5];
    char               properties_string[3];

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start, sizeof( json_object_start ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/gatt/nodes/<node1>/characteristics/<characteristic1>"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,         sizeof( json_data_self_start         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,               strlen( address_string               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_gatt_slash_nodes_slash, sizeof( slash_gatt_slash_nodes_slash ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node,                         strlen( node                         )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_characteristics_slash,  sizeof( slash_characteristics_slash  ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, characteristic,               strlen( characteristic               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,           sizeof( json_data_self_end           ) - 1 ) );

    /* "handle" : "<characteristic1>",\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_handle, sizeof( json_data_handle ) - 1 ) );
    unsigned_to_hex_string( (uint32_t)handle, handle_string, 4, 4 );
    WICED_VERIFY( wiced_http_response_stream_write( stream, handle_string,    strlen( handle_string    )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,    sizeof( json_data_end    ) - 1 ) );

    /* "uuid" : "<uuid1>,\r\n" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_uuid, sizeof( json_data_handle ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, uuid,           strlen( uuid             )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,  sizeof( json_data_end    ) - 1 ) );

    /* "properties" : "<prop1>,\r\n" */
    memset( &properties_string, 0, sizeof( properties_string ) );
    unsigned_to_hex_string( (uint32_t)properties, properties_string, 2, 2 );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_properties, sizeof( json_data_properties ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, properties_string,    strlen( properties_string    )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,        sizeof( json_data_end        ) - 1 ) );

    /* },\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end, sizeof( json_object_end ) - 1 ) );

    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_descriptor( wiced_http_response_stream_t* stream, const char* node, const char* descriptor, uint16_t handle, const char* uuid )
{
    wiced_ip_address_t address;
    char               address_string[16];
    char               handle_string[5];

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start, sizeof( json_object_start ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/gatt/nodes/<node1>/descriptors/<descriptor1>"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,         sizeof( json_data_self_start         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,               strlen( address_string               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_gatt_slash_nodes_slash, sizeof( slash_gatt_slash_nodes_slash ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node,                         strlen( node                         )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_descriptors_slash,      sizeof( slash_descriptors_slash      ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, descriptor,                   strlen( descriptor                   )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,           sizeof( json_data_self_end           ) - 1 ) );

    /* "handle" : "<descriptor1>",\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_handle, sizeof( json_data_handle ) - 1 ) );
    unsigned_to_hex_string( (uint32_t)handle, handle_string, 4, 4 );
    WICED_VERIFY( wiced_http_response_stream_write( stream, handle_string,    strlen( handle_string    )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,    sizeof( json_data_end    ) - 1 ) );

    /* "uuid" : "<uuid1>,\r\n" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_uuid, sizeof( json_data_handle ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, uuid,           strlen( uuid             )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,  sizeof( json_data_end    ) - 1 ) );

    /* },\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end, sizeof( json_object_end ) - 1 ) );

    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_long_characteristic_value_start( wiced_http_response_stream_t* stream, const char* node_handle, const char* characteristic_value_handle )
{
    wiced_ip_address_t address;
    char               address_string[16];

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start, sizeof( json_object_start ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/gatt/nodes/<node1>/characteristics/<characteristic1>"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,         sizeof( json_data_self_start         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,               strlen( address_string               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_gatt_slash_nodes_slash, sizeof( slash_gatt_slash_nodes_slash ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node_handle,                  strlen( node_handle                  )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_characteristics_slash,  sizeof( slash_characteristics_slash  ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, characteristic_value_handle,  strlen( characteristic_value_handle  )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,           sizeof( json_data_self_end           ) - 1 ) );

    /* "handle" : "<characteristic_value_handle>",\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_handle,            sizeof( json_data_handle            ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, characteristic_value_handle, strlen( characteristic_value_handle )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,               sizeof( json_data_end               ) - 1 ) );

    /* "value" : "<value>,\r\n" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_value, sizeof( json_data_value ) - 1 ) );
    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_long_partial_characteristic_value( wiced_http_response_stream_t* stream, const uint8_t* partial_value, uint32_t length )
{
    uint32_t a;

    for ( a = 0; a < length; a++ )
    {
        char value_char[3];

        memset( value_char, 0, sizeof( value_char ) );
        unsigned_to_hex_string( partial_value[a], value_char, 2, 2 );
        WICED_VERIFY( wiced_http_response_stream_write( stream, value_char, 2 ) );
    }
    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_long_characteristic_value_end( wiced_http_response_stream_t* stream )
{
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,  sizeof( json_data_end    ) - 1 ) );

    /* },\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end, sizeof( json_object_end ) - 1 ) );
    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_characteristic_value( restful_smart_response_stream_t* stream, const char* node, const char* characteristic, uint16_t handle, const uint8_t* value, uint32_t value_length )
{
    wiced_ip_address_t address;
    char               address_string[16];
    char               handle_string[5];
    int32_t            a;

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start, sizeof( json_object_start ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/gatt/nodes/<node1>/characteristics/<characteristic1>"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,         sizeof( json_data_self_start         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,               strlen( address_string               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_gatt_slash_nodes_slash, sizeof( slash_gatt_slash_nodes_slash ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node,                         strlen( node                         )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_characteristics_slash,  sizeof( slash_characteristics_slash  ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, characteristic,               strlen( characteristic               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,           sizeof( json_data_self_end           ) - 1 ) );

    /* "handle" : "<handle>",\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_handle, sizeof( json_data_handle ) - 1 ) );
    unsigned_to_hex_string( (uint32_t)handle, handle_string, 4, 4 );
    WICED_VERIFY( wiced_http_response_stream_write( stream, handle_string,    strlen( handle_string    )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,    sizeof( json_data_end    ) - 1 ) );

    /* "value" : "<value>,\r\n" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_value, sizeof( json_data_value ) - 1 ) );
    for ( a = value_length - 1; a >= 0; a-- )
    {
        char value_char[3];

        memset( value_char, 0, sizeof( value_char ) );
        unsigned_to_hex_string( value[a], value_char, 2, 2 );
        WICED_VERIFY( wiced_http_response_stream_write( stream, value_char, 2 ) );
    }
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,  sizeof( json_data_end    ) - 1 ) );

    /* },\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end, sizeof( json_object_end ) - 1 ) );

    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_descriptor_value( restful_smart_response_stream_t* stream, const char* node, const char* descriptor_value_handle, const uint8_t* value, uint32_t value_length )
{
    wiced_ip_address_t address;
    char               address_string[16];
    int32_t            a;

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start, sizeof( json_object_start ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/gatt/nodes/<node1>/descriptors/<descriptor1>/value"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,         sizeof( json_data_self_start         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,               strlen( address_string               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_gatt_slash_nodes_slash, sizeof( slash_gatt_slash_nodes_slash ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node,                         strlen( node                         )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_descriptors_slash,      sizeof( slash_descriptors_slash      ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, descriptor_value_handle,      strlen( descriptor_value_handle      )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,           sizeof( json_data_self_end           ) - 1 ) );

    /* "value" : "<value>,\r\n" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_value, sizeof( json_data_value ) - 1 ) );
    for ( a = value_length - 1; a >= 0; a-- )
    {
        char value_char[3];

        memset( value_char, 0, sizeof( value_char ) );
        unsigned_to_hex_string( value[a], value_char, 2, 2 );
        WICED_VERIFY( wiced_http_response_stream_write( stream, value_char, 2 ) );
    }
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,  sizeof( json_data_end    ) - 1 ) );

    /* },\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end, sizeof( json_object_end ) - 1 ) );

    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_cached_value( wiced_http_response_stream_t* stream, const char* handle, const char* value )
{
    return WICED_SUCCESS;
}

wiced_result_t restful_smart_write_bonded_nodes( wiced_http_response_stream_t* stream, const char* node_handle, const char* bdaddr, const char* bdaddrtype, uint8_t key_mask )
{
    wiced_ip_address_t address;
    char               address_string[16];
    char*              json_data_state;

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start, sizeof( json_object_start ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/management/nodes/<node1>"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,               sizeof( json_data_self_start               ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,                     strlen( address_string                     )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_management_slash_nodes_slash, sizeof( slash_management_slash_nodes_slash ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node_handle,                        strlen( node_handle                        )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,                 sizeof( json_data_self_end                 ) - 1 ) );

    /* "handle" : "<node>", */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_handle,                   sizeof( json_data_handle                   ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node_handle,                        strlen( node_handle                        )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,                      sizeof( json_data_end                      ) - 1 ) );

    /* "bdaddr" : "<bdaddr1>" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_bdaddr,                   sizeof( json_data_bdaddr                   ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, bdaddr,                             strlen( bdaddr                             )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,                      sizeof( json_data_end                      ) - 1 ) );

    /* "bdaddrType" : "<bdaddrType>", */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_bdaddrtype,               sizeof( json_data_bdaddrtype               ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, bdaddrtype,                         strlen( bdaddrtype                         )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,                      sizeof( json_data_end                      ) - 1 ) );

    /* "LTK" : <true|false>, */
    json_data_state = (char*)( ( key_mask & LTK_MASK ) ? json_data_true : json_data_false );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_ltk,                      sizeof( json_data_ltk                      ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_state,                   strlen( json_data_state                   )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end2,                     sizeof( json_data_end2                     ) - 1 ) );

    /* "EDIV" : <true|false>, */
    json_data_state = (char*)( ( key_mask & EDIV_MASK ) ? json_data_true : json_data_false );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_ediv,                     sizeof( json_data_ediv                     ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_state,                   strlen( json_data_state                   )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end2,                     sizeof( json_data_end2                     ) - 1 ) );

    /* "IRK" : <true|false>, */
    json_data_state = (char*)( ( key_mask & IRK_MASK ) ? json_data_true : json_data_false );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_irk,                      sizeof( json_data_irk                      ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_state,                   strlen( json_data_state                   )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end2,                     sizeof( json_data_end2                     ) - 1 ) );

    /* "CSRK" : <true|false>, */
    json_data_state = (char*)( ( key_mask & CSRK_MASK ) ? json_data_true : json_data_false );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_csrk,                     sizeof( json_data_csrk                     ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_state,                   strlen( json_data_state                   )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end2,                     sizeof( json_data_end2                     ) - 1 ) );

    /* },\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end,                    sizeof( json_object_end                    ) - 1 ) );

    return WICED_SUCCESS;
}

wiced_result_t restful_smart_write_gap_node( wiced_http_response_stream_t* stream, const char* node_handle, const char* bdaddr, const char* bdaddrtype, const char* rssi )
{
    wiced_ip_address_t address;
    char               address_string[16];

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start,            sizeof( json_object_start            ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/gap/nodes/<node1>"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,         sizeof( json_data_self_start         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,               strlen( address_string               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_gap_slash_nodes_slash,  sizeof( slash_gap_slash_nodes_slash  ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node_handle,                  strlen( node_handle                  )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,           sizeof( json_data_self_end           ) - 1 ) );

    /* "handle" : "<node>", */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_handle,             sizeof( json_data_handle             ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node_handle,                  strlen( node_handle                  )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,                sizeof( json_data_end                ) - 1 ) );

    /* "bdaddr" : "<bdaddr>" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_bdaddr,             sizeof( json_data_bdaddr             ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, bdaddr,                       strlen( bdaddr                       )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,                sizeof( json_data_end                ) - 1 ) );

    if( bdaddrtype != NULL )
    {
        /* "bdaddrType" : "<bdaddrType>", */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_bdaddrtype,     sizeof( json_data_bdaddrtype         ) - 1 ) );
        WICED_VERIFY( wiced_http_response_stream_write( stream, bdaddrtype,               strlen( bdaddrtype                   )     ) );
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,            sizeof( json_data_end                ) - 1 ) );
    }

    if( rssi != NULL )
    {
    /* "rssi" : "<signed value>", */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_rssi,           sizeof( json_data_rssi               ) - 1 ) );
        WICED_VERIFY( wiced_http_response_stream_write( stream, rssi,                     strlen( rssi                         )     ) );
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end2,           sizeof( json_data_end2               ) - 1 ) );
    }

    if( ( bdaddrtype == NULL ) && ( rssi == NULL ) )
    {
        /* },\r\n */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end,          sizeof( json_object_end              ) - 1 ) );
    }

    return WICED_SUCCESS;
}

wiced_result_t restful_smart_write_adv_data( wiced_http_response_stream_t* stream, const char* adv_type, const char* adv_data )
{
    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start, sizeof( json_object_start ) - 1 ) );

    /* "ADType" : <type>, */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_adv_type,     sizeof( json_adv_type     ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, adv_type,          strlen( adv_type          )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end2,    sizeof( json_data_end2    ) - 1 ) );

    /* "ADValue" : "<value>" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_adv_value,    sizeof( json_adv_value    ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, adv_data,          strlen( adv_data          )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end3,    sizeof( json_data_end3    ) - 1 ) );

    /* },\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_end,   sizeof( json_object_end   ) - 1 ) );

    return WICED_SUCCESS;
}

wiced_result_t restful_smart_write_node_name( wiced_http_response_stream_t* stream, const char* node_handle, const char* bdaddr, const char* node_name )
{
    wiced_ip_address_t address;
    char               address_string[16];

    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start,            sizeof( json_object_start            ) - 1 ) );

    /* "self" : {"href"="http://<gateway>/gatt/nodes/<node1>"},\r\n */
    wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address );
    memset( &address_string, 0, sizeof( address_string ) );
    ip_to_str( (const wiced_ip_address_t*)&address, address_string );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_start,         sizeof( json_data_self_start         ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, address_string,               strlen( address_string               )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, slash_gatt_slash_nodes_slash, sizeof( slash_gatt_slash_nodes_slash ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node_handle,                  strlen( node_handle                  )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_self_end,           sizeof( json_data_self_end           ) - 1 ) );

    /* "name" : "<name>" */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_name,               sizeof( json_data_name               ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, node_name,                    strlen( node_name                    )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end4,               sizeof( json_data_end4               ) - 1 ) );

    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_bonding( wiced_http_response_stream_t* stream, const char* node_handle, uint8_t code, const char* pairingID, const char* display, const char* ra, const char* ca )
{
    /* {\r\n */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_object_start,                        sizeof( json_object_start                        ) - 1 ) );

    /* pairingStatusCode: */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_pairing_status_code,            sizeof( json_data_pairing_status_code            ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, pairing_status_table[code].status_code,   strlen( pairing_status_table[code].status_code   )     ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end2,                           sizeof( json_data_end2                           ) - 1 ) );

    /* pairingStatus: */
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_pairing_status,                 sizeof( json_data_pairing_status                 ) - 1 ) );
    WICED_VERIFY( wiced_http_response_stream_write( stream, pairing_status_table[code].status_string, strlen( pairing_status_table[code].status_string )     ) );

    if( display != NULL )
    {
        /* ",\r\n */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,            sizeof( json_data_end                ) - 1 ) );


        /* display: "0x */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_display,        sizeof( json_data_display            ) - 1 ) );
        WICED_VERIFY( wiced_http_response_stream_write( stream, display,                  strlen( display                      )     ) );
    }

    if( pairingID != NULL )
    {
        /* ",\r\n */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,            sizeof( json_data_end                ) - 1 ) );

        /* pairingID: "0x */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_pairing_id,     sizeof( json_data_pairing_id         ) - 1 ) );
        WICED_VERIFY( wiced_http_response_stream_write( stream, pairingID,                strlen( pairingID                    )     ) );
    }

    if( ra != NULL )
    {
        /* ",\r\n */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,            sizeof( json_data_end                ) - 1 ) );

        /* bdaddra: " */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_bdaddra,        sizeof( json_data_bdaddra            ) - 1 ) );
        WICED_VERIFY( wiced_http_response_stream_write( stream, node_handle,              strlen( node_handle                  )     ) );

        /* ",\r\n */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,            sizeof( json_data_end                ) - 1 ) );

        /* ra: "0x */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_ra,             sizeof( json_data_ra                 ) - 1 ) );
        WICED_VERIFY( wiced_http_response_stream_write( stream, ra,                       strlen( ra                           )     ) );

        /* ",\r\n */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end,            sizeof( json_data_end                ) - 1 ) );

        /* ca: "0x */
        WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_ca,             sizeof( json_data_ca                 ) - 1 ) );
        WICED_VERIFY( wiced_http_response_stream_write( stream, ca,                       strlen( ca                           )     ) );
    }

    WICED_VERIFY( wiced_http_response_stream_write( stream, json_data_end4,               sizeof( json_data_end4               ) - 1 ) );

    return WICED_SUCCESS;
}

wiced_result_t restful_gateway_write_node_array_start( wiced_http_response_stream_t* stream )
{
    return wiced_http_response_stream_write( stream, json_node_array_start, sizeof( json_node_array_start ) - 1 );
}

wiced_result_t restful_gateway_write_service_array_start( wiced_http_response_stream_t* stream )
{
    return wiced_http_response_stream_write( stream, json_service_array_start, sizeof( json_service_array_start ) - 1 );
}

wiced_result_t restful_gateway_write_characteristic_array_start( wiced_http_response_stream_t* stream )
{
    return wiced_http_response_stream_write( stream, json_characteristic_array_start, sizeof( json_characteristic_array_start ) - 1 );
}

wiced_result_t restful_gateway_write_descriptor_array_start( wiced_http_response_stream_t* stream )
{
    return wiced_http_response_stream_write( stream, json_descriptor_array_start, sizeof( json_descriptor_array_start ) - 1 );
}

wiced_result_t restful_smart_write_adv_array_start( wiced_http_response_stream_t* stream )
{
    return wiced_http_response_stream_write( stream, json_adv_array_start, sizeof( json_adv_array_start ) - 1 );
}

wiced_result_t restful_gateway_write_array_end( wiced_http_response_stream_t* stream )
{
    WICED_VERIFY( wiced_http_response_stream_write( stream, json_array_end,  sizeof( json_array_end  ) - 1 ) );
    return WICED_SUCCESS;
}

void string_to_device_address( const char* string, smart_address_t* address )
{
    uint32_t i;
    uint32_t j;

    /* BD_ADDR is passed within the URL. Parse it here */
    for ( i = 0, j = 0; i < 6; i++, j += 2 )
    {
        char  buffer[3] = {0};
        char* end;

        buffer[0] = string[j];
        buffer[1] = string[j + 1];
        address->address[i] = strtoul( buffer, &end, 16 );
    }
}

void device_address_to_string( const smart_address_t* device_address, char* string )
{
    uint8_t a;

    for ( a = 0; a < 6; a++ )
    {
        unsigned_to_hex_string( (uint32_t)device_address->address[a], string, 2, 2 );
        string += 2;
    }
}

void string_to_uuid( const char* string, wiced_bt_uuid_t* uuid )
{
    int8_t   i;
    uint8_t  j;
    uint8_t* uuid_iter = (uint8_t*)&uuid->uu;

    uuid->len = strlen( string ) / 2;

    for ( i = uuid->len - 1, j = 0; i >= 0; i--, j += 2 )
    {
        char  buffer[3] = {0};
        char* end;

        buffer[0] = string[j];
        buffer[1] = string[j + 1];
        uuid_iter[i] = strtoul( buffer, &end, 16 );
    }
}

void uuid_to_string( const wiced_bt_uuid_t* uuid, char* string )
{
    int8_t   i;
    uint8_t  j;
    uint8_t* uuid_iter = (uint8_t*)&uuid->uu;

    for ( i = uuid->len - 1, j = 0; i >= 0; i--, j += 2 )
    {
        unsigned_to_hex_string( (uint32_t)uuid_iter[i], &string[j], 2, 2 );
    }
}

void format_node_string( char* output, const smart_address_t* address, wiced_bt_ble_address_type_t type )
{
    device_address_to_string( address, output );
    unsigned_to_hex_string( (uint32_t)type, output + 12, 1, 1 );
}

void format_service_string( char* output, uint16_t start_handle, uint16_t end_handle )
{
    unsigned_to_hex_string( (uint32_t)end_handle,   &output[0], 4 , 4 );
    unsigned_to_hex_string( (uint32_t)start_handle, &output[4], 4 , 4 );
}

void format_characteristic_string( char* output, uint16_t start_handle, uint16_t end_handle, uint16_t value_handle )
{
    unsigned_to_hex_string( (uint32_t)end_handle,   &output[0], 4, 4 );
    unsigned_to_hex_string( (uint32_t)value_handle, &output[4], 4, 4 );
    unsigned_to_hex_string( (uint32_t)start_handle, &output[8], 4, 4 );
}

int ip_to_str( const wiced_ip_address_t* address, char* string )
{
    if ( address->version == WICED_IPV4 )
    {
        unsigned_to_decimal_string( ( ( address->ip.v4 >> 24 ) & 0xff ), string, 1, 3 );
        string += strlen( string );
        *string++ = '.';

        unsigned_to_decimal_string( ( ( address->ip.v4 >> 16 ) & 0xff ), string, 1, 3 );
        string += strlen( string );
        *string++ = '.';

        unsigned_to_decimal_string( ( ( address->ip.v4 >> 8 ) & 0xff ), string, 1, 3 );
        string += strlen( string );
        *string++ = '.';

        unsigned_to_decimal_string( ( address->ip.v4 & 0xff ), string, 1, 3 );
        string += strlen( string );
        *string++ = '\0';
    }
    else
    {
        return -1;
    }

    return 0;
}
