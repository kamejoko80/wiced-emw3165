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

#include "platform.h"
#include "wiced_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

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
 *  Measure the temperature of the thermistor
 *
 * @param[in]  adc                    Which ADC to measure
 * @param[out] celcius_degree_tenths  Variable which receives the temperature in tenths of a degree Celcius.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t thermistor_take_sample( wiced_adc_t adc, int16_t* celcius_degree_tenths );

#ifdef __cplusplus
} /* extern "C" */
#endif
