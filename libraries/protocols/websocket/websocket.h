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

/*
 * NOTE: Only client websockets are implemented currently
 */

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define SUB_PROTOCOL_STRING_LENGTH 10

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WEBSOCKET_CONNECTING = 0, /* The connection has not yet been established */
    WEBSOCKET_OPEN,           /* The WebSocket connection is established and communication is possible */
    WEBSOCKET_CLOSING,        /* The connection is going through the closing handshake */
    WEBSOCKET_CLOSED,         /* The connection has been closed or could not be opened */
    WEBSOCKET_NOT_REGISTERED
} wiced_websocket_state_t;

typedef enum
{
    WEBSOCKET_CONTINUATION_FRAME = 0,
    WEBSOCKET_TEXT_FRAME,
    WEBSOCKET_BINARY_FRAME,
    WEBSOCKET_RESERVED_3,
    WEBSOCKET_RESERVED_4,
    WEBSOCKET_RESERVED_5,
    WEBSOCKET_RESERVED_6,
    WEBSOCKET_RESERVED_7,
    WEBSOCKET_CONNECTION_CLOSE,
    WEBSOCKET_PING,
    WEBSOCKET_PONG,
    WEBSOCKET_RESERVED_B,
    WEBSOCKET_RESERVED_C,
    WEBSOCKET_RESERVED_D,
    WEBSOCKET_RESERVED_E,
    WEBSOCKET_RESERVED_F
} wiced_websocket_payload_type_t;

typedef enum
{
    WEBSOCKET_NO_ERROR = 0,
    WEBSOCKET_CLIENT_CONNECT_ERROR,
    WEBSOCKET_NO_AVAILABLE_SOCKET,
    WEBSOCKET_SERVER_HANDSHAKE_RESPONSE_INVALID,
    WEBSOCKET_CREATE_SOCKET_ERROR,
    WEBSOCKET_FRAME_SEND_ERROR,
    WEBSOCKET_HANDSHAKE_SEND_ERROR,
    WEBSOCKET_PONG_SEND_ERROR,
    WEBSOCKET_RECEIVE_ERROR,
    WEBSOCKET_DNS_RESOLVE_ERROR,
    WEBSOCKET_SUBPROTOCOL_NOT_SUPPORTED
} wiced_websocket_error_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef wiced_result_t (*wiced_websocket_callback_t)( void* websocket );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_websocket_callback_t on_open;
    wiced_websocket_callback_t on_error;
    wiced_websocket_callback_t on_close;
    wiced_websocket_callback_t on_message;
} wiced_websocket_callbacks_t;

typedef struct
{
    wiced_bool_t                    final_frame;
    wiced_websocket_payload_type_t  payload_type;
    uint16_t                        payload_length;
    void*                           payload;
    uint16_t                        payload_buffer_size;
} wiced_websocket_frame_t;

typedef struct
{
    wiced_tcp_socket_t           socket;
    wiced_websocket_error_t      error_type;
    wiced_websocket_state_t      state;
    wiced_websocket_callbacks_t  callbacks;
    wiced_websocket_frame_t      rx_frame;
    wiced_websocket_frame_t      tx_frame;
    const char                   subprotocol[SUB_PROTOCOL_STRING_LENGTH];
} wiced_websocket_t;

typedef struct
{
    char* request_uri;
    char* host;
    char* origin;
    char* sec_websocket_protocol;
} wiced_websocket_handshake_fields_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* @brief                          perform opening handshake on port 80 with server and establish a connection
 *
 * @param websocket                websocket identifier
 * @param websocket_header         http header information to be used in handshake
 *
 * @return                         WICED_SUCCESS if successful, or WICED_ERROR.
 *
 * @note                           For additional error information, check the wiced_websocket_error_t field
 *                                 of the  wiced_websocket_t structure
 */
wiced_result_t wiced_websocket_connect( wiced_websocket_t* websocket, const wiced_websocket_handshake_fields_t* websocket_header );

/* @brief                          perform opening handshake on port 443 with server and establish a connection
 *
 * @param websocket                websocket identifier
 * @param websocket_header         http header information to be used in handshake
 *
 * @return                         WICED_SUCCESS if successful, or WICED_ERROR.
 *
 * @note                           For additional error information, check the wiced_websocket_error_t field
 *                                 of the  wiced_websocket_t structure
 */
wiced_result_t wiced_websocket_secure_connect( wiced_websocket_t* websocket, const wiced_websocket_handshake_fields_t* websocket_header );

/* @brief                           send data to websocket server
 *
 * @param websocket                 websocket to send on
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_send ( wiced_websocket_t* websocket );

/* @brief                           receive data from websocket server. This is a blocking call.
 *
 * @param websocket                 websocket to receive on
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_receive ( wiced_websocket_t* websocket );

/* @brief                           close and delete websocket, and send close message to websocket server
 *
 * @param websocket                 websocket to close
 * @param optional_close_message    optional closing message to send server.
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_close ( wiced_websocket_t* websocket, const char* optional_close_message) ;

/* @brief                           Register the on_open, on_close, on_message and on_error callbacks
 *
 * @param websocket                 websocket on which to register the callbacks
 * @param on_open_callback          called on open websocket  connection
 * @param on_close_callback         called on close websocket connection
 * @param on_message_callback       called on websocket receive data
 * @param on_error_callback         called on websocket error
 *
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_register_callbacks ( wiced_websocket_t* websocket, wiced_websocket_callback_t on_open_callback, wiced_websocket_callback_t on_close_callback, wiced_websocket_callback_t on_message_callback, wiced_websocket_callback_t on_error ) ;

/* @brief                           Un-Register the on_open, on_close, on_message and on_error callbacks for a given websocket
 *
 * @param websocket                 websocket on which to unregister the callbacks
 *
 */
void wiced_websocket_unregister_callbacks ( wiced_websocket_t* websocket );

/* @brief                           Initialise the websocket transmit frame
 *
 * @param websocket                 Websocket who's tx frame we are initialising
 * @param wiced_bool_t final_frame  Indicates if this is the final frame of the message
 * @param payload_type              Type of payload (ping frame, binary frame, text frame etc).
 * @param payload_length            Length of payload being sent
 * @param payload_buffer            Pointer to buffer containing the payload
 * @param payload_buffer_size       Length of buffer containing payload
 *
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_initialise_tx_frame( wiced_websocket_t* websocket, wiced_bool_t final_frame, wiced_websocket_payload_type_t payload_type, uint16_t payload_length, void* payload_buffer, uint16_t payload_buffer_size );


/* @brief                           Initialise the websocket receive frame
 *
 * @param websocket                 Websocket who's rx frame we are initialising
 * @param payload_buffer            Pointer to buffer containing the payload
 * @param payload_buffer_size       Length of buffer containing payload
 *
 * @return                          WICED_SUCCESS if successful, or WICED_ERROR.
 */
wiced_result_t wiced_websocket_initialise_rx_frame( wiced_websocket_t* websocket, void* payload_buffer, uint16_t payload_buffer_size );
#ifdef __cplusplus
} /* extern "C" */
#endif






