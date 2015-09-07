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
#include <string.h>
#include "wiced_rtos.h"
#include "wiced_nfc_api.h"
#include "wiced_utilities.h"
//#include "wiced_nfc_utils.h"
#include "platform_resource.h"
#include "platform_nfc.h"
#include "resources.h"
#include "nfc_host.h"
#include "wwd_assert.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/


#define GKI_TIMER_INTERVAL (100)

#define WICED_NFC_SUPPORTED_TECHNOLOGIES  ( 0xFF ) /* all */
#define WICED_NFC_POLL_BIT_RATE           ( 1 )
#define WICED_NFC_NDEP_BIT_RATE           ( 1 )

#define WICED_NFC_NUM_MSG                 ( 1 )

/* Task States: (For OSRdyTbl) */
#define TASK_DEAD       0   /* b0000 */
#define TASK_READY      1   /* b0001 */
#define TASK_WAIT       2   /* b0010 */
#define TASK_DELAY      4   /* b0100 */
#define TASK_SUSPEND    8   /* b1000 */

#define NFC_INITIALIZATION_TIMEOUT     (8000)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/



typedef struct
{
    wiced_mutex_t GKI_mutex;
    wiced_thread_t thread_id[GKI_MAX_TASKS];
    wiced_mutex_t thread_evt_mutex[GKI_MAX_TASKS];
    wiced_queue_t thread_evt_queue[GKI_MAX_TASKS];
} tGKI_OS;


/******************************************************
 *               Function Declarations
 ******************************************************/

static void gki_update_timer_cback( );

/******************************************************
 *               Variables Definitions
 ******************************************************/

wiced_nfc_workspace_t* wiced_nfc_workspace_ptr;

tGKI_OS gki_cb_os;
wiced_timer_t update_tick_timer;

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Enables the NFC protocol layer */
wiced_result_t wiced_nfc_init ( wiced_nfc_workspace_t* workspace )
{
    wiced_nfc_queue_element_t message;

    wiced_assert("wiced_nfc_init - invalid param", workspace != NULL);

    /* Initialize NFC fwk workspace */
    memset( workspace, 0, sizeof( wiced_nfc_workspace_t ) );

    wiced_nfc_workspace_ptr = workspace;
    memset( &gki_cb_os,  0, sizeof( gki_cb_os ) );
    nfc_fwk_boot_entry( );

    wiced_rtos_init_queue( &wiced_nfc_workspace_ptr->queue, NULL, sizeof(wiced_nfc_queue_element_t), WICED_NFC_NUM_MSG );

    if ( nfc_internal_init( ) != 0 )
    {
        return WICED_ERROR;
    }

    /* Wait for the first event which should occur once NFC device has been initialized */
    if ( wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, NFC_INITIALIZATION_TIMEOUT ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

/* Disable NFC protocol layer */
wiced_result_t wiced_nfc_deinit( void )
{
    wiced_assert("wiced_nfc not initialized", wiced_nfc_workspace_ptr != NULL);

    nfc_internal_deinit( );

    wiced_rtos_deinit_queue( &wiced_nfc_workspace_ptr->queue );

    return WICED_SUCCESS;
}

/* NFC read tag */
wiced_result_t wiced_nfc_read_tag( uint8_t* read_buffer, uint32_t* length, uint32_t timeout )
{
    wiced_nfc_queue_element_t message;
    wiced_result_t            result = WICED_TIMEOUT;

    wiced_assert("wiced_nfc not initialized", wiced_nfc_workspace_ptr != NULL);
    if ( read_buffer == NULL || length == NULL )
    {
        return WICED_BADARG;
    }

    /* Flush the queue if it has any data due to the timeout and data receive simultaneously for previous read,-Do dummy read*/
    result = wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, WICED_NO_WAIT );

    /* Its cleared back in the NDEF data event */
    wiced_nfc_workspace_ptr->rw.read_buffer_ptr = read_buffer;

    /* Length should be always initialized second because only length check is done in the data callback*/
    wiced_nfc_workspace_ptr->rw.read_length_ptr = length;

    result = wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, timeout );

    if ( result != WICED_TIMEOUT )
    {
        /* If the given buffer length is less than the received data then we return the status of Bad argument here */
        result = message.status;
    }
    else
    {
        wiced_nfc_workspace_ptr->rw.read_length_ptr = NULL;
    }
    return result;
}


/* Write NFC tag */
wiced_result_t wiced_nfc_write_tag( wiced_nfc_tag_msg_t* write_data, uint32_t timeout )
{
    wiced_result_t            result;
    wiced_nfc_queue_element_t message;

    wiced_assert("wiced_nfc not initialized", wiced_nfc_workspace_ptr != NULL);
    if ( write_data == NULL )
    {
        return WICED_BADARG;
    }

    /* Flush the queue if it has any data due to the timeout and data receive simulatiously for previous read,-Do dummy read*/
    result = wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, WICED_NO_WAIT );

    result = nfc_fwk_ndef_build( write_data, &wiced_nfc_workspace_ptr->rw.write_buffer_ptr, &wiced_nfc_workspace_ptr->rw.write_length );

    if ( result == WICED_SUCCESS )
    {
        result = wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, timeout );
    }
    nfc_fwk_mem_co_free( wiced_nfc_workspace_ptr->rw.write_buffer_ptr );
    wiced_nfc_workspace_ptr->rw.write_buffer_ptr = NULL;

    return result;
}



