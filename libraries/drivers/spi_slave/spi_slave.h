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

typedef enum
{
    SPI_SLAVE_REGISTER_DATA_STATIC,
    SPI_SLAVE_REGISTER_DATA_DYNAMIC
} spi_slave_register_data_type_t;

typedef enum
{
    SPI_SLAVE_ACCESS_READ_ONLY,
    SPI_SLAVE_ACCESS_WRITE_ONLY,
    SPI_SLAVE_ACCESS_READ_WRITE
} spi_slave_access_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * SPI slave device
 */
typedef struct
{
    const struct spi_slave_device_config* config;
    wiced_thread_t                        thread;
    wiced_spi_slave_data_buffer_t*        buffer;
    uint32_t                              buffer_size;
    wiced_bool_t                          quit;
} spi_slave_t;

/**
 * SPI slave device configuration
 */
typedef struct spi_slave_device_config
{
    wiced_spi_t                      spi;
    wiced_spi_slave_config_t         config;
    const struct spi_slave_register* register_list;
    uint32_t                         register_count;
} spi_slave_device_config_t;

typedef wiced_result_t (*spi_slave_register_callback_t)( spi_slave_t* device, uint8_t* buffer );

/**
 * SPI slave register structure
 */
typedef struct spi_slave_register
{
    uint16_t                       address;
    spi_slave_access_t             access;
    spi_slave_register_data_type_t data_type;
    uint16_t                       data_length;
    uint8_t*                       static_data;
    spi_slave_register_callback_t  read_callback;
    spi_slave_register_callback_t  write_callback;
} spi_slave_register_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t spi_slave_init  ( spi_slave_t* device, const spi_slave_device_config_t* config );
wiced_result_t spi_slave_deinit( spi_slave_t* device );

#ifdef __cplusplus
} /* extern "C" */
#endif
