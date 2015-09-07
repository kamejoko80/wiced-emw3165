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

#include "linked_list.h"
#include "wiced_bt_types.h"
#include "wiced_bt_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef wiced_bt_gatt_status_t (*smartbridge_callback_t)( smart_connection_t* connection, wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t* data );

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)
/**
 * Handle range
 */
typedef struct
{
    uint16_t start_handle;
    uint16_t end_handle;
} smart_handle_range_t;

typedef struct
{
    linked_list_node_t     this_node;
    smartbridge_callback_t callback;
} smartbridge_interface_t;
#pragma pack()

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t         smartbridge_add_interface     ( smartbridge_interface_t* interface, smartbridge_callback_t callback );

wiced_result_t         smartbridge_remove_interface  ( smartbridge_interface_t* interface );

wiced_bt_gatt_status_t smartbridge_connect           ( smartbridge_interface_t* interface, const smart_address_t* address, wiced_bt_ble_address_type_t address_type, wiced_bt_ble_conn_mode_t connection_mode );

wiced_bt_gatt_status_t smartbridge_disconnect        ( smartbridge_interface_t* interface, smart_connection_t* connection );

wiced_bt_gatt_status_t smartbridge_cancel_connect    ( smartbridge_interface_t* interface, const smart_address_t* address );

wiced_bt_gatt_status_t smartbridge_discover          ( smartbridge_interface_t* interface, smart_connection_t* connection, wiced_bt_gatt_discovery_type_t type, const wiced_bt_gatt_discovery_param_t* parameter );

wiced_bt_gatt_status_t smartbridge_read              ( smartbridge_interface_t* interface, smart_connection_t* connection, wiced_bt_gatt_read_type_t type, const wiced_bt_gatt_read_param_t* parameter );

wiced_bt_gatt_status_t smartbridge_write             ( smartbridge_interface_t* interface, smart_connection_t* connection, wiced_bt_gatt_write_type_t type, const wiced_bt_gatt_value_t* value );

wiced_bt_gatt_status_t smartbridge_execute_write     ( smartbridge_interface_t* interface, smart_connection_t* connection );

wiced_bt_gatt_status_t smartbridge_confirm_indication( smartbridge_interface_t* interface, smart_connection_t* connection, uint16_t handle );

wiced_result_t         smartbridge_read_cached_value ( smart_connection_t* connection, uint16_t handle, uint8_t* buffer, uint32_t buffer_length, uint32_t* available_data_length );

#ifdef __cplusplus
} /* extern "C" */
#endif
