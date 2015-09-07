#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_rtp_player

#WICED_ENABLE_TRACEX := 1

GLOBAL_DEFINES     += AUDIOPCM_ENABLE
#GLOBAL_DEFINES     += CONSOLE_ENABLE_WL
GLOBAL_DEFINES     += WWD_TEST_NVRAM_OVERRIDE

GLOBAL_DEFINES     += APPLICATION_STACK_SIZE=8000

$(NAME)_SOURCES    := rtp_player.c
$(NAME)_SOURCES    += apollo_config.c

$(NAME)_COMPONENTS := audio/apollo/audio_render \
                      audio/apollo/apollocore \
                      utilities/command_console \
                      utilities/command_console/wifi

ifneq (,$(findstring AUDIOPCM_ENABLE,$(GLOBAL_DEFINES)))
$(info Enabling audiopcm libraries...)
$(NAME)_COMPONENTS += audio/apollo/audiopcm
$(NAME)_COMPONENTS += audio/apollo/audioplc
endif

ifneq (,$(findstring CONSOLE_ENABLE_WL,$(GLOBAL_DEFINES)))
$(NAME)_COMPONENTS += test/wl_tool
endif

APPLICATION_DCT    := apollo_audio_dct.c

WIFI_CONFIG_DCT_H  := wifi_config_dct.h

ifdef WICED_ENABLE_TRACEX
$(info apollo_audio using tracex lib)
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_DDR_OFFSET=0x0
GLOBAL_DEFINES     += WICED_TRACEX_BUFFER_SIZE=0x200000
endif

GLOBAL_DEFINES     += TX_PACKET_POOL_SIZE=5
GLOBAL_DEFINES     += RX_PACKET_POOL_SIZE=10

GLOBAL_DEFINES     += WICED_USE_AUDIO

#GLOBAL_DEFINES     += WICED_DISABLE_WATCHDOG

VALID_OSNS_COMBOS  := ThreadX-NetX_Duo
VALID_PLATFORMS    :=
VALID_PLATFORMS    += BCM943909WCD1
VALID_PLATFORMS    += BCM943909WCD1_3

