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
#include "platform_dct.h"
#include "wiced_result.h"
#include "wiced_apps_common.h"

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

#if defined(__ICCARM__)
#pragma section="wiced_apps_lut_section"
#endif

#if defined ( __IAR_SYSTEMS_ICC__ )
#pragma section="wiced_apps_lut_section"
static const app_header_t wiced_apps_lut[8] @ "wiced_apps_lut_section";
static const uint8_t wiced_apps_lut_fill[4096  - sizeof(initial_apps)] @ "wiced_apps_lut_section";
#endif /* if defined ( __IAR_SYSTEMS_ICC__ ) */

#define WICED_APPS_LUT_FILL_SIZE (4096  - sizeof(wiced_apps_lut))

static const app_header_t wiced_apps_lut[ 8 ] =
{
    [DCT_FR_APP_INDEX] =
        {
            .count   = FR_APP_ENTRY_COUNT,
            .sectors = { {FR_APP_SECTOR_START,FR_APP_SECTOR_COUNT} }
        },
    [DCT_DCT_IMAGE_INDEX] =
        {
            .count   = DCT_IMAGE_ENTRY_COUNT,
            .sectors = { {DCT_IMAGE_SECTOR_START,DCT_IMAGE_SECTOR_COUNT} }
        },
    [DCT_OTA_APP_INDEX] =
        {
            .count   = OTA_APP_ENTRY_COUNT,
            .sectors = { {OTA_APP_SECTOR_START,OTA_APP_SECTOR_COUNT} }
        },
    [DCT_FILESYSTEM_IMAGE_INDEX] =
        {
            .count   = FILESYSTEM_IMAGE_ENTRY_COUNT,
            .sectors = { {FILESYSTEM_IMAGE_SECTOR_START,FILESYSTEM_IMAGE_SECTOR_COUNT} }
        },
    [DCT_WIFI_FIRMWARE_INDEX] =
        {
            .count   = WIFI_FIRMWARE_ENTRY_COUNT,
            .sectors = { {WIFI_FIRMWARE_SECTOR_START,WIFI_FIRMWARE_SECTOR_COUNT} }
        },
    [DCT_APP0_INDEX] =
        {
            .count   = APP0_ENTRY_COUNT,
            .sectors = { {APP0_SECTOR_START,APP0_SECTOR_COUNT} }
        },
    [DCT_APP1_INDEX] =
        {
            .count   = APP1_ENTRY_COUNT,
            .sectors = { {APP1_SECTOR_START,APP1_SECTOR_COUNT} }
        },
    [DCT_APP2_INDEX] =
        {
            .count   = APP2_ENTRY_COUNT,
            .sectors = { {APP2_SECTOR_START,APP2_SECTOR_COUNT} }
        },

};

/* Make sure the rest of hte sector is Empty */
static const uint8_t wiced_apps_lut_fill[WICED_APPS_LUT_FILL_SIZE] = { [0 ... WICED_APPS_LUT_FILL_SIZE-1] 0xFF };

#if defined ( __IAR_SYSTEMS_ICC__ )
4__root int wiced_program_start(void)
{
    /* in iar we must reference dct structure, otherwise it may not be included in */
    /* the dct image */
    return initial_apps[0].count;
}
#endif /* if defined ( __IAR_SYSTEMS_ICC__ ) */


/******************************************************
 *               Function Definitions
 ******************************************************/
