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
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum {
    CHANNEL_MAP_NONE  = 0,          /* None or undefined    */
    CHANNEL_MAP_FL    = (1 << 0),   /* Front Left           */
    CHANNEL_MAP_FR    = (1 << 1),   /* Front Right          */
    CHANNEL_MAP_FC    = (1 << 2),   /* Front Center         */
    CHANNEL_MAP_LFE1  = (1 << 3),   /* LFE-1                */
    CHANNEL_MAP_BL    = (1 << 4),   /* Back Left            */
    CHANNEL_MAP_BR    = (1 << 5),   /* Back Right           */
    CHANNEL_MAP_FLC   = (1 << 6),   /* Front Left Center    */
    CHANNEL_MAP_FRC   = (1 << 7),   /* Front Right Center   */
    CHANNEL_MAP_BC    = (1 << 8),   /* Back Center          */
    CHANNEL_MAP_LFE2  = (1 << 9),   /* LFE-2                */
    CHANNEL_MAP_SIL   = (1 << 10),  /* Side Left            */
    CHANNEL_MAP_SIR   = (1 << 11),  /* Side Right           */
    CHANNEL_MAP_TPFL  = (1 << 12),  /* Top Front Left       */
    CHANNEL_MAP_TPFR  = (1 << 13),  /* Top Front Right      */
    CHANNEL_MAP_TPFC  = (1 << 14),  /* Top Front Center     */
    CHANNEL_MAP_TPC   = (1 << 15),  /* Top Center           */
    CHANNEL_MAP_TPBL  = (1 << 16),  /* Top Back Left        */
    CHANNEL_MAP_TPBR  = (1 << 17),  /* Top Back Right       */
    CHANNEL_MAP_TPSIL = (1 << 18),  /* Top Side Left        */
    CHANNEL_MAP_TPSIR = (1 << 19),  /* Top Side Right       */
    CHANNEL_MAP_TPBC  = (1 << 20),  /* Top Back Center      */
    CHANNEL_MAP_BTFC  = (1 << 21),  /* Bottom Front Center  */
    CHANNEL_MAP_BTFL  = (1 << 22),  /* Bottom Front Left    */
    CHANNEL_MAP_BTFR  = (1 << 23)   /* Bottom Front Right   */
} APOLLO_CHANNEL_MAP_T;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Initialize the Apollo audio network.
 *
 * This routine should be used in place of wiced_network_init
 * for Apollo audio applications.
 *
 * @return    Status of the operation.
 */
wiced_result_t apollo_network_up_default(void);


#ifdef __cplusplus
} /* extern "C" */
#endif
