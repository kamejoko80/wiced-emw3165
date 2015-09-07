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
 *  Broadcom WLAN SDIO Protocol interface
 *
 *  Implements the WWD Bus Protocol Interface for SDIO
 *  Provides functions for initialising, de-intitialising 802.11 device,
 *  sending/receiving raw packets etc
 */


#include <string.h> /* For memcpy */
#include "wwd_assert.h"
#include "network/wwd_buffer_interface.h"
#include "internal/wwd_sdpcm.h"
#include "internal/wwd_internal.h"
#include "RTOS/wwd_rtos_interface.h"
#include "platform/wwd_platform_interface.h"
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"
#include "chip_constants.h"
#include "wiced_resource.h"         /* TODO: remove include dependency */
#include "resources.h"        /* TODO: remove include dependency */
#include "platform_map.h"
#include "platform_mcu_peripheral.h"
#include "wifi_nvram_image.h" /* TODO: remove include dependency */
#include "wiced_utilities.h"
#include "wiced_resource.h"
#include "platform_m2m.h"
#include "platform/wwd_bus_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define VERIFY_RESULT( x )  { wwd_result_t verify_result; verify_result = (x); if ( verify_result != WWD_SUCCESS ) return verify_result; }

#define ROUNDUP(x, y)        ((((x) + ((y) - 1)) / (y)) * (y))

/******************************************************
 *             Constants
 ******************************************************/

#define    I_PC        (1 << 10)    /* descriptor error */
#define    I_PD        (1 << 11)    /* data error */
#define    I_DE        (1 << 12)    /* Descriptor protocol Error */
#define    I_RU        (1 << 13)    /* Receive descriptor Underflow */
#define    I_RI        (1 << 16)    /* Receive Interrupt */
#define    I_XI        (1 << 24)    /* Transmit Interrupt */
#define    I_ERRORS    (I_PC | I_PD | I_DE | I_RU )    /* DMA Errors */

#define SICF_CPUHALT        (0x0020)

#define WLAN_RAM_STARTING_ADDRESS     PLATFORM_ATCM_RAM_BASE(0x0)

#define FF_ROM_SHADOW_INDEX_REGISTER     *((volatile uint32_t*)(PLATFORM_WLANCR4_REGBASE(0x080)))
#define FF_ROM_SHADOW_DATA_REGISTER      *((volatile uint32_t*)(PLATFORM_WLANCR4_REGBASE(0x084)))


/*IDM wrapper register regions */
#define ARMCR4_MWRAPPER_ADR       PLATFORM_WLANCR4_MASTER_WRAPPER_REGBASE(0)

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/

static wiced_bool_t bus_is_up = WICED_FALSE;
static wiced_bool_t wwd_bus_flow_controlled;
static uint32_t fake_backplane_window_addr = 0;

/******************************************************
 *             Function declarations
 ******************************************************/

static wwd_result_t boot_wlan( void );
static wwd_result_t shutdown_wlan(void);
static void write_reset_instruction( uint32_t reset_inst );

/******************************************************
 *             Global Function definitions
 ******************************************************/

/* Device data transfer functions */
wwd_result_t wwd_bus_send_buffer( wiced_buffer_t buffer )
{
    m2m_dma_tx_data( buffer, 0 );
    return WWD_SUCCESS;
}

wwd_result_t wwd_bus_init( void )
{
    boot_wlan();

    M2M_INIT_DMA_SYNC();

    return WWD_SUCCESS;
}

wwd_result_t wwd_bus_deinit( void )
{
    m2m_deinit_dma();
    shutdown_wlan();

    /* Put device in reset. */
    host_platform_reset_wifi( WICED_TRUE );
    return WWD_SUCCESS;
}

uint32_t wwd_bus_packet_available_to_read(void)
{
    return 1;
}

/*
 * From internal documentation: hwnbu-twiki/SdioMessageEncapsulation
 * When data is available on the device, the device will issue an interrupt:
 * - the device should signal the interrupt as a hint that one or more data frames may be available on the device for reading
 * - the host may issue reads of the 4 byte length tag at any time -- that is, whether an interupt has been issued or not
 * - if a frame is available, the tag read should return a nonzero length (>= 4) and the host can then read the remainder of the frame by issuing one or more CMD53 reads
 * - if a frame is not available, the 4byte tag read should return zero
 */

/*
 * The format of the rx wiced buffer :
 * |SDPCMD_RXOFFSET | wiced_buffer_t |   SDPCM HDR    | DATA|
 *       8 bytes         4 bytes          22 bytes
 */

