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
 * SPI Master Application
 *
 * This application demonstrates how to drive a WICED SPI slave device provided in apps/snip/spi_slave
 *
 */

#include "wiced.h"
#include "spi_master.h"
#include "../spi_slave_app.h"

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

static void data_ready_callback( spi_master_t* host );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static spi_master_t             spi_master;
static const wiced_spi_device_t spi_device =
{
    .port        = WICED_SPI_1,
    .chip_select = WICED_GPIO_12,
    .speed       = SPI_CLOCK_SPEED_HZ,
    .mode        = SPI_MODE,
    .bits        = SPI_BIT_WIDTH
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( void )
{
    uint32_t       a;
    uint32_t       device_id;
    wiced_result_t result;
    uint32_t       led1_state = WICED_FALSE;
    uint32_t       led2_state = WICED_FALSE;

    /* Initialise the WICED device */
    wiced_init();

    /* Initialise SPI slave device */
    spi_master_init( &spi_master, &spi_device, WICED_GPIO_11, data_ready_callback );

    /* Read device ID */
    result = spi_master_read_register( &spi_master, REGISTER_DEVICE_ID_ADDRESS, REGISTER_DEVICE_ID_LENGTH, (uint8_t*)&device_id );
    if ( result == WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ( "Device ID : 0x%.08X\n", (unsigned int)device_id ) );
    }
    else
    {
        WPRINT_APP_INFO( ( "Retrieving device ID failed. Please check hardware connections\n" ) );
    }

    WPRINT_APP_INFO( ( "LED 1 and 2 on the SPI slave device will start blinking alternately\n" ) );

    /* Toggle LED1 and 2 alternately */
    for ( a = 0; a < 10; a++ )
    {
        if ( led1_state == WICED_TRUE )
        {
            /* Switch LED1 on and LED2 off */
            led1_state = WICED_FALSE;
            led2_state = WICED_TRUE;
        }
        else
        {
            /* Switch LED1 off and LED2 on */
            led1_state = WICED_TRUE;
            led2_state = WICED_FALSE;
        }

        spi_master_write_register( &spi_master, REGISTER_LED1_CONTROL_ADDRESS, REGISTER_LED1_CONTROL_LENGTH, (uint8_t*)&led1_state );
        spi_master_write_register( &spi_master, REGISTER_LED2_CONTROL_ADDRESS, REGISTER_LED2_CONTROL_LENGTH, (uint8_t*)&led2_state );
        wiced_rtos_delay_milliseconds( 500 );
    }

    WPRINT_APP_INFO( ( "Test invalid register address ..." ) );

    result = spi_master_write_register( &spi_master, 0x0004, 1, (uint8_t*)&led1_state );
    if ( result == WICED_PLATFORM_SPI_SLAVE_ADDRESS_UNAVAILABLE )
    {
        WPRINT_APP_INFO( ( "success\n" ) );
    }
    else
    {
        WPRINT_APP_INFO( ( "failed\n" ) );
    }

}

static void data_ready_callback( spi_master_t* host )
{
    UNUSED_PARAMETER( host );
    /* This callback is called when the SPI slave device asynchronously indicates that a new data is available */
}
