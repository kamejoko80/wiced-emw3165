/**
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
#include "platform_config.h"
#include "wwd_assert.h"
#include "wiced.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "nfc_host.h"
#include "platform_nfc.h"

/******************************************************
 *                      Macros
 ******************************************************/

/* Verify if WICED Platform API returns success.
 * Otherwise, returns the error code immediately.
 * Assert in DEBUG build.
 */
#define VERIFY_RETVAL( x ) \
do \
{ \
    wiced_result_t verify_result = (x); \
    if ( verify_result != WICED_SUCCESS ) \
    { \
        wiced_assert( "bus error", 0!=0 ); \
        return verify_result; \
    } \
} while( 0 )

/* Macro for checking of bus is initialised */
#define IS_BUS_INITIALISED( ) \
do \
{ \
    if ( bus_initialised == WICED_FALSE ) \
    { \
        wiced_assert( "bus uninitialised", 0!=0 ); \
        return WICED_ERROR; \
    } \
} while ( 0 )

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

void nfc_rx_irq_handler( void* arg );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static volatile wiced_bool_t bus_initialised = WICED_FALSE;
static volatile wiced_bool_t device_powered  = WICED_FALSE;

wiced_i2c_device_t wiced_nfc_device =
{
    .port          = WICED_I2C_2,
    .address       = 0x76,
    .address_width = I2C_ADDRESS_WIDTH_7BIT,
    .speed_mode    = I2C_STANDARD_SPEED_MODE,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t nfc_bus_init( void )
{
    wiced_result_t status;
    wiced_bool_t   device_found;

    if ( bus_initialised == WICED_TRUE )
    {
        return WICED_SUCCESS;
    }

    /* Configure transport to I2C */
    VERIFY_RETVAL( platform_gpio_init( wiced_nfc_control_pins[ WICED_NFC_PIN_TRANS_SELECT ], OUTPUT_OPEN_DRAIN_PULL_UP ) );
    VERIFY_RETVAL( platform_gpio_output_high( wiced_nfc_control_pins[ WICED_NFC_PIN_TRANS_SELECT ] ) );

    VERIFY_RETVAL( platform_gpio_init( wiced_nfc_control_pins[ WICED_NFC_PIN_WAKE ], OUTPUT_OPEN_DRAIN_PULL_UP ) );
    VERIFY_RETVAL( platform_gpio_output_low( wiced_nfc_control_pins[ WICED_NFC_PIN_WAKE ] ) );
    wiced_rtos_delay_milliseconds( 100 );

    /* Configure Reg Enable pin to output. Set to HIGH */
    VERIFY_RETVAL( platform_gpio_init( wiced_nfc_control_pins[ WICED_NFC_PIN_POWER ], OUTPUT_OPEN_DRAIN_PULL_UP ) );
    VERIFY_RETVAL( platform_gpio_output_high( wiced_nfc_control_pins[ WICED_NFC_PIN_POWER ] ) );

    device_powered = WICED_TRUE;

    /* Configure IRQ REQ pin to input pull-up */
    VERIFY_RETVAL( platform_gpio_init( wiced_nfc_control_pins[ WICED_NFC_PIN_IRQ_REQ ], INPUT_PULL_UP ) );
    platform_gpio_irq_enable( wiced_nfc_control_pins[ WICED_NFC_PIN_IRQ_REQ ], IRQ_TRIGGER_RISING_EDGE, nfc_rx_irq_handler, (void*) 0 );

    status = wiced_i2c_init( &wiced_nfc_device );
    if ( status != WICED_SUCCESS )
    {
        WPRINT_LIB_ERROR( ( "Initializing I2C interface failed with code: %d\r\n", status ) );
        return WICED_ERROR;
    }

    /* Wait until the NFC chip stabilises */
    wiced_rtos_delay_milliseconds( 100 );

    device_found = wiced_i2c_probe_device( &wiced_nfc_device, 3 );
    if ( device_found != WICED_TRUE )
    {
        WPRINT_LIB_ERROR( ( "Probe I2C interface failed with code: %d\r\n", status ) );
        return WICED_ERROR;
    }

    bus_initialised = WICED_TRUE;

    return WICED_SUCCESS;
}

wiced_result_t nfc_bus_deinit( void )
{
    if ( bus_initialised == WICED_FALSE )
    {
        return WICED_SUCCESS;
    }

    VERIFY_RETVAL( platform_gpio_output_low( wiced_nfc_control_pins[WICED_NFC_PIN_TRANS_SELECT] ) );
    VERIFY_RETVAL( platform_gpio_output_high( wiced_nfc_control_pins[WICED_NFC_PIN_WAKE] ) );

    /* Deinitialise I2C */
    VERIFY_RETVAL( wiced_i2c_deinit( &wiced_nfc_device ) );
    bus_initialised = WICED_FALSE;

    return WICED_SUCCESS;
}

wiced_result_t nfc_bus_transmit( const uint8_t* data_out, uint32_t size )
{
    wiced_i2c_message_t message;
    IS_BUS_INITIALISED();

    VERIFY_RETVAL( wiced_i2c_init_tx_message( &message, data_out, size, 3, 1 ) );

    wiced_i2c_transfer( &wiced_nfc_device, &message, 1 );

    return WICED_SUCCESS;
}

wiced_result_t nfc_bus_receive( uint8_t* data_in, uint32_t size, uint32_t timeout_ms )
{
    int                 i;
    wiced_result_t      status = WICED_SUCCESS;
    wiced_i2c_message_t message;

    IS_BUS_INITIALISED();

    for ( i = 0; i < size; i++ )
    {
        VERIFY_RETVAL( wiced_i2c_init_rx_message( &message, &data_in[ i ], 1, 3, 1 ) );
        status = wiced_i2c_transfer( &wiced_nfc_device, &message, 1 );
    }
    return status;
}

wiced_bool_t nfc_bus_is_ready( void )
{
    return ( bus_initialised == WICED_FALSE ) ? WICED_FALSE : ( ( platform_gpio_input_get( wiced_nfc_uart_pins[ WICED_NFC_PIN_UART_CTS ] ) == WICED_TRUE ) ? WICED_FALSE : WICED_TRUE );
}

wiced_bool_t nfc_bus_is_on( void )
{
    return device_powered;
}

wiced_bool_t nfc_receive_ready(void)
{
    return platform_gpio_input_get( wiced_nfc_control_pins[ WICED_NFC_PIN_IRQ_REQ ] );
}

void nfc_rx_irq_handler( void* arg )
{
    nfc_signal_rx_irq( );
}
