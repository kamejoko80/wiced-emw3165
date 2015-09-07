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

/**
 * Pairing status
 */
typedef enum
{
    PAIRING_FAILED,
    PAIRING_SUCCESSFUL,
    PAIRING_ABORTED,
    LE_LEGACY_OOB_EXPECTED,
    LE_SECURE_OOB_EXPECTED,
    PASSKEY_INPUT_EXPECTED,
    PASSKEY_DISPLAY_EXPECTED,
    NUMERIC_COMPARISON_EXPECTED,
} smart_pairing_status_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)
typedef struct
{
    uint8_t address[6]; /* BD address */
} smart_address_t;

typedef struct
{
    char                    pairing_id[33];
    wiced_bool_t            is_bonded;
    smart_pairing_status_t  status;
    uint32_t                passkey;
    uint32_t                display;
} smart_bonding_t;

typedef struct
{
    linked_list_node_t this_node;         /* Linked-list node of this connection */
    smart_address_t    device_address;    /* Bluetooth device address */
    uint8_t            address_type;      /* Device address type */
    uint16_t           connection_id;     /* Connection ID */
    linked_list_t      service_list;      /* Service linked-list */
    linked_list_t      cached_value_list; /* Linked list of cached notified/indicated values */
    smart_bonding_t    bonding_info;      /* Bonding configuration and status */
} smart_connection_t;
#pragma pack()

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/**
 * Initialise SmartBridge
 *
 * @return @ref wiced_result_t
 */
wiced_result_t smartbridge_init( void );

/**
 * Deinitialise SmartBridge
 *
 * @return @ref wiced_result_t
 */
wiced_result_t smartbridge_deinit( void );

/**
 * Get a pointer to the from of the connection list from SmartBridge.
 *
 * @param[out] connection_list_front : Pointer to the front of the connection list object
 * @param[out] count                 : Number of connections in the list
 *
 * @return @ref wiced_result_t
 */
wiced_result_t smartbridge_get_connection_list( linked_list_node_t** connection_list_front, uint32_t* count );

/**
 * Find an existing connection with the given Bluetooth device address
 *
 * @param[in]  device_address   : Bluetooth device address to find in connection list
 * @param[out] connection_found : Pointer that will point to the object of the connection found. NULL if not found.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t smartbridge_find_connection_by_device_address( const smart_address_t* device_address, smart_connection_t** connection_found );

/**
 * Find an existing connection with the given Bluetooth device address
 *
 * @param[in]  device_address   : Bluetooth device address to find in connection list
 * @param[out] connection_found : Pointer that will point to the object of the connection found. NULL if not found.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t smartbridge_find_connection_by_id( uint16_t connection_id, smart_connection_t** connection_found );

#ifdef __cplusplus
} /* extern "C" */
#endif
