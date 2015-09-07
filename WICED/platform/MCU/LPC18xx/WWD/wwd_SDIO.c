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
 * Defines WWD SDIO functions for STM32F4xx MCU
 */
#include <string.h> /* For memcpy */
#include "wwd_platform_common.h"
#include "wwd_bus_protocol.h"
#include "wwd_assert.h"
#include "platform/wwd_platform_interface.h"
#include "platform/wwd_sdio_interface.h"
#include "platform/wwd_bus_interface.h"
#include "RTOS/wwd_rtos_interface.h"
#include "network/wwd_network_constants.h"
#include "wwd_rtos_isr.h"
//#include "misc.h"
#include "platform_cmsis.h"
#include "platform_peripheral.h"
#include "platform_config.h"
#include "chip.h"

/******************************************************
 *             Constants
 ******************************************************/

#define SDIO_ENUMERATION_TIMEOUT_MS          (500)

/* Helper definition: all SD error conditions in the status word */
#define SD_INT_ERROR (MCI_INT_RESP_ERR | MCI_INT_RCRC | MCI_INT_DCRC | \
                      MCI_INT_RTO | MCI_INT_DTO | MCI_INT_HTO | MCI_INT_FRUN | MCI_INT_HLE | \
                      MCI_INT_SBE | MCI_INT_EBE)

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/
static uint8_t                      temp_dma_buffer[MAX(2*1024,WICED_LINK_MTU+ 64)];
static volatile int32_t             sdio_wait_exit = 1;
static sdif_device                  sdif_dev;

/******************************************************
 *             Static Function Declarations
 ******************************************************/

/******************************************************
 *             Function definitions
 ******************************************************/
#ifndef  WICED_DISABLE_MCU_POWERSAVE
static void sdio_oob_irq_handler( void* arg )
{
    UNUSED_PARAMETER(arg);
    platform_mcu_powersave_exit_notify( );
    wwd_thread_notify_irq( );
}
#endif /* ifndef  WICED_DISABLE_MCU_POWERSAVE */

#ifndef WICED_DISABLE_MCU_POWERSAVE
wwd_result_t host_enable_oob_interrupt( void )
{
    /* Set GPIO_B[1:0] to input. One of them will be re-purposed as OOB interrupt */
    platform_gpio_init( &wifi_sdio_pins[WWD_PIN_SDIO_OOB_IRQ], INPUT_HIGH_IMPEDANCE );
    platform_gpio_irq_enable( &wifi_sdio_pins[WWD_PIN_SDIO_OOB_IRQ], IRQ_TRIGGER_RISING_EDGE, sdio_oob_irq_handler, 0 );
    return WWD_SUCCESS;
}

uint8_t host_platform_get_oob_interrupt_pin( void )
{
    return WICED_WIFI_OOB_IRQ_GPIO_PIN;
}
#endif /* ifndef  WICED_DISABLE_MCU_POWERSAVE */

wwd_result_t host_platform_bus_init( void )
{
    uint8_t          a;

    platform_mcu_powersave_disable();

#ifdef WICED_WIFI_USE_GPIO_FOR_BOOTSTRAP_0
    /* Set GPIO_B[1:0] to 00 to put WLAN module into SDIO mode */
    platform_gpio_init( &wifi_control_pins[WWD_PIN_BOOTSTRAP_0], OUTPUT_PUSH_PULL );
    platform_gpio_output_low( &wifi_control_pins[WWD_PIN_BOOTSTRAP_0] );
#endif
#ifdef WICED_WIFI_USE_GPIO_FOR_BOOTSTRAP_1
    platform_gpio_init( &wifi_control_pins[WWD_PIN_BOOTSTRAP_1], OUTPUT_PUSH_PULL );
    platform_gpio_output_low( &wifi_control_pins[WWD_PIN_BOOTSTRAP_1] );
#endif

    /* Setup GPIO pins for SDIO data & clock */
    for ( a = WWD_PIN_SDIO_CLK; a < WWD_PIN_SDIO_MAX; a++ )
    {
        platform_pin_set_alternate_function( &wifi_sdio_pins[ a ].hw_pin );
    }

    /* The SDIO driver needs to know the SDIO clock rate */
    Chip_SDIF_Init( LPC_SDMMC );

    Chip_SDIF_SetClock(LPC_SDMMC, Chip_Clock_GetBaseClocktHz(CLK_BASE_SDIO), 400000);

    return WWD_SUCCESS;
}

