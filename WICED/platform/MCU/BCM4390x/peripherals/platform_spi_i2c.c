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
#include <stdint.h>
#include <string.h>
#include "typedefs.h"
#include "platform_peripheral.h"
#include "platform_isr.h"
#include "platform_isr_interface.h"
#include "wwd_rtos.h"
#include "wwd_assert.h"
#include "wiced_osl.h"

#include "platform_mcu_peripheral.h"
#include "platform_map.h"

#include "bcmutils.h"
#include "sbchipc.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define REG32(ADDR)  *((volatile uint32_t*)(ADDR))

#define GSIO_I2C_WRITE_REG_VAL          \
    (                                   \
        GSIO_GC_I2C_SOURCE_FROMDATA |   \
        GSIO_READ_I2C_WRITE         |   \
        GSIO_NOACK_I2C_CHECK_ACK    |   \
        GSIO_NOD_1                  |   \
        GSIO_ENDIANNESS_BIG         |   \
        GSIO_GM_I2C                 |   \
        0                               \
    )

#define GSIO_I2C_READ_REG_VAL           \
    (                                   \
        GSIO_GC_I2C_SOURCE_FROMDATA |   \
        GSIO_READ_I2C_READ          |   \
        GSIO_NOD_1                  |   \
        GSIO_ENDIANNESS_BIG         |   \
        GSIO_GM_I2C                 |   \
        0                               \
    )

/******************************************************
 *                    Constants
 ******************************************************/

#define I2C_TRANSACTION_TIMEOUT_MS  ( 5 )

#define GSIO_0_IF                   (PLATFORM_CHIPCOMMON_REGBASE(OFFSETOF(chipcregs_t, gsioctrl)))
#define GSIO_0_CLKDIV               (PLATFORM_CHIPCOMMON_REGBASE(OFFSETOF(chipcregs_t, clkdiv2)))
#define GSIO_1_IF                   (PLATFORM_CHIPCOMMON_REGBASE(OFFSETOF(chipcregs_t, gsio1ctrl)))
#define GSIO_1_CLKDIV               (PLATFORM_CHIPCOMMON_REGBASE(OFFSETOF(chipcregs_t, gsio1clkdiv)))
#define GSIO_2_IF                   (PLATFORM_CHIPCOMMON_REGBASE(OFFSETOF(chipcregs_t, gsio2ctrl)))
#define GSIO_2_CLKDIV               (PLATFORM_CHIPCOMMON_REGBASE(OFFSETOF(chipcregs_t, gsio2clkdiv)))
#define GSIO_3_IF                   (PLATFORM_CHIPCOMMON_REGBASE(OFFSETOF(chipcregs_t, gsio3ctrl)))
#define GSIO_3_CLKDIV               (PLATFORM_CHIPCOMMON_REGBASE(OFFSETOF(chipcregs_t, gsio3clkdiv)))

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct gsio_43909_interface_regs gsio_43909_interface_regs_t;
typedef struct gsio_43909_regs gsio_43909_regs_t;

/******************************************************
 *                    Structures
 ******************************************************/

struct gsio_43909_interface_regs
{
     uint32_t   ctrl;
     uint32_t   address;
     uint32_t   data;
     /* clockdiv outside of this struct */
};

struct gsio_43909_regs
{
    volatile gsio_43909_interface_regs_t    *interface;
    volatile uint32_t                       *clkdiv;
};

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

static const gsio_43909_regs_t gsio_regs[BCM4390X_GSIO_MAX] =
{
    /* BCM4390X_GSIO_0 */
    {
        .interface          = (volatile gsio_43909_interface_regs_t *)GSIO_0_IF,
        .clkdiv             = (volatile uint32_t *)GSIO_0_CLKDIV,
    },

    /* BCM4390X_GSIO_1 */
    {
        .interface          = (volatile gsio_43909_interface_regs_t *)GSIO_1_IF,
        .clkdiv             = (volatile uint32_t *)GSIO_1_CLKDIV,
    },

    /* BCM4390X_GSIO_2 */
    {
        .interface          = (volatile gsio_43909_interface_regs_t *)GSIO_2_IF,
        .clkdiv             = (volatile uint32_t *)GSIO_2_CLKDIV,
    },

    /* BCM4390X_GSIO_3 */
    {
        .interface          = (volatile gsio_43909_interface_regs_t *)GSIO_3_IF,
        .clkdiv             = (volatile uint32_t *)GSIO_3_CLKDIV,
    },
};

