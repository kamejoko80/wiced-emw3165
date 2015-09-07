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
 * Defines BCM439x filesystem
 */
#include "stdint.h"
#include "string.h"
#include "platform_init.h"
#include "platform_peripheral.h"
#include "platform_mcu_peripheral.h"
#include "platform_stdio.h"
#include "platform_sleep.h"
#include "platform_config.h"
#include "platform_sflash_dct.h"
#include "platform_dct.h"
#include "wwd_constants.h"
#include "wwd_rtos.h"
#include "wwd_assert.h"
#include "RTOS/wwd_rtos_interface.h"
#include "spi_flash.h"
#include "wicedfs.h"
#include "wiced_framework.h"
#include "wiced_dct_common.h"
#include "wiced_apps_common.h"

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

extern uint32_t        sfi_read      ( uint32_t addr, uint32_t len, const uint8_t *buffer )   __attribute__((long_call));
extern uint32_t        sfi_write     ( uint32_t addr, uint32_t len, const uint8_t *buffer )   __attribute__((long_call));
extern uint32_t        sfi_size      ( void )                                                 __attribute__((long_call));
extern void            sfi_erase     ( uint32_t addr, uint32_t len )                          __attribute__((long_call));
static wicedfs_usize_t read_callback ( void* user_param, void* buf, wicedfs_usize_t size, wicedfs_usize_t pos );
extern wiced_result_t platform_read_with_copy    ( void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size );
/******************************************************
 *               Variable Definitions
 ******************************************************/

extern char bcm439x_platform_inited;

host_semaphore_type_t sflash_mutex;
sflash_handle_t       wicedfs_sflash_handle;
wiced_filesystem_t    resource_fs_handle;
static  wiced_app_t   fs_app;
static  uint8_t       fs_init_done = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

void platform_sflash_init( void )
{

    host_rtos_init_semaphore( &sflash_mutex );

    host_rtos_set_semaphore( &sflash_mutex, WICED_FALSE );

}

/* To handle WWD applications that don't go through wiced_init() (yet need to use the file system):
 * File system initialization is called twice
 * wiced_init()->wiced_core_init()->wiced_platform_init()->platform_filesystem_init()
 * wwd_management_init()->wwd_management_wifi_on()->host_platform_init()->platform_filesystem_init()
 */
platform_result_t platform_filesystem_init( void )
{
    int             result;
    sflash_handle_t sflash_handle;

    if ( fs_init_done == 0)
    {
        if ( init_sflash( &sflash_handle, 0, SFLASH_WRITE_ALLOWED ) != WICED_SUCCESS )
        {
            return PLATFORM_ERROR;
        }

        if ( wiced_framework_app_open( DCT_FILESYSTEM_IMAGE_INDEX, &fs_app ) != WICED_SUCCESS )
        {
            return PLATFORM_ERROR;
        }

        result = wicedfs_init( 0, read_callback, &resource_fs_handle, &wicedfs_sflash_handle );
        wiced_assert( "wicedfs init fail", result == 0 );
        fs_init_done = ( result == 0 ) ? 1 : 0;
        return ( result == 0 ) ? PLATFORM_SUCCESS : PLATFORM_ERROR;
    }
    return PLATFORM_SUCCESS;
}

static wicedfs_usize_t read_callback( void* user_param, void* buf, wicedfs_usize_t size, wicedfs_usize_t pos )
{
    wiced_result_t retval;
    (void) user_param;
    retval = wiced_framework_app_read_chunk( &fs_app, pos, (uint8_t*) buf, size );
    return ( ( WICED_SUCCESS == retval ) ? size : 0 );
}

#ifndef SPI_DRIVER_SFLASH
int init_sflash( sflash_handle_t* const handle, void* peripheral_id, sflash_write_allowed_t write_allowed )
{
    UNUSED_PARAMETER( handle );
    UNUSED_PARAMETER( peripheral_id );
    UNUSED_PARAMETER( write_allowed );

    platform_ptu2_enable_serial_flash();

    if ( sfi_size( ) )
    {
        platform_ptu2_disable_serial_flash();
        return 0;
    }

    platform_ptu2_disable_serial_flash();
    return -1;
}

int deinit_sflash( sflash_handle_t* const handle )
{
    UNUSED_PARAMETER( handle );
    return 0;
}

int sflash_read( const sflash_handle_t* const handle, unsigned long device_address, /*@out@*/ /*@dependent@*/ void* data_addr, unsigned int size )
{
    uint32_t num_read;

    UNUSED_PARAMETER( handle );

    if ( bcm439x_platform_inited )
    {
        host_rtos_get_semaphore( &sflash_mutex, NEVER_TIMEOUT, WICED_FALSE );
    }

    platform_ptu2_enable_serial_flash();
    num_read = sfi_read( device_address, size, data_addr );
    platform_ptu2_disable_serial_flash();

    if ( bcm439x_platform_inited )
    {
        host_rtos_set_semaphore( &sflash_mutex, WICED_FALSE );
    }

    return ( num_read == size ) ? 0 : -1;
}

int sflash_write( const sflash_handle_t* const handle, unsigned long device_address, const void* const data_addr, unsigned int size )
{
    uint32_t num_written;

    UNUSED_PARAMETER( handle );

    if ( bcm439x_platform_inited )
    {
        host_rtos_get_semaphore( &sflash_mutex, NEVER_TIMEOUT, WICED_FALSE );
    }

    platform_ptu2_enable_serial_flash();
    num_written = sfi_write( device_address, size, data_addr );
    platform_ptu2_disable_serial_flash();

    if ( bcm439x_platform_inited )
    {
        host_rtos_set_semaphore( &sflash_mutex, WICED_FALSE );
    }
    return ( num_written == size ) ? 0 : -1;
}

int sflash_chip_erase( const sflash_handle_t* const handle )
{
    UNUSED_PARAMETER( handle );

    if ( bcm439x_platform_inited )
    {
        host_rtos_get_semaphore( &sflash_mutex, NEVER_TIMEOUT, WICED_FALSE );
    }

    platform_ptu2_enable_serial_flash();
    sfi_erase( 0, sfi_size( ) );
    platform_ptu2_disable_serial_flash();

    if ( bcm439x_platform_inited )
    {
        host_rtos_set_semaphore( &sflash_mutex, WICED_FALSE );
    }
    return 0;
}

int sflash_sector_erase( const sflash_handle_t* const handle, unsigned long device_address )
{
    UNUSED_PARAMETER( handle );

    if ( bcm439x_platform_inited )
    {
        host_rtos_get_semaphore( &sflash_mutex, NEVER_TIMEOUT, WICED_FALSE );
    }

    platform_ptu2_enable_serial_flash();
    sfi_erase( device_address, 4096 );
    platform_ptu2_disable_serial_flash();

    if ( bcm439x_platform_inited )
    {
        host_rtos_set_semaphore( &sflash_mutex, WICED_FALSE );
    }
    return 0;
}

int sflash_get_size( const sflash_handle_t* const handle, unsigned long* size )
{
    UNUSED_PARAMETER( handle );

//    *size = sfi_size();
    *size = 2 * 1024 * 1024;
    return 0;
}
#endif /* ifndef SPI_DRIVER_SFLASH */

platform_result_t platform_get_sflash_dct_loc( sflash_handle_t* sflash_handle, uint32_t* loc )
{
    UNUSED_PARAMETER( sflash_handle );

    *loc = 0;
    return PLATFORM_SUCCESS;
}
