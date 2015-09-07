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
#include "linked_list.h"

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

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

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

wiced_result_t smartbridge_cache_remove_descriptors( cached_smart_characteristic_t* characteristic )
{
    wiced_result_t      result;
    linked_list_node_t* current_node;

    while ( linked_list_get_front_node( &characteristic->descriptor_list, &current_node ) == WICED_SUCCESS )
    {
        cached_smart_descriptor_t* current_descriptor = (cached_smart_descriptor_t*)current_node;

        result = smartbridge_cache_remove_descriptor( characteristic, current_descriptor );
        if ( result != WICED_SUCCESS )
        {
            wiced_assert( "Failed to remove descriptor", 0!=0 );
            return result;
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t smartbridge_cache_remove_characteristics( cached_smart_service_t* service )
{
    wiced_result_t      result;
    linked_list_node_t* current_node;

    while ( linked_list_get_front_node( &service->characteristic_list, &current_node ) == WICED_SUCCESS )
    {
        cached_smart_characteristic_t* current_characteristic = (cached_smart_characteristic_t*)current_node;

        result = smartbridge_cache_remove_characteristic( service, current_characteristic );
        if ( result != WICED_SUCCESS )
        {
            wiced_assert( "Failed to remove characteristic", 0!=0 );
            return result;
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t smartbridge_cache_remove_services( smart_connection_t* connection )
{
    wiced_result_t      result;
    linked_list_node_t* current_node;

    while ( linked_list_get_front_node( &connection->service_list, &current_node ) == WICED_SUCCESS )
    {
        cached_smart_service_t* current_service = (cached_smart_service_t*)current_node;

        result = smartbridge_cache_remove_service( connection, current_service );
        if ( result != WICED_SUCCESS )
        {
            wiced_assert( "Failed to remove service", 0!=0 );
            return result;
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t smartbridge_add_service( smart_connection_t* connection, uint16_t service_handle, const wiced_bt_uuid_t* service_uuid, wiced_bool_t is_primary_service, cached_smart_service_t** service_added )
{
    cached_smart_service_t* new_service;
    wiced_result_t   result;

    wiced_assert( "bad arg", ( connection != NULL ) && ( service_uuid != NULL ) && ( service_added != NULL ) );

    /* Allocate buffer for new service */
    result = smartbridge_get_buffer( (void**)&new_service, sizeof( *new_service ) );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Init characteristic list */
    result = linked_list_init( &new_service->characteristic_list );
    if ( result != WICED_SUCCESS )
    {
        smartbridge_release_buffer( (void*)new_service );
        return result;
    }

    /* Copy content */
    memcpy( &new_service->service_uuid, service_uuid, sizeof( wiced_bt_uuid_t ) );
    new_service->service_handle     = service_handle;
    new_service->is_primary_service = is_primary_service;

    /* Pass new service to caller */
    *service_added = new_service;

    return result;
}

wiced_result_t smartbridge_cache_remove_service( smart_connection_t* connection, cached_smart_service_t* service )
{
    wiced_result_t result;

    wiced_assert( "bad arg", ( connection != NULL ) && ( service != NULL ) );

    /* Remove characteristics */
    result = smartbridge_cache_remove_characteristics( service );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Remove service from services list */
    result = linked_list_remove_node( &connection->service_list, &service->this_node );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Release buffer */
    return smartbridge_release_buffer( (void*)service );
}

static wiced_bool_t compare_service_by_handle( linked_list_node_t* node_to_compare, void* user_data )
{
    cached_smart_service_t* current_service = (cached_smart_service_t*)node_to_compare;
    uint32_t         service_handle  = (uint32_t)user_data;

    if ( current_service->service_handle == service_handle )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t smartbridge_cache_find_service_by_handle( smart_connection_t* connection, uint16_t service_handle, cached_smart_service_t** service_found )
{
    wiced_assert( "bad arg", ( connection != NULL ) && ( service_handle != 0 ) && ( service_found != NULL ) );

    return linked_list_find_node( &connection->service_list, compare_service_by_handle, (void*)( service_handle & 0xffffffff ), (linked_list_node_t**)service_found );
}

static wiced_bool_t compare_service_by_uuid( linked_list_node_t* node_to_compare, void* user_data )
{
    cached_smart_service_t* current_service = (cached_smart_service_t*)node_to_compare;
    wiced_bt_uuid_t* service_uuid    = (wiced_bt_uuid_t*)user_data;

    if ( memcmp( &current_service->service_uuid, service_uuid, sizeof( wiced_bt_uuid_t) ) == 0 )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t smartbridge_cache_find_service_by_uuid( smart_connection_t* connection, const wiced_bt_uuid_t* service_uuid, cached_smart_service_t** service_found )
{
    wiced_assert( "bad arg", ( connection != NULL ) && ( service_uuid != NULL ) && ( service_found != NULL ) );

    return linked_list_find_node( &connection->service_list, compare_service_by_uuid, (void*)service_uuid, (linked_list_node_t**)service_found );
}

wiced_result_t smartbridge_cache_add_characteristic( cached_smart_service_t* service, uint16_t characteristic_handle, const wiced_bt_uuid_t* characteristic_uuid, uint8_t characteristic_properties, cached_smart_characteristic_t** characteristic_added )
{
    cached_smart_characteristic_t* new_characteristic;
    wiced_result_t          result;

    wiced_assert( "bad arg", ( service != NULL ) && ( characteristic_uuid != NULL ) && ( characteristic_added != NULL ) );

    /* Get buffer */
    result = smartbridge_get_buffer( (void**)&new_characteristic, sizeof( *new_characteristic ) );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Create descriptor list */
    result = linked_list_init( &new_characteristic->descriptor_list );
    if ( result != WICED_SUCCESS )
    {
        smartbridge_release_buffer( (void*)new_characteristic );
        return result;
    }

    /* Copy content to buffer */
    new_characteristic->characteristic_handle       =  characteristic_handle;
    new_characteristic->characteristic_uuid         = *characteristic_uuid;
    new_characteristic->characteristic_properties   =  characteristic_properties;
    new_characteristic->characteristic_value_length = 0;
    new_characteristic->characteristic_value        = NULL;

    /* Add to service */
    result = linked_list_insert_node_at_rear( &service->characteristic_list, &new_characteristic->this_node );
    if ( result != WICED_SUCCESS )
    {
        smartbridge_release_buffer( (void*)new_characteristic );
        return result;
    }

    /* Pass new characteristic to caller */
    *characteristic_added = new_characteristic;

    return result;
}

wiced_result_t smartbridge_cache_remove_characteristic( cached_smart_service_t* service, cached_smart_characteristic_t* characteristic )
{
    wiced_result_t result;

    wiced_assert( "bad arg", ( service != NULL ) && ( characteristic != NULL ) );

    /* Remove characteristic from service */
    result = linked_list_remove_node( &service->characteristic_list, &characteristic->this_node );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Delete descriptors */
    result = smartbridge_cache_remove_descriptors( characteristic );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Delete value */
    if ( characteristic->characteristic_value != NULL )
    {
        result = smartbridge_release_buffer( (void*)characteristic->characteristic_value );
        if ( result != WICED_SUCCESS )
        {
            return result;
        }
    }

    /* Delete characteristic */
    return smartbridge_release_buffer( (void*)characteristic );
}

wiced_result_t smartbridge_cache_add_characteristic_value( cached_smart_characteristic_t* characteristic, uint16_t length, const uint8_t* value )
{
    wiced_result_t result;
    void*          value_buffer;

    wiced_assert( "bad arg", ( characteristic != NULL ) && ( value != NULL ) );

    /* Allocate buffer for value */
    result = smartbridge_get_buffer( (void**)&value_buffer, length );
    if ( result == WICED_SUCCESS )
    {
        memcpy( value_buffer, (void*)value, length );
        characteristic->characteristic_value        = value_buffer;
        characteristic->characteristic_value_length = length;
    }

    return result;
}

static wiced_bool_t compare_characteristic_by_handle( linked_list_node_t* node_to_compare, void* user_data )
{
    cached_smart_characteristic_t* current_characteristic = (cached_smart_characteristic_t*)node_to_compare;
    uint16_t                       characteristic_handle  = (uint16_t)( (uint32_t)user_data & 0xffff );

    if ( current_characteristic->characteristic_handle == characteristic_handle )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t smartbridge_cache_find_characteristic_by_handle( cached_smart_service_t* service, uint16_t characteristic_handle, cached_smart_characteristic_t** characteristic_found )
{
    wiced_assert( "bad arg", ( service != NULL ) && ( characteristic_found != NULL ) );

    return linked_list_find_node( &service->characteristic_list, compare_characteristic_by_handle, (void*)( characteristic_handle & 0xffffffff ), (linked_list_node_t**)characteristic_found );
}

static wiced_bool_t compare_characteristic_by_uuid( linked_list_node_t* node_to_compare, void* user_data )
{
    cached_smart_characteristic_t* current_characteristic = (cached_smart_characteristic_t*)node_to_compare;
    wiced_bt_uuid_t*        characteristic_uuid    = (wiced_bt_uuid_t*)user_data;

    if ( memcmp( &current_characteristic->characteristic_uuid, characteristic_uuid, sizeof( wiced_bt_uuid_t ) ) == 0 )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t smartbridge_cache_find_characteristic_by_uuid( cached_smart_service_t* service, const wiced_bt_uuid_t* characteristic_uuid, cached_smart_characteristic_t** characteristic_found )
{
    wiced_assert( "bad arg", ( service != NULL ) && ( characteristic_uuid != NULL ) && ( characteristic_found != NULL ) );

    return linked_list_find_node( &service->characteristic_list, compare_characteristic_by_uuid, (void*) characteristic_uuid, (linked_list_node_t**) ( characteristic_found ) );
}

wiced_result_t smartbridge_cache_add_descriptor( cached_smart_characteristic_t* characteristic, uint16_t descriptor_handle, const wiced_bt_uuid_t* descriptor_uuid, cached_smart_descriptor_t** descriptor_added )
{
    wiced_result_t      result;
    cached_smart_descriptor_t* new_descriptor;
    cached_smart_descriptor_t* temp;

    /* Find in list. If already in there, return error */
    result = smartbridge_cache_find_descriptor_by_handle( characteristic, descriptor_handle, &temp );
    if ( result == WICED_SUCCESS )
    {
        /* TODO: Use more descriptive error code */
        return WICED_ERROR;
    }

    /* Allocate memory space for new descriptor */
    result = smartbridge_get_buffer( (void**)&new_descriptor, sizeof( cached_smart_descriptor_t ) );
    if ( result == WICED_SUCCESS )
    {
        return result;
    }

    /* Copy parameters to descriptor */
    new_descriptor->descriptor_handle       =  descriptor_handle;
    new_descriptor->descriptor_uuid         = *descriptor_uuid;
    new_descriptor->descriptor_value_length =  0;
    new_descriptor->descriptor_value        =  NULL;

    result = linked_list_insert_node_at_rear( &characteristic->descriptor_list, &new_descriptor->this_node );
    if ( result == WICED_SUCCESS )
    {
        return result;
    }

    /* Copy descriptor to output */
    *descriptor_added = new_descriptor;

    return result;
}

wiced_result_t smartbridge_cache_remove_descriptor( cached_smart_characteristic_t* characteristic, cached_smart_descriptor_t* descriptor )
{
    wiced_result_t result;

    wiced_assert( "bad arg", ( descriptor != NULL ) );

    result = linked_list_remove_node( &characteristic->descriptor_list, &descriptor->this_node );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Release value buffer */
    result = smartbridge_release_buffer( (void*)descriptor->descriptor_value );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Release descriptor */
    return smartbridge_release_buffer( (void*)descriptor );
}

wiced_result_t smartbridge_cache_add_descriptor_value( cached_smart_descriptor_t* descriptor, uint16_t value_length, const uint8_t* value )
{
    wiced_result_t result;
    void*          value_buffer;

    wiced_assert( "bad arg", ( descriptor != NULL ) && ( value != NULL ) );

    /* Allocate value buffer */
    result = smartbridge_get_buffer( &value_buffer, (uint32_t)value_length );
    if ( result == WICED_SUCCESS )
    {
        /* Copy content to buffer and assign value to descriptor */
        memcpy( value_buffer, (void*)value, value_length );
        descriptor->descriptor_value_length = value_length;
        descriptor->descriptor_value        = value_buffer;
    }

    return result;
}

static wiced_bool_t compare_descriptor_by_handle( linked_list_node_t* node_to_compare, void* user_data )
{
    cached_smart_descriptor_t* current_descriptor = (cached_smart_descriptor_t*)node_to_compare;
    uint16_t                   descriptor_handle  = (uint16_t)( (uint32_t)user_data & 0xffff );

    if ( current_descriptor->descriptor_handle == descriptor_handle )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t smartbridge_cache_find_descriptor_by_handle( cached_smart_characteristic_t* characteristic, uint16_t descriptor_handle, cached_smart_descriptor_t** descriptor_found )
{
    wiced_assert( "bad arg", ( characteristic != NULL ) && ( descriptor_found != NULL ) );

    return linked_list_find_node( &characteristic->descriptor_list, compare_descriptor_by_handle, (void*)( descriptor_handle & 0xffffffff ), (linked_list_node_t**)descriptor_found );
}

static wiced_bool_t compare_descriptor_by_uuid( linked_list_node_t* node_to_compare, void* user_data )
{
    cached_smart_descriptor_t* current_descriptor = (cached_smart_descriptor_t*)node_to_compare;
    wiced_bt_uuid_t*    descriptor_uuid    = (wiced_bt_uuid_t*)user_data;

    if ( memcmp(&current_descriptor->descriptor_uuid, descriptor_uuid, sizeof( wiced_bt_uuid_t ) ) == 0 )
    {
        return WICED_TRUE;
    }
    else
    {
        return WICED_FALSE;
    }
}

wiced_result_t smartbridge_cache_find_descriptor_by_uuid( cached_smart_characteristic_t* characteristic, const wiced_bt_uuid_t* descriptor_uuid, cached_smart_descriptor_t** descriptor_found )
{
    wiced_assert( "bad arg", ( characteristic != NULL ) && ( descriptor_found != NULL ) );

    return linked_list_find_node( &characteristic->descriptor_list, compare_descriptor_by_uuid, (void*)descriptor_uuid, (linked_list_node_t**)descriptor_found );
}
