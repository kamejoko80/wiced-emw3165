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
 * Defines BCM439x WICED application framework functions
 */
#include "waf_platform.h"
#include "wiced_framework.h"
#include "wiced_apps_common.h"
#include "wiced_deep_sleep.h"
#include "wicedfs.h"

/******************************************************
 *                      Macros
 ******************************************************/

/* Application linker script ensures that tiny bootloader binary is starting from the beginning of Always-On memory.
 * Tiny bootloader linker script ensures that its configuration structure is starting from the beginning of Always-On memory.
 */
#define WICED_DEEP_SLEEP_TINY_BOOTLOADER_CONFIG    ( (volatile wiced_deep_sleep_tiny_bootloader_config_t *)PLATFORM_SOCSRAM_CH0_AON_RAM_BASE(0x0) )

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
 *              Function Definitions
 ******************************************************/

/******************************************************
 *              Global Variables
 ******************************************************/

/******************************************************
 *              Function Declarations
  ******************************************************/

/******************************************************
 *                 DCT Functions
 ******************************************************/

void platform_load_app_chunk_from_fs( const image_location_t* app_header_location, void* file_handler, void* physical_address, uint32_t size)
{
    const uint32_t     destination = (uint32_t)physical_address;
    const wiced_bool_t aon_segment = WICED_DEEP_SLEEP_IS_AON_SEGMENT( destination, size ) ? WICED_TRUE : WICED_FALSE;
    WFILE*             stream      = file_handler;

    UNUSED_PARAMETER( app_header_location );

    if ( !aon_segment || !platform_mcu_powersave_is_warmboot() )
    {
        wicedfs_fread( physical_address, size, 1, stream );

        if ( aon_segment )
        {
            WICED_DEEP_SLEEP_TINY_BOOTLOADER_CONFIG->app_elf_fs_address = stream->location;
        }
    }
}

void platform_erase_app_area( uint32_t physical_address, uint32_t size )
{
    /* App is in RAM, no need for erase */
    (void) physical_address;
    (void) size;
}

void platform_load_app_chunk(const image_location_t* app_header_location, uint32_t offset, void* physical_address, uint32_t size)
{
    wiced_apps_read( app_header_location, physical_address, offset, size);
}
