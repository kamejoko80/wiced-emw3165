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
 *
 */
#pragma once

#include <stdint.h>
#include "platform_toolchain.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#if defined( __GNUC__ ) && ( ! defined( __clang__ ) )
/* XXX More appropriate inclusion in platform_toolchain.h? */
#define ALWAYS_INLINE __attribute__((always_inline))  /* Required for ROM to ensure there are not repeated */
#endif /* if defined( __GNUC__ ) && ( ! defined( __clang__ ) ) */

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/**
 * Read the Instruction Fault Address Register
 */
static inline ALWAYS_INLINE uint32_t get_IFAR( void )
{
    uint32_t value;
    __asm__( "mrc p15, 0, %0, c6, c0, 2" : "=r" (value) : );
    return value;
}

/**
 * Read the Instruction Fault Status Register
 */
static inline ALWAYS_INLINE uint32_t get_IFSR( void )
{
    uint32_t value;
    __asm__( "mrc p15, 0, %0, c5, c0, 1" : "=r" (value) : );
    return value;
}



/**
 * Read the Data Fault Address Register
 */
static inline ALWAYS_INLINE uint32_t get_DFAR( void )
{
    uint32_t value;
    __asm__( "mrc p15, 0, %0, c6, c0, 0" : "=r" (value) : );
    return value;
}

/**
 * Read the Data Fault Status Register
 */
static inline ALWAYS_INLINE uint32_t get_DFSR( void )
{
    uint32_t value;
    __asm__( "mrc p15, 0, %0, c5, c0, 0" : "=r" (value) : );
    return value;
}


/**
 * Read the Program Counter and Processor Status Register
 */
static inline ALWAYS_INLINE uint32_t get_CPSR( void )
{
    uint32_t value;
    __asm__( "mrs %0, cpsr" : "=r" (value) : );
    return value;
}

/**
 * Wait for interrupts
 */
static inline ALWAYS_INLINE void cpu_wait_for_interrupt( void )
{
    __asm__( "wfi" : : : "memory");
}

/**
 * Data Synchronisation Barrier
 */
static inline ALWAYS_INLINE void cpu_data_synchronisation_barrier( void )
{
    __asm__( "dsb" : : : "memory");
}

static inline void cr4_init_cycle_counter( void )
{
    /* Reset cycle counter. Performance Monitors Control Register - PMCR */
    __asm__ __volatile__ ("MCR p15, 0, %0, c9, c12, 0" : : "r"(0x00000005));
    /* Enable the count - Performance Monitors Count Enable Set Register-  PMCNTENSET */
    __asm__ __volatile__ ("MCR p15, 0, %0, c9, c12, 1" : : "r"(0x80000000));
    /* Clear overflows - Performance Monitors Overflow Flag Status Register - PMOVSR */
    __asm__ __volatile__ ("MCR p15, 0, %0, c9, c12, 3" : : "r"(0x80000000));
}

static inline ALWAYS_INLINE uint32_t cr4_get_cycle_counter( void )
{
    uint32_t count;
    __asm__ __volatile__ ("MRC p15, 0, %0, c9, c13, 0" : "=r"(count));
    return count;
}


/*
 * Should be defined by MCU
 */
void platform_backplane_debug( void );

#ifdef __cplusplus
} /*extern "C" */
#endif