void nfc_host_free_patch_ram_resource( const uint8_t* buffer )
{
    resource_free_readonly_buffer ( &resources_nfc_DIR_patch_DIR_43341B0_DIR_patch_ram_ncd_bin, buffer );
}

void nfc_host_free_i2c_pre_patch_resource( const uint8_t* buffer )
{
    resource_free_readonly_buffer ( &resources_nfc_DIR_patch_DIR_43341B0_DIR_i2c_pre_patch_ncd_bin, buffer );
}

void nfc_host_get_patch_ram_resource( const uint8_t** buffer, uint32_t* size )
{
    resource_get_readonly_buffer( &resources_nfc_DIR_patch_DIR_43341B0_DIR_patch_ram_ncd_bin, 0, 0, size, (const void**)buffer );
}

void nfc_host_get_i2c_pre_patch_resource( const uint8_t** buffer, uint32_t* size  )
{
    resource_get_readonly_buffer( &resources_nfc_DIR_patch_DIR_43341B0_DIR_i2c_pre_patch_ncd_bin, 0, 0, size, (const void**)buffer );
}

void nfc_host_wake_pin_low( void )
{
    platform_gpio_output_low( wiced_nfc_control_pins[WICED_NFC_PIN_WAKE] );
}

void nfc_host_wake_pin_high( void )
{
    platform_gpio_output_high( wiced_nfc_control_pins[WICED_NFC_PIN_WAKE] );
}


void GKI_exception( uint16_t code, char *msg )
{
    WPRINT_LIB_ERROR(( "GKI_exception(): Task State Table"));

    WPRINT_LIB_ERROR(("GKI_exception %d %s", code, msg));
    WPRINT_LIB_ERROR(( "********************************************************************"));
    WPRINT_LIB_ERROR(( "* GKI_exception(): %d %s", code, msg));
    WPRINT_LIB_ERROR(( "********************************************************************"));


#if (GKI_DEBUG == TRUE)
    nfc_internal_debug_exception( code, msg );
#endif

    WPRINT_LIB_ERROR(("GKI_exception %d %s done", code, msg));
}

uint8_t GKI_get_taskid( void )
{
    int i;
    wiced_result_t ret;

    for ( i = 0; i < GKI_MAX_TASKS; i++ )
    {
        ret = wiced_rtos_is_current_thread( &gki_cb_os.thread_id[i] );
        if ( ret == WICED_SUCCESS )
        {
            return ( i );
        }
    }
    return ( -1 );
}

void GKI_enable( void )
{
    wiced_rtos_unlock_mutex( &gki_cb_os.GKI_mutex );
    return;
}

void GKI_disable( void )
{
    wiced_rtos_lock_mutex( &gki_cb_os.GKI_mutex );
    return;
}



void nfc_delay_milliseconds( uint32_t delay_ms )
{
    wiced_rtos_delay_milliseconds( delay_ms );
}



uint8_t GKI_create_task( void (*task_entry)(uint32_t), uint8_t task_id, int8_t *taskname, uint16_t *stack, uint16_t stacksize, uint8_t priority )
{
    wiced_result_t ret = 0;

    if ( task_id >= GKI_MAX_TASKS )
    {
        return ( GKI_FAILURE );
    }

    nfc_init_task( task_id, taskname );

    ret = wiced_rtos_init_mutex( &gki_cb_os.thread_evt_mutex[task_id] );
    if ( ret != WICED_SUCCESS )
    {
        return GKI_FAILURE;
    }

    ret = wiced_rtos_init_queue( &gki_cb_os.thread_evt_queue[task_id], NULL, nfc_event_queue_message_size, nfc_event_queue_message_count );
    if ( ret != WICED_SUCCESS )
    {
        return GKI_FAILURE;
    }

    ret = wiced_rtos_create_thread( &gki_cb_os.thread_id[task_id], priority, (const char *) taskname, (void *) task_entry, stacksize, NULL );
    if ( ret != WICED_SUCCESS )
    {
        return GKI_FAILURE;
    }

    return ( GKI_SUCCESS );
}


void GKI_exit_task( uint8_t task_id )
{
    GKI_disable( );

    nfc_mark_thread_dead( task_id );

    /* Destroy mutex and condition variable objects */
    wiced_rtos_deinit_mutex( &gki_cb_os.thread_evt_mutex[task_id] );
    wiced_rtos_deinit_queue( &gki_cb_os.thread_evt_queue[task_id] );
    wiced_rtos_delete_thread( &gki_cb_os.thread_id[task_id] );

    GKI_enable( );
    return;
}

