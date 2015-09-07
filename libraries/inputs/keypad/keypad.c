/**
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 **/
#include "wiced.h"
#include "wiced_time.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "wwd_constants.h"
#include "keypad_internal.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define DEFAULT_TIMER_INTERVAL_MS   (250)

#define DEBOUNCE_TIME_MS            (150)
#define SHORT_EVT_DURATION_MIN      (150)
#define SHORT_EVT_DURATION_MAX      (1000)
#define LONG_EVT_DURATION_MIN       (1000)
#define LONG_EVT_DURATION_MAX       (3000)
#define VERY_LONG_EVT_DURATION_MIN  (3000)
#define VERY_LONG_EVT_DURATION_MAX  (5000)
#define EXTENDED_EVT_DURATION_MIN   (5000)
#define EXTENDED_EVT_DURATION_MAX   (8000)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

static void           key_interrupt_handler     ( void* arg );
static void           check_state_timer_handler ( void* arg );
static wiced_result_t key_pressed_event_handler ( void* arg );
static wiced_result_t key_held_event_handler    ( void* arg );
static wiced_result_t key_released_event_handler( void* arg );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Keeping this wrapper for now so that Application code calls a function with lesser number of arguments */
wiced_result_t keypad_register( const platform_key_t* app_key_list, keypad_t* keypad, keypad_event_handler_t handler, uint8_t number_keys )
{
    wiced_assert( "Bad args", (app_key_list != NULL) && (keypad != NULL) && (number_keys != 0) );
    return keypad_enable( keypad, WICED_HARDWARE_IO_WORKER_THREAD, handler, DEFAULT_TIMER_INTERVAL_MS, number_keys, app_key_list );
}

wiced_result_t  keypad_enable( keypad_t* keypad, wiced_worker_thread_t* thread, keypad_event_handler_t function, uint32_t held_event_interval_ms, uint32_t total_keys, const platform_key_t* key_list )
{
    key_internal_t*    key_internal;
    uint32_t           malloc_size;
    uint32_t           i;

    wiced_assert("Bad args", (keypad != NULL) && (thread != NULL) && (function != NULL) &&
                    (total_keys != 0) && (key_list != NULL));

    malloc_size      = sizeof(keypad_internal_t) + total_keys * sizeof(key_internal_t);
    keypad->internal = (keypad_internal_t*) malloc_named("key internal", malloc_size);

    if( keypad->internal == NULL )
    {
        return WICED_ERROR;
    }

    memset( keypad->internal, 0, sizeof( *( keypad->internal ) ) );

    key_internal = (key_internal_t*)((uint8_t*)keypad->internal + sizeof(keypad_internal_t));

    keypad->internal->function   = function;
    keypad->internal->thread     = thread;
    keypad->internal->total_keys = total_keys;
    keypad->internal->key_list   = (platform_key_t*)key_list;

    wiced_rtos_init_timer( &keypad->internal->check_state_timer, held_event_interval_ms, check_state_timer_handler, (void*) keypad->internal );

    switch( keypad->type )
    {
        case KEYPAD_HW_GPIO:
            keypad->internal->init          = platform_gpio_key_init;
            keypad->internal->input_enable  = platform_gpio_key_enable;
            keypad->internal->input_disable = platform_gpio_key_disable;
            keypad->internal->get_input     = platform_gpio_key_get_input;
            break;

        case KEYPAD_HW_I2C:
            keypad->internal->init          = platform_i2c_key_init;
            keypad->internal->input_enable  = platform_i2c_key_enable;
            keypad->internal->input_disable = platform_i2c_key_disable;
            keypad->internal->get_input     = platform_i2c_key_get_input;
            break;

        default:
            break;
    }

    for( i = 0; i < total_keys; i++ )
    {
        key_internal[i].key   = (platform_key_t*)&key_list[i];
        key_internal[i].owner = keypad->internal;
        key_internal[i].last_irq_timestamp = 0;
    }

    if( keypad->internal->init )
    {
        keypad->internal->init( keypad );
    }

    if( keypad->internal->input_enable )
    {
        keypad->internal->input_enable( keypad, key_interrupt_handler, key_internal );
    }

    WPRINT_LIB_INFO( ("Keypad Initialized....\n" ) );
    return WICED_SUCCESS;
}

wiced_result_t keypad_disable( keypad_t *keypad )
{
    malloc_transfer_to_curr_thread(keypad->internal);

    wiced_rtos_deinit_timer( &keypad->internal->check_state_timer );

    if(keypad->internal->input_disable)
    {
        keypad->internal->input_disable( keypad );
    }

    free( keypad->internal );
    keypad->internal = 0;

    malloc_leak_check( NULL, LEAK_CHECK_THREAD );
    return WICED_SUCCESS;
}

