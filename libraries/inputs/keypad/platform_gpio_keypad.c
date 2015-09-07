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
#include "keypad_internal.h"
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

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Initializes platform data structures for all the keys in the keypad */
wiced_result_t  platform_gpio_key_init( keypad_t *keypad )
{
    keypad_internal_t   *keypad_internal;
    platform_key_t      *key_list;
    uint8_t i;
    wiced_assert("Bad args", keypad != NULL);

    keypad_internal = keypad->internal;
    wiced_assert("Bad args", keypad->internal != NULL);
    key_list = keypad_internal->key_list;
    wiced_assert("Bad args", keypad->internal->key_list != NULL);

    for ( i = 0; i < keypad_internal->total_keys; i++ )
    {
        wiced_gpio_init( key_list[i].config_data.key_gpio_num,\
                    ( ( key_list[i].polarity == KEY_POLARITY_HIGH ) ? INPUT_PULL_UP : INPUT_PULL_DOWN ) );
    }
    return WICED_SUCCESS;
}

/* Enable the Keys on the keypad, i.e. after this , events will start arriving.
 * Register the handler as well */
wiced_result_t  platform_gpio_key_enable ( keypad_t *keypad, key_handler_t handler, key_internal_t *key_internal )
{
    keypad_internal_t   *keypad_internal;
    platform_key_t      *key_list;
    wiced_gpio_irq_trigger_t trigger;
    uint8_t i;
    wiced_assert("Bad args", keypad != NULL);

    keypad_internal = keypad->internal;
    wiced_assert("Bad args", keypad->internal != NULL);
    key_list = keypad_internal->key_list;
    wiced_assert("Bad args", keypad->internal->key_list != NULL);

    for ( i = 0; i < keypad_internal->total_keys; i++ )
    {
        trigger = ( key_list[i].polarity == KEY_POLARITY_HIGH )\
                        ? IRQ_TRIGGER_FALLING_EDGE : IRQ_TRIGGER_RISING_EDGE;

        wiced_gpio_input_irq_enable( key_list[i].config_data.key_gpio_num, trigger,
                            handler, (void*)&key_internal[i] );
    }
    return WICED_SUCCESS;
}
/* Disable the keys on the keypad */
wiced_result_t platform_gpio_key_disable ( keypad_t * keypad )
{
    keypad_internal_t   *keypad_internal;
    platform_key_t      *key_list;
    uint8_t i;
    wiced_assert("Bad args", keypad != NULL);

    keypad_internal = keypad->internal;
    wiced_assert("Bad args", keypad->internal != NULL);
    key_list = keypad_internal->key_list;
    wiced_assert("Bad args", key_list != NULL);

    for ( i = 0; i < keypad->internal->total_keys; i++ )
    {
        wiced_gpio_input_irq_disable( key_list[i].config_data.key_gpio_num );
    }
    return WICED_SUCCESS;
}

/* Get current value of key */
uint8_t platform_gpio_key_get_input( platform_key_t *key)
{
    wiced_assert("Bad args", key != NULL);

    return (uint8_t) wiced_gpio_input_get( key->config_data.key_gpio_num );
}
