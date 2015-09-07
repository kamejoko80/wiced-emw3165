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
#include "smartbridge_cache.h"
#include "smartbridge_gatt.h"
#include "linked_list.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_stack.h"
#include "bt_types.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define STACK_INIT_TIMEOUT_MS  ( 30 * SECONDS )

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
    linked_list_t            connection_list;
    linked_list_t            interface_list;
    smartbridge_interface_t* current_interface;
} smartbridge_t;

typedef struct
{
    linked_list_node_t   this_node;
    wiced_bt_gatt_data_t gatt_data;
} smartbridge_cached_value_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

static wiced_result_t         smartbridge_get_buffer       ( void** buffer, uint32_t size );
static wiced_result_t         smartbridge_release_buffer   ( void* buffer );
static wiced_bt_dev_status_t  smartbridge_stack_callback   ( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t* data );
static wiced_bt_gatt_status_t smartbridge_gatt_callback    ( wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t* data );
static wiced_result_t         smartbridge_add_connection   ( const smart_address_t* device_address, uint16_t connection_id, uint8_t address_type, smart_connection_t** connection_added );
static wiced_result_t         smartbridge_remove_connection( smart_connection_t* connection );
static wiced_result_t         smartbridge_add_cached_value ( smart_connection_t* connection, wiced_bt_gatt_data_t* data );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static smartbridge_t                  smartbridge_object;
static wiced_semaphore_t              wait_semaphore;
static wiced_result_t                 init_result;
extern wiced_bt_cfg_settings_t        wiced_bt_cfg_settings;
extern const wiced_bt_cfg_buf_pool_t  wiced_bt_cfg_buf_pools[];
extern smart_address_t                local_address;
extern smart_address_t                remote_address;
extern wiced_bt_dev_ble_io_caps_req_t local_io_caps_ble;
extern wiced_bt_smp_sc_oob_t          sc_oob_data;

/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_result_t smartbridge_get_buffer( void** buffer, uint32_t size )
{
    /* Allocate buffer object */
    *buffer = malloc_named( "smartbridge_buffer", size );
    wiced_assert( "failed to malloc", ( buffer != NULL ) );
    if ( *buffer  == NULL )
    {
        return WICED_OUT_OF_HEAP_SPACE;
    }

    return WICED_SUCCESS;
}

static wiced_result_t smartbridge_release_buffer( void* buffer )
{
    wiced_assert( "buffer is NULL", ( buffer != NULL ) );
    free( buffer );
    return WICED_SUCCESS;
}

static wiced_result_t smartbridge_add_cached_value( smart_connection_t* connection, wiced_bt_gatt_data_t* data )
{
    smartbridge_cached_value_t* new_cached_data;
    wiced_result_t result;

    /* Allocate connection object */
    result = smartbridge_get_buffer( (void**)&new_cached_data, sizeof( smartbridge_cached_value_t ) + data->len );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    memcpy( &new_cached_data->gatt_data, data, sizeof( new_cached_data->gatt_data ) );
    new_cached_data->gatt_data.p_data = (uint8_t*)(new_cached_data + 1);
    memcpy( new_cached_data->gatt_data.p_data, data->p_data, data->len );

    return linked_list_insert_node_at_front( &connection->cached_value_list, &new_cached_data->this_node );
}


wiced_result_t smartbridge_init( void )
{
    wiced_result_t result;

    smartbridge_object.current_interface = NULL;

    result = linked_list_init( &smartbridge_object.connection_list );
    if ( result == WICED_SUCCESS )
    {
        result = linked_list_init( &smartbridge_object.interface_list );
    }

    wiced_rtos_init_semaphore( &wait_semaphore );

    /* Initialize BT host stack and controller */
    result = wiced_bt_stack_init( smartbridge_stack_callback, &wiced_bt_cfg_settings, wiced_bt_cfg_buf_pools );
    if ( result != WICED_SUCCESS )
    {
        wiced_rtos_deinit_semaphore( &wait_semaphore );
        return result;
    }

    /* Wait for stack initialisation to complete */
    result = wiced_rtos_get_semaphore( &wait_semaphore, STACK_INIT_TIMEOUT_MS );
    wiced_rtos_deinit_semaphore( &wait_semaphore );

    return result;
}

