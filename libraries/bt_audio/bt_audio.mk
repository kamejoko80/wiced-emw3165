#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_bt_audio

BT_AUDIO_LIBRARY_NAME :=bt_audio.$(RTOS).$(NETWORK).$(HOST_ARCH).release.a

ifneq ($(wildcard $(CURDIR)$(BT_AUDIO_LIBRARY_NAME)),)
$(NAME)_PREBUILT_LIBRARY :=$(BT_AUDIO_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)bt_audio_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(BT_AUDIO_LIBRARY_NAME)),)

GLOBAL_INCLUDES := .
GLOBAL_INCLUDES += include

# Build firmware image for the BT chip specified
$(NAME)_SOURCES += ../drivers/bluetooth/firmware/$(BT_CHIP)$(BT_CHIP_REVISION)/bt_firmware_image.c \
                   wiced_logmsg.c \
                   wiced_bt.c
