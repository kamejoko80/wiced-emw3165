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
 * BLE RESTful API Demo Application
 *
 */

#include "string.h"
#include "wiced.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_cfg.h"
#include "smartbridge.h"
#include "smartbridge_gatt.h"
#include "restful_smart_ble.h"
#include "restful_smart_response.h"
#include "http_server.h"
#include "bt_types.h"

/******************************************************
 *                    Constants
 ******************************************************/

#define STACK_INIT_TIMEOUT_MS  ( 30 * SECONDS )
#define CONNECT_TIMEOUT_MS     ( 10 * SECONDS )
#define BT_LE_SCAN_REMOTE_NAME_MAXLEN   32

#define RESPONDER_OOB 0

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_bt_gatt_status_t smartbridge_callback( smart_connection_t* connection, wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *data );
static void                   string_to_hex_array ( const char* string, uint8_t* array, uint8_t size );
extern void                   SMP_ConfirmReply    ( BD_ADDR bd_addr, uint8_t res );

/******************************************************
 *               External Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_bool_t                     long_value = WICED_FALSE;
static wiced_bool_t                     characteristic_value = WICED_FALSE;
static wiced_semaphore_t                wait_semaphore;
smart_characteristic_handle_t           current_characteristic_handle;
wiced_bt_gatt_char_declaration_t        current_characteristic;
wiced_bt_gatt_char_declaration_t        previous_characteristic;
static volatile uint16_t                current_service_handle_end;
static restful_smart_response_stream_t* current_stream          = NULL;
static volatile uint32_t                current_procedure_count = 0;
extern wiced_bt_cfg_settings_t          wiced_bt_cfg_settings;
smart_address_t                         local_address;
smart_address_t                         remote_address;
wiced_bt_smp_sc_oob_t                   sc_oob_data;

wiced_bt_dev_ble_io_caps_req_t     local_io_caps_ble  =
{
        { 0 },
        BTM_IO_CAPABILIES_KEYBOARD_DISPLAY, //io_caps
        0, //oob_data
        BTM_LE_AUTH_REQ_BOND|BTM_LE_AUTH_REQ_MITM, //auth_req
        //BTM_LE_AUTH_REQ_SC_MITM_BOND,
        16, // max_key_size
        (BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_PLK), // init_keys - Keys to be distributed, bit mask
        (BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_PLK)  // resp_keys - Keys to be distributed, bit mask
};

static smartbridge_interface_t smartbridge_interface;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t restful_smart_init_smartbridge( void )
{
    wiced_result_t result;

    wiced_rtos_init_semaphore( &wait_semaphore );

    /* Initialise smartbridge */
    result = smartbridge_init( );
    if ( result != WICED_SUCCESS )
    {
        wiced_rtos_deinit_semaphore( &wait_semaphore );
        return result;
    }

    /* Initialize BT host stack and controller */
    result = smartbridge_add_interface( &smartbridge_interface, smartbridge_callback );
    if ( result != WICED_SUCCESS )
    {
        wiced_rtos_deinit_semaphore( &wait_semaphore );
        return result;
    }

    return result;
}

wiced_result_t restful_smart_deinit_smartbridge( void )
{
    return WICED_SUCCESS;
}

