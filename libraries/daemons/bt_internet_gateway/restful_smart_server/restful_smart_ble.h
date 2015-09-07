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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define INVALID_PIN 0xFFFF

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    smart_address_t             address;
    wiced_bt_ble_address_type_t type;
} smart_node_handle_t;

typedef struct
{
    uint16_t start_handle;
    uint16_t end_handle;
} smart_service_handle_t;

typedef struct
{
    uint16_t start_handle;
    uint16_t end_handle;
    uint16_t value_handle;
} smart_characteristic_handle_t;

typedef struct
{
    uint32_t length;
    uint8_t  value[1];
} smart_value_handle_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t restful_smart_init_smartbridge                           ( void );
wiced_result_t restful_smart_deinit_smartbridge                         ( void );

wiced_result_t restful_smart_connect                                    ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node );

wiced_result_t restful_smart_discover_all_primary_services              ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node );
wiced_result_t restful_smart_discover_primary_services_by_uuid          ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const wiced_bt_uuid_t* uuid );
wiced_result_t restful_smart_discover_characteristics_of_a_service      ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_service_handle_t* service );
wiced_result_t restful_smart_discover_characteristics_by_uuid           ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const wiced_bt_uuid_t* uuid );
wiced_result_t restful_smart_discover_characteristic_descriptors        ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );

wiced_result_t restful_smart_read_characteristic_value                  ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_read_characteristic_long_value             ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_read_characteristic_values_by_uuid         ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const wiced_bt_uuid_t* uuid );
wiced_result_t restful_smart_read_cached_value                          ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic );
wiced_result_t restful_smart_read_descriptor_value                      ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, uint16_t descriptor_handle );

wiced_result_t restful_smart_write_characteristic_value                 ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const smart_value_handle_t* value );
wiced_result_t restful_smart_write_long_characteristic_value            ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const smart_value_handle_t* value );
wiced_result_t restful_smart_write_characteristic_value_without_response( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const smart_value_handle_t* value );
wiced_result_t restful_smart_write_descriptor_value                     ( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, uint16_t descriptor_handle, const smart_value_handle_t* value );

wiced_result_t restful_smart_configure_bond                             ( restful_smart_response_stream_t* stream, smart_address_t* device_address, uint8_t* io_capabilities, uint8_t* oob_type );
wiced_result_t restful_smart_configure_pairing                          ( restful_smart_response_stream_t* stream, smart_address_t* device_address, char* pairing_id, char* tk, uint32_t* passkey, char* rb, char* cb, wiced_bool_t confirmed );
wiced_result_t restful_smart_write_bonded_nodes                         ( restful_smart_response_stream_t* stream, const char* node_handle, const char* bdaddr, const char* bdaddrtype, uint8_t key_mask );
wiced_result_t restful_smart_write_gap_node                             ( restful_smart_response_stream_t* stream, const char* node_handle, const char* bdaddr, const char* bdaddrtype, const char* rssi );
wiced_result_t restful_smart_write_adv_array_start                      ( restful_smart_response_stream_t* stream );
wiced_result_t restful_smart_write_adv_data                             ( restful_smart_response_stream_t* stream, const char* adv_type, const char* adv_data );
wiced_result_t restful_smart_write_node_name                            ( restful_smart_response_stream_t* stream, const char* node_handle, const char* bdaddr, const char* node_name );
wiced_result_t restful_smart_return_bonded_nodes                        ( restful_smart_response_stream_t* stream, smart_address_t* device_address );
wiced_result_t restful_smart_remove_bond                                ( restful_smart_response_stream_t* stream, smart_address_t* device_address );
wiced_result_t restful_smart_cancel_bond                                ( restful_smart_response_stream_t* stream, smart_address_t* device_address );
wiced_result_t restful_smart_passive_scan                               ( restful_smart_response_stream_t* stream );
wiced_result_t restful_smart_active_scan                                ( restful_smart_response_stream_t* stream );
wiced_result_t restful_smart_enabled_devices                            ( restful_smart_response_stream_t* stream );
wiced_result_t restful_smart_enabled_node_data                          ( restful_smart_response_stream_t* stream, const smart_address_t* device_address );
wiced_result_t restful_smart_remove_device                              ( restful_smart_response_stream_t* stream, const smart_address_t* device_address );
wiced_result_t restful_smart_discover_node_name                         ( restful_smart_response_stream_t* stream, const smart_address_t* device_address );

#ifdef __cplusplus
} /* extern "C" */
#endif
