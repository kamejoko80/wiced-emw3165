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
#include <stdint.h>
#include "elf.h"
#include "wiced_deep_sleep.h"
#include "waf_platform.h"
#include "spi_flash.h"
#include "platform_peripheral.h"
#include "platform_checkpoint.h"
#include "platform_toolchain.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define link_bss_size   ((unsigned long)&link_bss_end  -  (unsigned long)&link_bss_location )
#define link_data_size  ((unsigned long)&link_aon_data_end  -  (unsigned long)&link_aon_data_location )

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

/******************************************************
 *               Variable Definitions
 ******************************************************/

/* Linker script defined symbols */
extern void* link_bss_location;
extern void* link_bss_end;
extern void* link_aon_data_location;
extern void* link_aon_data_end;
extern void* link_ram_data_location;
extern void* _low_start;

static sflash_handle_t sflash_handle;

static wiced_deep_sleep_tiny_bootloader_config_t SECTION(".config") config = { .entry_point = (uint32_t)&_low_start };

/******************************************************
 *               Function Declarations
 ******************************************************/

void _start( void )      NORETURN;
void _exit( int status ) NORETURN;

/******************************************************
 *               Function Definitions
 ******************************************************/

static void load_app( uint32_t app_elf_fs_address )
{
    int i;
    elf_header_t header;
    elf_program_header_t prog_header;
    uint32_t offset;

    WICED_BOOT_CHECKPOINT_WRITE_C( 250 );

    init_sflash( &sflash_handle, 0, SFLASH_WRITE_NOT_ALLOWED );

    WICED_BOOT_CHECKPOINT_WRITE_C( 251 );

    sflash_read( &sflash_handle, app_elf_fs_address, &header, sizeof(header) );

    WICED_BOOT_CHECKPOINT_WRITE_C( 252 );

    for( i = 0; i < header.program_header_entry_count; i++ )
    {
        offset = app_elf_fs_address + (header.program_header_offset + header.program_header_entry_size * i);
        sflash_read( &sflash_handle, offset, &prog_header, sizeof(prog_header) );
        if ( ( prog_header.data_size_in_file == 0 ) ||     /* size is zero */
             ( ( prog_header.type & 0x1 ) == 0 ) )         /* non- loadable segment */
        {
            continue;
        }

        if ( !WICED_DEEP_SLEEP_IS_AON_SEGMENT( prog_header.physical_address, prog_header.data_size_in_file ) )
        {
            offset = app_elf_fs_address + prog_header.data_offset;
            sflash_read( &sflash_handle, offset, (void*)prog_header.physical_address, prog_header.data_size_in_file );
        }
    }

    WICED_BOOT_CHECKPOINT_WRITE_C( 253 );

    platform_start_app( header.entry );

    WICED_BOOT_CHECKPOINT_WRITE_C( 254 );
}

void _start(void)
{
    /* Initialize BSS */
    memset( &link_bss_location, 0, (size_t) link_bss_size );

    /* Copy data section from AoN-RAM to RAM */
    memcpy( &link_ram_data_location, &link_aon_data_location, link_data_size );

    WICED_BOOT_CHECKPOINT_WRITE_C( 200 );

    /* Need to profile if the added code size of around 250bytes
     * that platform_cpu_clock_init() adds gives the benefits
     * in terms of speed
     */
    //platform_cpu_clock_init(PLATFORM_CPU_CLOCK_FREQUENCY_320_MHZ);

    WICED_BOOT_CHECKPOINT_WRITE_C( 201 );

    /* Check address, load and run application */
    if ( config.app_elf_fs_address != 0 )
    {
        load_app( config.app_elf_fs_address );
    }

    WICED_BOOT_CHECKPOINT_WRITE_C( 202 );

    /* Nothing can do */
#if !WICED_BOOT_CHECKPOINT_ENABLED
    platform_mcu_reset( ); /* Failed to run, try reboot via cold boot */
#else
    while( 1 ); /* Loop forever */
#endif
}

void _exit( int status )
{
    UNUSED_PARAMETER( status );
    platform_mcu_reset( ); /* Failed to run, try reboot via cold boot */
}

