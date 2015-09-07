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
#include "waf_platform.h"

#define PLATFORM_APP_START_SECTOR       ( 0  )
#define PLATFORM_APP_END_SECTOR         ( 29 )
#define APP_CODE_START_ADDR             ((uint32_t)&app_code_start_addr_loc)
#define SRAM_START_ADDR                 ((uint32_t)&sram_start_addr_loc)
#define SRAM_SIZE                       ((uint32_t)&sram_size_loc)

/* These come from the linker script */
extern void* app_code_start_addr_loc;
extern void* sram_start_addr_loc;
extern void* sram_size_loc;

#ifdef WICED_DCT_INCLUDE_BT_CONFIG
        [DCT_BT_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, bt_config ),
#endif
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

platform_result_t platform_erase_flash( uint16_t start_sector, uint16_t end_sector )
{
    UNUSED_PARAMETER(start_sector);
    UNUSED_PARAMETER(end_sector);
    return PLATFORM_UNSUPPORTED;
}


platform_result_t platform_write_flash_chunk( uint32_t address, const void* data, uint32_t size )
{
    UNUSED_PARAMETER(address);
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(size);
    return PLATFORM_UNSUPPORTED;
}

void platform_load_app_chunk( const image_location_t* app_header_location, uint32_t offset, void* physical_address, uint32_t size )
{
    UNUSED_PARAMETER(app_header_location);
    UNUSED_PARAMETER(offset);
    UNUSED_PARAMETER(physical_address);
    UNUSED_PARAMETER(size);
}

void platform_erase_app_area( uint32_t physical_address, uint32_t size )
{
    UNUSED_PARAMETER( physical_address );
    UNUSED_PARAMETER( size );
}
