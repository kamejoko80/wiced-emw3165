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

#include "wiced_result.h"
#include "wiced_nfc_api.h"
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
#define GKI_SUCCESS         0x00
#define GKI_FAILURE         0x01

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef uint8_t tUSERIAL_PORT;
typedef uint8_t tUSERIAL_EVT;

/******************************************************
 *                    Structures
 ******************************************************/

typedef union
{
    uint8_t sigs;
    uint8_t error;
} tUSERIAL_EVT_DATA;

typedef void (tUSERIAL_CBACK)(tUSERIAL_PORT, tUSERIAL_EVT, tUSERIAL_EVT_DATA *);

typedef struct
{
    void    *p_first;
    void    *p_last;
    uint16_t   count;
} BUFFER_Q;

typedef struct
{
    uint16_t          event;
    uint16_t          len;
    uint16_t          offset;
    uint16_t          layer_specific;
} BT_HDR;

typedef struct
{
    uint16_t fmt;          /* Data format                       */
    uint8_t  baud;         /* Baud rate                         */
    uint8_t  fc;           /* Flow control                      */
    uint8_t  buf;          /* Data buffering mechanism          */
    uint8_t  pool;         /* GKI buffer pool for received data */
    uint16_t size;         /* Size of GKI buffer pool           */
    uint16_t offset;       /* Offset in GKI buffer pool         */
} tUSERIAL_OPEN_CFG;

/******************************************************
 *                 Global Variables
 ******************************************************/

extern const uint16_t nfc_shutdown_event;
extern const uint16_t nfc_event_queue_message_size;
extern const uint16_t nfc_event_queue_message_count;
extern const uint8_t nfc_task_value;

/******************************************************
 *          NFC to Host Function Declarations
 ******************************************************/

wiced_result_t nfc_bus_init( void );
wiced_result_t nfc_bus_deinit( void );
wiced_result_t nfc_bus_transmit( const uint8_t* data_out, uint32_t size );
wiced_result_t nfc_bus_receive( uint8_t* data_in, uint32_t size, uint32_t timeout_ms );

void nfc_host_free_patch_ram_resource( const uint8_t* buffer );
void nfc_host_free_i2c_pre_patch_resource( const uint8_t* buffer );
void nfc_host_get_patch_ram_resource( const uint8_t** buffer, uint32_t* size );
void nfc_host_get_i2c_pre_patch_resource( const uint8_t** buffer, uint32_t* size  );


uint16_t get_common_wait_for_event_flag( uint8_t rtask );
void set_common_wait_for_event_flag( uint8_t rtask, uint16_t flag );
uint16_t get_common_wait_event_flag( uint8_t task_id );
void set_common_wait_event_flag( uint8_t task_id, uint16_t flag );
void nfc_internal_debug_exception( uint16_t code, char *msg );
int nfc_internal_init( void );
int nfc_internal_deinit( void );
uint8_t GKI_send_event( uint8_t task_id, uint16_t event );
void nfc_signal_rx_irq( void );
int nfc_fwk_boot_entry( void );
wiced_result_t nfc_fwk_ndef_build( wiced_nfc_tag_msg_t* p_ndef_msg, uint8_t** pp_ndef_buf, uint32_t* p_ndef_size );
void nfc_fwk_mem_co_free( void *p_buf );
void nfc_init_task( uint8_t task_id, int8_t *taskname );
uint8_t nfc_is_thread_dead( uint8_t task_id );
void nfc_mark_thread_dead( uint8_t task_id );
void GKI_timer_update( uint32_t ticks_since_last_update );
void nfc_delay_milliseconds( uint32_t delay_ms );
void nfc_push_to_queue( void* message );
void USERIAL_Init( void *p_cfg );
void GKI_enqueue( BUFFER_Q *p_q, void *p_buf );
void GKI_init_q( BUFFER_Q *p_q );
void *GKI_getbuf( uint16_t size );
void GKI_freebuf( void *p_buf );
uint8_t GKI_send_event( uint8_t task_id, uint16_t event );
void *GKI_dequeue( BUFFER_Q *p_q );
uint32_t nfc_task (uint32_t param);
void UPIO_Init( void *p_cfg );
void HAL_NfcInitialize( void );
void GKI_run( void *p_task_id );
void GKI_enable( void );
void GKI_disable( void );
void GKI_delay( uint32_t timeout );
void GKI_init( void );
uint8_t GKI_create_task( void (*task_entry)(uint32_t), uint8_t task_id, int8_t *taskname, uint16_t *stack, uint16_t stacksize, uint8_t priority );

/******************************************************
 *           Host to NFC Function Declarations
 ******************************************************/



#ifdef __cplusplus
} /* extern "C" */
#endif
