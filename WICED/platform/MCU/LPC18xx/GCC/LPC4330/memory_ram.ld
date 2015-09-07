/* * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation. * mc_hijack */MEMORY{    LOCAL_SRAM_1 (rwx)   : ORIGIN = 0x10000000, LENGTH = 128K    LOCAL_SRAM_2 (rwx)   : ORIGIN = 0x10080000, LENGTH = 72K    LOCAL_SRAM_3 (rwx)   : ORIGIN = 0x20000000, LENGTH = 32K}