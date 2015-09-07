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
NOTE - CCM_SRAM section does not really exist.  split 96K of SRAM into 64K SRAM + 32K CCM_SRAM
This was done to allow the SPI flash program to execute properly from SRAM
*/


MEMORY
{
    /* OTA app resides in RAM only */
    SHARED_API (rx)  : ORIGIN = 0x08000200, LENGTH = 512
    SRAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 64K
    CCM_SRAM (rwx)   : ORIGIN = 0x20010000, LENGTH = 32K
    OTA_HDR (rx) : ORIGIN = 0x1FFFFE00, LENGTH = 512   /* App Header for OTA only goes in SFLASH, hence does not need a valid address */
}
