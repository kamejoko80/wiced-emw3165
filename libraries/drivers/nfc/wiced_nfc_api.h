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

#include <stdint.h>
#include "wiced_result.h"
#include "wiced_rtos.h"
#include "nfc_queue.h"

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

/* NFC tag message types */
typedef enum
{
    WICED_NFC_TAG_TYPE_TEXT,
    WICED_NFC_TAG_TYPE_WSC
} wiced_nfc_tag_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    wiced_nfc_tag_type_t type;
    uint8_t*             buffer;
    uint32_t             buffer_length;
} wiced_nfc_tag_msg_t;

/* wiced_nfc structure for send NDEF messages */
typedef struct
{
    nfc_rw_pointer_struct_t rw;
    wiced_queue_t queue;
} wiced_nfc_workspace_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Initialize the NFC protocol layer
 *  This api enables NFC stack and controller.
 *  This takes few seconds to return , beacuse of firmware download.
 *
 * @param     void
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_nfc_init( wiced_nfc_workspace_t* workspace );


/** Deinitialize  NFC protocol layer
 *
 * @param     void
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_nfc_deinit( void );

/** Read NFC tag function
 *  If read data is already available it returns the data or wait tills the timeout
 *  This function should not be called twice at the same time by different tasks
 *
 * @param[out]    read_buffer - Pointer to buffer to store received data
 * @param[in/out] length      - [in]  = The length of the read_buffer.
 *                              [out] = The amount of data received
 * @param[in]     timeout     - wait duration in milliseconds.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_nfc_read_tag( uint8_t* read_buffer, uint32_t* length, uint32_t timeout );

/** Write NFC tag function
 *  Writes the data given by the api to the tag when it comes to the contact within the given timeout.
 *  This function should not be called twice at the same time by different tasks
 *
 * @param[in] write_data - Data to write to NFC tag
 * @param[in] timeout    - wait duration in milliseconds
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_nfc_write_tag( wiced_nfc_tag_msg_t* write_data, uint32_t timeout );
