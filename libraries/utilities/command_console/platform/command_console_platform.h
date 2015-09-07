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
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define PLATFORM_COMMANDS \
    { "reboot",         reboot,        0, NULL, NULL, NULL,    "Reboot the device"}, \
    { "mcu_powersave",  mcu_powersave, 1, NULL, NULL, "<0|1>", "Enable/disable MCU powersave"},

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

int reboot( int argc, char* argv[] );
int mcu_powersave( int argc, char *argv[] );
int platform_enable_mcu_powersave ( void );
int platform_disable_mcu_powersave( void );

#ifdef __cplusplus
} /*extern "C" */
#endif