/******************************************************
 *               Function Declarations
 ******************************************************/

static void                 gsio_set_clock      ( int port, uint32_t target_speed_hz );
static platform_result_t    gsio_wait_for_xfer_to_complete( int port );
static platform_result_t    i2c_xfer            ( const platform_i2c_t *i2c, uint32_t regval );
static void                 i2c_stop            ( const platform_i2c_t *i2c );
static platform_result_t    i2c_read            ( const platform_i2c_t *i2c, const platform_i2c_config_t *config, uint8_t *data_in, uint16_t length );
static platform_result_t    i2c_write           ( const platform_i2c_t *i2c, const platform_i2c_config_t *config, const uint8_t *data_out, uint16_t length );

/******************************************************
 *               Function Definitions
 ******************************************************/

static void gsio_set_clock( int port, uint32_t target_speed_hz )
{
    uint32_t clk_rate, divider;

    /* FIXME This code assumes that the HT clock is always employed and remains unchanged
     * throughout the lifetime an initialized I2C/SPI port.
     */
    clk_rate = osl_ht_clock();
    wiced_assert("Backplane clock value can not be identified properly", clk_rate != 0);

    divider = clk_rate / target_speed_hz;
    if (clk_rate % target_speed_hz) {
        divider++;
    }
    divider /= 4;
    divider -= 1;

    divider <<= GSIO_GD_SHIFT;
    divider &= GSIO_GD_MASK;

    REG32(gsio_regs[port].clkdiv) &= ~GSIO_GD_MASK;
    REG32(gsio_regs[port].clkdiv) |= divider;
}

platform_result_t platform_i2c_init( const platform_i2c_t* i2c, const platform_i2c_config_t* config )
{
    uint32_t target_speed_hz;

    wiced_assert( "bad argument", ( i2c != NULL ) && ( config != NULL ) && ( config->flags & I2C_DEVICE_USE_DMA ) == 0);

    if ( ( config->flags & I2C_DEVICE_USE_DMA ) || config->address_width != I2C_ADDRESS_WIDTH_7BIT)
    {
        return PLATFORM_UNSUPPORTED;
    }

    if ( config->speed_mode == I2C_LOW_SPEED_MODE )
    {
        target_speed_hz = 10000;
    }
    else if ( config->speed_mode == I2C_STANDARD_SPEED_MODE )
    {
        target_speed_hz = 100000;
    }
    else if ( config->speed_mode == I2C_HIGH_SPEED_MODE )
    {
        target_speed_hz = 400000;
    }
    else
    {
        return PLATFORM_BADOPTION;
    }

    gsio_set_clock(i2c->port, target_speed_hz);

    /* XXX Is it appropriate to set the address here? */
    gsio_regs[i2c->port].interface->address = config->address;

    /* WAR to get SCL line in hi-z since GSIO defaults to SPI and
     * drives SCL low.
     */
    gsio_regs[i2c->port].interface->ctrl = GSIO_GM_I2C;
    /* XXX Might need to wait here. */

    return PLATFORM_SUCCESS;
}

platform_result_t platform_i2c_deinit( const platform_i2c_t* i2c, const platform_i2c_config_t* config )
{
    wiced_assert( "bad argument", ( i2c != NULL ) && ( config != NULL ) );

    UNUSED_PARAMETER( i2c );
    UNUSED_PARAMETER( config );

    return PLATFORM_SUCCESS;
}

static platform_result_t gsio_wait_for_xfer_to_complete( int port )
{
    uint32_t reg_ctrl_val;

    do
    {
        reg_ctrl_val    = gsio_regs[port].interface->ctrl;
    }
    while
    (
        /* FIXME Add "relaxing" here so as not to starve backplane access. */
        ( ( reg_ctrl_val & GSIO_SB_MASK ) == GSIO_SB_BUSY )
    );

    /* Likely due to .Read=1, .NoAck=0, .NoA=1 and slave-address NACK'd. */
    /* Ignore error since it would take a big-hammer reset to chip common
     * to clear this bit.  Experimentation shows that GSIOCtrl.Error can be safely
     * ignored.
     */
    //wiced_assert("GSIO error", (gsio_regs[i2c->port].interface->ctrl & GSIO_ERROR_MASK) != GSIO_ERROR);

    return PLATFORM_SUCCESS;
}

static platform_result_t i2c_xfer( const platform_i2c_t *i2c, uint32_t regval )
{
    gsio_regs[i2c->port].interface->ctrl  = regval;
    gsio_regs[i2c->port].interface->ctrl |= GSIO_SB_BUSY;

    /* Wait until transaction is complete. */
    return gsio_wait_for_xfer_to_complete( i2c->port );
}

