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
#include <string.h>
#include <stdlib.h>
#include "platform_cmsis.h"
#include "platform_dct.h"
#include "wiced_result.h"
#include "wiced_apps_common.h"
#include "waf_platform.h"

#if defined ( __ICCARM__ )

static inline void __jump_to( uint32_t addr )
{
    __asm( "MOV R1, #0x00000001" );
    __asm( "ORR R0, R1, #0" );  /* Last bit of jump address indicates whether destination is Thumb or ARM code */
    __asm( "BX R0" );
}

#elif defined ( __GNUC__ )

__attribute__( ( always_inline ) ) static __INLINE void __jump_to( uint32_t addr )
{
    addr |= 0x00000001;  /* Last bit of jump address indicates whether destination is Thumb or ARM code */
  __ASM volatile ("BX %0" : : "r" (addr) );
}

#endif

void platform_start_app( uint32_t entry_point )
{

    /* Simulate a reset for the app: */
    /*   Switch to Thread Mode, and the Main Stack Pointer */
    /*   Set other registers to reset values (esp LR) */
    /*   Jump to the entry point */
    /*   The App will change the vector table offset address to point to its vector table */

    __asm( "MOV LR,        #0xFFFFFFFF" );
    __asm( "MOV R1,        #0x01000000" );
    __asm( "MSR APSR_nzcvq,     R1" );
    __asm( "MOV R1,        #0x00000000" );
    __asm( "MSR PRIMASK,   R1" );
    __asm( "MSR FAULTMASK, R1" );
    __asm( "MSR BASEPRI,   R1" );
    __asm( "MSR CONTROL,   R1" );

    __jump_to( entry_point );
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
