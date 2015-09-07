/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * @file
 *
 * API for accessing SPI flash chips.
 *
 */

#ifndef INCLUDED_SPI_FLASH_API_H
#define INCLUDED_SPI_FLASH_API_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>


struct sflash_capabilities;


typedef enum
{
    SFLASH_WRITE_NOT_ALLOWED = 0,
    SFLASH_WRITE_ALLOWED     = 1,
} sflash_write_allowed_t;


/**
 * Handle for a sflash access instance
 * Users should not access these values - they are provided here only
 * to provide the compiler with datatype size information allowing static declarations
 */
typedef /*@abstract@*/ /*@immutable@*/ struct
{
    uint32_t device_id;
    void * platform_peripheral;
    const struct sflash_capabilities* capabilities;
    sflash_write_allowed_t write_allowed;
} sflash_handle_t;


/**
 *  Initializes a SPI Flash chip
 *
 *  Internally this initializes the associated SPI port, then
 *  reads the chip ID from the SPI flash to determine what type it is.
 *
 * @param[in] handle            Handle structure that will be used for this sflash instance - allocated by caller.
 * @param[in] peripheral_id     An ID value which is passed to the underlying sflash_platform_init function
 * @param[in] write_allowed_in  Determines whether writing will be allowed to this sflash handle
 *
 * @return @ref wiced_result_t
 */
int init_sflash         ( /*@out@*/ sflash_handle_t* const handle, /*@shared@*/ void* peripheral_id, sflash_write_allowed_t write_allowed_in );

/**
 *  De-initializes a SPI Flash chip
 *
 * @param[in] handle            Handle structure that will be used for this sflash instance - allocated by caller.
 *
 * @return @ref wiced_result_t
 */
int deinit_sflash       ( /*@out@*/ sflash_handle_t* const handle);

/**
 *  Reads data from a SPI Flash chip
 *
 * @param[in]  handle            Handle structure that was initialized with @ref init_sflash
 * @param[in]  device_address    The location on the SPI flash where reading will start
 * @param[out] data_addr         Destination buffer in memory that will receive the data
 * @param[in]  size              Number of bytes to read from the chip
 *
 * @return @ref wiced_result_t
 */
int sflash_read         ( const sflash_handle_t* const handle, unsigned long device_address, /*@out@*/  /*@dependent@*/ void* data_addr, unsigned int size );

/**
 *  Write data to a SPI Flash chip
 *
 * @param[in]  handle            Handle structure that was initialized with @ref init_sflash
 * @param[in]  device_address    The location on the SPI flash where writing will start
 * @param[in]  data_addr         Pointer to the buffer in memory that contains the data being written
 * @param[in]  size              Number of bytes to write to the chip
 *
 * @return @ref wiced_result_t
 */
int sflash_write        ( const sflash_handle_t* const handle, unsigned long device_address,  /*@observer@*/ const void* const data_addr, unsigned int size );


/**
 *  Erase the contents of a SPI Flash chip
 *
 * @param[in]  handle            Handle structure that was initialized with @ref init_sflash
 *
 * @return @ref wiced_result_t
 */
int sflash_chip_erase   ( const sflash_handle_t* const handle );


/**
 *  Erase one sector of a SPI Flash chip
 *
 * @param[in]  handle            Handle structure that was initialized with @ref init_sflash
 * @param[in]  device_address    The location on the sflash chip of the first byte of the sector to erase
 *
 * @return @ref wiced_result_t
 */
int sflash_sector_erase ( const sflash_handle_t* const handle, unsigned long device_address );


/**
 *  Erase one sector of a SPI Flash chip
 *
 * @param[in]  handle    Handle structure that was initialized with @ref init_sflash
 * @param[out] size      Variable which will receive the capacity size in bytes of the sflash chip
 *
 * @return @ref wiced_result_t
 */
int sflash_get_size     ( const sflash_handle_t* const handle, /*@out@*/ unsigned long* size );


#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_SPI_FLASH_API_H */