static wiced_bt_gatt_status_t smartbridge_callback( smart_connection_t* connection, wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *data )
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;

    switch ( event )
    {
    case GATT_CONNECTION_STATUS_EVT:
        /* Write response only if connection status event is originated from app */
        if ( ( data != NULL ) && ( current_stream != NULL ) && ( data->connection_status.connected == WICED_TRUE ) )
        {
            char address_string[BD_ADDR_BUFFER_LENGTH] = { 0 };
            char node_string   [NODE_BUFFER_LENGTH]    = { 0 };

            restful_gateway_write_status_code( current_stream, BT_REST_GATEWAY_STATUS_200 );
            restful_gateway_write_node_array_start( current_stream );
            format_node_string( node_string, &connection->device_address, connection->address_type );
            device_address_to_string( &connection->device_address, address_string );
            restful_gateway_write_node( current_stream, (const char*)node_string, (const char*)address_string );
            restful_gateway_write_array_end( current_stream );
            wiced_rtos_set_semaphore( &wait_semaphore );

#if RESPONDER_OOB
            tBLE_BD_ADDR peer_addr;

            memcpy( &peer_addr.bda, data->connection_status.bd_addr, 6 );
            peer_addr.type = BLE_ADDR_PUBLIC;

            SMP_CrLocScOobData( &peer_addr );
#endif
        }
        break;

    case GATT_DISCOVERY_RESULT_EVT:
    {
        if ( ( data != NULL ) && ( current_stream != NULL ) )
        {
            char node[NODE_BUFFER_LENGTH] = { 0 };

            format_node_string( node, &connection->device_address, connection->address_type );

            if ( data->discovery_result.discovery_type == GATT_DISCOVER_SERVICES_ALL || data->discovery_result.discovery_type == GATT_DISCOVER_SERVICES_BY_UUID )
            {
                char  service[SERVICE_BUFFER_LENGTH];
                char  uuid   [UUID_BUFFER_LENGTH];

                memset( &service, 0, sizeof( service ) );
                memset( &uuid, 0, sizeof( uuid ) );

                if ( current_procedure_count == 0 )
                {
                    restful_gateway_write_status_code( current_stream, BT_REST_GATEWAY_STATUS_200 );
                    restful_gateway_write_service_array_start( current_stream );
                }

                format_service_string( service, data->discovery_result.discovery_data.group_value.s_handle, data->discovery_result.discovery_data.group_value.e_handle );
                uuid_to_string( &data->discovery_result.discovery_data.group_value.service_type, uuid );
                restful_gateway_write_service( current_stream, node, service, data->discovery_result.discovery_data.group_value.s_handle, uuid, WICED_TRUE );
                current_procedure_count++;
            }
            else if ( data->discovery_result.discovery_type == GATT_DISCOVER_CHARACTERISTICS )
            {
                memcpy( &current_characteristic, &data->discovery_result.discovery_data.characteristic_declaration, sizeof( current_characteristic ) );

                /* Send previous discovered characteristic now because the end handle is calculated from the current characteristic handle range */
                if ( current_procedure_count > 0 )
                {
                    char  characteristic[CHARACTERISTIC_BUFFER_LENGTH];
                    char  uuid          [UUID_BUFFER_LENGTH];

                    format_characteristic_string( characteristic, previous_characteristic.handle, current_characteristic.handle - 1, previous_characteristic.val_handle );
                    uuid_to_string( &previous_characteristic.char_uuid, uuid );
                    restful_gateway_write_characteristic( current_stream, node, characteristic, previous_characteristic.handle, uuid, previous_characteristic.characteristic_properties );
                }
                else if ( current_procedure_count == 0 )
                {
                    restful_gateway_write_status_code( current_stream, BT_REST_GATEWAY_STATUS_200 );
                    restful_gateway_write_characteristic_array_start( current_stream );
                }

                memcpy( &previous_characteristic, &data->discovery_result.discovery_data.characteristic_declaration, sizeof( previous_characteristic ) );
                current_procedure_count++;
            }
            else if ( data->discovery_result.discovery_type == GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS )
            {
                char  descriptor[DESCRIPTOR_BUFFER_LENGTH];
                char  uuid      [UUID_BUFFER_LENGTH];

                if ( current_procedure_count == 0 )
                {
                    restful_gateway_write_status_code( current_stream, BT_REST_GATEWAY_STATUS_200 );
                    restful_gateway_write_descriptor_array_start( current_stream );
                }

                unsigned_to_hex_string( (uint32_t)data->discovery_result.discovery_data.char_descr_info.handle, descriptor, 4, 4 );
                uuid_to_string( &data->discovery_result.discovery_data.char_descr_info.type, uuid );
                restful_gateway_write_descriptor( current_stream, node, descriptor, data->discovery_result.discovery_data.char_descr_info.handle, uuid );
                current_procedure_count++;
            }
        }
        break;
    }
    case GATT_DISCOVERY_CPLT_EVT:
        if ( ( data != NULL ) && ( current_stream != NULL ) )
        {
            if ( ( data->discovery_complete.disc_type == GATT_DISCOVER_CHARACTERISTICS ) && ( current_procedure_count > 0 ) )
            {
                char  characteristic[CHARACTERISTIC_BUFFER_LENGTH];
                char  node          [NODE_BUFFER_LENGTH];
                char  uuid          [UUID_BUFFER_LENGTH];

                memset( &characteristic, 0, sizeof ( characteristic ) );
                memset( &node, 0, sizeof( node ) );
                memset( &uuid, 0, sizeof( uuid ) );

                format_node_string( node, &connection->device_address, connection->address_type );
                format_characteristic_string( characteristic, current_characteristic.handle, current_service_handle_end, current_characteristic.val_handle );
                uuid_to_string( &current_characteristic.char_uuid, uuid );
                restful_gateway_write_characteristic( current_stream, node, characteristic, current_characteristic.handle, uuid, current_characteristic.characteristic_properties );
            }

            restful_gateway_write_array_end( current_stream );
            wiced_rtos_set_semaphore( &wait_semaphore );
        }
        break;

    case GATT_OPERATION_CPLT_EVT:
    {
        if ( data != NULL )
        {
            char node[NODE_BUFFER_LENGTH] = { 0 };

            device_address_to_string( &connection->device_address, node );

            if ( data->operation_complete.op == GATTC_OPTYPE_READ )
            {
                if ( characteristic_value == WICED_TRUE )
                {
                    char characteristic[CHARACTERISTIC_BUFFER_LENGTH] = { 0 };

                    format_characteristic_string( characteristic, current_characteristic_handle.start_handle, current_characteristic_handle.end_handle, current_characteristic_handle.value_handle );

                    if ( long_value == WICED_FALSE )
                    {
                        restful_gateway_write_status_code( current_stream, BT_REST_GATEWAY_STATUS_200 );
                        restful_gateway_write_characteristic_value( current_stream, node, characteristic, data->operation_complete.response_data.att_value.handle, data->operation_complete.response_data.att_value.p_data, data->operation_complete.response_data.att_value.len );
                        wiced_rtos_set_semaphore( &wait_semaphore );
                    }
                    else
                    {
                        if ( current_procedure_count == 0 )
                        {
                            restful_gateway_write_status_code( current_stream, BT_REST_GATEWAY_STATUS_200 );
                            restful_gateway_write_long_characteristic_value_start( current_stream, node, characteristic );
                        }

                        current_procedure_count++;
                        restful_gateway_write_long_partial_characteristic_value( current_stream, data->operation_complete.response_data.att_value.p_data, data->operation_complete.response_data.att_value.len );

                        if ( data->operation_complete.response_data.att_value.len < ( GATT_DEF_BLE_MTU_SIZE - 1 ) )
                        {
                            restful_gateway_write_long_characteristic_value_end( current_stream );
                            wiced_rtos_set_semaphore( &wait_semaphore );
                        }
                        else
                        {
                            /* Still more data to be read. Issue another read with offset set to */
                            wiced_bt_gatt_read_param_t parameter;

                            parameter.partial.auth_req = GATT_AUTH_REQ_NONE;
                            parameter.partial.handle   = data->operation_complete.response_data.att_value.handle;
                            parameter.partial.offset   = current_procedure_count * ( GATT_DEF_BLE_MTU_SIZE - 1 );

                            if ( wiced_bt_gatt_send_read( connection->connection_id, GATT_READ_PARTIAL, &parameter) != WICED_BT_SUCCESS )
                            {
                                wiced_rtos_set_semaphore( &wait_semaphore );
                            }
                        }
                    }
                }
                else
                {
                    char descriptor[DESCRIPTOR_BUFFER_LENGTH] = { 0 };

                    unsigned_to_hex_string( data->operation_complete.response_data.att_value.handle, descriptor, 4, 4 );
                    restful_gateway_write_status_code( current_stream, BT_REST_GATEWAY_STATUS_200 );
                    restful_gateway_write_descriptor_value( current_stream, node, descriptor, data->operation_complete.response_data.att_value.p_data, data->operation_complete.response_data.att_value.len );
                    wiced_rtos_set_semaphore( &wait_semaphore );
                }
            }
            else if ( data->operation_complete.op == GATTC_OPTYPE_WRITE )
            {
                restful_gateway_write_status_code( current_stream, BT_REST_GATEWAY_STATUS_200 );
                wiced_rtos_set_semaphore( &wait_semaphore );
            }
            else if ( data->operation_complete.op == GATTC_OPTYPE_NOTIFICATION )
            {
                printf("Notification\n");
            }
        }
        break;
    }

    case GATT_ATTRIBUTE_REQUEST_EVT:
        WPRINT_LIB_INFO(( "REST: Received GATT_ATTRIBUTE_REQUEST_EVT\n" ));
        break;

    default:
        break;
    }

    return (status);
}