/*@only@*//*@null@*/wwd_result_t wwd_bus_read_frame( wiced_buffer_t* buffer )
{
    uint32_t  intstatus;
    uint16_t* hwtag;
    void*     p0 = NULL;

    M2M_INIT_DMA_ASYNC( );

    intstatus = m2m_read_intstatus( );

    /* Handle DMA interrupts */
    if ( ( intstatus & I_XI ) != 0 )
    {
        WPRINT_WWD_INFO(("DMA:  TX reclaim\n"));
        m2m_dma_tx_reclaim( );
    }

    m2m_refill_dma( );

    /* Handle DMA errors */
    if ( ( intstatus & I_ERRORS ) != 0 )
    {
        m2m_refill_dma( );
        WPRINT_WWD_INFO(("RX errors: intstatus: 0x%x\n", (unsigned int)intstatus));
    }

    /* Handle DMA receive interrupt */
    p0 = m2m_read_dma_packet( &hwtag );
    if ( p0 == NULL )
    {
        host_platform_bus_enable_interrupt();
        WPRINT_WWD_INFO(("intstatus: 0x%x, NO PACKT\n", (uint)intstatus));
        return WWD_NO_PACKET_TO_RECEIVE;
    }

    *buffer = p0;
    WPRINT_WWD_INFO(("read pkt , p0: 0x%x\n", (uint)p0));

    /* move the data pointer 12 bytes(sizeof(wwd_buffer_header_t))
     * back to the start of the pakcet
     */
    host_buffer_add_remove_at_front( buffer, -(int)sizeof(wwd_buffer_header_t) );
    wwd_sdpcm_update_credit( (uint8_t*) hwtag );

    m2m_refill_dma( );

    /* XXX: Where are buffers from dma_rx and dma_getnextrxp created? */

    return WWD_SUCCESS;
}

wwd_result_t wwd_bus_ensure_is_up( void )
{
    if ( bus_is_up == WICED_FALSE )
    {
        M2M_POWERSAVE_COMM_BEGIN( );
        bus_is_up = WICED_TRUE;
    }

    return WWD_SUCCESS;
}

wwd_result_t wwd_bus_allow_wlan_bus_to_sleep( void )
{
    if ( bus_is_up == WICED_TRUE )
    {
        bus_is_up = WICED_FALSE;
        M2M_POWERSAVE_COMM_END( );
    }

    return WWD_SUCCESS;
}

wwd_result_t wwd_bus_set_flow_control( uint8_t value )
{
    if ( value != 0 )
    {
        wwd_bus_flow_controlled = WICED_TRUE;
    }
    else
    {
        wwd_bus_flow_controlled = WICED_FALSE;
    }
    return WWD_SUCCESS;
}

wiced_bool_t wwd_bus_is_flow_controlled( void )
{
    return wwd_bus_flow_controlled;
}

wwd_result_t wwd_bus_poke_wlan( void )
{
    return WWD_SUCCESS;
}

/******************************************************
 *             Static Function definitions
 ******************************************************/

/*
 * copy reset vector to FLOPS
 * WLAN Address = {Programmable Register[31:18],
 * Current Transaction's HADDR[17:0]}
 */
static void write_reset_instruction( uint32_t reset_instruction )
{
    FF_ROM_SHADOW_INDEX_REGISTER = 0x0;
    FF_ROM_SHADOW_DATA_REGISTER  = reset_instruction;
}

wwd_result_t wwd_bus_write_wifi_firmware_image( void )
{
#ifndef NO_WIFI_FIRMWARE
    uint32_t offset = 0;
    uint32_t total_size;
    uint32_t size_read;
    uint32_t reset_instruction;

    /* Halt ARM and remove from reset */
    WPRINT_WWD_INFO(("Reset wlan core..\n"));
    VERIFY_RESULT( wwd_reset_device_core( WLAN_ARM_CORE, WLAN_CORE_FLAG_CPU_HALT ) );

    total_size = (uint32_t) resource_get_size( &wifi_firmware_image );

    resource_read ( &wifi_firmware_image, 0, 4, &size_read, &reset_instruction );

    while ( total_size > offset )
    {
        resource_read ( &wifi_firmware_image, 0, PLATFORM_WLAN_RAM_SIZE, &size_read, (uint8_t *)(WLAN_RAM_STARTING_ADDRESS+offset) );
        offset += size_read;
    }

    WPRINT_WWD_INFO(("load_wlan_fw: write reset_inst : 0x%x\n", (uint)reset_instruction));

    write_reset_instruction( reset_instruction );
#else
    wiced_assert("wifi_firmware_image is not included resource build", 0 == 1);
#endif

    return WWD_SUCCESS;
}

/*
 * Load the nvram to the bottom of the WLAN TCM
 */
