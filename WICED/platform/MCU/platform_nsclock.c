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

#include "platform_config.h"
#include "platform/wwd_platform_interface.h"
#include "platform_peripheral.h"
/* FIXME Needed for CPU_CLOCK_HZ! */
#include "platform_isr.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define absolute_value(a) ( (a) < 0 ) ? -(a) : (a)

/******************************************************
 *                    Constants
 ******************************************************/

const uint32_t NR_NS_PER_SECOND     = 1*1000*1000*1000;

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* Nanosecond accumulator. */
static uint64_t nsclock_nsec = 0;
static uint32_t nsclock_sec = 0;
static uint32_t prev_cycles = 0;
static uint32_t ns_divisor = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

uint64_t platform_get_nanosecond_clock_value(void)
{
    uint64_t nanos;
    uint32_t cycles;
    uint32_t diff;

    cycles = host_platform_get_cycle_count( );

    /* add values to the ns part of the time which can be divided by ns_divisor */
    /* every time such value is added we will increment our clock by 1 / (CPU_CLOCK_HZ / ns_divisor).
     * For example, for CPU_CLOCK_HZ of 120MHz, ns_divisor of 3, the granularity is 25ns = 1 / (120MHz/3) or 1 / (40MHz).
     * Note that the cycle counter is running on the CPU frequency.
     */
    /* Values will be a multiple of ns_divisor (e.g. 1*ns_divisor, 2*ns_divisor, 3*ns_divisor, ...).
     * Roll-over taken care of by subtraction
     */
    diff = cycles - prev_cycles;
    {
        const uint32_t ns_per_unit = NR_NS_PER_SECOND / (CPU_CLOCK_HZ / ns_divisor);
        nsclock_nsec += ( (uint64_t)( diff / ns_divisor ) * ns_per_unit);
    }

    /* when ns counter rolls over, add one second */
    if( nsclock_nsec >= NR_NS_PER_SECOND )
    {
        /* Accumulate seconds portion of nanoseconds. */
        nsclock_sec += (uint32_t)(nsclock_nsec / NR_NS_PER_SECOND);
        /* Store remaining nanoseconds. */
        nsclock_nsec = nsclock_nsec - (nsclock_nsec / NR_NS_PER_SECOND) * NR_NS_PER_SECOND;
    }
    /* Subtract off unaccounted for cycles, so that they are accounted next time. */
    prev_cycles = cycles - (diff % ns_divisor);

    nanos = nsclock_sec;
    nanos *= NR_NS_PER_SECOND;
    nanos += nsclock_nsec;
    return nanos;
}


void platform_deinit_nanosecond_clock(void)
{
    platform_reset_nanosecond_clock( );

    ns_divisor   = 0;
}

void platform_reset_nanosecond_clock(void)
{
    nsclock_nsec = 0;
    nsclock_sec  = 0;
    prev_cycles  = 0;
}


void platform_init_nanosecond_clock(void)
{
    platform_reset_nanosecond_clock( );
    ns_divisor = 0;
    /* Calculate a divisor that will produce an
     * integer nanosecond value for the CPU
     * clock frequency.
     */
    while (NR_NS_PER_SECOND % (CPU_CLOCK_HZ / ++ns_divisor) != 0);
}