wiced_result_t restful_smart_connect( restful_smart_response_stream_t* stream, const smart_node_handle_t* node )
{
    wiced_result_t result = WICED_ERROR;

    current_stream = stream;

    if ( smartbridge_connect( &smartbridge_interface, &node->address, node->type, BLE_CONN_MODE_HIGH_DUTY ) == WICED_BT_GATT_CMD_STARTED )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream = NULL;
    return result;
}

wiced_result_t restful_smart_discover_all_primary_services( restful_smart_response_stream_t* stream, const smart_node_handle_t* node )
{
    wiced_bt_gatt_discovery_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = 1;
    parameter.e_handle = 0xffff;

    current_stream = stream;

    if ( smartbridge_discover( &smartbridge_interface, connection, GATT_DISCOVER_SERVICES_ALL, &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream = NULL;
    return result;
}

wiced_result_t restful_smart_discover_primary_services_by_uuid( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const wiced_bt_uuid_t* uuid )
{
    wiced_bt_gatt_discovery_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    memcpy( (void*)&parameter.uuid, (void*)uuid, sizeof( wiced_bt_uuid_t ) );
    parameter.s_handle = 1;
    parameter.e_handle = 0xffff;

    current_stream = stream;

    if ( smartbridge_discover( &smartbridge_interface, connection, GATT_DISCOVER_SERVICES_BY_UUID, &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream = NULL;
    return result;
}

wiced_result_t restful_smart_read_service( wiced_http_response_stream_t* stream, const smart_address_t* device_address, uint16_t service_handle )
{
    wiced_bt_gatt_discovery_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( device_address, &connection );

    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = service_handle;
    parameter.e_handle = service_handle;

    current_stream = stream;

    if ( smartbridge_discover( &smartbridge_interface, connection, GATT_DISCOVER_SERVICES_ALL, &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream = NULL;
    return result;
}

wiced_result_t restful_smart_discover_characteristics_of_a_service( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_service_handle_t* service )
{
    wiced_bt_gatt_discovery_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = service->start_handle;
    parameter.e_handle = service->end_handle;

    current_stream             = stream;
    current_procedure_count    = 0;
    current_service_handle_end = service->end_handle;

    if ( smartbridge_discover( &smartbridge_interface, connection, GATT_DISCOVER_CHARACTERISTICS, &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_discover_characteristics_by_uuid( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const wiced_bt_uuid_t* uuid )
{
    wiced_bt_gatt_discovery_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    memcpy( &parameter.uuid, uuid, sizeof( wiced_bt_uuid_t ) );
    parameter.s_handle = 1;
    parameter.e_handle = 0xffff;

    current_stream          = stream;
    current_procedure_count = 0;

    if (smartbridge_discover( &smartbridge_interface, connection, GATT_DISCOVER_CHARACTERISTICS, &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_discover_characteristic_descriptors( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic )
{
    wiced_bt_gatt_discovery_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    memset( &parameter.uuid, 0, sizeof( parameter.uuid ) );
    parameter.s_handle = characteristic->value_handle + 1;
    parameter.e_handle = characteristic->end_handle;

    current_stream          = stream;
    current_procedure_count = 0;

    if ( smartbridge_discover( &smartbridge_interface, connection, GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS, &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_read_characteristic_value( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic )
{
    wiced_bt_gatt_read_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    parameter.by_handle.auth_req = GATT_AUTH_REQ_NONE;
    parameter.by_handle.handle   = characteristic->value_handle;
    characteristic_value         = WICED_TRUE;
    memcpy( &current_characteristic_handle, characteristic, sizeof( current_characteristic_handle ) );

    current_stream          = stream;
    current_procedure_count = 0;

    if ( smartbridge_read( &smartbridge_interface, connection, GATT_READ_BY_HANDLE, &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    characteristic_value    = WICED_FALSE;
    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_read_characteristic_long_value( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic )
{
    wiced_bt_gatt_read_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    parameter.partial.auth_req = GATT_AUTH_REQ_NONE;
    parameter.partial.handle   = characteristic->value_handle;
    parameter.partial.offset   = 0;
    characteristic_value       = WICED_TRUE;
    memcpy( &current_characteristic_handle, characteristic, sizeof( current_characteristic_handle ) );

    current_stream          = stream;
    current_procedure_count = 0;
    long_value              = WICED_TRUE;

    if ( smartbridge_read( &smartbridge_interface, connection, GATT_READ_PARTIAL, &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    characteristic_value    = WICED_FALSE;
    current_stream          = NULL;
    current_procedure_count = 0;
    long_value              = WICED_FALSE;
    return result;
}

wiced_result_t restful_smart_read_characteristic_values_by_uuid( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const wiced_bt_uuid_t* uuid )
{
    wiced_bt_gatt_read_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    parameter.char_type.auth_req = GATT_AUTH_REQ_NONE;
    parameter.char_type.s_handle = characteristic->start_handle;
    parameter.char_type.e_handle = characteristic->end_handle;
    characteristic_value         = WICED_TRUE;
    memcpy( &parameter.char_type.uuid, uuid, sizeof( wiced_bt_uuid_t) );
    memcpy( &current_characteristic_handle, characteristic, sizeof( current_characteristic_handle ) );

    current_stream          = stream;
    current_procedure_count = 0;

    if ( smartbridge_read( &smartbridge_interface, connection, GATT_READ_BY_TYPE , &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    characteristic_value    = WICED_FALSE;
    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_write_characteristic_value( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const smart_value_handle_t* value )
{
    uint8_t                buffer[100] = { 0 };
    wiced_bt_gatt_value_t* write_value = (wiced_bt_gatt_value_t*)buffer;
    smart_connection_t*    connection;
    wiced_result_t         result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    write_value->auth_req = GATT_AUTH_REQ_NONE;
    write_value->handle   = characteristic->value_handle;
    write_value->len      = value->length;
    write_value->offset   = 0;
    memcpy( write_value->value, value->value, value->length );
    memcpy( &current_characteristic_handle, characteristic, sizeof( current_characteristic_handle ) );

    current_stream          = stream;
    current_procedure_count = 0;

    if ( smartbridge_write( &smartbridge_interface, connection, GATT_WRITE, write_value ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_write_long_characteristic_value( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const smart_value_handle_t* value )
{
    uint8_t                buffer[100] = { 0 };
    wiced_bt_gatt_value_t* write_value = (wiced_bt_gatt_value_t*)buffer;
    smart_connection_t*    connection;
    wiced_result_t         result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    write_value->auth_req = GATT_AUTH_REQ_NONE;
    write_value->handle   = characteristic->value_handle;
    write_value->len      = MIN( ( GATT_DEF_BLE_MTU_SIZE - 1 ), value->length );
    write_value->offset   = 0;
    memcpy( write_value->value, value->value, write_value->len );
    memcpy( &current_characteristic_handle, characteristic, sizeof( current_characteristic_handle ) );

    long_value              = WICED_TRUE;
    current_stream          = stream;
    current_procedure_count = 0;

    if ( smartbridge_write( &smartbridge_interface, connection, GATT_WRITE, write_value ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    long_value              = WICED_FALSE;
    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_write_characteristic_value_without_response( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic, const smart_value_handle_t* value )
{
    uint8_t                buffer[100] = { 0 };
    wiced_bt_gatt_value_t* write_value = (wiced_bt_gatt_value_t*)buffer;
    smart_connection_t*    connection;
    wiced_result_t         result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    write_value->auth_req = GATT_AUTH_REQ_NONE;
    write_value->handle   = characteristic->value_handle;
    write_value->len      = value->length;
    write_value->offset   = 0;
    memcpy( write_value->value, value->value, value->length );
    memcpy( &current_characteristic_handle, characteristic, sizeof( current_characteristic_handle ) );

    current_stream          = stream;
    current_procedure_count = 0;

    if ( smartbridge_write( &smartbridge_interface, connection, GATT_WRITE_NO_RSP, write_value ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_read_descriptor_value( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, uint16_t descriptor_handle )
{
    wiced_bt_gatt_read_param_t parameter;
    smart_connection_t* connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    parameter.by_handle.auth_req = GATT_AUTH_REQ_NONE;
    parameter.by_handle.handle   = descriptor_handle;

    current_stream          = stream;
    current_procedure_count = 0;

    if ( smartbridge_read( &smartbridge_interface, connection, GATT_READ_BY_HANDLE, &parameter ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_write_descriptor_value( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, uint16_t descriptor_handle, const smart_value_handle_t* value )
{
    uint8_t                buffer[100] = { 0 };
    wiced_bt_gatt_value_t* write_value = (wiced_bt_gatt_value_t*)buffer;
    smart_connection_t*    connection;
    wiced_result_t result = WICED_ERROR;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    write_value->auth_req = GATT_AUTH_REQ_NONE;
    write_value->handle   = descriptor_handle;
    write_value->len      = value->length;
    write_value->offset   = 0;
    memcpy( write_value->value, value->value, value->length );

    current_stream          = stream;
    current_procedure_count = 0;

    if ( smartbridge_write( &smartbridge_interface, connection, GATT_WRITE, write_value ) == WICED_BT_GATT_SUCCESS )
    {
        result = wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
    }

    current_stream          = NULL;
    current_procedure_count = 0;
    return result;
}

wiced_result_t restful_smart_read_cached_value( restful_smart_response_stream_t* stream, const smart_node_handle_t* node, const smart_characteristic_handle_t* characteristic )
{
    smart_connection_t* connection;
    uint8_t buffer_hex[100];
    char node_handle[NODE_BUFFER_LENGTH];
    char characteristic_handle[CHARACTERISTIC_BUFFER_LENGTH] = { 0 };

    uint32_t available_length;

    smartbridge_find_connection_by_device_address( &node->address, &connection );

    smartbridge_read_cached_value( connection, characteristic->value_handle, buffer_hex, sizeof( buffer_hex ), &available_length );

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );
    format_node_string( node_handle, &node->address, connection->address_type );
    format_characteristic_string( characteristic_handle, characteristic->start_handle, characteristic->end_handle, characteristic->value_handle );
    restful_gateway_write_characteristic_value( stream, node_handle, characteristic_handle, characteristic->value_handle, buffer_hex, available_length );
    return WICED_SUCCESS;
}

/* REST Management APIs */
wiced_result_t restful_smart_configure_bond( wiced_http_response_stream_t* stream, smart_address_t* device_address, uint8_t* io_capabilities, uint8_t* oob_type )
{
    wiced_result_t      status = WICED_SUCCESS;
    smart_connection_t* connection;

    char     node_handle[13]   = { 0 };
    char     display_string[6] = { 0 };
    char     ra_string[33]     = { 0 };
    char     ca_string[33]     = { 0 };
    char     pairingID_tmp[]   = { 0x30, 0x30, 0x30, 0x31, 0x30, 0x32, 0x30, 0x33, 0x30, 0x34, 0x30, 0x35, 0x30, 0x36, 0x30, 0x37, 0x30, 0x38, 0x30, 0x39, 0x30, 0x41, 0x30, 0x42, 0x30, 0x43, 0x30, 0x44, 0x30, 0x45, 0x30, 0x46, 0x0 };

    uint8_t  i;
    uint8_t  pairing_status_code = 0;
    uint32_t display_value       = INVALID_PIN;

    char *pairingID = NULL;
    char *display   = NULL;
    char *ra        = NULL;
    char *ca        = NULL;

    current_stream  = stream;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );

    if( smartbridge_find_connection_by_device_address( device_address, &connection ) == WICED_SUCCESS )
    {
        local_io_caps_ble.local_io_cap = ( *io_capabilities != BTM_IO_CAPABILIES_MAX ) ? *io_capabilities: BTM_IO_CAPABILIES_NONE;

        if( *oob_type != 0 )
        {
            local_io_caps_ble.oob_data = 1;
        }
        if( *oob_type == 2 ) // SC OOB - need to generate local sc oob data
        {
            wiced_bt_smp_create_local_sc_oob_data( device_address->address, BLE_ADDR_PUBLIC );

            connection->bonding_info.status = LE_SECURE_OOB_EXPECTED;
        }
        else
        {
            /* Initiate Bonding */
            wiced_bt_dev_sec_bond( connection->device_address.address, BLE_ADDR_PUBLIC, BT_TRANSPORT_LE, 0, NULL );
        }

        wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );

        pairing_status_code = connection->bonding_info.status;

        switch( pairing_status_code )
        {
            case PAIRING_SUCCESSFUL:
                connection->bonding_info.is_bonded = WICED_TRUE;
                break;
            case LE_SECURE_OOB_EXPECTED:
            case LE_LEGACY_OOB_EXPECTED:
            case PASSKEY_INPUT_EXPECTED:
            case PASSKEY_DISPLAY_EXPECTED:
            case NUMERIC_COMPARISON_EXPECTED:
                device_address_to_string( &connection->device_address, node_handle );
                memcpy( &connection->bonding_info.pairing_id,  &pairingID_tmp, 33 );

                pairingID = &connection->bonding_info.pairing_id[0];
                memcpy( pairingID+20, &node_handle, 12);

                if ( pairing_status_code == PASSKEY_DISPLAY_EXPECTED    ) display_value = connection->bonding_info.passkey;
                if ( pairing_status_code == NUMERIC_COMPARISON_EXPECTED ) display_value = connection->bonding_info.display;
                if ( display_value != INVALID_PIN )
                {
                    unsigned_to_decimal_string( display_value, display_string, 6, 6 );
                    display = &display_string[0];
                }
                if ( pairing_status_code == LE_SECURE_OOB_EXPECTED )
                {
                    device_address_to_string( &local_address, node_handle );

                    for( i = 0; i < 16; i++ )
                    {
                        unsigned_to_hex_string( (uint32_t)sc_oob_data.local_oob_data.randomizer[i], &ra_string[i*2], 2, 2 );
                        unsigned_to_hex_string( (uint32_t)sc_oob_data.local_oob_data.commitment[i], &ca_string[i*2], 2, 2 );
                    }
                    ra = &ra_string[0];
                    ca = &ca_string[0];
                }
                break;

            default:
                break;
        }

    }

    restful_gateway_write_bonding( stream, (const char*)node_handle, pairing_status_code, (const char*)pairingID, (const char*)display, (const char*)ra, (const char*)ca );

    current_stream       = NULL;

    return status;
}

wiced_result_t restful_smart_configure_pairing( wiced_http_response_stream_t* stream, smart_address_t* device_address, char* pairing_id, char* tk, uint32_t* passkey, char* rb, char* cb, wiced_bool_t confirmed )
{
    wiced_result_t      status     = WICED_SUCCESS;
    smart_connection_t* connection = NULL;

    uint8_t pairing_status_code = 0;

    uint8_t tk_value[16] = { 0 };
    uint8_t rb_value[16] = { 0 };
    uint8_t cb_value[16] = { 0 };

    current_stream       = stream;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );

    if( tk != NULL ) string_to_hex_array( tk, tk_value, sizeof( tk_value ) );
    if( rb != NULL )
    {
        string_to_hex_array( rb, rb_value, sizeof( rb_value ) );
        memcpy( &sc_oob_data.peer_oob_data.randomizer,   &rb_value, sizeof( BT_OCTET16 ) );
    }
    if( cb != NULL )
    {
        string_to_hex_array( cb, cb_value, sizeof( cb_value ) );
        memcpy( &sc_oob_data.peer_oob_data.commitment,   &cb_value, sizeof( BT_OCTET16 ) );
    }

    if( smartbridge_find_connection_by_device_address( device_address, &connection ) == WICED_SUCCESS )
    {
        if( strcasecmp( pairing_id, connection->bonding_info.pairing_id ) == WICED_SUCCESS )
        {
            pairing_status_code = connection->bonding_info.status;

            switch( pairing_status_code )
            {
                case PAIRING_SUCCESSFUL:
                    connection->bonding_info.is_bonded = WICED_TRUE;
                    goto SendResponse;
                    break;
                case PAIRING_ABORTED:
                case PASSKEY_DISPLAY_EXPECTED:
                case PAIRING_FAILED:
                    goto SendResponse;
                    break;
                case LE_SECURE_OOB_EXPECTED:
                    wiced_bt_dev_sec_bond( device_address->address, BLE_ADDR_PUBLIC, BT_TRANSPORT_LE, 0, NULL );
                    break;
                case LE_LEGACY_OOB_EXPECTED:
                    wiced_bt_smp_oob_data_reply( device_address->address, WICED_SUCCESS, sizeof( tk_value ), tk_value );
                    break;
                case PASSKEY_INPUT_EXPECTED:
                    wiced_bt_dev_pass_key_req_reply( WICED_SUCCESS, device_address->address, *passkey );
                    break;
                case NUMERIC_COMPARISON_EXPECTED:
                    SMP_ConfirmReply( device_address->address, ( ( confirmed ) ? WICED_SUCCESS : WICED_ERROR ) );
                    break;

                default:
                    break;
            }
            wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );
        }
    }

SendResponse:
    if ( connection != NULL )
    {
        restful_gateway_write_bonding( stream, NULL, connection->bonding_info.status, NULL, NULL, NULL, NULL );
    }
    else
    {
        restful_gateway_write_bonding( stream, NULL, PAIRING_FAILED, NULL, NULL, NULL, NULL );
    }

    current_stream = NULL;

    return status;
}

wiced_result_t restful_smart_return_bonded_nodes( wiced_http_response_stream_t* stream, smart_address_t* device_address )
{
    uint16_t                          num_devices       = 10;
    wiced_bt_dev_bonded_device_info_t p_paired_device_list[ num_devices ];

    uint8_t  i;
    uint32_t count;
    char node_handle[NODE_BUFFER_LENGTH]     = { 0 };
    char bd_addr    [BD_ADDR_BUFFER_LENGTH]  = { 0 };
    char bdaddrtype [2]= { 0 };

    smart_connection_t* current_connection;
    linked_list_node_t* current_node;

    current_stream = stream;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );
    restful_gateway_write_node_array_start( stream );

    smartbridge_get_connection_list( &current_node, &count );

    current_connection = (smart_connection_t*)current_node->data;

    for( i = 0; i < count; i++ )
    {
        if( current_connection->bonding_info.is_bonded == WICED_TRUE )
        {
            if( ( device_address == NULL ) || ( memcmp( &device_address->address, current_connection->device_address.address, 6 ) == WICED_SUCCESS ) )
            {
                format_node_string( node_handle, &current_connection->device_address, (uint8_t)current_connection->address_type );
                device_address_to_string( &current_connection->device_address, bd_addr );
                unsigned_to_hex_string( (uint32_t)p_paired_device_list[i].addr_type, bdaddrtype, 1, 2 );

                restful_smart_write_bonded_nodes( stream, (const char*)node_handle, (const char*)bd_addr, bdaddrtype, 0x0E );
            }
        }
        current_node = current_node->next;
        current_connection = (smart_connection_t*)current_node->data;
    }

    restful_gateway_write_array_end( stream );

    current_stream = NULL;

    return ( WICED_SUCCESS );
}

wiced_result_t restful_smart_remove_bond( wiced_http_response_stream_t* stream, smart_address_t* device_address )
{
    wiced_result_t status;

    current_stream = stream;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );

    status = wiced_bt_dev_delete_bonded_device( device_address->address );

    current_stream = NULL;

    return status;
}

wiced_result_t restful_smart_cancel_bond( wiced_http_response_stream_t* stream, smart_address_t* device_address )
{
    wiced_result_t status;

    current_stream = stream;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );

    status = wiced_bt_dev_sec_bond_cancel( device_address->address );

    current_stream = NULL;

    return status;
}

/* REST GAP APIs */
static void restful_gateway_gap_scan_result_callback( wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data )
{
    if ( p_scan_result )
    {
        char            node_handle[14]    = { 0 };
        char            bd_addr[13]        = { 0 };
        char            rssi[5]            = { 0 };
        char            bdaddrtype[2]      = { 0 };

        char            adv_type_string[2] = { 0 };
        uint8_t         adv_type;

        smart_address_t address;
        uint8_t         adv_data_length;
        char            adv_data[31*2] = {0};
        uint8_t         *p = p_adv_data;
        uint8_t         i;

        address.address[0] = p_scan_result->remote_bd_addr[0];
        address.address[1] = p_scan_result->remote_bd_addr[1];
        address.address[2] = p_scan_result->remote_bd_addr[2];
        address.address[3] = p_scan_result->remote_bd_addr[3];
        address.address[4] = p_scan_result->remote_bd_addr[4];
        address.address[5] = p_scan_result->remote_bd_addr[5];

        format_node_string( node_handle, &address, (uint8_t)p_scan_result->ble_addr_type );
        device_address_to_string( &address, bd_addr );
        signed_to_decimal_string( (int32_t)p_scan_result->rssi, rssi, 1, 5 );
        unsigned_to_hex_string( (uint32_t)p_scan_result->ble_addr_type, bdaddrtype, 1, 2 );
        restful_smart_write_gap_node( current_stream, (const char*)node_handle, (const char*)bd_addr, (const char*)bdaddrtype, (const char*)rssi );
        restful_smart_write_adv_array_start( current_stream );

        STREAM_TO_UINT8( adv_data_length, p );

        while( adv_data_length && (p - p_adv_data <= 31 ) )
        {
            memset( adv_data, 0, sizeof( adv_data ) );

            STREAM_TO_UINT8( adv_type, p );
            unsigned_to_hex_string( (uint32_t)adv_type, adv_type_string, 1, 2 );

            for( i = 0; i < ( adv_data_length-1 ); i++ )
            {
                unsigned_to_hex_string( (uint32_t)p[i], &adv_data[i*2], 2, 2 );
            }

            restful_smart_write_adv_data( current_stream, (const char*)adv_type_string, (const char*)adv_data );

            p += adv_data_length - 1; /* skip the length of data */
            STREAM_TO_UINT8( adv_data_length, p );
        }

        restful_gateway_write_array_end( current_stream );
    }
    else
    {
        WPRINT_LIB_INFO(( "REST: LE scan completed.\n" ));
        wiced_rtos_set_semaphore( &wait_semaphore );
    }
}

wiced_result_t restful_smart_passive_scan( wiced_http_response_stream_t* stream )
{
    current_stream       = stream;

    wiced_bt_cfg_settings.ble_scan_cfg.scan_mode = BTM_BLE_SCAN_MODE_PASSIVE;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );
    restful_gateway_write_node_array_start( stream );

    wiced_bt_ble_scan( BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_TRUE, restful_gateway_gap_scan_result_callback );

    wiced_rtos_get_semaphore( &wait_semaphore, WICED_WAIT_FOREVER );

    restful_gateway_write_array_end( stream );

    current_stream       = NULL;

    return WICED_SUCCESS;
}

wiced_result_t restful_smart_active_scan( wiced_http_response_stream_t* stream )
{
    current_stream = stream;

    wiced_bt_cfg_settings.ble_scan_cfg.scan_mode = BTM_BLE_SCAN_MODE_ACTIVE;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );
    restful_gateway_write_node_array_start( stream );

    wiced_bt_ble_scan( BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_TRUE, restful_gateway_gap_scan_result_callback );

    wiced_rtos_get_semaphore( &wait_semaphore, WICED_WAIT_FOREVER );

    restful_gateway_write_array_end( stream );

    current_stream = NULL;

    return WICED_SUCCESS;
}

wiced_result_t restful_smart_enabled_devices( wiced_http_response_stream_t* stream )
{
    linked_list_node_t* current_node;
    smart_connection_t* current_connection;
    uint32_t            count;
    uint8_t             i;

    current_stream = stream;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );
    restful_gateway_write_node_array_start( stream );

    smartbridge_get_connection_list( &current_node, &count );

    current_connection = (smart_connection_t*)current_node->data;

    for( i = 0; i < count; i++ )
    {
        char node_handle[NODE_BUFFER_LENGTH]    = { 0 };
        char bd_addr    [BD_ADDR_BUFFER_LENGTH] = { 0 };

        format_node_string( node_handle, &current_connection->device_address, (uint8_t)current_connection->address_type );
        device_address_to_string( &current_connection->device_address, bd_addr );
        restful_smart_write_gap_node( stream, (const char*)node_handle, (const char*)node_handle, NULL, NULL );

        current_node = current_node->next;
        current_connection = (smart_connection_t*)current_node->data;
    }

    restful_gateway_write_array_end( stream );

    current_stream = NULL;

    return WICED_SUCCESS;
}

wiced_result_t restful_smart_enabled_node_data( wiced_http_response_stream_t* stream, const smart_address_t* device_address )
{
    smart_connection_t* connection;

    current_stream = stream;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );
    restful_gateway_write_node_array_start( stream );

    if( smartbridge_find_connection_by_device_address( device_address, &connection ) == WICED_SUCCESS )
    {
        char node_handle[NODE_BUFFER_LENGTH]    = { 0 };
        char bd_addr    [BD_ADDR_BUFFER_LENGTH] = { 0 };

        format_node_string( node_handle, &connection->device_address, (uint8_t)connection->address_type );
        device_address_to_string( &connection->device_address, bd_addr );
        restful_smart_write_gap_node( stream, (const char*)node_handle, (const char*)bd_addr, NULL, NULL );
    }

    restful_gateway_write_array_end( stream );

    current_stream = NULL;

    return WICED_SUCCESS;
}

wiced_result_t restful_smart_remove_device( wiced_http_response_stream_t* stream, const smart_address_t* device_address )
{
    smart_connection_t* connection;

    current_stream = stream;

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );

    if( smartbridge_find_connection_by_device_address( device_address, &connection ) == WICED_SUCCESS )
    {
        wiced_bt_gatt_disconnect( connection->connection_id );
    }

    current_stream = NULL;

    return WICED_SUCCESS;
}

static void restful_gateway_gap_name_scan_result_callback( wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data )
{
    if ( p_scan_result )
    {
        char            node_handle[14] = { 0 };
        char            bd_addr[13] = { 0 };
        char            devname[BT_LE_SCAN_REMOTE_NAME_MAXLEN] = { 0 };

        smart_address_t address;
        uint8_t         adv_node_name_len;

        uint8_t         *p_adv_devname;

        if( memcmp( p_scan_result->remote_bd_addr, remote_address.address, 6 ) == WICED_SUCCESS )
        {
            WPRINT_LIB_INFO( ( "Device found!\n" ) );

            if( ( p_adv_devname = wiced_bt_ble_check_advertising_data( p_adv_data, BTM_BLE_ADVERT_TYPE_NAME_COMPLETE, &adv_node_name_len ) ) != NULL)
            {
                address.address[0] = p_scan_result->remote_bd_addr[0];
                address.address[1] = p_scan_result->remote_bd_addr[1];
                address.address[2] = p_scan_result->remote_bd_addr[2];
                address.address[3] = p_scan_result->remote_bd_addr[3];
                address.address[4] = p_scan_result->remote_bd_addr[4];
                address.address[5] = p_scan_result->remote_bd_addr[5];

                format_node_string( node_handle, &address, p_scan_result->ble_addr_type );
                device_address_to_string( &address, bd_addr );

                if (adv_node_name_len > BT_LE_SCAN_REMOTE_NAME_MAXLEN)
                    adv_node_name_len = BT_LE_SCAN_REMOTE_NAME_MAXLEN;

                memcpy( devname, p_adv_devname, adv_node_name_len );

                devname[ adv_node_name_len ] = '\0';

                restful_smart_write_node_name( current_stream, node_handle, bd_addr, devname );

                WPRINT_LIB_INFO(( "Name: %s\n", devname ));
            }
        }
    }
    else
    {
        WPRINT_LIB_INFO(( "REST: LE scan completed.\n" ));
        wiced_rtos_set_semaphore( &wait_semaphore );
    }
}

wiced_result_t restful_smart_discover_node_name( wiced_http_response_stream_t* stream, const smart_address_t* device_address )
{
    current_stream = stream;

    wiced_bt_cfg_settings.ble_scan_cfg.scan_mode = BTM_BLE_SCAN_MODE_ACTIVE;

    memcpy( &remote_address, device_address, sizeof( smart_address_t ) );

    WPRINT_LIB_INFO( ( "Device Address : %02X:%02X:%02X:%02X:%02X:%02X\n",
                        remote_address.address[0],
                        remote_address.address[1],
                        remote_address.address[2],
                        remote_address.address[3],
                        remote_address.address[4],
                        remote_address.address[5] ) );

    restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_200 );

    wiced_bt_ble_scan( BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_TRUE, restful_gateway_gap_name_scan_result_callback );

    wiced_rtos_get_semaphore( &wait_semaphore, CONNECT_TIMEOUT_MS );

    current_stream = NULL;

    return WICED_SUCCESS;
}

static void string_to_hex_array( const char* string, uint8_t* array, uint8_t size )
{
    uint32_t i;
    uint32_t j;

    for ( i = 0, j = 0; i < size; i++, j += 2 )
    {
        char  buffer[3] = {0};
        char* end;

        buffer[0] = string[j];
        buffer[1] = string[j + 1];
        array[i] = strtoul( buffer, &end, 16 );
    }
}
