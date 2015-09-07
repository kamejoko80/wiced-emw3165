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
    KEY_POLARITY_LOW,   /* logic value 0 when key/button is released */
    KEY_POLARITY_HIGH,  /* logic value 1 when key/button is released */

} input_key_polarity_t;

typedef enum
{
    KEY_EVENT_PRESSED           = 0x0,                  /* Key has been pressed  */
    KEY_EVENT_RELEASED          = 0x1,                  /* Key has been released */
    KEY_EVENT_HELD              = 0x2,                  /* Key is being held */
    KEY_EVENT_SHORT_RELEASED    = KEY_EVENT_RELEASED,   /* Same as normal release */
    KEY_EVENT_LONG_RELEASED     = 0x3,                  /* Key has been released after long press */
    KEY_EVENT_VERYLONG_RELEASED = 0x4,                  /* Key has been released after very long press */
    KEY_EVENT_EXTLONG_RELEASED  = 0x5,                  /* Key has been released after extended long press */
    KEY_EVENT_DOUBLE_CLICK      = 0x6,                  /* Key has been double clicked */
} key_event_t;

typedef enum
{
    KEYPAD_HW_GPIO,
    KEYPAD_HW_I2C,
} keypad_hw_t;

typedef enum
{
    KEY_ID_0,
    KEY_ID_1,
    KEY_ID_2,
    KEY_ID_3,
    KEY_ID_4,
    KEY_ID_5,
    KEY_ID_MAX,
} key_id_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
typedef union
{
    wiced_gpio_t    key_gpio_num;
    uint32_t        key_i2c_addr;
    uint32_t        value;
} key_hw_config_t;

typedef struct
{
    input_key_polarity_t    polarity;
    key_hw_config_t         config_data;
    key_id_t                id;
} platform_key_t;

typedef struct
{
   keypad_hw_t              type;
   struct keypad_internal*  internal;
} keypad_t;

struct key_action_map
{
    key_event_t event;
    uint32_t    action;
};

typedef struct
{
    struct key_action_map*  actions;
    int                     number_entries;
} key_event_action_map_t;

typedef wiced_result_t (*keypad_event_handler_t)(platform_key_t* key, key_event_t event);
/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
wiced_result_t keypad_disable( keypad_t* keypad );
wiced_result_t keypad_enable ( keypad_t* keypad, wiced_worker_thread_t* thread, keypad_event_handler_t function, uint32_t held_key_interval_ms, uint32_t total_keys, const platform_key_t* key_list );
wiced_result_t  keypad_register( const platform_key_t* app_key_list, keypad_t* keypad, keypad_event_handler_t handler, uint8_t number_keys );
#ifdef __cplusplus
} /* extern "C" */
#endif
