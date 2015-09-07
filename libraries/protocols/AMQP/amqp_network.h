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
 * Network APIs.
 *
 * Internal, not to be used directly by applications.
 */
#pragma once

#include "wiced.h"

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
typedef struct
{
        const char         *ca_cert;        /* CA certificate, common between client and server */
        const char         *cert;           /* Client certificate in pem formate                */
        const char         *key;            /* Client private key                               */
}wiced_amqp_socket_security_t;

typedef struct
{
        wiced_bool_t        quit;
        wiced_tcp_socket_t  socket;
        wiced_queue_t       queue;
        wiced_semaphore_t   net_semaphore;
        wiced_thread_t      net_thread;
        wiced_thread_t      queue_thread;
        wiced_ip_address_t  server_ip_address;
        uint16_t            portnumber;
        void               *p_user;
        wiced_tls_advanced_context_t tls_context;
}wiced_amqp_socket_t;

typedef struct wiced_amqp_buffer_s
{
    wiced_packet_t         *packet;
    uint8_t                *data;
}wiced_amqp_buffer_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/*
 * Network functions.
 *
 * Internal functions not to be used by user applications.
 */
wiced_result_t amqp_network_init            ( const wiced_ip_address_t *server_ip_address, uint16_t portnumber, wiced_interface_t interface, void *p_user, wiced_amqp_socket_t *socket, const wiced_amqp_socket_security_t *security );
wiced_result_t amqp_network_deinit          ( wiced_amqp_socket_t *socket );
wiced_result_t amqp_network_connect         ( wiced_amqp_socket_t *socket );
wiced_result_t amqp_network_disconnect      ( wiced_amqp_socket_t *socket );

wiced_result_t amqp_network_create_buffer   ( wiced_amqp_buffer_t *buffer, uint16_t size, wiced_amqp_socket_t *socket );
wiced_result_t amqp_network_send_buffer     ( const wiced_amqp_buffer_t *buffer, wiced_amqp_socket_t *socket );
wiced_result_t amqp_network_delete_buffer   ( wiced_amqp_buffer_t *buffer );

#ifdef __cplusplus
} /* extern "C" */
#endif

