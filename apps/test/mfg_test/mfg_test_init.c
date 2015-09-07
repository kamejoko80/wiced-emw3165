/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * @file
 * This is the main file for the manufacturing test app
 *
 * To use the manufacturing test application, please
 * read the instructions in the WICED Manufacturing
 * Test User Guide provided in the <WICED-SDK>/Doc
 * directory: WICED-MFG2xx-R.pdf
 *
 */

#include "wiced_management.h"
#include "wl_tool.h"
#include "mfg_test.h"
#include "internal/wwd_internal.h"
#include "wwd_wifi.h"

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
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/
void application_start( )
{
   wiced_init( );

   /* Disable WIFI sleeping */
   wwd_wifi_set_iovar_value( IOVAR_STR_MPC, 0, WWD_STA_INTERFACE );

#ifdef MFG_TEST_ENABLE_ETHERNET_SUPPORT
   wiced_wlu_server_eth_start( ETHERNET_PORT, 0);
#else
   wiced_wlu_server_serial_start( STDIO_UART );
#endif
}

