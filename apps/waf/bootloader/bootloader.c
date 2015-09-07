/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <stdio.h>
#include "wicedfs.h"
#include "platform_dct.h"
#include "elf.h"
#include "wiced_framework.h"
#include "wiced_utilities.h"
#include "platform_config.h"
#include "platform_resource.h"
#include "waf_platform.h"
#include "wwd_rtos.h"

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

static void load_program( const load_details_t * load_details, uint32_t* new_entry_point );

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

int main( void )
{
    const load_details_t* load_details;
    uint32_t              entry_point;
    boot_detail_t         boot;

#ifdef PLATFORM_HAS_OTA
    NoOS_setup_timing( );

    if ( wiced_waf_check_factory_reset( ) )
    {
        wiced_dct_restore_factory_reset( );
        wiced_waf_app_set_boot( DCT_FR_APP_INDEX, PLATFORM_DEFAULT_LOAD );
    }

    NoOS_stop_timing( );
#endif

    boot.load_details.valid = 0;
    boot.entry_point        = 0;
    wiced_dct_read_with_copy( &boot, DCT_INTERNAL_SECTION, OFFSET( platform_dct_header_t, boot_detail ), sizeof(boot_detail_t) );

    load_details = &boot.load_details;
    entry_point  = boot.entry_point;

    if ( load_details->valid != 0 )
    {
        load_program( load_details, &entry_point );
    }

    wiced_waf_start_app( entry_point );

    while(1)
    {
    }

    /* Should never get here */
    return 0;
}

static void load_program( const load_details_t * load_details, uint32_t* new_entry_point )
{
    /* Image copy has been requested */
    if ( load_details->destination.id == EXTERNAL_FIXED_LOCATION)
    {
        /* External serial flash destination. Currently not allowed */
        return;
    }

    /* Internal Flash/RAM destination */
    if ( load_details->source.id == EXTERNAL_FIXED_LOCATION )
    {
        /* Fixed location in serial flash source - i.e. no filesystem */
        wiced_waf_app_load( &load_details->source, new_entry_point );
    }
    else if ( load_details->source.id == EXTERNAL_FILESYSTEM_FILE )
    {
        /* Filesystem location in serial flash source */
        wiced_waf_app_load( &load_details->source, new_entry_point );
    }

    if ( load_details->load_once != 0 )
    {
        boot_detail_t boot;

        boot.entry_point                 = 0;
        boot.load_details.load_once      = 1;
        boot.load_details.valid          = 0;
        boot.load_details.destination.id = INTERNAL;
        wiced_dct_write( &boot, DCT_INTERNAL_SECTION, OFFSET( platform_dct_header_t, boot_detail ), sizeof(boot_detail_t) );
    }
}
