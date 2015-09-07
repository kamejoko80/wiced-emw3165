/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <string.h>
#include <stdlib.h>
#include "spi_flash.h"
#include "platform_config.h"
#include "platform_peripheral.h"
#include "wwd_assert.h"
#include "wiced_framework.h"
#include "wiced_apps_common.h"

/******************************************************
 *                    Constants
 ******************************************************/

#define SAM4S_FLASH_START                    ( IFLASH0_ADDR )
#define SAM4S_LOCK_REGION_SIZE               ( IFLASH0_LOCK_REGION_SIZE )
#define SAM4S_PAGE_SIZE                      ( IFLASH0_PAGE_SIZE )
#define SAM4S_PAGES_PER_LOCK_REGION          ( SAM4S_LOCK_REGION_SIZE / SAM4S_PAGE_SIZE )
#define SAM4S_PAGES_PER_ERASE                ( 8 )

#define APP_HDR_START_ADDR                   ((uint32_t)&app_hdr_start_addr_loc)
#define APP_CODE_START_ADDR                  ((uint32_t)&app_code_start_addr_loc)
#define SRAM_START_ADDR                      ((uint32_t)&sram_start_addr_loc)
#define SRAM_SIZE                            ((uint32_t)&sram_size_loc)
#define SAM4S_GET_LOCK_REGION_ADDR( region ) ( (region) * SAM4S_LOCK_REGION_SIZE + SAM4S_FLASH_START )
#define SAM4S_GET_PAGE_ADDR( page )          ( (page) * SAM4S_PAGE_SIZE + SAM4S_FLASH_START )
#define SAM4S_GET_PAGE_FROM_ADDR( addr )     ( (uint32_t)( ( ( addr ) - SAM4S_FLASH_START ) / SAM4S_PAGE_SIZE ) )

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    SAM4S_FLASH_ERASE_4_PAGES = 0x04,
    SAM4S_FLASH_ERASE_8_PAGES = 0x08,
    SAM4S_FLASH_ERASE_16_PAGES = 0x10,
    SAM4S_FLASH_ERASE_32_PAGES = 0x20,
} sam4s_flash_erase_page_amount_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static platform_result_t platform_write_flash( uint32_t start_address, const uint8_t* data, uint32_t data_size );
static platform_result_t platform_unlock_flash( uint32_t start_address, uint32_t end_address );
static platform_result_t platform_lock_flash( uint32_t start_address, uint32_t end_address );

/******************************************************
 *               Variable Definitions
 ******************************************************/
static uint8_t buf[ 512 ];
#ifdef PLATFORM_HAS_OTA
static uint8_t page_buff[ 4096 ]; /* Size required for full page erase and rewrite */
#endif

/* These come from linker */
extern void* app_hdr_start_addr_loc;
extern void* app_code_start_addr_loc;
extern void* sram_start_addr_loc;
extern void* sram_size_loc;

/******************************************************
 *               Function Definitions
 ******************************************************/

#if defined ( __ICCARM__ )

static inline void __jump_to( uint32_t addr )
{
    __asm( "MOV R1, #0x00000001" );
    __asm( "ORR R0, R1, #0" ); /* Last bit of jump address indicates whether destination is Thumb or ARM code */
    __asm( "BX R0" );
}

#elif defined ( __GNUC__ )

__attribute__( ( always_inline ) ) static __INLINE void __jump_to( uint32_t addr )
{
    addr |= 0x00000001; /* Last bit of jump address indicates whether destination is Thumb or ARM code */
    __ASM volatile ("BX %0" : : "r" (addr) );
}

#endif

