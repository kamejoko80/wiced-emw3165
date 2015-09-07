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

#define TFTP_ERROR_NOT_DEFINED          (0)
#define TFTP_ERROR_FILE_NOT_FOUND       (1)
#define TFTP_ERROR_ACCESS_VIOLATION     (2)
#define TFTP_ERROR_DISK_FULL            (3)
#define TFTP_ERROR_ILLEGAL_OPERATION    (4)
#define TFTP_ERROR_UNKNOWN_TID          (5)
#define TFTP_ERROR_FILE_EXISTS          (6)
#define TFTP_ERROR_NO_SUCH_USER         (7)
#define TFTP_NO_ERROR                   (255)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct tftp_client_s tftp_client_t;

typedef enum
{
    TFTP_MODE_NETASCII = 0,
    TFTP_MODE_OCTET = 1,
    TFTP_MODE_MAIL = 2
}tftp_mode_t;

typedef enum
{
    TFTP_GET,
    TFTP_PUT
}tftp_request_t;

typedef struct tftp_s
{
        uint16_t                timeout;
        uint16_t                block_size;
        uint32_t                transfer_size;
        const char*             filename;
        tftp_mode_t             mode;
        tftp_request_t          request;
}tftp_t;

typedef struct tftp_callback_s
{
        wiced_result_t (*tftp_establish) ( tftp_t* tftp, void* p_user );
        wiced_result_t (*tftp_read)      ( tftp_t* tftp, uint8_t* data, void* p_user );
        wiced_result_t (*tftp_write)     ( tftp_t* tftp, uint8_t* data, void* p_user );
        wiced_result_t (*tftp_close)     ( tftp_t* tftp, int status, void* p_user );
}tftp_callback_t;

typedef struct tftp_connection_s
{
        wiced_thread_t          thread;
        wiced_udp_socket_t      socket;
        wiced_interface_t       interface;
        wiced_ip_address_t      ip;
        uint16_t                tid;
        uint16_t                blk_num;
        uint16_t                retry;
        tftp_t                  tftp;
        void*                   p_user;
        tftp_callback_t         callback;
}tftp_connection_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* tftp client management functions */
wiced_result_t tftp_server_start( tftp_connection_t* server, wiced_interface_t interface, tftp_callback_t* callback, void* p_user );

wiced_result_t tftp_server_stop ( tftp_connection_t* server );

wiced_result_t tftp_client_get  ( tftp_connection_t* client, wiced_ip_address_t host, wiced_interface_t interface, const char * filename, tftp_mode_t mode, tftp_callback_t *callback, void *p_user );

wiced_result_t tftp_client_put  ( tftp_connection_t* client, wiced_ip_address_t host, wiced_interface_t interface, const char * filename, tftp_mode_t mode, tftp_callback_t *callback, void *p_user );


#ifdef __cplusplus
} /* extern "C" */
#endif