wiced_result_t smartbridge_deinit( void )
{
    wiced_result_t      result;
    linked_list_node_t* current_node;

    /* Remove all connections */
    while ( linked_list_get_front_node( &smartbridge_object.connection_list, &current_node ) == WICED_SUCCESS )
    {
        smart_connection_t* current_connection = (smart_connection_t*)current_node;

        result = smartbridge_remove_connection( current_connection );
        if ( result != WICED_SUCCESS )
        {
            wiced_assert( "Failed to remove connection", 0!=0 );
            return result;
        }
    }

    /* Deinit connection list */
    result = linked_list_deinit( &smartbridge_object.connection_list );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Remove all interfaces */
    while ( linked_list_get_front_node( &smartbridge_object.interface_list, &current_node ) == WICED_SUCCESS )
    {
        linked_list_remove_node( &smartbridge_object.interface_list, current_node );
    }

    /* Deinit interfaces list */
    result = linked_list_deinit( &smartbridge_object.interface_list );

    return result;
}

wiced_result_t smartbridge_add_interface( smartbridge_interface_t* interface, smartbridge_callback_t callback )
{
    interface->callback = callback;

    return linked_list_insert_node_at_front( &smartbridge_object.interface_list, &interface->this_node );
}

wiced_result_t smartbridge_remove_interface( smartbridge_interface_t* interface )
{
    return linked_list_remove_node( &smartbridge_object.interface_list, &interface->this_node );
}

wiced_bt_gatt_status_t smartbridge_connect( smartbridge_interface_t* interface, const smart_address_t* address, wiced_bt_ble_address_type_t address_type, wiced_bt_ble_conn_mode_t connection_mode )
{
    wiced_bt_device_address_t device_address;

    smartbridge_object.current_interface = interface;

    memcpy( &device_address, &address->address, sizeof(wiced_bt_device_address_t) );

    return wiced_bt_gatt_le_connect( device_address, address_type, connection_mode, WICED_TRUE );
}

wiced_bt_gatt_status_t smartbridge_disconnect( smartbridge_interface_t* interface, smart_connection_t* connection )
{
    smartbridge_object.current_interface = interface;
    return wiced_bt_gatt_disconnect( connection->connection_id );
}

wiced_bt_gatt_status_t smartbridge_cancel_connect( smartbridge_interface_t* interface, const smart_address_t* address )
{
    wiced_bt_device_address_t device_address;

    smartbridge_object.current_interface = interface;

    memcpy( &device_address, &address->address, sizeof(wiced_bt_device_address_t) );

    return wiced_bt_gatt_cancel_connect( device_address, WICED_TRUE );
}

wiced_bt_gatt_status_t smartbridge_discover( smartbridge_interface_t* interface, smart_connection_t* connection, wiced_bt_gatt_discovery_type_t type, const wiced_bt_gatt_discovery_param_t* parameter )
{
    smartbridge_object.current_interface = interface;
    return wiced_bt_gatt_send_discover( connection->connection_id, type, (wiced_bt_gatt_discovery_param_t*)parameter );
}

wiced_bt_gatt_status_t smartbridge_read( smartbridge_interface_t* interface, smart_connection_t* connection, wiced_bt_gatt_read_type_t type, const wiced_bt_gatt_read_param_t* parameter )
{
    smartbridge_object.current_interface = interface;
    return wiced_bt_gatt_send_read( connection->connection_id, type, (wiced_bt_gatt_read_param_t*)parameter );
}

wiced_bt_gatt_status_t smartbridge_write( smartbridge_interface_t* interface, smart_connection_t* connection, wiced_bt_gatt_write_type_t type, const wiced_bt_gatt_value_t* value )
{
    smartbridge_object.current_interface = interface;
    return wiced_bt_gatt_send_write( connection->connection_id, type, (wiced_bt_gatt_value_t*)value );
}

