#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_wlu_server

$(NAME)_SOURCES  := $(WLAN_CHIP)$(WLAN_CHIP_REVISION)/wl/exe/wlu_server_shared.c \
                   wlu_server.c \
                   wlu_wiced.c \
                   $(WLAN_CHIP)$(WLAN_CHIP_REVISION)/wl/exe/wlu_pipe.c \
                   $(WLAN_CHIP)$(WLAN_CHIP_REVISION)/wl/exe/wlu.c \
                   $(WLAN_CHIP)$(WLAN_CHIP_REVISION)/shared/miniopt.c \
                   $(WLAN_CHIP)$(WLAN_CHIP_REVISION)/shared/bcmutils.c \
                   $(WLAN_CHIP)$(WLAN_CHIP_REVISION)/shared/bcm_app_utils.c

$(NAME)_INCLUDES := ./$(WLAN_CHIP)$(WLAN_CHIP_REVISION)/include \
                    ./$(WLAN_CHIP)$(WLAN_CHIP_REVISION)/common/include \
                    ./$(WLAN_CHIP)$(WLAN_CHIP_REVISION)/wl/exe \
                    ./$(WLAN_CHIP)$(WLAN_CHIP_REVISION)/shared/bcmwifi/include \
                    ./$(WLAN_CHIP)$(WLAN_CHIP_REVISION)/wl/ppr/include


CHIP := $(WLAN_CHIP)$(WLAN_CHIP_REVISION)
include $(CURDIR)$(WLAN_CHIP)$(WLAN_CHIP_REVISION)/$(WLAN_CHIP)$(WLAN_CHIP_REVISION).mk
$(NAME)_SOURCES += $($(CHIP)_SOURCE_WL)

#Serial Support
$(NAME)_DEFINES  := RWL_SERIAL TARGET_wiced
#Ethernet Support
#$(NAME)_DEFINES  := RWL_SOCKET TARGET_wiced

#$(NAME)_COMPONENTS += test/malloc_debug

GLOBAL_INCLUDES  += .

#AVOID_GLOMMING_IOVAR AVOID_APSTA SET_COUNTRY_WITH_IOCTL_NOT_IOVAR

