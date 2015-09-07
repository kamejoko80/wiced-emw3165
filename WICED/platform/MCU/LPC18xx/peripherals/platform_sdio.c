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
 *This implemenration is not complete.
 */
#include "platform_peripheral.h"
#include "chip.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wwd_assert.h"

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
 *                   Variables
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

uint8_t platform_spi_get_port_number( platform_spi_port_t* spi )
{
    UNUSED_PARAMETER (spi );
    return 0;
}

platform_result_t platform_sdio_init( const platform_sdio_t* sdio )
{

    return WICED_SUCCESS;
}

platform_result_t platform_sdio_deinit( const platform_sdio_t* sdio )
{
    /* TODO: unimplemented */
    return PLATFORM_UNSUPPORTED;
}

platform_result_t platform_sdio_transfer( const platform_sdio_t* sdio )
{
    return WICED_SUCCESS;
}