uint8_t GKI_send_event( uint8_t task_id, uint16_t event )
{
    wiced_result_t ret;

    /* use efficient coding to avoid pipeline stalls */
    if ( task_id < GKI_MAX_TASKS )
    {
        /* protect OSWaitEvt[task_id] from manipulation in GKI_wait() */
        wiced_rtos_lock_mutex( &gki_cb_os.thread_evt_mutex[task_id] );

        /* Set the event bit */
        set_common_wait_event_flag( task_id, event | get_common_wait_event_flag( task_id ) );

        ret = wiced_rtos_push_to_queue( &gki_cb_os.thread_evt_queue[task_id], (void*) &event, WICED_NO_WAIT );
        WPRINT_LIB_DEBUG(( "GKI_send_event wiced_rtos_push_to_queue task_id=0x%x ret=0x%x queueData=0x%x", task_id, ret, event ));
        REFERENCE_DEBUG_ONLY_VARIABLE( ret );

        wiced_rtos_unlock_mutex( &gki_cb_os.thread_evt_mutex[task_id] );

        return ( GKI_SUCCESS );
    }
    return ( GKI_FAILURE );
}


void GKI_run( void *p_task_id )
{
    wiced_rtos_init_timer( &update_tick_timer, GKI_TIMER_INTERVAL, gki_update_timer_cback, NULL );
    wiced_rtos_start_timer( &update_tick_timer );
}


void gki_update_timer_cback( void )
{
    wiced_rtos_start_timer( &update_tick_timer ); // Start time timer over again
    GKI_timer_update( GKI_TIMER_INTERVAL );
}

uint16_t GKI_wait( uint16_t flag, uint32_t timeout )
{
    uint8_t rtask;
    uint8_t check;
    uint16_t evt;
    uint32_t queueData = 0;
    wiced_result_t ret;

    rtask = GKI_get_taskid( );
    if ( rtask >= GKI_MAX_TASKS )
    {
        return 0;
    }

    set_common_wait_for_event_flag( rtask, flag );

    wiced_rtos_lock_mutex( &gki_cb_os.thread_evt_mutex[rtask] );
    check = !( get_common_wait_event_flag( rtask ) & flag );
    wiced_rtos_unlock_mutex( &gki_cb_os.thread_evt_mutex[rtask] );

    if ( check )
    {
        timeout = ( timeout ? timeout : WICED_WAIT_FOREVER );
        if ( ( ret = wiced_rtos_pop_from_queue( &gki_cb_os.thread_evt_queue[rtask], (void*) &queueData, timeout ) ) != WICED_SUCCESS )
        {
            WPRINT_LIB_DEBUG(( "GKI_wait wiced_rtos_pop_from_queue failed ret=0x%x", ret));
        }

        wiced_rtos_lock_mutex( &gki_cb_os.thread_evt_mutex[rtask] );
        if ( nfc_is_thread_dead( rtask )== 1 )
        {
            set_common_wait_event_flag( rtask, 0 );
            WPRINT_LIB_DEBUG(( "GKI TASK_DEAD received. exit thread %d...", rtask ));
            GKI_exit_task( rtask );
            return nfc_shutdown_event;
        }
    }
    else
    {
        ret = wiced_rtos_pop_from_queue( &gki_cb_os.thread_evt_queue[rtask], (void*) &queueData, 0 );
        if ( ret != WICED_SUCCESS )
        {
            WPRINT_LIB_DEBUG(( "GKI_wait wiced_rtos_pop_from_queue failed ret=0x%x", ret));
        }
    }

    /* Clear the wait for event mask */
    set_common_wait_for_event_flag( rtask, 0 );

    /* Return only those bits which user wants... */

    evt = get_common_wait_event_flag( rtask ) & flag;

    /* Clear only those bits which user wants... */
    set_common_wait_event_flag( rtask, ~flag & get_common_wait_event_flag( rtask ) );

    WPRINT_LIB_DEBUG((
            "GKI_wait taskid=0x%x check=0x%x OSWaitEvt=0x%x flag=0x%x queueData=0x%x evt=0x%x",
            rtask,
            check,
            gki_cb_com.OSWaitEvt[rtask],
            flag,
            queueData & 0x0000FFFF,
            evt));

    /* unlock thread_evt_mutex as wiced_cond_wait() does auto lock mutex when cond is met */
    wiced_rtos_unlock_mutex( &gki_cb_os.thread_evt_mutex[rtask] );

    return ( evt );
}

void *GKI_os_malloc( uint32_t size )
{
    return ( malloc( size ) );
}



void nfc_push_to_queue( void* message )
{
    wiced_rtos_push_to_queue( &wiced_nfc_workspace_ptr->queue, message, WICED_NO_WAIT );
}

