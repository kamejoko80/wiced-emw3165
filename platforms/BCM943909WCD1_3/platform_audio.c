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
#include "platform.h"
#include "platform_init.h"
#include "platform_peripheral.h"
#include "platform_config.h"
#include "wm8533_dac.h"
#include "ak4954.h"
#include "platform_external_memory.h"

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

wiced_i2c_device_t wm8533_control_port =
{
    .port          = WICED_I2C_1,
    .address       = 0x1B,
    .address_width = I2C_ADDRESS_WIDTH_7BIT,
    .speed_mode    = I2C_STANDARD_SPEED_MODE, //I2C_LOW_SPEED_MODE,
};

wm8533_device_data_t wm8533 =
{
    /* assign a proper GPIO to a reset pin */
    .addr0          = WICED_GPIO_MAX,
    .cifmode        = WICED_GPIO_MAX,
    .i2c_data       = &wm8533_control_port,
    .data_port      = WICED_I2S_3,
    .fmt            = WM8533_FMT_CCS_CFS,
};

wiced_i2c_device_t ak4954_control_port =
{
    .port          = WICED_I2C_1,
    .address       = 0x12,
    .address_width = I2C_ADDRESS_WIDTH_7BIT,
    .speed_mode    = I2C_STANDARD_SPEED_MODE, //I2C_LOW_SPEED_MODE,
};

ak4954_device_cmn_data_t ak4954 =
{
    .id                 = AK4954_DEVICE_ID_0,
    .i2c_data           = &ak4954_control_port,
    .ck                 = ak4954_pll_slave,
    .pdn                = WICED_GPIO_8,
};

ak4954_device_data_t ak4954_dac =
{
    .route              = &ak4954_dac_hp,
    .cmn                = &ak4954,
    .data_port          = WICED_I2S_1,
};

ak4954_device_data_t ak4954_adc =
{
    .route              = &ak4954_adc_mic,
    .cmn                = &ak4954,
    .data_port          = WICED_I2S_2,
};

/******************************************************
 *               Function Declarations
 ******************************************************/

extern wiced_result_t wm8533_device_register( wm8533_device_data_t* device_data, const char* name );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t platform_init_audio( void )
{
    /* Register audio device */
    wm8533_device_register( &wm8533, "wm8533_dac" );
    ak4954_device_register( &ak4954_dac, "ak4954_dac" );
    ak4954_device_register( &ak4954_adc, "ak4954_adc" );

    return WICED_SUCCESS;
}

wiced_result_t platform_deinit_audio( void )
{
    return WICED_UNSUPPORTED;
}

wiced_result_t wm8533_platform_configure( wm8533_device_data_t* device_data, uint32_t mclk, uint32_t fs, uint8_t width )
{
    UNUSED_PARAMETER( device_data );
    UNUSED_PARAMETER( mclk );
    UNUSED_PARAMETER( device_data );
    UNUSED_PARAMETER( width );


    return WICED_SUCCESS;
}
