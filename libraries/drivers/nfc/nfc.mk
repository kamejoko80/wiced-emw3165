#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_NFC

$(NAME)_RESOURCES  := nfc/patch/43341B0/i2c_pre_patch_ncd.bin \
                      nfc/patch/43341B0/patch_ram_ncd.bin

$(NAME)_SOURCES :=  wiced_nfc.c \
                    wiced_nfc_platform.c \
                    wiced_nfc_serial.c \
                    nfc_bus.c

$(NAME)_INCLUDES :=  Components/gki/wiced \
                     Components/gki/common \
                     Components/hcis \
                     Components/nfc-hal/int \
                     Components/nfc-hal/include \
                     Components/nfc/include \
                     Components/nfc/int \
                     Components/nfc/brcm \
                     Components/stack/include \
                     Components/nfa/include \
                     Components/nfa/int \
                     Components/nfa/brcm \
                     Components/rpc/include \
                     Components/udrv/include \
                     Projects/wiced_nfc/include \
                     Projects/bte/build \
                     Projects/bte/main

NFC_LIBRARY_NAME :=nfc.$(HOST_ARCH).release.a

ifneq ($(wildcard $(CURDIR)$(NFC_LIBRARY_NAME)),)
# Architecture specific prebuilt library
$(NAME)_PREBUILT_LIBRARY :=$(NFC_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)nfc_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(NFC_LIBRARY_NAME)),)

$(NAME)_DEFINES += GKI_MAX_TASKS=4
GLOBAL_DEFINES += WICED_NFC
GLOBAL_DEFINES += BUILDCFG \
                  __linux__

GLOBAL_INCLUDES += .