static void key_interrupt_handler( void* arg )
{
    key_internal_t* key = (key_internal_t*)arg;
    keypad_internal_t* keypad = key->owner;
    uint32_t current_time;

    wiced_assert("Bad args", key != NULL);
    wiced_time_get_time( &current_time );

    if( ( current_time - key->last_irq_timestamp > DEBOUNCE_TIME_MS ) && ( keypad->current_key_pressed == NULL ) )
    {
        wiced_time_get_time( &key->last_irq_timestamp );
        wiced_rtos_send_asynchronous_event( keypad->thread, key_pressed_event_handler, (void*)key );
    }
}

static void check_state_timer_handler( void* arg )
{
    keypad_internal_t* keypad = (keypad_internal_t*)arg;
    key_internal_t* key = keypad->current_key_pressed;
    uint8_t key_value;
    uint32_t current_time;

    wiced_assert( "Bad args", (keypad != NULL) && (key != NULL) );

    wiced_time_get_time( &current_time );
    key->held_duration = current_time - key->last_irq_timestamp;

    if( !keypad->get_input)
    {
        return;
    }

    if( key->key->polarity == KEY_POLARITY_HIGH )
    {
        key_value = keypad->get_input( key->key );
        /* If key value is 0, it is being pressed/held, else released */
        wiced_rtos_send_asynchronous_event( keypad->thread, (key_value ? key_released_event_handler: key_held_event_handler), (void*)key );
    }

    else
    {
        key_value = keypad->get_input( key->key );
        /* if Key value is 0, it has been released, else held/pressed */
        wiced_rtos_send_asynchronous_event( keypad->thread, (key_value ? key_held_event_handler : key_released_event_handler), (void*)key );
    }

}

static wiced_result_t key_pressed_event_handler( void* arg )
{
    key_internal_t* key = (key_internal_t*)arg;
    keypad_internal_t* keypad = key->owner;

    wiced_assert("Bad args", (key != NULL) && (keypad != NULL) );
    if( keypad->current_key_pressed == NULL )
    {
        keypad->current_key_pressed = key;

        wiced_rtos_start_timer( &keypad->check_state_timer );

        WPRINT_LIB_DEBUG( ( "Key Pressed\n" ) );
        keypad->function( key->key, KEY_EVENT_PRESSED );

        return WICED_SUCCESS;
    }
    return WICED_ERROR;
}

static wiced_result_t key_held_event_handler( void* arg )
{
    key_internal_t* key = (key_internal_t*)arg;
    keypad_internal_t* keypad = key->owner;
    wiced_assert("Bad args", (key != NULL) && (keypad != NULL) );

    if( keypad->current_key_pressed == key )
    {
        WPRINT_LIB_DEBUG( ( "Key Pressed\n" ) );
        keypad->function( key->key, KEY_EVENT_HELD );
        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}

static wiced_result_t key_released_event_handler( void* arg )
{
    key_internal_t* key = (key_internal_t*)arg;
    keypad_internal_t* keypad = key->owner;
    key_event_t evt;
    uint32_t time_diff;
    wiced_assert ("Bad args", (key != NULL) && (keypad != NULL) );

    time_diff = key->held_duration;
    if( keypad->current_key_pressed == key )
    {
        keypad->current_key_pressed = NULL;

        wiced_rtos_stop_timer( &keypad->check_state_timer );

        if( time_diff < SHORT_EVT_DURATION_MAX && time_diff >= SHORT_EVT_DURATION_MIN )
        {
            evt = KEY_EVENT_SHORT_RELEASED;
        }

        else if( time_diff < LONG_EVT_DURATION_MAX && time_diff >= LONG_EVT_DURATION_MIN )
        {
            evt = KEY_EVENT_LONG_RELEASED;
        }

        else if( time_diff < VERY_LONG_EVT_DURATION_MAX && time_diff >= VERY_LONG_EVT_DURATION_MIN )
        {
            evt = KEY_EVENT_VERYLONG_RELEASED;
        }

        else if( time_diff < EXTENDED_EVT_DURATION_MAX && time_diff >= EXTENDED_EVT_DURATION_MIN )
        {
            evt = KEY_EVENT_EXTLONG_RELEASED;
        }
        else
        {
            evt = KEY_EVENT_RELEASED;
        }
        WPRINT_LIB_DEBUG( ( "Key Released, time_diff:%u\n", (unsigned int)time_diff ) );
        keypad->function( key->key, evt );

        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}
