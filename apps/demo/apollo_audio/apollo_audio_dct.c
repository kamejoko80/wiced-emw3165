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

#include "wiced_framework.h"
#include "apollo_audio_dct.h"

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

DEFINE_APP_DCT(apollo_audio_dct_t)
{
    .speaker_name         = "Apollo",
    .channel              = CHANNEL_MAP_FR,
    .buffering_ms         = APOLLO_BUFFERING_MS_DEFAULT,
    .threshold_ms         = APOLLO_THRESHOLD_MS_DEFAULT,
    .auto_play            = 1,
    .clock_enable         = 0,  /* 0 = blind push, 1 = use AS clock */
    .volume               = APOLLO_VOLUME_DEFAULT,
};

/******************************************************
 *               Function Definitions
 ******************************************************/
