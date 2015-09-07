/* * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation. */MEMORY{    /* No Bootloader or DCT */       APP_CODE_A(rx) : ORIGIN = 0x1A000000, LENGTH = 512K    CODE_READ_PROTECTION_A(rx) : ORIGIN = 0x1A0002FC, LENGTH = 4    APP_CODE_B(rx) : ORIGIN = 0x1B000000, LENGTH = 512K    CODE_READ_PROTECTION_B(rx) : ORIGIN = 0x1B0002FC, LENGTH = 4    SRAM_FAST(rwx) : ORIGIN = 0x10000000,  LENGTH = 32K    SRAM_DATA(rwx) : ORIGIN = 0x10004000,  LENGTH = 40K    SRAM_STACK(rwx) : ORIGIN = 0x20000000, LENGTH = 32K}