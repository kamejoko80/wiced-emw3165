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
  File: typedef.h
  Version: 0.1
  Date: Dec 21, 2006
  Author: Jes Thyssen
*/

#ifndef __TYPEDEF__
#define __TYPEDEF__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#ifndef WIN32
typedef int64_t __int64;
#endif
typedef double Float;
typedef int16_t Word16;
typedef int32_t Word32;
typedef int Flag;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
