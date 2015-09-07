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
 * Manages writing applications to flash(internal/external)
 */

/*
 * Concept: WICED supports a fixed number of applications to be written on it.
 * The word applications may be a bit confusing, it is binary files. Some of
 * These files are applications that can be loaded, while other are files used
 * for other purposes, these include file system image, wifi firmware, BT firm
 * ware, etc. Following is a description of the very tiny file system that can
 * handle these binaries and yet allow them to be updated.
 *
 * Operations supported:
 * Erase App: An app (or binary file in general) needs to be erased before it can
 * be rewritten.
 *
 * Write File: Once an app is erased it can be written, a new file can be uploaded
 * to the location of the file. The new file doesn't need to be the same size, it
 * can be bigger size (details on how to handle this will come later)
 *
 * Close File: Once a file is written, it must be closed. This will update the
 * entries in the app file system to use the new file.
 *
 * Read File: An app can always be read.
 *
 * Design:
 * App header locations: The DCT contains a list of the application locations that
 * WICED will support. At this point, it includes 8 blobs (Factory reset App, OTA
 * upgrade app, APP0, resources file system, Wifi firmware, APP1, APP2, APP3).
 * Each entry in the DCT points not the app location, but to the location of its
 * description within a log block (this is 1 block assigned for logging the apps).
 *
 * Log Block: The log block stores the apps locations. Apps are located with a
 * granulaity of a block. each app location has the following data:
 * - Number of entry for the app
 * - Entries : Each entry is a start block and count, indicated a consequetive list
 *             of blocks with for the app.
 * As long as the new file is covered by these entries, no update is needed for the
 * app header. Once a new block is needed to be written A new header will be added
 * at the ended of the log block and the DCT location for the app header will be
 * updated. This is done in the close operation.
 *
 * Erasing a file: Once an erase command is issued, the header address is obtained
 * from the DCT, the header is read and all entries are erase.
 *
 * Writing to a file: Before a file is written it has to be erased. As long as the
 * writing offset and size is within the entries of the app, the writing is done
 * with no change. Once the writing exceeds the already assigned sizes, the next
 * empty block is going to be written wiht the new data. Writing will continue to
 * the next blocks as required (the number of the first block and the count of blocks
 * will be stored in ram for update)
 *
 * Closing a file: When closing a file, if no new entries is added, then nothing to be
 * done. If however a new entry was needed, then the program header is read and updated
 * with a new entry and written as the end of the log block, DCT app locations is updated
 * to point to the new app header location.
 *
 * Reading of a file: When a file is read, the offset is mapped to the actual address in the
 * flash:
 *      Physical address = entry physical adddress + (offset - entry file offset)
 *      Entry file offset = Count of previous blocks * size of block
 *      Entry physical address = Entry * size of block
 *
 */
#include "wwd_assert.h"
#include "wiced_result.h"
#include "spi_flash.h"
#include "platform_dct.h"
#include "wiced_apps_common.h"
#include "wiced_waf_common.h"
#include "wiced_dct_common.h"
#include "wiced_utilities.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define PLATFORM_SFLASH_PERIPHERAL_ID  ( 0 )
#define SFLASH_SECTOR_SIZE             ( 4096UL )
#define FIRST_APP_HEADER_ADDRESS       ( 0x20000UL )
#define LAST_APP_HEADER_ADDRESS        ( FIRST_APP_HEADER_ADDRESS + SFLASH_SECTOR_SIZE - sizeof(app_header_t) )

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
static wiced_result_t wiced_apps_get_physical_address( app_header_t *app_header, uint32_t offset, uint32_t* address, uint32_t* size );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_apps_get_size( const image_location_t* app_header_location, uint32_t* size )
{
    sflash_handle_t sflash_handle;
    app_header_t    app_header;
    uint8_t         index;

    /* Loop on each entry and erase the sectors*/
    *size = 0;
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
    sflash_read( &sflash_handle, app_header_location->detail.external_fixed.location, &app_header, sizeof(app_header_t) );
    for ( index = 0; index < app_header.count; index++ )
    {
        *size = *size + app_header.sectors[ index ].count;
    }
    *size = *size * SFLASH_SECTOR_SIZE;

    deinit_sflash( &sflash_handle);
    return WICED_SUCCESS;
}

/*
 * Returns the last (+1) sector after the application in the sflash
 */
static uint16_t wiced_apps_find_last_sector( image_location_t* app_header_location )
{
    sflash_handle_t sflash_handle;
    app_header_t    app_header;
    uint16_t        next_sector = 0;
    uint8_t         index;

    sflash_read( &sflash_handle, app_header_location->detail.external_fixed.location, &app_header, sizeof(app_header_t) );
    for ( index = 0; index < app_header.count; index++ )
    {
        uint16_t sector = (uint16_t) ( app_header.sectors[ index ].start + app_header.sectors[ index ].count );
        if ( sector > next_sector )
        {
            next_sector = sector;
        }
    }
    return next_sector;
}
/*
 * Returns an empty entry in the apps lookup table section.
 * It also returns an empty sector to write data to.
 * The function was written to handle both, to avoid re reading the DCT Lookup table twice.
 */
