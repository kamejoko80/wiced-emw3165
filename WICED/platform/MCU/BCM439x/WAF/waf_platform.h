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

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                   Constants
 ******************************************************/

/******************************************************
 *                    Macros
 ******************************************************/
#define PLATFORM_SFLASH_PERIPHERAL_ID (0)

#define PLATFORM_DCT_COPY1_START_SECTOR      ( 0 )
#define PLATFORM_DCT_COPY1_START_ADDRESS     ( 0 )
#define PLATFORM_DCT_COPY1_END_SECTOR        ( 4 )
#define PLATFORM_DCT_COPY1_SIZE              ( 16 * 1024 )
#define PLATFORM_DCT_COPY1_END_ADDRESS       ( PLATFORM_DCT_COPY1_START_ADDRESS + PLATFORM_DCT_COPY1_SIZE )
#define PLATFORM_DCT_COPY2_START_SECTOR      ( PLATFORM_DCT_COPY1_END_SECTOR  )
#define PLATFORM_DCT_COPY2_START_ADDRESS     ( PLATFORM_DCT_COPY1_SIZE )
#define PLATFORM_DCT_COPY2_END_SECTOR        ( PLATFORM_DCT_COPY1_END_SECTOR * 2  )
#define PLATFORM_DCT_COPY2_SIZE              ( PLATFORM_DCT_COPY1_SIZE )
#define PLATFORM_DCT_COPY2_END_ADDRESS       ( PLATFORM_DCT_COPY2_START_ADDRESS + PLATFORM_DCT_COPY2_SIZE )

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
void platform_start_app         ( uint32_t vector_table_address );
void platform_load_app_chunk    ( const image_location_t *app_header_location, uint32_t offset, void * physical_address, uint32_t size);
void platform_erase_app_area    ( uint32_t physical_address, uint32_t size );

/* Factory reset required function (defined in platform.c) */
wiced_bool_t platform_check_factory_reset( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

