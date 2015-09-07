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
 * Defines functions to manipulate caches
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "platform_cache.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    PLATFORM_DCACHE_CLEAN_OPERATION,
    PLATFORM_DCACHE_INV_OPERATION,
    PLATFORM_DCACHE_CLEAN_AND_INV_OPERATION,
} platform_dcache_range_operation_t;

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

/******************************************************
 *               Function Definitions
 ******************************************************/

#ifdef PLATFORM_L1_CACHE_SHIFT

static void platform_dcache_range_operation( platform_dcache_range_operation_t operation, const volatile void *p, uint32_t length )
{
    uint32_t addr_start = PLATFORM_L1_CACHE_ROUND_DOWN((uint32_t)p);
    const uint32_t addr_end = PLATFORM_L1_CACHE_ROUND_UP((uint32_t)p + length);

    __asm__ __volatile__ ("" : : : "memory");

    while (addr_start < addr_end)
    {
        switch ( operation )
        {
            case PLATFORM_DCACHE_CLEAN_OPERATION:
                __asm__ __volatile__ ("MCR p15, 0, %0, c7, c10, 1" : : "r"(addr_start));
                break;

            case PLATFORM_DCACHE_INV_OPERATION:
                __asm__ __volatile__ ("MCR p15, 0, %0, c7, c6, 1" : : "r"(addr_start));
                break;

            case PLATFORM_DCACHE_CLEAN_AND_INV_OPERATION:
                __asm__ __volatile__ ("MCR p15, 0, %0, c7, c14, 1" : : "r"(addr_start));
                break;

            default:
                break;
        }

        addr_start += PLATFORM_L1_CACHE_BYTES;
    }

    __asm__ __volatile__ ("DSB");
}

void platform_dcache_inv_range( volatile void *p, uint32_t length )
{
    /* Invalidate d-cache lines from range */
    platform_dcache_range_operation( PLATFORM_DCACHE_INV_OPERATION, p, length);
}

void platform_dcache_clean_range( const volatile void *p, uint32_t length )
{
    /* Clean (save dirty lines) d-cache lines from range */

#ifndef WICED_DCACHE_WTHROUGH

    platform_dcache_range_operation( PLATFORM_DCACHE_CLEAN_OPERATION, p, length);

#else

    UNUSED_PARAMETER(p);
    UNUSED_PARAMETER(length);

    __asm__ __volatile__ ("DSB");

#endif /* WICED_DCACHE_WTHROUGH */
}

void platform_dcache_clean_and_inv_range( const volatile void *p, uint32_t length)
{
    /* Clean and invalidate (save dirty lines and then invalidate) d-cache lines from range */
    platform_dcache_range_operation( PLATFORM_DCACHE_CLEAN_AND_INV_OPERATION, p, length);
}

#endif /* PLATFORM_L1_CACHE_SHIFT */