static wiced_result_t wiced_apps_find_empty_location( image_location_t* new_header_location, uint16_t* new_sector_location )
{
    uint8_t          index;
    image_location_t existing_header_location;
    sflash_handle_t  sflash_handle;

    new_header_location->id = EXTERNAL_FIXED_LOCATION;
    new_header_location->detail.external_fixed.location = 0;
    *new_sector_location = 0;

    for ( index = 0; index < DCT_MAX_APP_COUNT; index++ )
    {
        WICED_VERIFY( wiced_dct_get_app_header_location( index, &existing_header_location ) );
        if ( existing_header_location.id == EXTERNAL_FIXED_LOCATION )
        {
            uint16_t app_last_sector;

            init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
            app_last_sector = wiced_apps_find_last_sector( &existing_header_location );
            deinit_sflash( &sflash_handle );
            if ( existing_header_location.detail.external_fixed.location >= new_header_location->detail.external_fixed.location )
            {
                new_header_location->detail.external_fixed.location = existing_header_location.detail.external_fixed.location + sizeof(app_header_t);
            }
            if ( app_last_sector > *new_sector_location )
            {
                *new_sector_location = app_last_sector;
            }
        }
    }
    /* printf( " final address 0x%x \n", (unsigned int) new_header_location->detail.external_fixed.location ); */
    if ( new_header_location->detail.external_fixed.location > LAST_APP_HEADER_ADDRESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_apps_set_size( image_location_t* app_header_location, uint32_t size )
{
    sflash_handle_t sflash_handle;
    app_header_t    app_header;
    uint8_t         index;
    uint32_t        current_size = 0;
    uint16_t        empty_sector;
    uint32_t        flash_memory_size;
    uint32_t        available_sectors;
    uint32_t        sectors_needed;

    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
    sflash_read( &sflash_handle, app_header_location->detail.external_fixed.location, &app_header, sizeof(app_header_t) );
    for ( index = 0; index < app_header.count; index++ )
    {
        current_size += app_header.sectors[ index ].count;
    }
    current_size *= SFLASH_SECTOR_SIZE;
    if ( current_size >= size )
    {
        deinit_sflash( &sflash_handle);
        return WICED_SUCCESS;
    }

    current_size   = ( size - current_size );
    sectors_needed = current_size / SFLASH_SECTOR_SIZE;
    sectors_needed = ( current_size % SFLASH_SECTOR_SIZE ) ? ( sectors_needed + 1 ) : ( sectors_needed );

    deinit_sflash( &sflash_handle);
    if ( wiced_apps_find_empty_location( app_header_location, &empty_sector ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    sflash_get_size( &sflash_handle, &flash_memory_size );
    available_sectors = ( flash_memory_size / SFLASH_SECTOR_SIZE ) - empty_sector;

    /* Check whether number of sectors which are empty is more that the number of sectors required for an application */
    if ( sectors_needed > available_sectors )
    {
        wiced_assert("Flash memory doesn't have enough space for this application update", 0!=0);
        /* Application with requested size wont fit into flash memory chip */
        return WICED_ERROR;
    }

    app_header.sectors[ app_header.count ].start   = empty_sector;
    app_header.sectors[ app_header.count++ ].count = (uint16_t) sectors_needed;
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
    /* printf("sectory count %d %d %d\n", app_header.count, app_header.sectors[ app_header.count-1 ].start, app_header.sectors[ app_header.count-1 ].count); */
    sflash_write( &sflash_handle, app_header_location->detail.external_fixed.location, &app_header, sizeof(app_header_t) );
    sflash_read( &sflash_handle, app_header_location->detail.external_fixed.location, &app_header, sizeof(app_header_t) );
    /* printf("sectory count %d %d %d\n", app_header.count, app_header.sectors[ app_header.count-1 ].start, app_header.sectors[ app_header.count-1 ].count); */

    deinit_sflash( &sflash_handle);
    return WICED_SUCCESS;
}

wiced_result_t wiced_apps_erase( const image_location_t* app_header_location )
{
    sflash_handle_t sflash_handle;
    app_header_t    app_header;
    uint8_t         index;

    /* Loop on each entry and erase the sectors*/
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
    sflash_read( &sflash_handle, app_header_location->detail.external_fixed.location, &app_header, sizeof(app_header_t) );
    for ( index = 0; index < app_header.count; index++ )
    {
        unsigned long sector;
        for ( sector = 0; sector < app_header.sectors[ index ].count; sector++ )
        {
            sflash_sector_erase( &sflash_handle, ( app_header.sectors[ index ].start + sector ) * SFLASH_SECTOR_SIZE );
        }
    }
    deinit_sflash( &sflash_handle);
    return WICED_SUCCESS;
}

static wiced_result_t wiced_apps_get_physical_address( app_header_t *app_header, uint32_t offset, uint32_t* address, uint32_t* size )
{
    uint8_t  index;
    uint32_t current_offset = 0;

    for ( index = 0; index < app_header->count; index++ )
    {
        uint32_t current_size = app_header->sectors[ index ].count * SFLASH_SECTOR_SIZE;
        if ( ( offset >= current_offset ) && ( offset < ( current_offset + current_size ) ) )
        {
            uint32_t diff = offset - current_offset;
            *address = app_header->sectors[ index ].start * SFLASH_SECTOR_SIZE + diff;
            *size = current_size - diff;
            return WICED_SUCCESS;
        }
        current_offset += app_header->sectors[ index ].count * SFLASH_SECTOR_SIZE;
    }
    return WICED_ERROR;
}

static uint32_t wiced_apps_erase_sections( uint32_t offset, uint32_t size, uint32_t last_erased_sector )
{
    sflash_handle_t sflash_handle;
    uint32_t        sector_address;
    uint32_t        start_sector_address, end_sector_address;
    const uint32_t  sector_size = 0x1000;

    start_sector_address = ( offset ) & ~( sector_size - 1 );
    end_sector_address   = ( offset + size -1 ) & ~( sector_size - 1 );
    sector_address       = start_sector_address;
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
    while ( sector_address <= end_sector_address )
    {
        if ( sector_address != last_erased_sector )
        {
            /* printf("Erasing secotr 0x%x\n", (unsigned int)sector_address); */
            sflash_sector_erase( &sflash_handle, sector_address );
            last_erased_sector = sector_address;
        }
        sector_address += sector_size;
    }
    deinit_sflash( &sflash_handle);
    return last_erased_sector;
}

wiced_result_t wiced_apps_write( const image_location_t* app_header_location, const uint8_t* data, uint32_t offset, uint32_t size, uint32_t *last_erased_sector )
{
    sflash_handle_t sflash_handle;
    app_header_t    app_header;
    uint32_t        address, available_size;

    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
    sflash_read( &sflash_handle, app_header_location->detail.external_fixed.location, &app_header, sizeof(app_header_t) );
    deinit_sflash( &sflash_handle );
    while ( size )
    {
        if ( wiced_apps_get_physical_address( &app_header, offset, &address, &available_size ) != WICED_SUCCESS )
        {
            /* offset is not within the app size */
            return WICED_ERROR;
        }
        else
        {
            /* offset is within app, but we need to make sure the size fits */
            if ( size <= available_size )
            {
                /* it all fit in to this sector */
                if ( last_erased_sector )
                {
                    /* User requests erase before write */
                    *last_erased_sector = wiced_apps_erase_sections( address, size, *last_erased_sector );
                };
                init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
                sflash_write( &sflash_handle, address, data, size );
                deinit_sflash( &sflash_handle );
                size = 0;
            }
            else
            {
                if ( last_erased_sector )
                {
                    /* User requests erase before write */
                    *last_erased_sector = wiced_apps_erase_sections( address, size, *last_erased_sector );
                };
                init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
                sflash_write( &sflash_handle, address, data, available_size );
                deinit_sflash( &sflash_handle );
                size   -= available_size;
                offset += available_size;
                data   += available_size;
            }
        }
    }
    /* We need to add a new app header at the end of the log section */
    return WICED_SUCCESS;
}

wiced_result_t wiced_apps_read( const image_location_t* app_header_location, uint8_t* data, uint32_t offset, uint32_t size )
{
    sflash_handle_t sflash_handle;
    app_header_t    app_header;
    uint32_t        address, available_size;

    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED );
    sflash_read( &sflash_handle, app_header_location->detail.external_fixed.location, &app_header, sizeof(app_header_t) );
    while ( size )
    {
        if ( wiced_apps_get_physical_address( &app_header, offset, &address, &available_size ) != WICED_SUCCESS )
        {
            /* offset is not within the app size, error */
            return WICED_ERROR;
        }
        else
        {
            /* offset is within app, but we need to make sure the size fits */
            if ( size <= available_size )
            {
                /* it all fit in to this sector */
                sflash_read( &sflash_handle, address, data, size );
                size = 0;
            }
            else
            {
                sflash_read( &sflash_handle, address, data, available_size );
                size   -= available_size;
                offset += available_size;
                data   += available_size;
            }
        }
    }
    deinit_sflash( &sflash_handle);
    /* We need to add a new app header at the end of the log section */
    return WICED_SUCCESS;
}
