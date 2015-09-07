/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include <stdint.h>
#include "wiced_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct spi_slave_host
{
    const wiced_spi_device_t*            device;
    wiced_bool_t                         in_transaction;
    wiced_gpio_t                         data_ready_pin;
    wiced_semaphore_t                    data_ready_semaphore;
    void                                 (*data_ready_callback)( struct spi_slave_host* host );
} spi_master_t;

typedef void (*spi_master_data_ready_callback_t)( spi_master_t* master );

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t spi_master_init          ( spi_master_t* master, const wiced_spi_device_t* device, wiced_gpio_t data_ready_pin, spi_master_data_ready_callback_t data_ready_callback );
wiced_result_t spi_master_write_register( spi_master_t* master, uint16_t address, uint16_t size, uint8_t* data_out );
wiced_result_t spi_master_read_register ( spi_master_t* master, uint16_t address, uint16_t size, uint8_t* data_in  );

#ifdef __cplusplus
} /* extern "C" */
#endif