/* GSIO will transfer one more byte before generating stop-condition (P). */
static void i2c_stop( const platform_i2c_t *i2c )
{
    gsio_regs[i2c->port].interface->ctrl  = GSIO_GM_I2C | GSIO_READ_I2C_READ | GSIO_NOACK_I2C_NO_ACK | GSIO_GG_I2C_GENERATE_STOP;
    gsio_regs[i2c->port].interface->ctrl |= GSIO_SB_BUSY;

    (void)gsio_wait_for_xfer_to_complete( i2c->port );

    return;
}

static platform_result_t i2c_read( const platform_i2c_t *i2c, const platform_i2c_config_t *config,
        uint8_t *data_in, uint16_t length )
{
    platform_result_t   result;
    unsigned            i;

    result = PLATFORM_SUCCESS;

    if ( data_in == NULL || length == 0 || config->address_width != I2C_ADDRESS_WIDTH_7BIT)
    {
        return PLATFORM_BADARG;
    }

    /* XXX This is only necessary if the configuration changes! */
    gsio_regs[i2c->port].interface->address = config->address;

    for ( i = 0; i < length; i++ )
    {
        uint32_t tmpreg = 0;

        gsio_regs[i2c->port].interface->data = 0;

        if ( i == 0 )
        {
            tmpreg |= GSIO_NOA_I2C_START_7BITS;
        }
        if ( i < length-1 )
        {
            tmpreg |= GSIO_NOACK_I2C_SEND_ACK;
        }
        else
        {
            tmpreg |= GSIO_GG_I2C_GENERATE_STOP;
            tmpreg |= GSIO_NOACK_I2C_NO_ACK;
        }

        /* Execute GSIO command. */
        result = i2c_xfer(i2c, GSIO_I2C_READ_REG_VAL | tmpreg);

        /* Insert a stop-condition if the slave-address is NACK'd.
         * In the 1-byte case, this will be handled above.  With >1 bytes,
         * the GSIOCtrl.RxAck will be a NACK if the start-procedure NACKs.
         * Note that GSIOCtrl.RxAck seems to reflect our ACK,
         * which will be a NACK for the last byte according the the I2C
         * spec.
         */
        /* FIXME Won't error if slave-address is NACK'd in 1-byte case!
         *
         */
        if ( i == 0 && length > 1 && (gsio_regs[i2c->port].interface->ctrl & GSIO_RXACK_MASK) == GSIO_RXACK_I2C_NACK)
        {
            i2c_stop(i2c);
            result = PLATFORM_ERROR;
        }

        if ( PLATFORM_SUCCESS != result )
        {
            break;
        }

        /* Copy-out. */
        data_in[i] = (gsio_regs[i2c->port].interface->data >> 24) & 0xFF;
    }

    return result;
}

static platform_result_t i2c_write( const platform_i2c_t *i2c, const platform_i2c_config_t *config,
        const uint8_t *data_out, uint16_t length )
{
    platform_result_t   result;
    unsigned            i;

    result = PLATFORM_SUCCESS;

    if ( data_out == NULL || length == 0 || config->address_width != I2C_ADDRESS_WIDTH_7BIT)
    {
        return PLATFORM_BADARG;
    }

    /* XXX This is only necessary if the configuration changes! */
    gsio_regs[i2c->port].interface->address = config->address;

    for ( i = 0; i < length; i++ )
    {
        uint32_t tmpreg = 0;

        /* Prepare one byte on GSIOData register. */
        gsio_regs[i2c->port].interface->data = (data_out[i] << 24);

        /* Add start/stop conditions. */
        if ( i == 0 )
        {
            tmpreg |= GSIO_NOA_I2C_START_7BITS;
        }
        if ( i == length-1 )
        {
            tmpreg |= GSIO_GG_I2C_GENERATE_STOP;
        }

        /* Execute GSIO command. */
        result = i2c_xfer(i2c, GSIO_I2C_WRITE_REG_VAL | tmpreg);

        /* Slave NACK'd. */
        if ( ( gsio_regs[i2c->port].interface->ctrl & GSIO_RXACK_MASK ) == GSIO_RXACK_I2C_NACK )
        {
            /* Check if stop-condition already generated. */
            if ( ( tmpreg & GSIO_GG_MASK ) != GSIO_GG_I2C_GENERATE_STOP )
            {
                /* Generate a stop-conditon. */
                i2c_stop(i2c);
            }
            /* A NACK is an error. */
            result = PLATFORM_ERROR;
        }

        if ( PLATFORM_SUCCESS != result )
        {
            break;
        }
    }

    return result;
}