wwd_result_t wwd_bus_write_wifi_nvram_image( void )
{
    uint32_t nvram_size;
    uint32_t nvram_destination_address;
    uint32_t nvram_size_in_words;

    nvram_size = ROUNDUP(sizeof(wifi_nvram_image), 4);

    /* Put the NVRAM image at the end of RAM leaving the last 4 bytes for the size */
    nvram_destination_address = ( PLATFORM_WLAN_RAM_SIZE - 4 ) - nvram_size;
    nvram_destination_address += WLAN_RAM_STARTING_ADDRESS;

    /* Write NVRAM image into WLAN RAM */
    memcpy( (uint8_t *) nvram_destination_address, wifi_nvram_image, nvram_size );

    /* Calculate the NVRAM image size in words (multiples of 4 bytes) and its bitwise inverse */
    nvram_size_in_words = nvram_size / 4;
    nvram_size_in_words = ( ~nvram_size_in_words << 16 ) | ( nvram_size_in_words & 0x0000FFFF );

    memcpy( (uint8_t*) ( WLAN_RAM_STARTING_ADDRESS + PLATFORM_WLAN_RAM_SIZE - 4 ), (uint8_t*) &nvram_size_in_words, 4 );

    return WWD_SUCCESS;
}

#ifdef WWD_TEST_NVRAM_OVERRIDE
wwd_result_t wwd_bus_get_wifi_nvram_image( char** nvram, uint32_t* size)
{
    if (nvram == NULL || size == NULL)
    {
        return WICED_ERROR;
    }

    *nvram = (char *)wifi_nvram_image;
    *size  = sizeof(wifi_nvram_image);

    return WWD_SUCCESS;
}
#endif

static wwd_result_t boot_wlan(void)
{
    /* Load WLAN firmware & NVRAM */
#ifdef MFG_TEST_ALTERNATE_WLAN_DOWNLOAD
    VERIFY_RESULT( external_write_wifi_firmware_and_nvram_image( ) );
#else
    VERIFY_RESULT( wwd_bus_write_wifi_firmware_image( ) );
    VERIFY_RESULT( wwd_bus_write_wifi_nvram_image( ) );
#endif /* ifdef MFG_TEST_ALTERNATE_WLAN_DOWNLOAD */

    /* Release ARM core */
    WPRINT_WWD_INFO(("Release WLAN core..\n"));
    VERIFY_RESULT( wwd_wlan_armcore_run( WLAN_ARM_CORE, WLAN_CORE_FLAG_NONE ) );

    host_rtos_delay_milliseconds( 10 );

    return WWD_SUCCESS;
}

static wwd_result_t shutdown_wlan(void)
{
    /*  disable wlan core  */
    VERIFY_RESULT( wwd_disable_device_core( WLAN_ARM_CORE, WLAN_CORE_FLAG_CPU_HALT ) );

    /* Stop wlan side M2MDMA */
    m2m_wlan_dma_deinit();

    return WWD_SUCCESS;
}

wwd_result_t wwd_bus_set_backplane_window( uint32_t addr )
{
    /* No such thing as a backplane window on 4390 */
    fake_backplane_window_addr = addr & (~((uint32_t)BACKPLANE_ADDRESS_MASK));
    return WWD_SUCCESS;
}



wwd_result_t wwd_bus_write_backplane_value( uint32_t address, uint8_t register_length, uint32_t value )
{
    MEMORY_BARRIER_AGAINST_COMPILER_REORDERING();

    if ( register_length == 4 )
    {
        REGISTER_WRITE_WITH_BARRIER( uint32_t, address, value );
    }
    else if ( register_length == 2 )
    {
        REGISTER_WRITE_WITH_BARRIER( uint16_t, address, value );
    }
    else if ( register_length == 1 )
    {
        REGISTER_WRITE_WITH_BARRIER( uint8_t, address, value );
    }
    else
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wwd_result_t wwd_bus_read_backplane_value( uint32_t address, uint8_t register_length, /*@out@*/ uint8_t* value )
{
    MEMORY_BARRIER_AGAINST_COMPILER_REORDERING();

    if ( register_length == 4 )
    {
        *((uint32_t*)value) = REGISTER_READ( uint32_t, address );
    }
    else if ( register_length == 2 )
    {
        *((uint16_t*)value) = REGISTER_READ( uint16_t, address );
    }
    else if ( register_length == 1 )
    {
        *value = REGISTER_READ( uint8_t, address );
    }
    else
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wwd_result_t wwd_bus_transfer_bytes( wwd_bus_transfer_direction_t direction, wwd_bus_function_t function, uint32_t address, uint16_t size, /*@in@*/ /*@out@*/ wwd_transfer_bytes_packet_t* data )
{
    if ( function != BACKPLANE_FUNCTION )
    {
        wiced_assert( "Only backplane available on 43909", 0 != 0 );
        return WWD_DOES_NOT_EXIST;
    }

    if ( direction == BUS_WRITE )
    {
        memcpy( (uint8_t *)(address + fake_backplane_window_addr), data->data, size );
        if ( address == 0 )
        {
            uint32_t resetinst = *((uint32_t*)data->data);
            write_reset_instruction( resetinst );
        }
    }
    else
    {
        memcpy( data->data, (uint8_t *)(address + fake_backplane_window_addr), size );
    }
    return WWD_SUCCESS;
}

