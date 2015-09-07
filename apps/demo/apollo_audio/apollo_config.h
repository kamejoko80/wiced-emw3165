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

/** Set configuration values / save configuration
 *
 *  validates and saves configuration values
 *
 *  auto_play
 *  network to connect to
 *  channel to connect to network on
 *
 * @param argc
 * @param argv
 */
void apollo_set_config(rtp_player_t* player, int argc, char *argv[]);

void apollo_config_print_info(rtp_player_t* player);


#ifdef __cplusplus
} /* extern "C" */
#endif