wiced_bt_gatt_status_t smartbridge_execute_write( smartbridge_interface_t* interface, smart_connection_t* connection )
{
    smartbridge_object.current_interface = interface;
    return wiced_bt_gatt_send_execute_write( connection->connection_id, TRUE );
}

wiced_bt_gatt_status_t smartbridge_confirm_indication( smartbridge_interface_t* interface, smart_connection_t* connection, uint16_t handle )
{
    smartbridge_object.current_interface = interface;
    return wiced_bt_gatt_send_indication_confirm( connection->connection_id, handle );
}

static wiced_bool_t compare_cached_value_by_handle( linked_list_node_t* node_to_compare, void* user_data )
{
    smartbridge_cached_value_t* current_value = (smartbridge_cached_value_t*)node_to_compare;
    uint16_t                    handle        = (uint16_t)( (uint32_t)user_data & 0xffff );

    if ( current_value->gatt_data.handle == handle )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t smartbridge_read_cached_value( smart_connection_t* connection, uint16_t handle, uint8_t* buffer, uint32_t buffer_length, uint32_t* available_data_length )
{
    smartbridge_cached_value_t* value_found;

    if ( linked_list_find_node( &connection->cached_value_list, compare_cached_value_by_handle, (void*)( (uint32_t)handle ), (linked_list_node_t**)&value_found ) == WICED_SUCCESS )
    {
        memcpy( buffer, value_found->gatt_data.p_data, MIN( buffer_length, value_found->gatt_data.len ) );
        *available_data_length = value_found->gatt_data.len;
        return WICED_SUCCESS;
    }

    return WICED_NOT_FOUND;
}

wiced_result_t smartbridge_get_connection_list( linked_list_node_t** connection_list_front, uint32_t* count )
{
    *connection_list_front = (linked_list_node_t*)(&smartbridge_object.connection_list.front);

    *count = smartbridge_object.connection_list.count;

    return WICED_SUCCESS;
}

static wiced_result_t smartbridge_add_connection( const smart_address_t* device_address, uint16_t connection_id, uint8_t address_type, smart_connection_t** connection_added )
{
    wiced_result_t      result;
    smart_connection_t* new_connection;

    wiced_assert( "bad arg", ( device_address != NULL) && ( connection_added != NULL ) );

    /* Allocate connection object */
    result = smartbridge_get_buffer( (void**)&new_connection, sizeof( *new_connection ) );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Create service linked-list */
    result = linked_list_init( &new_connection->service_list );
    if ( result != WICED_SUCCESS )
    {
        smartbridge_release_buffer( (void*)new_connection );
        return result;
    }

    memcpy( &new_connection->device_address, device_address, sizeof( new_connection->device_address ) );
    new_connection->connection_id = connection_id;
    new_connection->address_type  = address_type;
    *connection_added             = new_connection;

    return linked_list_insert_node_at_front( &smartbridge_object.connection_list, &new_connection->this_node );
}

static wiced_result_t smartbridge_remove_connection( smart_connection_t* connection )
{
    wiced_result_t result;

    wiced_assert( "bad arg", ( connection != NULL ) );

    /* Remove all services */
    result = smartbridge_cache_remove_services( connection );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Remove connection from connection list */
    result = linked_list_remove_node( &smartbridge_object.connection_list, &connection->this_node );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Release buffer */
    return smartbridge_release_buffer( (void*)connection );
}

static wiced_bool_t compare_connection_by_device_address( linked_list_node_t* node_to_compare, void* user_data )
{
    smart_connection_t* current_connection = (smart_connection_t*)node_to_compare;
    smart_address_t*    device_address     = (smart_address_t*)user_data;

    if ( memcmp( &current_connection->device_address, device_address, sizeof( smart_address_t ) ) == 0 )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t smartbridge_find_connection_by_device_address( const smart_address_t* device_address, smart_connection_t** connection_found )
{
    wiced_assert( "bad arg", ( device_address != NULL ) && ( connection_found != NULL ) );

    return linked_list_find_node( &smartbridge_object.connection_list, compare_connection_by_device_address, (void*) device_address, (linked_list_node_t**)connection_found );
}

static wiced_bool_t compare_connection_by_id( linked_list_node_t* node_to_compare, void* user_data )
{
    smart_connection_t* current_connection = (smart_connection_t*)node_to_compare;
    uint32_t            connection_id      = (uint32_t)user_data;

    if ( current_connection->connection_id == connection_id )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t smartbridge_find_connection_by_id( uint16_t connection_id, smart_connection_t** connection_found )
{
    wiced_assert( "bad arg", ( connection_found != NULL ) );

    return linked_list_find_node( &smartbridge_object.connection_list, compare_connection_by_id, (void*)( connection_id & 0xffffffff ), (linked_list_node_t**)connection_found );
}

static wiced_bt_dev_status_t smartbridge_stack_callback( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data )
{
    wiced_bt_dev_status_t status = WICED_BT_SUCCESS;
    smart_address_t       device_address;
    smart_connection_t*   connection;
    wiced_bool_t          release_sem = WICED_TRUE;

    WPRINT_BT_APP_INFO(( "restful_gateway_stack_callback event = %d\n", event ));

    switch ( event )
    {
        case BTM_ENABLED_EVT:
        {
            /* Initialize GATT REST API Server once Bluetooth controller and host stack is enabled */
            if ( ( status = p_event_data->enabled.status ) == WICED_BT_SUCCESS )
            {
                wiced_bt_dev_read_local_addr(local_address.address);
                WPRINT_BT_APP_INFO(( "Local Bluetooth Address: [%02X:%02X:%02X:%02X:%02X:%02X]\n", local_address.address[0], local_address.address[1], local_address.address[2],
                                                                                                   local_address.address[3], local_address.address[4], local_address.address[5]) );

                /* Register for GATT event notifications */
                wiced_bt_gatt_register( smartbridge_gatt_callback );
            }
            else
            {
                status = WICED_BT_ERROR;
            }

            /* SmartBridge is initialised. Notify thread that calls bt_smartbridge_init() */
            init_result = status;
            wiced_rtos_set_semaphore( &wait_semaphore );
            break;
        }
        case BTM_SECURITY_REQUEST_EVT:
        {
            WPRINT_LIB_INFO(( "Security grant request\n" ));
            wiced_bt_ble_security_grant( p_event_data->security_request.bd_addr, WICED_BT_SUCCESS );
            break;
        }
        case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
        {
            WPRINT_LIB_INFO(( "Pairing IO Capabilities request, local_io_cap = 0x%x\n", local_io_caps_ble.local_io_cap ));
            p_event_data->pairing_io_capabilities_ble_request.local_io_cap = local_io_caps_ble.local_io_cap;
            p_event_data->pairing_io_capabilities_ble_request.oob_data     = local_io_caps_ble.oob_data;
            p_event_data->pairing_io_capabilities_ble_request.auth_req     = local_io_caps_ble.auth_req;
            p_event_data->pairing_io_capabilities_ble_request.max_key_size = local_io_caps_ble.max_key_size;
            p_event_data->pairing_io_capabilities_ble_request.init_keys    = local_io_caps_ble.init_keys;
            p_event_data->pairing_io_capabilities_ble_request.resp_keys    = local_io_caps_ble.resp_keys;

            break;
        }
        case BTM_PASSKEY_REQUEST_EVT:
        case BTM_SMP_REMOTE_OOB_DATA_REQUEST_EVT:
        case BTM_USER_CONFIRMATION_REQUEST_EVT:
        case BTM_PASSKEY_NOTIFICATION_EVT:
        {
            memcpy( &device_address.address, &p_event_data->user_passkey_request.bd_addr, 6 );

            if( smartbridge_find_connection_by_device_address( &device_address, &connection ) == WICED_SUCCESS )
            {
                switch ( event )
                {
                    case BTM_PASSKEY_REQUEST_EVT:
                    {
                        connection->bonding_info.status  = PASSKEY_INPUT_EXPECTED;
                        break;
                    }
                    case BTM_SMP_REMOTE_OOB_DATA_REQUEST_EVT:
                    {
                        connection->bonding_info.status = LE_LEGACY_OOB_EXPECTED;
                        break;
                    }
                    case BTM_USER_CONFIRMATION_REQUEST_EVT:
                    {
                        connection->bonding_info.status  = NUMERIC_COMPARISON_EXPECTED;
                        connection->bonding_info.display = p_event_data->user_confirmation_request.numeric_value;
                        WPRINT_LIB_INFO(( "BTM_USER_CONFIRMATION_REQUEST_EVT value to confirm = %lu\n", connection->bonding_info.display ));
                        break;
                    }
                    case BTM_PASSKEY_NOTIFICATION_EVT:
                    {
                        connection->bonding_info.status  = PASSKEY_DISPLAY_EXPECTED;
                        connection->bonding_info.passkey = p_event_data->user_passkey_notification.passkey;
                        WPRINT_LIB_INFO(( "BTM_PASSKEY_NOTIFICATION_EVT client must respond with passkey = %lu\n", connection->bonding_info.passkey ));
                        break;
                    }
                }


            }
            wiced_rtos_set_semaphore( &wait_semaphore );
            break;
        }
        case BTM_PAIRING_COMPLETE_EVT:
        {
            WPRINT_LIB_INFO(( "Pairing complete status=%i, reason=0x%x.\n", p_event_data->pairing_complete.pairing_complete_info.ble.status,
                p_event_data->pairing_complete.pairing_complete_info.ble.reason ));

            memcpy( &device_address.address, p_event_data->pairing_complete.bd_addr, 6 );

            /* Check if device is in the list */
            if( smartbridge_find_connection_by_device_address( &device_address, &connection ) == WICED_SUCCESS )
            {
                if ( connection->bonding_info.status == PASSKEY_DISPLAY_EXPECTED )
                {
                    release_sem = WICED_FALSE;
                }
                if ( p_event_data->pairing_complete.pairing_complete_info.ble.status == WICED_SUCCESS )
                {
                    connection->bonding_info.status = PAIRING_SUCCESSFUL;
                }
                else
                {
                    connection->bonding_info.status = PAIRING_FAILED;
                }
            }
            if( release_sem )
                wiced_rtos_set_semaphore( &wait_semaphore );
            break;
        }

        case BTM_SMP_SC_LOCAL_OOB_DATA_NOTIFICATION_EVT:
        {
            sc_oob_data.peer_oob_data.present   = WICED_FALSE;
            sc_oob_data.local_oob_data.present  = WICED_TRUE;

            memcpy( &sc_oob_data.local_oob_data.randomizer,   &p_event_data->p_smp_sc_local_oob_data->randomizer, sizeof( BT_OCTET16 ) );
            memcpy( &sc_oob_data.local_oob_data.commitment,   &p_event_data->p_smp_sc_local_oob_data->commitment, sizeof( BT_OCTET16 ) );
            memcpy( &sc_oob_data.local_oob_data.addr_sent_to, &p_event_data->p_smp_sc_local_oob_data->addr_sent_to, sizeof( tBLE_BD_ADDR ) );
            memcpy( &sc_oob_data.local_oob_data.private_key_used, &p_event_data->p_smp_sc_local_oob_data->private_key_used, sizeof( BT_OCTET32 ) );
            memcpy( &sc_oob_data.local_oob_data.public_key_used, &p_event_data->p_smp_sc_local_oob_data->public_key_used, sizeof( wiced_bt_public_key_t ) );
#if !RESPONDER_OOB
            wiced_rtos_set_semaphore( &wait_semaphore );
#endif
            break;
        }

        case BTM_SMP_SC_REMOTE_OOB_DATA_REQUEST_EVT:
        {
            uint8_t oob_type;

            oob_type = p_event_data->smp_sc_remote_oob_data_request.oob_type;
            switch ( oob_type )
            {
                case BTM_OOB_PEER:
                    WPRINT_LIB_INFO(( "oob_type = BTM_OOB_PEER\n" ));
                    sc_oob_data.local_oob_data.present = WICED_FALSE;
                    sc_oob_data.peer_oob_data.present  = WICED_TRUE;
                    break;
                case BTM_OOB_LOCAL:
                    WPRINT_LIB_INFO(( "oob_type = BTM_OOB_LOCAL\n" ));
                    sc_oob_data.local_oob_data.present = WICED_TRUE;
                    sc_oob_data.peer_oob_data.present  = WICED_FALSE;
                    break;
                case BTM_OOB_BOTH:
                    WPRINT_LIB_INFO(( "oob_type = BTM_OOB_BOTH\n" ));
                    break;
                default:
                    WPRINT_LIB_INFO(( "Invalid oob_type" ));
                    break;
            }
            wiced_bt_smp_sc_oob_reply( (uint8_t*)&sc_oob_data );
            break;
        }
        default:
            WPRINT_LIB_INFO(( "restful_gateway_stack_callback unhandled event = %d\n", event ));
            break;
    }
    return (status);
}

static wiced_bt_gatt_status_t smartbridge_gatt_callback( wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t* data )
{
    smartbridge_interface_t* current_interface;
    smart_connection_t*      connection;
    smart_address_t          address;

    /* Update internal data */
    switch ( event )
    {
        case GATT_CONNECTION_STATUS_EVT:
            memcpy( &address.address, data->connection_status.bd_addr, sizeof( address ) );

            if ( data->connection_status.connected == WICED_TRUE ) /*&& ( smartbridge_find_connection_by_device_address( (const smart_address_t*)&address, &connection ) == WICED_NOT_FOUND )*/
            {
                smartbridge_add_connection( &address, data->connection_status.conn_id, (uint8_t)data->connection_status.addr_type, &connection );
            }
            else if ( ( data->connection_status.connected == WICED_FALSE ) && ( smartbridge_find_connection_by_device_address( (const smart_address_t*)&address, &connection ) == WICED_SUCCESS ) )
            {
                smartbridge_remove_connection( connection );
            }

            /* Call interface callbacks */
            linked_list_get_front_node( &smartbridge_object.interface_list, (linked_list_node_t**)&current_interface );
            while ( current_interface != NULL )
            {
                current_interface->callback( connection, event, data );
                current_interface = (smartbridge_interface_t*)current_interface->this_node.next;
            }
            break;

        case GATT_OPERATION_CPLT_EVT:
            smartbridge_find_connection_by_id( data->operation_complete.conn_id, &connection );

            if ( ( data->operation_complete.op == GATTC_OPTYPE_NOTIFICATION ) || ( data->operation_complete.op == GATTC_OPTYPE_INDICATION ) )
            {
                smartbridge_add_cached_value( connection, (wiced_bt_gatt_data_t*)&data->operation_complete.response_data.att_value );
            }
            else
            {
                smartbridge_object.current_interface->callback( connection, event, data );
            }
            break;

        case GATT_DISCOVERY_RESULT_EVT:
            smartbridge_find_connection_by_id( data->discovery_result.conn_id, &connection );
            smartbridge_object.current_interface->callback( connection, event, data );
            break;
        case GATT_DISCOVERY_CPLT_EVT:
            smartbridge_find_connection_by_id( data->discovery_complete.conn_id, &connection );
            smartbridge_object.current_interface->callback( connection, event, data );
            break;

        case GATT_ATTRIBUTE_REQUEST_EVT:
            smartbridge_find_connection_by_id( data->attribute_request.conn_id, &connection );
            smartbridge_object.current_interface->callback( connection, event, data );
            break;

        default:
            break;
    }


    return WICED_SUCCESS;
}