void platform_start_app( uint32_t entry_point )
{

    /* Simulate a reset for the app: */
    /*   Switch to Thread Mode, and the Main Stack Pointer */
    /*   Change the vector table offset address to point to the app vector table */
    /*   Set other registers to reset values (esp LR) */
    /*   Jump to the reset vector */

    if ( entry_point == 0 )
    {
        uint32_t* vector_table = (uint32_t*) APP_CODE_START_ADDR;
        entry_point = vector_table[ 1 ];
    }

    __asm( "MOV LR,        #0xFFFFFFFF" );
    __asm( "MOV R1,        #0x01000000" );
    __asm( "MSR APSR_nzcvq,     R1" );
    __asm( "MOV R1,        #0x00000000" );
    __asm( "MSR PRIMASK,   R1" );
    __asm( "MSR FAULTMASK, R1" );
    __asm( "MSR BASEPRI,   R1" );
    __asm( "MSR CONTROL,   R1" );

    /*  Now rely on the app crt0 to load VTOR / Stack pointer

     SCB->VTOR = vector_table_address; - Change the vector table to point to app vector table
     __set_MSP( *stack_ptr ); */

    __jump_to( entry_point );

}

static platform_result_t platform_erase_flash_pages( uint32_t start_page, sam4s_flash_erase_page_amount_t total_pages )
{
    uint32_t erase_result = 0;
    uint32_t argument = 0;

    platform_watchdog_kick( );

    if ( total_pages == 32 )
    {
        start_page &= ~( 32u - 1u );
        argument = ( start_page ) | 3; /* 32 pages */
    }
    else if ( total_pages == 16 )
    {
        start_page &= ~( 16u - 1u );
        argument = ( start_page ) | 2; /* 16 pages */
    }
    else if ( total_pages == 8 )
    {
        start_page &= ~( 8u - 1u );
        argument = ( start_page ) | 1; /* 8 pages */
    }
    else
    {
        start_page &= ~( 4u - 1u );
        argument = ( start_page ) | 0; /* 4 pages */
    }

    erase_result = efc_perform_command( EFC0, EFC_FCMD_EPA, argument );

    platform_watchdog_kick( );

    return ( erase_result == 0 ) ? PLATFORM_SUCCESS : PLATFORM_ERROR;
}

