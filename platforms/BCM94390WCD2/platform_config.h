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
 * Defines internal configuration of the BCM9490WCD2 board
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *  MCU Constants and Options
 ******************************************************/

/*  CPU clock : 48MHz */
#define CPU_CLOCK_HZ  ( 48000000 )

/*  WICED Resources uses a filesystem */
#define USES_RESOURCE_FILESYSTEM

/* The main app is stored in external serial flash */
#define BOOTLOADER_LOAD_MAIN_APP_FROM_EXTERNAL_LOCATION

/*  DCT is stored in external flash */
#define EXTERNAL_DCT

/*  OTP */
#define PLATFORM_HAS_OTP

/*  OTA */
#define PLATFORM_HAS_OTA


#ifdef __cplusplus
} /*extern "C" */
#endif
