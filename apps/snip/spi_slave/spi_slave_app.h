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

#include <stdint.h>
#include "wiced_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define SPI_CLOCK_SPEED_HZ             ( 1000000 )
#define SPI_BIT_WIDTH                  ( 8 )
#define SPI_MODE                       ( SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH | SPI_MSB_FIRST )
#define DEVICE_ID                      ( 0xABCD1234 )
#define REGISTER_DEVICE_ID_ADDRESS     ( 0x0001 )
#define REGISTER_DEVICE_ID_LENGTH      ( 4 )
#define REGISTER_LED1_CONTROL_ADDRESS  ( 0x0002 )
#define REGISTER_LED1_CONTROL_LENGTH   ( 4 )
#define REGISTER_LED2_CONTROL_ADDRESS  ( 0x0003 )
#define REGISTER_LED2_CONTROL_LENGTH   ( 4 )

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

#ifdef __cplusplus
} /* extern "C" */
#endif
