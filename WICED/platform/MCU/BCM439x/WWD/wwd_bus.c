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
 * Defines BCM439x WWD bus
 */
#include "string.h" /* For memcpy */
#include "wwd_assert.h"
#include "wwd_bus_protocol.h"
#include "wwd_bus_internal.h"
#include "platform/wwd_bus_interface.h"
#include "platform/wwd_sdio_interface.h"
#include "RTOS/wwd_rtos_interface.h"

/******************************************************
 *             Macros
 ******************************************************/

/******************************************************
 *             Constants
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/

/******************************************************
 *             Static Function Declarations
 ******************************************************/

/******************************************************
 *             Function definitions
 ******************************************************/

wwd_result_t host_platform_bus_init( void )
{
    return WWD_SUCCESS;
}

wwd_result_t host_platform_bus_deinit( void )
{
    return WWD_SUCCESS;
}

wwd_result_t host_platform_bus_enable_interrupt( void )
{
    wwd_bus_enable_dma_interrupt( );
    return WWD_SUCCESS;
}

wwd_result_t host_platform_bus_disable_interrupt( void )
{
    wwd_bus_disable_dma_interrupt( );
    return WWD_SUCCESS;
}

void host_platform_bus_buffer_freed( wwd_buffer_dir_t direction )
{
    int prev_active = wwd_bus_get_available_dma_rx_buffer_space( );

    UNUSED_PARAMETER( direction );

    wwd_bus_refill_dma_rx_buffer( );
    if ( ( prev_active == 0 ) && ( wwd_bus_get_available_dma_rx_buffer_space( ) != 0 ) )
    {
        // We will get flooded with interrupts if enabled when dma ring buffer is empty.
        wwd_bus_unmask_dma_interrupt( );
    }
}
