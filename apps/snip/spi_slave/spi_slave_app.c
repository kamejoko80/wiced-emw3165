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
 * SPI Slave Application
 *
 * This application demonstrates how to build a SPI slave device using WICED API. Refer to apps/snip/spi_slave/master
 * for the corresponding SPI master example application.
 *
 */

#include "wiced.h"
#include "spi_slave.h"
#include "spi_slave_app.h"

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
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t led1_read_state ( spi_slave_t* device, uint8_t* data_start );
static wiced_result_t led2_read_state ( spi_slave_t* device, uint8_t* data_start );
static wiced_result_t led1_write_state( spi_slave_t* device, uint8_t* data_start );
static wiced_result_t led2_write_state( spi_slave_t* device, uint8_t* data_start );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static spi_slave_t spi_slave_device;
static const uint32_t     spi_slave_device_id = DEVICE_ID;
static uint32_t           led1_state          = WICED_FALSE;
static uint32_t           led2_state          = WICED_FALSE;

static const spi_slave_register_t spi_slave_register_list[] =
{
    [0] = /* Device ID register */
    {
         .address        = REGISTER_DEVICE_ID_ADDRESS,
         .access         = SPI_SLAVE_ACCESS_READ_ONLY,
         .data_type      = SPI_SLAVE_REGISTER_DATA_STATIC,
         .data_length    = REGISTER_DEVICE_ID_LENGTH,
         .static_data    = (uint8_t*)&spi_slave_device_id,
         .read_callback  = NULL, /* Callback not needed */
         .write_callback = NULL, /* Callback not needed */
    },
    [1] = /* LED 1 control register */
    {
         .address        = REGISTER_LED1_CONTROL_ADDRESS,
         .access         = SPI_SLAVE_ACCESS_READ_WRITE,
         .data_type      = SPI_SLAVE_REGISTER_DATA_DYNAMIC,
         .data_length    = REGISTER_LED1_CONTROL_LENGTH,
         .static_data    = NULL, /* data will be generated dynamically in the callback */
         .read_callback  = led1_read_state,
         .write_callback = led1_write_state,
    },
    [2] = /* LED 2 control register */
    {
         .address        = REGISTER_LED2_CONTROL_ADDRESS,
         .access         = SPI_SLAVE_ACCESS_READ_WRITE,
         .data_type      = SPI_SLAVE_REGISTER_DATA_DYNAMIC,
         .data_length    = REGISTER_LED2_CONTROL_LENGTH,
         .static_data    = NULL, /* data will be generated dynamically in the callback */
         .read_callback  = led2_read_state,
         .write_callback = led2_write_state,
    },
};

static const spi_slave_device_config_t spi_slave_device_config =
{
    .spi            = WICED_SPI_2,
    .config         =
    {
        .bits       = 8,
        .mode       = (SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH | SPI_MSB_FIRST),
        .speed      = 1000000,
    },
    .register_list  = spi_slave_register_list,
    .register_count = sizeof( spi_slave_register_list ) / sizeof( spi_slave_register_t ),
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( void )
{
    /* Initialise the WICED device */
    wiced_init();

    /* Initialise SPI slave device */
    spi_slave_init( &spi_slave_device, &spi_slave_device_config );
}

static wiced_result_t led1_read_state( spi_slave_t* device, uint8_t* data_start )
{
    *data_start = (uint8_t)led1_state;
    return WICED_SUCCESS;
}

static wiced_result_t led2_read_state( spi_slave_t* device, uint8_t* data_start )
{
    *data_start = (uint8_t)led2_state;
    return WICED_SUCCESS;
}

static wiced_result_t led1_write_state( spi_slave_t* device, uint8_t* data_start )
{
    wiced_bool_t new_state = (wiced_bool_t)*data_start;

    led1_state = new_state;
    if ( new_state == WICED_TRUE )
    {
        wiced_gpio_output_high( WICED_LED1 );
    }
    else
    {
        wiced_gpio_output_low( WICED_LED1 );
    }

    return WICED_SUCCESS;
}

static wiced_result_t led2_write_state( spi_slave_t* device, uint8_t* data_start )
{
    wiced_bool_t new_state = (wiced_bool_t)*data_start;

    led2_state = new_state;
    if ( new_state == WICED_TRUE )
    {
        wiced_gpio_output_high( WICED_LED2 );
    }
    else
    {
        wiced_gpio_output_low( WICED_LED2 );
    }

    return WICED_SUCCESS;
}
