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

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * Characteristic Descriptor Structure
 */
typedef struct
{
    linked_list_node_t this_node;               /* Linked-list node of this descriptor */
    uint16_t           descriptor_handle;       /* Descriptor handle */
    wiced_bt_uuid_t    descriptor_uuid;         /* Descriptor UUID */
    uint16_t           descriptor_value_length; /* Descriptor value length in bytes */
    uint8_t*           descriptor_value;        /* Pointer to descriptor value */
} cached_smart_descriptor_t;

/**
 * Characteristic Structure
 */
typedef struct
{
    linked_list_node_t this_node;                   /* Linked-list node of this characteristic */
    uint16_t           characteristic_handle;       /* Characteristic handle */
    wiced_bt_uuid_t    characteristic_uuid;         /* Characteristic UUID */
    uint8_t            characteristic_properties;   /* Characteristic properties */
    uint16_t           descriptor_start_handle;     /* Descriptor start handle */
    uint16_t           descriptor_end_handle;       /* Descriptor end handle */
    linked_list_t      descriptor_list;             /* Descriptor linked-list */
    uint16_t           characteristic_value_length; /* Characteristic value length */
    uint8_t*           characteristic_value;        /* Pointer to characteristic value */
} cached_smart_characteristic_t;

/**
 * Service Structure
 */
typedef struct
{
    linked_list_node_t this_node;           /* Linked-list node of this service */
    uint16_t           service_handle;      /* Service handle */
    wiced_bt_uuid_t    service_uuid;        /* Service UUID */
    wiced_bool_t       is_primary_service;  /* WICED_TRUE is service is primary. WICED_FALSE if service is secondary */
    linked_list_t      characteristic_list; /* Characteristic linked-list */
} cached_smart_service_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t smartbridge_cache_add_service( smart_connection_t* connection, uint16_t service_handle, const wiced_bt_uuid_t* service_uuid, wiced_bool_t is_primary_service, cached_smart_service_t** service_added );

wiced_result_t smartbridge_cache_remove_service( smart_connection_t* connection, cached_smart_service_t* service );

wiced_result_t smartbridge_cache_find_service_by_uuid( smart_connection_t* connection, const wiced_bt_uuid_t* service_uuid, cached_smart_service_t** service_found );

wiced_result_t smartbridge_cache_find_service_by_handle( smart_connection_t* connection, uint16_t service_handle, cached_smart_service_t** service_found );

wiced_result_t smartbridge_cache_add_characteristic( cached_smart_service_t* service, uint16_t characteristic_handle, const wiced_bt_uuid_t* characteristic_uuid, uint8_t characteristic_properties, cached_smart_characteristic_t** characteristic_added );

wiced_result_t smartbridge_cache_remove_characteristic( cached_smart_service_t* service, cached_smart_characteristic_t* characteristic );

wiced_result_t smartbridge_cache_add_characteristic_value( cached_smart_characteristic_t* characteristic, uint16_t length, const uint8_t* value );

wiced_result_t smartbridge_cache_find_characteristic_by_handle( cached_smart_service_t* service, uint16_t characteristic_handle, cached_smart_characteristic_t** characteristic_found );

wiced_result_t smartbridge_cache_find_characteristic_by_uuid( cached_smart_service_t* service, const wiced_bt_uuid_t* characteristic_uuid, cached_smart_characteristic_t** characteristic_found );

wiced_result_t smartbridge_cache_add_descriptor( cached_smart_characteristic_t* characteristic, uint16_t descriptor_handle, const wiced_bt_uuid_t* descriptor_uuid, cached_smart_descriptor_t** descriptor_added );

wiced_result_t smartbridge_cache_remove_descriptor( cached_smart_characteristic_t* characteristic, cached_smart_descriptor_t* descriptor );

wiced_result_t smartbridge_cache_add_descriptor_value( cached_smart_descriptor_t* descriptor, uint16_t value_length, const uint8_t* value );

wiced_result_t smartbridge_cache_find_descriptor_by_handle( cached_smart_characteristic_t* characteristic, uint16_t descriptor_handle, cached_smart_descriptor_t** descriptor_found );

wiced_result_t smartbridge_cache_find_descriptor_by_uuid( cached_smart_characteristic_t* characteristic, const wiced_bt_uuid_t* descriptor_uuid, cached_smart_descriptor_t** descriptor_found );

wiced_result_t smartbridge_cache_remove_descriptors( cached_smart_characteristic_t* characteristic );

wiced_result_t smartbridge_cache_remove_characteristics( cached_smart_service_t* service );

wiced_result_t smartbridge_cache_remove_services( smart_connection_t* connection );

wiced_result_t smartbridge_cache_get_buffer( void** buffer, uint32_t size );

wiced_result_t smartbridge_cache_release_buffer( void* buffer );

#ifdef __cplusplus
} /* extern "C" */
#endif
