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
 * Manages DCT writing to external flash
 */
#include "string.h"
#include "wwd_assert.h"
#include "wiced_result.h"
#include "spi_flash.h"
#include "platform_dct.h"
#include "waf_platform.h"
#include "wiced_framework.h"
#include "wiced_dct_common.h"
#include "wiced_apps_common.h"
#include "elf.h"

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

static void wiced_dct_erase_copy( const sflash_handle_t* const sflash_handle, char copy_num );

/******************************************************
 *               Variables Definitions
 ******************************************************/
static const uint32_t DCT_section_offsets[ ] =
{
    [DCT_APP_SECTION]           = sizeof(platform_dct_data_t),
    [DCT_SECURITY_SECTION]      = OFFSETOF( platform_dct_data_t, security_credentials ),
    [DCT_MFG_INFO_SECTION]      = OFFSETOF( platform_dct_data_t, mfg_info ),
    [DCT_WIFI_CONFIG_SECTION]   = OFFSETOF( platform_dct_data_t, wifi_config ),
    [DCT_ETHERNET_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, ethernet_config ),
    [DCT_NETWORK_CONFIG_SECTION]  = OFFSETOF( platform_dct_data_t, network_config ),
#ifdef WICED_DCT_INCLUDE_BT_CONFIG
    [DCT_BT_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, bt_config ),
#endif
    [DCT_INTERNAL_SECTION]      = 0,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void* wiced_dct_get_current_address( dct_section_t section )
{
    uint32_t sector_num;

    const platform_dct_header_t hdr =
    {
        .write_incomplete    = 0,
        .is_current_dct      = 1,
        .app_valid           = 1,
        .mfg_info_programmed = 0,
        .magic_number        = BOOTLOADER_MAGIC_NUMBER,
        .load_app_func       = NULL
    };

    platform_dct_header_t  dct1_val;
    platform_dct_header_t  dct2_val;
    platform_dct_header_t* dct1 = &dct1_val;
    platform_dct_header_t* dct2 = &dct2_val;
    sflash_handle_t        sflash_handle;

    if ( init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_NOT_ALLOWED ) != 0 )
    {
        return GET_CURRENT_ADDRESS_FAILED;
    }
    sflash_read( &sflash_handle, PLATFORM_DCT_COPY1_START_ADDRESS, dct1, sizeof(platform_dct_header_t) );
    sflash_read( &sflash_handle, PLATFORM_DCT_COPY2_START_ADDRESS, dct2, sizeof(platform_dct_header_t) );

    if ( ( dct1->is_current_dct == 1 ) &&
         ( dct1->write_incomplete == 0 ) &&
         ( dct1->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        deinit_sflash( &sflash_handle );
        return (void*) ( PLATFORM_DCT_COPY1_START_ADDRESS + DCT_section_offsets[ section ] );
    }

    if ( ( dct2->is_current_dct == 1 ) &&
         ( dct2->write_incomplete == 0 ) &&
         ( dct2->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        deinit_sflash( &sflash_handle );
        return (void*) ( PLATFORM_DCT_COPY2_START_ADDRESS + DCT_section_offsets[ section ] );
    }

    /* No valid DCT! */
    /* Erase the first DCT and init it. */

    wiced_assert("BOTH DCTs ARE INVALID!", 0 != 0 );

    for ( sector_num = PLATFORM_DCT_COPY1_START_SECTOR; sector_num < PLATFORM_DCT_COPY1_END_SECTOR; sector_num++ )
    {
        sflash_sector_erase( &sflash_handle, sector_num );
    }

    sflash_write( &sflash_handle, PLATFORM_DCT_COPY1_START_ADDRESS, (uint8_t*) &hdr, sizeof( hdr ) );

    deinit_sflash( &sflash_handle );
    return (void*) ( PLATFORM_DCT_COPY1_START_ADDRESS + DCT_section_offsets[ section ] );
}

wiced_result_t wiced_dct_read_with_copy( void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size )
{
    uint32_t curr_dct = (uint32_t) wiced_dct_get_current_address( section );

    sflash_handle_t sflash_handle;
    if ( curr_dct != WICED_ERROR )
    {
        init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_NOT_ALLOWED );

        sflash_read( &sflash_handle, curr_dct + offset, info_ptr, size );
        deinit_sflash( &sflash_handle);
    }

    return WICED_SUCCESS;
}

static void copy_sflash( const sflash_handle_t* sflash_handle, uint32_t dest_loc, uint32_t src_loc, uint32_t size )
{
    char buff[ 64 ];

    while ( size > 0 )
    {
        uint32_t write_size = MIN( sizeof(buff), size);
        sflash_read( sflash_handle, src_loc, (unsigned char*)buff, write_size );
        sflash_write( sflash_handle, dest_loc, (unsigned char*)buff, write_size );

        src_loc  += write_size;
        dest_loc += write_size;
        size     -= write_size;
    }
}

static void erase_dct_copy( const sflash_handle_t* const sflash_handle, char copy_num )
{
    uint32_t sector_num;
    uint32_t start = ( copy_num == 1 ) ? PLATFORM_DCT_COPY1_START_SECTOR : PLATFORM_DCT_COPY2_START_SECTOR;
    uint32_t end   = ( copy_num == 1 ) ? PLATFORM_DCT_COPY1_END_SECTOR : PLATFORM_DCT_COPY2_END_SECTOR;

    for ( sector_num = start; sector_num < end; sector_num++ )
    {
        sflash_sector_erase( sflash_handle, sector_num * 4096 );
    }
}

/* Updates the boot part of the DCT header only.Currently platform_dct_write can't update header.
 * The function should be removed once platform_dct_write if fixed to write the header part as well.
 */
static int wiced_dct_update_boot( const boot_detail_t* boot_detail )
{
    uint32_t               new_dct;
    sflash_handle_t        sflash_handle;
    platform_dct_header_t  curr_dct_header;
    char                   zero_byte = 0;
    platform_dct_header_t* curr_dct  = &( (platform_dct_data_t*) wiced_dct_get_current_address( DCT_INTERNAL_SECTION ) )->dct_header;

    uint32_t bytes_after_header = ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) - ( sizeof(platform_dct_header_t) );
    platform_dct_header_t hdr =
    {
        .write_incomplete   = 1,
        .is_current_dct     = 1,
        .magic_number       = BOOTLOADER_MAGIC_NUMBER
    };

    /* initialise the serial flash */
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );

    /* Erase the non-current DCT */
    if ( curr_dct == ( (platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        new_dct = PLATFORM_DCT_COPY2_START_ADDRESS;
        wiced_dct_erase_copy( &sflash_handle, 2 );
    }
    else
    {
        new_dct = PLATFORM_DCT_COPY1_START_ADDRESS;
        wiced_dct_erase_copy( &sflash_handle, 1 );
    }

    /* Write every thing other than the header */
    copy_sflash( &sflash_handle, new_dct + sizeof(platform_dct_header_t), (uint32_t) curr_dct + sizeof(platform_dct_header_t), bytes_after_header );

    /* read the header from the old DCT */
    sflash_read( &sflash_handle, (uint32_t) curr_dct, &curr_dct_header, sizeof( curr_dct_header ) );

    hdr.full_size           = curr_dct_header.full_size;
    hdr.used_size           = curr_dct_header.used_size;
    hdr.app_valid           = curr_dct_header.app_valid;
    hdr.load_app_func       = curr_dct_header.load_app_func;
    hdr.mfg_info_programmed = curr_dct_header.mfg_info_programmed;
    hdr.boot_detail         = *boot_detail;
    memcpy( hdr.apps_locations, curr_dct_header.apps_locations, sizeof(image_location_t) * DCT_MAX_APP_COUNT );

    /* Write the new DCT header data */
    sflash_write( &sflash_handle, new_dct, &hdr, sizeof( hdr ) );

    /* Mark new DCT as complete and current */
    sflash_write( &sflash_handle, new_dct + OFFSETOF(platform_dct_header_t,write_incomplete), &zero_byte, sizeof( zero_byte ) );
    sflash_write( &sflash_handle, (unsigned long) curr_dct + OFFSETOF(platform_dct_header_t,is_current_dct), &zero_byte, sizeof( zero_byte ) );

    deinit_sflash( &sflash_handle );
    return 0;
}

static int wiced_dct_update_app_header_locations( uint32_t offset, const image_location_t *new_app_location, uint8_t count )
{
    uint32_t               new_dct;
    sflash_handle_t        sflash_handle;
    platform_dct_header_t  curr_dct_header;
    char                   zero_byte          = 0;
    platform_dct_header_t* curr_dct           = &( (platform_dct_data_t*) wiced_dct_get_current_address( DCT_INTERNAL_SECTION ) )->dct_header;
    uint32_t               bytes_after_header = ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) - ( sizeof(platform_dct_header_t) );
    platform_dct_header_t  hdr =
    {
        .write_incomplete   = 1,
        .is_current_dct     = 1,
        .magic_number       = BOOTLOADER_MAGIC_NUMBER
    };

    /* initialise the serial flash */
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );

    /* Erase the non-current DCT */
    if ( curr_dct == ( (platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        new_dct = PLATFORM_DCT_COPY2_START_ADDRESS;
        wiced_dct_erase_copy( &sflash_handle, 2 );
    }
    else
    {
        new_dct = PLATFORM_DCT_COPY1_START_ADDRESS;
        wiced_dct_erase_copy( &sflash_handle, 1 );
    }

    /* Write every thing other than the header */
    copy_sflash( &sflash_handle, new_dct + sizeof(platform_dct_header_t), (uint32_t) curr_dct + sizeof(platform_dct_header_t), bytes_after_header );

    /* read the header from the old DCT */
    sflash_read( &sflash_handle, (uint32_t) curr_dct, &curr_dct_header, sizeof( curr_dct_header ) );

    hdr.full_size           = curr_dct_header.full_size;
    hdr.used_size           = curr_dct_header.used_size;
    hdr.app_valid           = curr_dct_header.app_valid;
    hdr.load_app_func       = curr_dct_header.load_app_func;
    hdr.mfg_info_programmed = curr_dct_header.mfg_info_programmed;

    memcpy( &hdr.boot_detail, &curr_dct_header.boot_detail, sizeof(boot_detail_t) );
    memcpy( hdr.apps_locations, curr_dct_header.apps_locations, sizeof(image_location_t) * DCT_MAX_APP_COUNT );
    memcpy( ( (uint8_t*) &hdr ) + offset, new_app_location, sizeof(image_location_t) * count );

    /* Write the new DCT header data */
    sflash_write( &sflash_handle, new_dct, &hdr, sizeof( hdr ) );

    /* Mark new DCT as complete and current */
    sflash_write( &sflash_handle, new_dct + OFFSETOF(platform_dct_header_t,write_incomplete), &zero_byte, sizeof( zero_byte ) );
    sflash_write( &sflash_handle, (unsigned long) curr_dct + OFFSETOF(platform_dct_header_t,is_current_dct), &zero_byte, sizeof( zero_byte ) );

    deinit_sflash( &sflash_handle );
    return 0;
}

static int wiced_write_dct( uint32_t data_start_offset, const void* data, uint32_t data_length, int8_t app_valid, void(*func)( void ) )
{
    uint32_t               new_dct;
    platform_dct_header_t  curr_dct_header;
    uint32_t               bytes_after_data;
    platform_dct_header_t* curr_dct           = &( (platform_dct_data_t*) wiced_dct_get_current_address( DCT_INTERNAL_SECTION ) )->dct_header;
    char                   zero_byte          = 0;
    sflash_handle_t        sflash_handle;
    platform_dct_header_t hdr =
    {
        .write_incomplete = 1,
        .is_current_dct   = 1,
        .magic_number     = BOOTLOADER_MAGIC_NUMBER
    };

    /* Check if the data is too big to write */
    if ( data_length + data_start_offset > ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        return -1;
    }

    /* initialise the serial flash */
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );

    /* Erase the non-current DCT */
    if ( curr_dct == (platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS )
    {
        new_dct = PLATFORM_DCT_COPY2_START_ADDRESS;
        erase_dct_copy( &sflash_handle, 2 );
    }
    else
    {
        new_dct = PLATFORM_DCT_COPY1_START_ADDRESS;
        erase_dct_copy( &sflash_handle, 1 );
    }

    /* Write the mfg data and initial part of app data before provided data */
    copy_sflash( &sflash_handle, new_dct + sizeof(platform_dct_header_t), (uint32_t) curr_dct + sizeof(platform_dct_header_t), (unsigned long) data_start_offset - sizeof(platform_dct_header_t) );

    /* Write the app data */
    sflash_write( &sflash_handle, new_dct + data_start_offset, data, data_length );

    /* Calculate how many bytes need to be written after the end of the requested data write */
    bytes_after_data = ( PLATFORM_DCT_COPY1_SIZE ) - ( data_start_offset + data_length );

    if ( bytes_after_data != 0 )
    {
        /* There is data after end of requested write - copy it from old DCT to new DCT */
        copy_sflash( &sflash_handle, new_dct + data_start_offset + data_length, (uint32_t) curr_dct + data_start_offset + data_length, bytes_after_data );
    }

    /* read the header from the old DCT */
    sflash_read( &sflash_handle, (uint32_t) curr_dct, (unsigned char*)&curr_dct_header, sizeof(curr_dct_header) );

    /* Copy values from old DCT header to new DCT header */
    hdr.full_size = curr_dct_header.full_size;
    hdr.used_size = curr_dct_header.used_size;

    memcpy( &hdr.boot_detail, &curr_dct_header.boot_detail, sizeof(boot_detail_t) );
    memcpy( hdr.apps_locations, curr_dct_header.apps_locations, sizeof(image_location_t) * DCT_MAX_APP_COUNT );
    hdr.app_valid = (char) ( ( app_valid == -1 ) ? curr_dct_header.app_valid : app_valid );
    hdr.mfg_info_programmed = curr_dct_header.mfg_info_programmed;

    /* If a new bootload startup function has been requested, set it */
    if ( func )
    {
        hdr.load_app_func = func;
    }
    else
    {
        hdr.load_app_func = curr_dct_header.load_app_func;
    }

    /* Write the new DCT header data */
    sflash_write( &sflash_handle, new_dct, &hdr, sizeof( hdr ) );

    /* Mark new DCT as complete and current */
    sflash_write( &sflash_handle, new_dct + OFFSETOF(platform_dct_header_t,write_incomplete), &zero_byte, sizeof( zero_byte ) );
    sflash_write( &sflash_handle, (unsigned long) curr_dct + OFFSETOF(platform_dct_header_t,is_current_dct), &zero_byte, sizeof( zero_byte ) );

    deinit_sflash( &sflash_handle );
    return 0;
}

wiced_result_t wiced_dct_update( const void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size )
{
    int retval;
    if ( ( section == DCT_INTERNAL_SECTION ) && ( offset == OFFSETOF( platform_dct_header_t, boot_detail ) ) && ( size == sizeof(boot_detail_t) ) )
    {
        retval = wiced_dct_update_boot( info_ptr );
    }
    else if ( ( section == DCT_INTERNAL_SECTION ) && ( offset >= OFFSETOF( platform_dct_header_t, apps_locations ) ) && ( offset < OFFSETOF( platform_dct_header_t, apps_locations ) + sizeof(image_location_t) * DCT_MAX_APP_COUNT ) && ( size == sizeof(image_location_t) ) )
    {
        retval = wiced_dct_update_app_header_locations( offset, info_ptr, 1 );
    }
    else
    {
        retval = wiced_write_dct( DCT_section_offsets[ section ] + offset, info_ptr, size, 1, NULL );
    }
    return ( retval == 0 ) ? WICED_SUCCESS : WICED_ERROR;
}

static wiced_result_t wiced_dct_check_apps_locations_valid( image_location_t* app_header_locations )
{
    if (   ( /* No FR APP */                    app_header_locations[DCT_FR_APP_INDEX].id  != EXTERNAL_FIXED_LOCATION )
        || ( /* FR App address incorrect */     app_header_locations[DCT_FR_APP_INDEX].detail.external_fixed.location != (SFLASH_APPS_HEADER_LOC + sizeof(app_header_t) * DCT_FR_APP_INDEX))
        || ( /* No FR DCT */                    app_header_locations[DCT_DCT_IMAGE_INDEX].id  != EXTERNAL_FIXED_LOCATION )
        || ( /* DCT address is incorrect */     app_header_locations[DCT_DCT_IMAGE_INDEX].detail.external_fixed.location != (SFLASH_APPS_HEADER_LOC + sizeof(app_header_t) * DCT_DCT_IMAGE_INDEX))
        )
    {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

static wiced_result_t wiced_dct_load( const image_location_t* dct_location )
{
    sflash_handle_t      sflash_handle;
    elf_header_t         header;
    elf_program_header_t prog_header;
    uint32_t             size;
    uint32_t             offset;
    uint32_t             dest_loc = PLATFORM_DCT_COPY1_START_ADDRESS;
    uint8_t              buff[64];

    /* initialize the serial flash */
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );

    /* Erase the application area */
    wiced_dct_erase_copy( &sflash_handle, 1 );

    /* Read the image header */
    wiced_apps_read( dct_location, (uint8_t*) &header, 0, sizeof( header ) );

    if ( header.program_header_entry_count != 1 )
    {
        deinit_sflash( &sflash_handle );
        return WICED_ERROR;
    }

    wiced_apps_read( dct_location, (uint8_t*) &prog_header, header.program_header_offset, sizeof( prog_header ) );

    size   = prog_header.data_size_in_file;
    offset = prog_header.data_offset;

    while ( size > 0 )
    {
        uint32_t write_size = MIN( sizeof(buff), size);
        wiced_apps_read( dct_location, buff, offset, write_size);
        sflash_write( &sflash_handle, dest_loc, buff, write_size );

        offset   += write_size;
        dest_loc += write_size;
        size     -= write_size;
    }

    deinit_sflash( &sflash_handle );
    return WICED_SUCCESS;
}

wiced_result_t wiced_dct_restore_factory_reset( void )
{
    uint8_t          apps_count = DCT_MAX_APP_COUNT;
    image_location_t app_header_locations[ DCT_MAX_APP_COUNT ];

    wiced_dct_read_with_copy( app_header_locations, DCT_INTERNAL_SECTION, DCT_APP_LOCATION_OF(DCT_FR_APP_INDEX), sizeof(image_location_t)* DCT_MAX_APP_COUNT );
    if (wiced_dct_check_apps_locations_valid ( app_header_locations ) == WICED_ERROR )
    {
        /* DCT was corrupted. Restore only FR Application address */
        app_header_locations[DCT_FR_APP_INDEX].id                                = EXTERNAL_FIXED_LOCATION;
        app_header_locations[DCT_FR_APP_INDEX].detail.external_fixed.location    = SFLASH_APPS_HEADER_LOC + sizeof(app_header_t) * DCT_FR_APP_INDEX;
        app_header_locations[DCT_DCT_IMAGE_INDEX].id                             = EXTERNAL_FIXED_LOCATION;
        app_header_locations[DCT_DCT_IMAGE_INDEX].detail.external_fixed.location = SFLASH_APPS_HEADER_LOC + sizeof(app_header_t) * DCT_DCT_IMAGE_INDEX;
        apps_count = 2;
    }
    /* OK Current DCT seems decent, lets keep apps locations. */
    wiced_dct_load( &app_header_locations[DCT_DCT_IMAGE_INDEX] );
    wiced_dct_update_app_header_locations( OFFSETOF( platform_dct_header_t, apps_locations ), app_header_locations, apps_count );

    return WICED_SUCCESS;
}

wiced_result_t wiced_dct_get_app_header_location( uint8_t app_id, image_location_t* app_header_location )
{
    if ( app_id >= DCT_MAX_APP_COUNT )
    {
        return WICED_ERROR;
    }
    return wiced_dct_read_with_copy( app_header_location, DCT_INTERNAL_SECTION, DCT_APP_LOCATION_OF(app_id), sizeof(image_location_t) );
}

wiced_result_t wiced_dct_set_app_header_location( uint8_t app_id, image_location_t* app_header_location )
{
    if ( app_id >= DCT_MAX_APP_COUNT )
    {
        return WICED_ERROR;
    }
    return wiced_dct_update( app_header_location, DCT_INTERNAL_SECTION, DCT_APP_LOCATION_OF(app_id), sizeof(image_location_t) );
}

static void wiced_dct_erase_copy( const sflash_handle_t* const sflash_handle, char copy_num )
{
    uint32_t sector_num;
    uint32_t start = ( copy_num == 1 ) ? PLATFORM_DCT_COPY1_START_SECTOR : PLATFORM_DCT_COPY2_START_SECTOR;
    uint32_t end = ( copy_num == 1 ) ? PLATFORM_DCT_COPY1_END_SECTOR : PLATFORM_DCT_COPY2_END_SECTOR;

    for ( sector_num = start; sector_num < end; sector_num++ )
    {
        sflash_sector_erase( sflash_handle, sector_num * 4096 );
    }
}