platform_result_t platform_erase_flash( uint16_t start_sector, uint16_t end_sector )
{
    uint32_t start_address = SAM4S_GET_PAGE_ADDR( start_sector * SAM4S_PAGES_PER_LOCK_REGION );
    uint32_t end_address = SAM4S_GET_PAGE_ADDR( end_sector * SAM4S_PAGES_PER_LOCK_REGION );
    uint32_t i;

    if ( platform_unlock_flash( start_address, end_address ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    for ( i = start_sector; i <= end_sector; i++ )
    {
        if ( platform_erase_flash_pages( i * SAM4S_PAGES_PER_LOCK_REGION, SAM4S_FLASH_ERASE_16_PAGES ) != PLATFORM_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
    }

    if ( platform_lock_flash( start_address, end_address ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    return PLATFORM_SUCCESS;
}

platform_result_t platform_write_flash_chunk( uint32_t address, const void* data, uint32_t size )
{
    platform_result_t write_result = PLATFORM_SUCCESS;

    if ( platform_unlock_flash( address, address + size -1 ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    write_result = platform_write_flash( address, data, size );

    if ( platform_lock_flash( address, address + size -1 ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    /* Successful */
    return write_result;
}

static platform_result_t platform_lock_flash( uint32_t start_address, uint32_t end_address )
{
    uint32_t start_page = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t end_page   = SAM4S_GET_PAGE_FROM_ADDR( end_address );

    start_page -= ( start_page % SAM4S_PAGES_PER_LOCK_REGION );

    while ( start_page <= end_page )
    {
        if ( efc_perform_command( EFC0, EFC_FCMD_SLB, start_page ) != 0 )
        {
            return PLATFORM_ERROR;
        }

        start_page += SAM4S_PAGES_PER_LOCK_REGION;
    }

    return PLATFORM_SUCCESS;
}

static platform_result_t platform_unlock_flash( uint32_t start_address, uint32_t end_address )
{
    uint32_t start_page = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t end_page   = SAM4S_GET_PAGE_FROM_ADDR( end_address );

    start_page -= ( start_page % SAM4S_PAGES_PER_LOCK_REGION );

    while ( start_page <= end_page )
    {
        if ( efc_perform_command( EFC0, EFC_FCMD_CLB, start_page ) != 0 )
        {
            return PLATFORM_ERROR;
        }

        start_page += SAM4S_PAGES_PER_LOCK_REGION;
    }

    return PLATFORM_SUCCESS;
}
/*
 * SAM4S Flash writing:
 * SAM4S is very pecurluar when comes to writing internal flash. The following limitations exist:
 * 1- SAM4S writes to a map page buffer of 512 bytes then it is later written to flash using WP command
 * 2- When erasing, it erases a minimum of 8 pages
 * 2- Partial page writing is not working reliably and pages need to be rewritten totally
 *
 * Solution:
 * Flash writing is happening in one of two scenarios:
 * 1- DCT Update:
 * When doing DCT update, the flash needs to be reprogrammed. This is the most often occuring scenario.
 * It is also usually small in size. Writing to DCT is sequential, The DCT is erased and rewriteen all
 * in chunks of 512 bytes. For this a 512 buffer cache is used to write to it and once the cache buffer
 * is full, writing to the flash is done.
 *
 * 2- Code update:
 * This is less frequent and is not sequential as it depends on the elf file sectors. For this the
 * writing has to be more flexible. This is handled with an 8 page cache buffer. One writing a new
 * piece of code, the 8 pages are loaded in to the cache, The flash is erased, the cache is updated
 * with the new data and the 8 pages are rewritten with the new contents.
 */
static platform_result_t platform_write_flash_page( uint32_t page_num, const uint8_t* data )
{
    uint32_t       byte;
    uint32_t       word;
    uint32_t       status;
    uint32_t*      dst_ptr = (uint32_t*) SAM4S_GET_PAGE_ADDR( page_num );
    const uint8_t* src_ptr = data;

    efc_set_wait_state( EFC0, 6 );
    for ( word = 0; word < 128; word++ )
    {
        uint32_t long_word;
        uint8_t* word_to_write = (uint8_t*) &long_word;
        uint32_t* word_to_write_ptr = &long_word;

        UNUSED_PARAMETER( word_to_write );

        for ( byte = 0; byte < 4; byte++ )
        {
            word_to_write[ byte ] = *src_ptr++ ;
        }
        /* 32-bit aligned write to the flash */
        *dst_ptr++ = *word_to_write_ptr;
    }

    /* Send write page command */
    if ( efc_perform_command( EFC0, EFC_FCMD_WP, page_num ) != 0 )
    {
        return PLATFORM_ERROR;
    }

    do
    {
        status = efc_get_status( EFC0 );
    } while ( ( status & 1 ) == 0 );

    /*verify */
    src_ptr = (uint8_t*) SAM4S_GET_PAGE_ADDR( page_num );
    for ( word = 0; word < 512; word++ )
    {
        if ( *src_ptr != *data )
        {
            return PLATFORM_ERROR;
        }
        src_ptr++ ;
        data++ ;
    }
    return PLATFORM_SUCCESS;
}

static platform_result_t platform_write_flash( uint32_t start_address, const uint8_t* data, uint32_t data_size )
{
    uint32_t start_page = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t end_page   = SAM4S_GET_PAGE_FROM_ADDR( ( start_address + data_size - 1) );
    uint32_t offset     = start_address & ( SAM4S_PAGE_SIZE - 1 );

    if ( start_page == end_page )
    {
        if ( data_size == SAM4S_PAGE_SIZE )
        {
            if ( platform_write_flash_page( start_page, data ) != PLATFORM_SUCCESS )
            {
                return PLATFORM_SUCCESS;
            }
        }
        else
        {
            /* PARTIAL WRITE, DON'T WRITE NOW, WAIT EITHER BUFFER IS FULL OR FINISH SIGNAL RECEIVED */
            memcpy( buf + offset, data, data_size );
        }
        return PLATFORM_SUCCESS;
    }

    if ( offset )
    {
        uint32_t size = SAM4S_PAGE_SIZE - offset;
        memcpy( buf + offset, data, size );

        if ( platform_write_flash_page( start_page, buf ) != PLATFORM_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
        start_address += size;
        data_size     -= size;
        data          += size;
        start_page++ ;
    }

    while ( ( start_page <= end_page ) && ( data_size >= SAM4S_PAGE_SIZE ) )
    {
        if ( platform_write_flash_page( start_page, data ) != PLATFORM_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
        data          += SAM4S_PAGE_SIZE;
        data_size     -= SAM4S_PAGE_SIZE;
        start_page++ ;
    }

    if ( data_size != 0 )
    {
        /* DONT WRITE NOW. WAIT UNTILL PAGE IS FULL */
        memcpy( buf, data, data_size );
    }

    return PLATFORM_SUCCESS;
}

#ifdef PLATFORM_HAS_OTA

static platform_result_t platform_erase_flash_chunk( uint32_t start_address, uint32_t size )
{
    uint32_t start_page     = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t end_address    = start_address + size - 1;
    uint32_t end_page       = SAM4S_GET_PAGE_FROM_ADDR( end_address );
    uint32_t i;

    if ( platform_unlock_flash( start_address, end_address ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    for ( i = start_page; i <= end_page; i += 8 )
    {
        if ( platform_erase_flash_pages( i, SAM4S_FLASH_ERASE_8_PAGES ) != PLATFORM_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
    }

    if ( platform_lock_flash( start_address, end_address ) != PLATFORM_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    return PLATFORM_SUCCESS;
}

static platform_result_t platform_copy_app_to_iflash( const image_location_t* app_header_location, uint32_t file_offset, uint32_t start_address, uint32_t data_size )
{
    uint32_t start_page = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t page       = start_page & ( ~( SAM4S_PAGES_PER_ERASE - 1 ) );

    while ( data_size )
    {
        uint32_t sector_start_address = SAM4S_GET_PAGE_ADDR( page );
        uint32_t offset               = start_address - sector_start_address;
        uint32_t copy_size            = SAM4S_PAGES_PER_ERASE * SAM4S_PAGE_SIZE - offset;

        copy_size = MIN( copy_size, data_size );

        memcpy( page_buff, (uint32_t*) sector_start_address, SAM4S_PAGES_PER_ERASE * SAM4S_PAGE_SIZE );
        if ( wiced_apps_read( app_header_location, page_buff + offset, file_offset, copy_size ) != WICED_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
        if ( platform_erase_flash_chunk( sector_start_address, SAM4S_PAGES_PER_ERASE * SAM4S_PAGE_SIZE ) != PLATFORM_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
        if ( platform_write_flash_chunk( sector_start_address, page_buff, SAM4S_PAGES_PER_ERASE * SAM4S_PAGE_SIZE ) != PLATFORM_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
        start_address += copy_size;
        file_offset   += copy_size;
        page          += SAM4S_PAGES_PER_ERASE;
        data_size     -= copy_size;
    }
    return PLATFORM_SUCCESS;
}

void platform_erase_app_area( uint32_t physical_address, uint32_t size )
{
    (void) physical_address;
    (void) size;
}

void platform_load_app_chunk( const image_location_t* app_header_location, uint32_t offset, void* physical_address, uint32_t size )
{
    if ( (uint32_t) physical_address < SRAM_START_ADDR )
    {
        platform_copy_app_to_iflash( app_header_location, offset, (uint32_t) physical_address, size );
    }
    else
    {
        wiced_apps_read( app_header_location, physical_address, offset, size );
    }
}
#else
void platform_load_app_chunk( const image_location_t* app_header_location, uint32_t offset, void* physical_address, uint32_t size )
{
    (void)app_header_location;
    (void)offset;
    (void)physical_address;
    (void)size;
}
#endif