wiced_bool_t platform_i2c_probe_device( const platform_i2c_t *i2c, const platform_i2c_config_t *config, int retries )
{
    uint32_t    i;
    uint8_t     dummy[2];

    /* Read two bytes from the addressed-slave.  The slave-address won't be
     * acknowledged if it isn't on the I2C bus.  The read won't trigger
     * a NACK from the slave (unless of error), since only the receiver can do that.
     * Can't use one byte here because the last byte NACK seems to be reflected back
     * on GSIOCtrl.RxAck.
     * If the slave-address is not acknowledged it will generate an error in GSIOCtrl,
     * which we happily ignore so that we can carry-on.
     * Don't use an I2C write here because the GSIO doesn't support
     * issuing only the I2C start-procedure.
     */
    for ( i = 0; i < retries; i++ )
    {
        if ( i2c_read( i2c, config, dummy, sizeof dummy ) == PLATFORM_SUCCESS )
        {
            return WICED_TRUE;
        }
    }

    return WICED_FALSE;
}

platform_result_t platform_i2c_transfer( const platform_i2c_t* i2c, const platform_i2c_config_t* config, platform_i2c_message_t* messages, uint16_t number_of_messages )
{
    platform_result_t result = PLATFORM_SUCCESS;
    uint32_t          message_count;
    uint32_t          retries;

    /* Check for message validity. Combo message is unsupported */
    for ( message_count = 0; message_count < number_of_messages; message_count++ )
    {
        if ( messages[message_count].rx_buffer != NULL && messages[message_count].tx_buffer != NULL )
        {
            return PLATFORM_UNSUPPORTED;
        }

        if ( (messages[message_count].tx_buffer == NULL && messages[message_count].tx_length != 0)  ||
             (messages[message_count].tx_buffer != NULL && messages[message_count].tx_length == 0)  ||
             (messages[message_count].rx_buffer == NULL && messages[message_count].rx_length != 0)  ||
             (messages[message_count].rx_buffer != NULL && messages[message_count].rx_length == 0)  )
        {
            return PLATFORM_BADARG;
        }

        if ( messages[message_count].tx_buffer == NULL && messages[message_count].rx_buffer == NULL )
        {
            return PLATFORM_BADARG;
        }
    }

    for ( message_count = 0; message_count < number_of_messages; message_count++ )
    {
        for ( retries = 0; retries < messages[message_count].retries; retries++ )
        {
            if ( messages[message_count].tx_length != 0 )
            {
                result = i2c_write( i2c, config, (const uint8_t*) messages[message_count].tx_buffer, messages[message_count].tx_length );
                if ( result == PLATFORM_SUCCESS )
                {
                    /* Transaction successful. Break from the inner loop and continue with the next message */
                    break;
                }
            }
            else if ( messages[message_count].rx_length != 0 )
            {
                result = i2c_read( i2c, config, (uint8_t*) messages[message_count].rx_buffer, messages[message_count].rx_length );
                if ( result == PLATFORM_SUCCESS )
                {
                    /* Transaction successful. Break from the inner loop and continue with the next message */
                    break;
                }
            }
        }

        /* Check if retry is maxed out. If yes, return immediately */
        if ( retries == messages[message_count].retries && result != PLATFORM_SUCCESS )
        {
            return result;
        }
    }

    return PLATFORM_SUCCESS;
}

platform_result_t platform_i2c_init_tx_message( platform_i2c_message_t* message, const void* tx_buffer, uint16_t tx_buffer_length, uint16_t retries, wiced_bool_t disable_dma )
{
    wiced_assert( "bad argument", ( message != NULL ) && ( tx_buffer != NULL ) && ( tx_buffer_length != 0 ) );

    /* This I2C peripheral does not support DMA */
    if ( disable_dma == WICED_FALSE )
    {
        return PLATFORM_UNSUPPORTED;
    }

    memset( message, 0x00, sizeof( *message ) );
    message->tx_buffer = tx_buffer;
    message->retries   = retries;
    message->tx_length = tx_buffer_length;
    message->flags     = 0;

    return PLATFORM_SUCCESS;
}

