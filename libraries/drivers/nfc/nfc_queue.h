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

/** WICED NFC queue data structure
 * The structure should be always 4 byte aligned.
 */
typedef struct
{
    uint32_t status;
} wiced_nfc_queue_element_t;

typedef struct
{
    uint8_t*      read_buffer_ptr;
    uint32_t*     read_length_ptr;
    uint8_t*      write_buffer_ptr;
    uint32_t      write_length;
} nfc_rw_pointer_struct_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *          NFC to Host Function Declarations
 ******************************************************/

/******************************************************
 *           Host to NFC Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
