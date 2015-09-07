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
 * Defines STM32F4xx filesystem
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

static wicedfs_usize_t read_callback ( void* user_param, void* buf, wicedfs_usize_t size, wicedfs_usize_t pos );

/******************************************************
 *               Variable Definitions
 ******************************************************/

sflash_handle_t       wicedfs_sflash_handle;
wiced_filesystem_t    resource_fs_handle;
static wiced_app_t    fs_app;
static  uint8_t       fs_init_done = 0;
/******************************************************
 *               Function Definitions
 ******************************************************/
/* To handle WWD applications that don't go through wiced_init() (yet need to use the file system):
 * File system initialization is called twice
 * wiced_init()->wiced_core_init()->wiced_platform_init()->platform_filesystem_init()
 * wwd_management_init()->wwd_management_wifi_on()->host_platform_init()->platform_filesystem_init()
 */
platform_result_t platform_filesystem_init( void )
{
    int              result;
    sflash_handle_t  sflash_handle;
    if ( fs_init_done == 0)
    {
        init_sflash( &sflash_handle, 0, SFLASH_WRITE_ALLOWED );
        if ( wiced_framework_app_open( DCT_FILESYSTEM_IMAGE_INDEX, &fs_app ) != WICED_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
        result = wicedfs_init( 0, read_callback, &resource_fs_handle, &wicedfs_sflash_handle );
        wiced_assert( "wicedfs init fail", result == 0 );
        fs_init_done = ( result == 0 ) ? 1 : 0;
        return (result == 0)? PLATFORM_SUCCESS : PLATFORM_ERROR;
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