wwd_result_t host_platform_sdio_enumerate( void )
{
    wwd_result_t result;
    uint32_t       loop_count;
    uint32_t       data = 0;

    loop_count = 0;
    do
    {
        /* Send CMD0 to set it to idle state */
        host_platform_sdio_transfer( BUS_WRITE, SDIO_CMD_0, SDIO_BYTE_MODE, SDIO_1B_BLOCK, 0, 0, 0, NO_RESPONSE, NULL );

        /* CMD5. */
        host_platform_sdio_transfer( BUS_READ, SDIO_CMD_5, SDIO_BYTE_MODE, SDIO_1B_BLOCK, 0, 0, 0, NO_RESPONSE, NULL );

        /* Send CMD3 to get RCA. */
        result = host_platform_sdio_transfer( BUS_READ, SDIO_CMD_3, SDIO_BYTE_MODE, SDIO_1B_BLOCK, 0, 0, 0, RESPONSE_NEEDED, &data );
        loop_count++;
        if ( loop_count >= (uint32_t) SDIO_ENUMERATION_TIMEOUT_MS )
        {
            return WWD_TIMEOUT;
        }
    } while ( ( result != WWD_SUCCESS ) && ( host_rtos_delay_milliseconds( (uint32_t) 1 ), ( 1 == 1 ) ) );
    /* If you're stuck here, check the platform matches your hardware */

    /* Send CMD7 with the returned RCA to select the card */
    host_platform_sdio_transfer( BUS_WRITE, SDIO_CMD_7, SDIO_BYTE_MODE, SDIO_1B_BLOCK, data, 0, 0, RESPONSE_NEEDED, NULL );
    return result == 0 ? WWD_SUCCESS : WWD_TIMEOUT;
}

wwd_result_t host_platform_bus_deinit( void )
{
    return WICED_ERROR;
}

wwd_result_t host_platform_sdio_transfer( wwd_bus_transfer_direction_t direction, sdio_command_t command, sdio_transfer_mode_t mode, sdio_block_size_t block_size, uint32_t argument, /*@null@*/ uint32_t* data, uint16_t data_size, sdio_response_needed_t response_expected, /*@out@*/ /*@null@*/ uint32_t* response )
{

    uint32_t long_response[4] = {0xFF, 0xFF, 0xFF, 0xFF };
    uint32_t status           = 0;
    uint32_t status_mask      = MCI_INT_CMD_DONE;
    uint32_t cmd_reg          = command;
    uint8_t *final_dma_buffer = (uint8_t*)data;
    uint32_t final_size       = data_size;

    wiced_assert("Bad args", !((command == SDIO_CMD_53) && (data == NULL)));

    if ( response != NULL )
    {
        *response = 0;
    }

    platform_mcu_powersave_disable();

    cmd_reg |= response_expected == RESPONSE_NEEDED ? MCI_CMD_RESP_EXP  | MCI_CMD_RESP_CRC : 0;
    cmd_reg |= command == SDIO_CMD_0 ? MCI_CMD_INIT : 0;

    if ( data_size )
    {
        /* Some actual data need to be exchanged */
        /* set number of bytes to read */
        if ( mode == SDIO_BYTE_MODE )
        {
            Chip_SDIF_SetBlkSizeByteCnt( LPC_SDMMC, final_size );
        }
        else
        {
            if ( data_size % block_size != 0 )
            {
                final_size = ( ( data_size / block_size ) + 1 ) * block_size;
                wiced_assert( "WWD SDIO error, data size > dma buffer size", final_size <= sizeof( temp_dma_buffer ) );
                final_dma_buffer = temp_dma_buffer;
                if ( direction == BUS_WRITE )
                {
                    memcpy( final_dma_buffer, data, data_size );
                }
            }
            Chip_SDIF_SetByteCnt( LPC_SDMMC, final_size );
            Chip_SDIF_SetBlkSize( LPC_SDMMC, block_size );
        }

        cmd_reg |= direction == BUS_WRITE ? MCI_CMD_DAT_WR : 0;
        cmd_reg |= MCI_CMD_DAT_EXP;
        status_mask = MCI_INT_DATA_OVER;
        Chip_SDIF_DmaSetup(LPC_SDMMC, &sdif_dev, (uint32_t) final_dma_buffer, final_size);
    }

    Chip_SDIF_SendCmd( LPC_SDMMC, cmd_reg, argument );

    /* For mfg_test a 1 msec delay is needed.
     * If using mfg_test uncomment the below line
     */
    host_rtos_delay_milliseconds( 1 );
    do
    {
        status = Chip_SDIF_GetIntStatus(LPC_SDMMC);
    }while( ( status & status_mask ) == 0 );

    Chip_SDIF_ClrIntStatus(LPC_SDMMC, status );

    /* We return an error if there is a timeout, even if we've fetched  a response */
    if (status & SD_INT_ERROR)
    {
        return WWD_TIMEOUT;
    }

    if ( final_dma_buffer != ( (uint8_t*)data ) )
    {
        if ( direction == BUS_READ )
        {
            memcpy( data, final_dma_buffer, data_size);
        }
    }

    if ( response != NULL )
    {
        Chip_SDIF_GetResponse( LPC_SDMMC, long_response );
        *response = long_response[0];
    }

    return WWD_SUCCESS;
}


