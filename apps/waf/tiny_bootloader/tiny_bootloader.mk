#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_WICED_tiny_Bootloader_$(PLATFORM)

$(NAME)_SOURCES    := tiny_bootloader.c

$(NAME)_LINK_FILES += tiny_bootloader.o

NoOS_START_STACK   := 512

APP_WWD_ONLY       := 1

GLOBAL_DEFINES     := WICED_NO_WIFI \
                      WICED_DISABLE_STDIO \
                      WICED_DISABLE_MCU_POWERSAVE \
                      WICED_DCACHE_WTHROUGH \
                      TINY_BOOTLOADER

GLOBAL_INCLUDES    += .
GLOBAL_INCLUDES    += ../../waf/bootloader

VALID_OSNS_COMBOS  := NoOS-NoNS
VALID_BUILD_TYPES  := release

VALID_PLATFORMS    := BCM943909WCD1 \
                      BCM943909WCD1_3 \
                      BCM943909FCBU \
                      BCM943909B0FCBU
