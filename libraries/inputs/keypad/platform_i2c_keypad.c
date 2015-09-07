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

wiced_result_t  platform_i2c_key_init( keypad_t *keypad )
{
    UNUSED_PARAMETER(keypad);
    return WICED_SUCCESS;
}

wiced_result_t  platform_i2c_key_enable ( keypad_t *keypad, key_handler_t handler, key_internal_t *key_internal )
{
    UNUSED_PARAMETER(keypad);
    UNUSED_PARAMETER(handler);
    UNUSED_PARAMETER(key_internal);
    return WICED_SUCCESS;
}

wiced_result_t platform_i2c_key_disable ( keypad_t * keypad )
{

    UNUSED_PARAMETER(keypad);
    return WICED_SUCCESS;
}

uint8_t platform_i2c_key_get_input( platform_key_t *key)
{
    UNUSED_PARAMETER(key);
    return WICED_SUCCESS;
}
