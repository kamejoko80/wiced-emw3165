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

#include "restful_smart_host.h"
#include "wiced_bt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Include NUL-terminator */
#define NODE_BUFFER_LENGTH           (14)
#define SERVICE_BUFFER_LENGTH        (9)
#define CHARACTERISTIC_BUFFER_LENGTH (13)
#define DESCRIPTOR_BUFFER_LENGTH     (5)
#define UUID_BUFFER_LENGTH           (33)
#define BD_ADDR_BUFFER_LENGTH        (13)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    BT_REST_GATEWAY_STATUS_200, /* OK */
    BT_REST_GATEWAY_STATUS_400, /* Bad Request */
    BT_REST_GATEWAY_STATUS_403,
    BT_REST_GATEWAY_STATUS_404, /* URI Not Found */
    BT_REST_GATEWAY_STATUS_405,
    BT_REST_GATEWAY_STATUS_406,
    BT_REST_GATEWAY_STATUS_412,
    BT_REST_GATEWAY_STATUS_415,
    BT_REST_GATEWAY_STATUS_504,
} bt_rest_gateway_status_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t restful_gateway_write_status_code                      ( restful_smart_response_stream_t* stream, bt_rest_gateway_status_t status );

wiced_result_t restful_gateway_write_node                             ( restful_smart_response_stream_t* stream, const char* node, const char* bdaddr );

wiced_result_t restful_gateway_write_service                          ( restful_smart_response_stream_t* stream, const char* node, const char* service, uint16_t handle, const char* uuid, wiced_bool_t is_primary_service );

wiced_result_t restful_gateway_write_characteristic                   ( restful_smart_response_stream_t* stream, const char* node, const char* characteristic, uint16_t handle, const char* uuid, uint8_t properties );

wiced_result_t restful_gateway_write_descriptor                       ( restful_smart_response_stream_t* stream, const char* node, const char* descriptor, uint16_t handle, const char* uuid );

wiced_result_t restful_gateway_write_characteristic_value             ( restful_smart_response_stream_t* stream, const char* node, const char* characteristic, uint16_t handle, const uint8_t* value, uint32_t value_length );

wiced_result_t restful_gateway_write_descriptor_value                 ( restful_smart_response_stream_t* stream, const char* node, const char* descriptor_value_handle, const uint8_t* value, uint32_t value_length );

wiced_result_t restful_gateway_write_cached_value                     ( restful_smart_response_stream_t* stream, const char* value_handle, const char* value );

wiced_result_t restful_gateway_write_node_array_start                 ( restful_smart_response_stream_t* stream );

wiced_result_t restful_gateway_write_service_array_start              ( restful_smart_response_stream_t* stream );

wiced_result_t restful_gateway_write_characteristic_array_start       ( restful_smart_response_stream_t* stream );

wiced_result_t restful_gateway_write_descriptor_array_start           ( restful_smart_response_stream_t* stream );

wiced_result_t restful_gateway_write_array_end                        ( restful_smart_response_stream_t* stream );

wiced_result_t restful_gateway_write_bonding                          ( restful_smart_response_stream_t* stream, const char* node, uint8_t code, const char* pairingID, const char* display, const char* ra, const char* ca );

wiced_result_t restful_gateway_write_long_characteristic_value_start  ( wiced_http_response_stream_t* stream, const char* node_handle, const char* characteristic_value_handle );

wiced_result_t restful_gateway_write_long_partial_characteristic_value( wiced_http_response_stream_t* stream, const uint8_t* partial_value, uint32_t length );

wiced_result_t restful_gateway_write_long_characteristic_value_end    ( wiced_http_response_stream_t* stream );

int            ip_to_str                                       ( const restful_smart_ip_address_t* address, char* string );

void           string_to_device_address                        ( const char* string, smart_address_t* device_address );

void           device_address_to_string                        ( const smart_address_t* device_address, char* string );

void           string_to_uuid                                  ( const char* string, wiced_bt_uuid_t* uuid );

void           uuid_to_string                                  ( const wiced_bt_uuid_t* uuid, char* string );

void           format_node_string                              ( char* output, const smart_address_t* address, wiced_bt_ble_address_type_t type );

void           format_service_string                           ( char* output, uint16_t start_handle, uint16_t end_handle );

void           format_characteristic_string                    ( char* output, uint16_t start_handle, uint16_t end_handle, uint16_t value_handle );

#ifdef __cplusplus
} /* extern "C" */
#endif
