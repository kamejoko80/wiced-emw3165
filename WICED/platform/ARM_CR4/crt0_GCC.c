/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/* For information on duties of crt0, see http://www.fhi-berlin.mpg.de/th/locserv/alphas/progs/doc/cygnus_doc-99r1/html/1_GS/int04.html#crt0,_the_main_startup_file
 * For details of various segments, see http://www.acsu.buffalo.edu/~charngda/elf.html
 */

#include <string.h>
#include "platform_init.h"
#include "platform_toolchain.h"
#include "platform_checkpoint.h"
#include "platform_peripheral.h"
#include "cr4.h"

extern void * link_bss_location;
extern void * link_bss_end;
#define link_bss_size   ((unsigned long)&link_bss_end  -  (unsigned long)&link_bss_location )

typedef void  (*constructor_ptr_t)( void );
extern constructor_ptr_t link_constructors_location[];
extern constructor_ptr_t link_constructors_end;

WEAK void _start( void )      NORETURN;
WEAK void _exit( int status ) NORETURN;

#define link_constructors_size   ((unsigned long)&link_constructors_end  -  (unsigned long)&link_constructors_location )


#ifdef ROM_OFFLOAD
extern void *link_rom_global_data_initial_values;
extern void *link_rom_global_data_start;
extern void *link_rom_global_data_end;
#define ROM_GLOBAL_DATA_SIZE     ((unsigned long)&link_rom_global_data_end - (unsigned long)&link_rom_global_data_start)

extern void *link_rom_global_bss_start;
extern void *link_rom_global_bss_end;
#define ROM_GLOBAL_BSS_SIZE      ((unsigned long)&link_rom_global_bss_end - (unsigned long)&link_rom_global_bss_start)
#endif /* ifdef ROM_OFFLOAD */

WEAK void _start( void )
{
    unsigned long ctor_num;

    cr4_init_cycle_counter( );

#ifdef ROM_OFFLOAD

    /*  Copy ROM global data & zero ROM global bss */
    memcpy(&link_rom_global_data_start, &link_rom_global_data_initial_values, ROM_GLOBAL_DATA_SIZE);

#endif /* ifdef ROM_OFFLOAD */

#ifndef PLATFORM_BSS_REQUIRE_HW_INIT
    /* BSS segment is for zero initialised elements, so memset it to zero */
    memset( &link_bss_location, 0, (size_t) link_bss_size );
    WICED_BOOT_CHECKPOINT_WRITE_C( 100 );

#ifdef ROM_OFFLOAD
    memset(&link_rom_global_bss_start, 0, ROM_GLOBAL_BSS_SIZE);
#endif /* ifdef ROM_OFFLOAD */
#endif /* !PLATFORM_BSS_REQUIRE_HW_INIT */


    /* Initialise clocks and memory. */
    platform_init_system_clocks();
    platform_init_memory();

    WICED_BOOT_CHECKPOINT_WRITE_C( 101 );

#ifdef PLATFORM_BSS_REQUIRE_HW_INIT
    /*
     * BSS segment is for zero initialised elements, so memset it to zero.
     * Above init_clocks() and init_memory() must NOT depend on globals as global data and bss sections aren't initialised yet.
     */
    memset( &link_bss_location, 0, (size_t) link_bss_size );
    WICED_BOOT_CHECKPOINT_WRITE_C( 102 );
#endif /* PLATFORM_BSS_REQUIRE_HW_INIT  */

    /* Complete platform initialization */
    platform_init_mcu_infrastructure( );
    platform_init_external_devices( );

    WICED_BOOT_CHECKPOINT_WRITE_C( 103 );

    /*
     * Run global C++ constructors if any
     */
    for ( ctor_num = 0; ctor_num < link_constructors_size/sizeof(constructor_ptr_t); ctor_num++ )
    {
        link_constructors_location[ctor_num]();
    }

    /* Last initialization step before start application */
    platform_init_complete( );

    WICED_BOOT_CHECKPOINT_WRITE_C( 104 );

    main( );

    WICED_BOOT_CHECKPOINT_WRITE_C( 105 );

    /* the main loop has returned - there is now nothing to do - reboot. */

#if !WICED_BOOT_CHECKPOINT_ENABLED
    platform_mcu_reset( ); /* Reset request */
#else
    while ( 1 ); /* Loop forever */
#endif
}

WEAK void _exit( int status )
{
    /* the main loop has returned - there is now nothing to do - reboot. */

    /* Allow some time for any printf calls to complete */
    volatile unsigned int i; /* try to make this not get optimized out by declaring the variable as volatile */
    volatile unsigned int j; /* try to make this not get optimized out by declaring the variable as volatile */

    (void) status; /* unused parameter */

    for ( i = 0; i < (unsigned int) 1000; i++ )
    {
        for ( j = 0; j < (unsigned int) 10000; j++ )
        {
            __asm("NOP");
        }
    }

    /* Reset request */
    platform_mcu_reset( );
}
