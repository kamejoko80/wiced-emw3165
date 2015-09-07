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

#include "wiced_tcpip.h"
#include "wiced_rtos.h"

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
typedef enum
{
    SERVICE_DISCOVER_RESULT_IPV4_ADDRESS_SLOT,
    SERVICE_DISCOVER_RESULT_IPV6_ADDRESS_SLOT
} gedday_discovery_result_ip_slot_t;
/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_ip_address_t     ip[ 2 ];
    /* TODO: IPv6 address */
    uint16_t               port;
    const char*            service_name;  /* This variable is used internally */
    /* This memory for txt string must be released as soon as service structure is processed by the application */
    char*                  txt;
    /* buffers for instance_name and hostname are allocated dynamically by Gedday
     * User of gedday_discover_service must make sure that the memory gets freed as soon as this names are no longer needed */
    char*                  instance_name;
    char*                  hostname;
    wiced_semaphore_t*     semaphore;     /* This variable is used internally */
    volatile wiced_bool_t  is_resolved;
} gedday_service_t;

typedef struct
{
    char*         buffer;
    uint16_t      buffer_length;
    unsigned int  current_size;
    wiced_mutex_t mutex;
} gedday_text_record_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern wiced_result_t gedday_init                          ( wiced_interface_t interface, const char* desired_name );
extern wiced_result_t gedday_discover_service              ( const char* service_query, gedday_service_t* service_result );
extern wiced_result_t gedday_update_service                ( const char* instance_name, const char* service_name );
extern wiced_result_t gedday_add_service                   ( const char* instance_name, const char* service_name, uint16_t port, uint32_t ttl, const char* txt );
extern wiced_result_t gedday_add_dynamic_text_record       ( const char* instance_name, const char* service_name, gedday_text_record_t* text_record );
extern wiced_result_t gedday_remove_service                ( const char* instance_name, const char* service_name );
extern const char*    gedday_get_hostname                  ( void );
extern wiced_result_t gedday_update_ip                     ( void );
extern void           gedday_deinit                        ( void );
wiced_result_t        gedday_text_record_create            ( gedday_text_record_t* text_record_ptr, uint16_t buffer_length, void *buffer );
wiced_result_t        gedday_text_record_delete            ( gedday_text_record_t* text_record_ptr );
wiced_result_t        gedday_text_record_set_key_value_pair( gedday_text_record_t* text_record_ptr, char* key, char* value );
char*                 gedday_text_record_get_string        ( gedday_text_record_t* text_record_ptr );
void                  gedday_print_debug_stats             ( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
