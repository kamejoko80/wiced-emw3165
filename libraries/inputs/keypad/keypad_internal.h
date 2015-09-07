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

#include "keypad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*key_handler_t)(void* args );

typedef struct
{
    platform_key_t*         key;
    uint32_t                last_irq_timestamp;
    uint32_t                held_duration;
    struct keypad_internal* owner;
} key_internal_t;

typedef struct keypad_internal
{
    uint32_t                total_keys;
    platform_key_t*         key_list;
    keypad_event_handler_t  function;
    wiced_worker_thread_t*  thread;
    key_internal_t*         current_key_pressed;
    wiced_timer_t           check_state_timer;
    wiced_result_t          (*init) ( keypad_t* keypad );
    wiced_result_t          (*input_enable) ( keypad_t* keypad , key_handler_t handler, key_internal_t* key_internal);
    wiced_result_t          (*input_disable) ( keypad_t* keypad );
    uint8_t                 (*get_input) ( platform_key_t* key);
} keypad_internal_t;

wiced_result_t  platform_gpio_key_init( keypad_t* keypad );
wiced_result_t  platform_gpio_key_enable ( keypad_t* keypad, key_handler_t handler, key_internal_t* key_internal );
wiced_result_t platform_gpio_key_disable ( keypad_t* keypad );
uint8_t platform_gpio_key_get_input( platform_key_t* key);
wiced_result_t  platform_i2c_key_init( keypad_t* keypad );
wiced_result_t  platform_i2c_key_enable ( keypad_t* keypad, key_handler_t handler, key_internal_t* key_internal );
wiced_result_t platform_i2c_key_disable ( keypad_t* keypad );
uint8_t platform_i2c_key_get_input( platform_key_t* key);

#ifdef __cplusplus
} /* extern "C" */
#endif
