/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "platform_dct.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                   Constants
 ******************************************************/

/******************************************************
 *                    Macros
 ******************************************************/

#define SECTOR_SIZE (4096)

#define PLATFORM_DCT_COPY1_START_ADDRESS     ( 0x00008000 )
#define PLATFORM_DCT_COPY1_SIZE              ( 4 * SECTOR_SIZE )

#define PLATFORM_DCT_COPY2_START_ADDRESS     ( PLATFORM_DCT_COPY1_START_ADDRESS + PLATFORM_DCT_COPY1_SIZE )

#define PLATFORM_DCT_COPY1_START_SECTOR      ( (PLATFORM_DCT_COPY1_START_ADDRESS) / SECTOR_SIZE )
#define PLATFORM_DCT_COPY1_END_SECTOR        ( (PLATFORM_DCT_COPY1_START_ADDRESS + PLATFORM_DCT_COPY1_SIZE) / SECTOR_SIZE )
#define PLATFORM_DCT_COPY1_END_ADDRESS       (  PLATFORM_DCT_COPY1_START_ADDRESS + PLATFORM_DCT_COPY1_SIZE )

#define PLATFORM_DCT_COPY2_SIZE              ( PLATFORM_DCT_COPY1_SIZE )
#define PLATFORM_DCT_COPY2_START_SECTOR      ( (PLATFORM_DCT_COPY2_START_ADDRESS) / SECTOR_SIZE )
#define PLATFORM_DCT_COPY2_END_SECTOR        ( (PLATFORM_DCT_COPY2_START_ADDRESS + PLATFORM_DCT_COPY2_SIZE) / SECTOR_SIZE )
#define PLATFORM_DCT_COPY2_END_ADDRESS       (  PLATFORM_DCT_COPY2_START_ADDRESS + PLATFORM_DCT_COPY2_SIZE )

#define PLATFORM_SFLASH_PERIPHERAL_ID        (0)

/* DEFAULT APPS (eg FR and OTA) need to be loaded always. */
#define PLATFORM_DEFAULT_LOAD                ( WICED_FRAMEWORK_LOAD_ALWAYS )

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* WAF platform functions */
void platform_start_app             ( uint32_t entry_point ); /* Defined in start_GCC.S */
void platform_load_app_chunk        ( const image_location_t* app_header_location, uint32_t offset, void* physical_address, uint32_t size );
void platform_load_app_chunk_from_fs( const image_location_t* app_header_location, void* file_handler, void* physical_address, uint32_t size );
void platform_erase_app_area        ( uint32_t physical_address, uint32_t size );

/* Factory reset required function (defined in platform.c) */
wiced_bool_t platform_check_factory_reset( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

