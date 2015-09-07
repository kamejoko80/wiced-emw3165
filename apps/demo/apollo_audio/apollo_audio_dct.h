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

#include "apollocore.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define APOLLO_SPEAKER_NAME_LENGTH  (35)

#define APOLLO_BUFFERING_MS_MIN        0
#define APOLLO_BUFFERING_MS_DEFAULT   50
#define APOLLO_BUFFERING_MS_MAX     1000

#define APOLLO_THRESHOLD_MS_MIN        0
#define APOLLO_THRESHOLD_MS_DEFAULT   40
#define APOLLO_THRESHOLD_MS_MAX     1000

#define APOLLO_VOLUME_MIN              0
#define APOLLO_VOLUME_DEFAULT         80
#define APOLLO_VOLUME_MAX            100

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

typedef struct apollo_audio_dct_s {
    char speaker_name[APOLLO_SPEAKER_NAME_LENGTH + 1];
    APOLLO_CHANNEL_MAP_T channel;
    int buffering_ms;
    int threshold_ms;
    int auto_play;
    int clock_enable;   /* enable AS clock */
    int volume;
} apollo_audio_dct_t;


#ifdef __cplusplus
} /* extern "C" */
#endif
