/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * Defines BSP-related configuration.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
#include "platform_isr.h"
#include "platform_map.h"
#endif

/* Define which frequency CPU run */
#ifndef PLATFORM_CPU_CLOCK_FREQUENCY
#define PLATFORM_CPU_CLOCK_FREQUENCY      PLATFORM_CPU_CLOCK_FREQUENCY_320_MHZ
#endif

/* Define RTOS timers default configuration */
//
#ifndef PLATFORM_TICK_TINY
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_TICK_TINY                1
#else
#define PLATFORM_TICK_TINY                0
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_TICK_TINY */
//
#ifndef PLATFORM_TICK_CPU
#define PLATFORM_TICK_CPU             !PLATFORM_TICK_TINY
#endif
//
#ifndef PLATFORM_TICK_PMU
#ifdef WICED_DISABLE_MCU_POWERSAVE
#define PLATFORM_TICK_PMU                 0
#else
#define PLATFORM_TICK_PMU                 !PLATFORM_TICK_TINY
#endif /* WICED_DISABLE_MCU_POWERSAVE */
#endif /* PLATFORM_TICK_PMU */

/* By default enable ticks statistic */
#ifndef PLATFORM_TICK_STATS
#define PLATFORM_TICK_STATS               !PLATFORM_TICK_TINY
#endif

/* By default use external LPO */
#ifndef PLATFORM_LPO_CLOCK_EXT
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_LPO_CLOCK_EXT            0
#else
#define PLATFORM_LPO_CLOCK_EXT            1
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_LPO_CLOCK_EXT */

/* Define various HIB parameters */
// Disable various HIB parameters by one shot
#ifndef PLATFORM_HIB_NOT_AVAILABLE
#define PLATFORM_HIB_NOT_AVAILABLE        0
#endif
#if PLATFORM_HIB_NOT_AVAILABLE
#define PLATFORM_HIB_ENABLE               0
#define PLATFORM_HIB_CLOCK_AS_EXT_LPO     0
#define PLATFORM_HIB_CLOCK_POWER_UP       0
#endif /* PLATFORM_HIB_NOT_AVAILABLE */
// HIB provide clock to be used as external LPO, use it by default
#ifndef PLATFORM_HIB_CLOCK_AS_EXT_LPO
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_HIB_CLOCK_AS_EXT_LPO     0
#else
#define PLATFORM_HIB_CLOCK_AS_EXT_LPO     1
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_HIB_CLOCK_AS_EXT_LPO */
// Power-up clocks in bootloader
#ifndef PLATFORM_HIB_CLOCK_POWER_UP
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_HIB_CLOCK_POWER_UP       1
#else
#define PLATFORM_HIB_CLOCK_POWER_UP       0
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_HIB_CLOCK_POWER_UP */
//
#ifndef PLATFORM_HIB_ENABLE
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_HIB_ENABLE               0
#else
#define PLATFORM_HIB_ENABLE               1
#endif /* BOOTLOADER || TINY_BOOTLOADER */
#endif /* PLATFORM_HIB_ENABLE */

/* No WLAN power-save feature when no WLAN itself */
#ifndef PLATFORM_WLAN_POWERSAVE
#ifdef WICED_NO_WIFI
#define PLATFORM_WLAN_POWERSAVE           0
#else
#define PLATFORM_WLAN_POWERSAVE           1
#endif /* WICED_NO_WIFI */
#endif /* PLATFORM_WLAN_POWERSAVE */

/* Define DDR default configuration */
#ifndef PLATFORM_NO_DDR
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_NO_DDR                   1
#else
#define PLATFORM_NO_DDR                   0
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_NO_DDR */

/* Define vectors default configuration */
#ifndef PLATFORM_NO_VECTORS
#if defined(TINY_BOOTLOADER)
#define PLATFORM_NO_VECTORS               1
#else
#define PLATFORM_NO_VECTORS               0
#endif /* TINY_BOOTLOADER */
#endif /* PLATFORM_NO_VECTORS */

/* Define SPI flash default configuration */
#ifndef PLATFORM_NO_SFLASH_WRITE
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_NO_SFLASH_WRITE          1
#else
#define PLATFORM_NO_SFLASH_WRITE          0
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_NO_SFLASH_WRITE */

/* Define backplane configuration */
#ifndef PLATFORM_NO_BP_INIT
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_NO_BP_INIT               1
#else
#define PLATFORM_NO_BP_INIT               0
#ifndef PLATFORM_BP_TIMEOUT
#define PLATFORM_BP_TIMEOUT               0xFF
#endif /* PLATFORM_BP_TIMEOUT */
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_NO_BP_INIT */

/* Define AON memory access attributes */
#ifdef PLATFORM_A0_AON_MEMORY
#ifdef TINY_BOOTLOADER
#define PLATFORM_AON_MEMORY_ACCESS        MPU_R_ACCESS_CACHED_WTHR_MEM
#else
#define PLATFORM_AON_MEMORY_ACCESS        MPU_R_ACCESS_STR_ORDERED_MEM
#endif /* TINY_BOOTLOADER */
#endif /* PLATFORM_A0_AON_MEMORY */

/* TraceX storage buffer in DDR */
#if defined(TX_ENABLE_EVENT_TRACE) && (PLATFORM_NO_DDR != 1)
#ifndef WICED_TRACEX_BUFFER_DDR_OFFSET
#define WICED_TRACEX_BUFFER_DDR_OFFSET    (0x0)
#endif
#define WICED_TRACEX_BUFFER_ADDRESS       ((uint8_t *)PLATFORM_DDR_BASE(WICED_TRACEX_BUFFER_DDR_OFFSET))
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