void host_platform_enable_high_speed_sdio( void )
{
    Chip_SDIF_SetClock(LPC_SDMMC, Chip_Clock_GetBaseClocktHz(CLK_BASE_SDIO), 20000000);

#ifndef SDIO_1_BIT
    Chip_SDIF_SetCardType( LPC_SDMMC, MCI_CTYPE_4BIT );
#else
    Chip_SDIF_SetCardType( LPC_SDMMC, 0 );
#endif
}

wwd_result_t host_platform_bus_enable_interrupt( void )
{
    NVIC_DisableIRQ( SDIO_IRQn );
    NVIC_ClearPendingIRQ( SDIO_IRQn );
    NVIC_SetPriority( SDIO_IRQn, 0xE );
    Chip_SDIF_SetIntMask(LPC_SDMMC, MCI_INT_SDIO);
    NVIC_EnableIRQ(SDIO_IRQn );
    return  WWD_SUCCESS;
}

wwd_result_t host_platform_bus_disable_interrupt( void )
{
    NVIC_DisableIRQ( SDIO_IRQn );
    NVIC_ClearPendingIRQ( SDIO_IRQn );
    Chip_SDIF_SetIntMask(LPC_SDMMC, 0);
    return  WWD_SUCCESS;
}

void host_platform_bus_buffer_freed( wwd_buffer_dir_t direction )
{
    UNUSED_PARAMETER( direction );
}

/******************************************************
 *             IRQ Handler Definitions
 ******************************************************/

WWD_RTOS_DEFINE_ISR( sdio_irq )
{
    uint32_t intstatus;


    /* for now we are only here if its an SDIO IRQ */
    intstatus = Chip_SDIF_GetIntStatus(LPC_SDMMC);

    /* Check whether the external interrupt was triggered */
    if ( ( intstatus & MCI_INT_SDIO ) != 0 )
    {
        /* Clear the interrupt and then inform WICED thread */
        Chip_SDIF_ClrIntStatus(LPC_SDMMC, MCI_INT_SDIO);
        wwd_thread_notify_irq( );
    }
}

WWD_RTOS_DEFINE_ISR( sdio_dma_irq )
{
#if 0
    wwd_result_t result;

    /* Clear interrupt */
    DMA2->LIFCR = (uint32_t) (0x3F << 22);

    result = host_rtos_set_semaphore( &sdio_transfer_finished_semaphore, WICED_TRUE );

    /* check result if in debug mode */
    wiced_assert( "failed to set dma semaphore", result == WWD_SUCCESS );

    /*@-noeffect@*/
    (void) result; /* ignore result if in release mode */
    /*@+noeffect@*/
#endif
}

/******************************************************
 *             IRQ Handler Mapping
 ******************************************************/

WWD_RTOS_MAP_ISR( sdio_irq,     SDIO_irq         )
//WWD_RTOS_MAP_ISR( sdio_dma_irq, DMA2_Stream3_irq )

