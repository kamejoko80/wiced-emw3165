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

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef WEAK /* Some non-WICED sources already define WEAK */
#define WEAK          __attribute__((weak))
#endif
#ifndef UNUSED /* Some non-WICED sources already define UNUSED */
#define UNUSED        __attribute__((unused))
#endif
#define NORETURN      __attribute__((noreturn))
#define ALIGNED(size) __attribute__((aligned(size)))
#define SECTION(name) __attribute__((section(name)))
#define NOINLINE      __attribute__((noinline))

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

void *memrchr( const void *s, int c, size_t n );

#ifdef __cplusplus
} /* extern "C" */
#endif