platform_result_t platform_i2c_init_rx_message( platform_i2c_message_t* message, void* rx_buffer, uint16_t rx_buffer_length, uint16_t retries, wiced_bool_t disable_dma )
{
    wiced_assert( "bad argument", ( message != NULL ) && ( rx_buffer != NULL ) && ( rx_buffer_length != 0 ) );

    /* This I2C peripheral does not support DMA */
    if ( disable_dma == WICED_FALSE )
    {
        return PLATFORM_UNSUPPORTED;
    }

    memset( message, 0x00, sizeof( *message ) );

    message->rx_buffer = rx_buffer;
    message->retries   = retries;
    message->rx_length = rx_buffer_length;
    message->flags     = 0;

    return PLATFORM_SUCCESS;
}

#if 0
platform_result_t platform_i2c_init_combined_message( platform_i2c_message_t* message, const void* tx_buffer, void* rx_buffer, uint16_t tx_buffer_length, uint16_t rx_buffer_length, uint16_t retries, wiced_bool_t disable_dma )
{
    wiced_assert( "bad argument", ( message != NULL ) && ( tx_buffer != NULL ) && ( tx_buffer_length != 0 ) && ( rx_buffer != NULL ) && ( rx_buffer_length != 0 ) );

    /* This I2C peripheral does not support DMA. */
    if ( disable_dma == WICED_FALSE )
    {
        return PLATFORM_UNSUPPORTED;
    }

    memset( message, 0x00, sizeof( *message ) );

    message->rx_buffer = rx_buffer;
    message->tx_buffer = tx_buffer;
    message->combined  = WICED_TRUE;
    message->retries   = retries;
    message->tx_length = tx_buffer_length;
    message->rx_length = rx_buffer_length;
    message->flags     = 0;

    return PLATFORM_SUCCESS;
}
#endif






platform_result_t platform_spi_init( const platform_spi_t* spi, const platform_spi_config_t* config )
{
    wiced_assert( "bad argument", ( spi != NULL ) && ( config != NULL ) && (( config->mode & SPI_USE_DMA ) == 0 ));

    gsio_set_clock(spi->port, config->speed);

    gsio_regs[spi->port].interface->ctrl = ( 0 & GSIO_GO_MASK) | GSIO_GC_SPI_NOD_DATA_IO | GSIO_NOD_1 | GSIO_GM_SPI;
    /* XXX Might need to wait here. */

    return PLATFORM_SUCCESS;
}

platform_result_t platform_spi_deinit( const platform_spi_t* spi )
{
    UNUSED_PARAMETER( spi );
    /* TODO: unimplemented */
    return PLATFORM_UNSUPPORTED;
}

platform_result_t platform_spi_transfer( const platform_spi_t* spi, const platform_spi_config_t* config, const platform_spi_message_segment_t* segments, uint16_t number_of_segments )
{
    platform_result_t result = PLATFORM_SUCCESS;
    uint32_t          count  = 0;
    uint16_t          i;

    wiced_assert( "bad argument", ( spi != NULL ) && ( config != NULL ) && ( segments != NULL ) && ( number_of_segments != 0 ) && ( config->bits == 8 ) );

    platform_mcu_powersave_disable();

    result = platform_spi_init( spi, config );
    if ( result != PLATFORM_SUCCESS )
    {
        return result;
    }

    /* Activate chip select */
    gsio_regs[spi->port].interface->ctrl |= GSIO_GG_SPI_CHIP_SELECT;

//    platform_gpio_output_low( config->chip_select );

    for ( i = 0; i < number_of_segments; i++ )
    {
        count = segments[i].length;

        const uint8_t* send_ptr = ( const uint8_t* )segments[i].tx_buffer;
        uint8_t*       rcv_ptr  = ( uint8_t* )segments[i].rx_buffer;

        while ( count-- )
        {
            uint16_t data = 0xFF;

            if ( send_ptr != NULL )
            {
                data = *send_ptr++;
            }

            gsio_regs[spi->port].interface->data = data;
            gsio_regs[spi->port].interface->ctrl |= GSIO_SB_BUSY;
            gsio_wait_for_xfer_to_complete( spi->port );
            data = gsio_regs[spi->port].interface->data;

            if ( rcv_ptr != NULL )
            {
                *rcv_ptr++ = (uint8_t)data;
            }
        }
    }

    /* Deassert chip select */
    gsio_regs[spi->port].interface->ctrl &= (~GSIO_GG_SPI_CHIP_SELECT);
//    platform_gpio_output_high( config->chip_select );

    platform_mcu_powersave_enable( );

    return result;
}

