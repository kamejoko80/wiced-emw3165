#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_Bluetooth_Audio

$(NAME)_SOURCES    := bluetooth_audio.c

$(NAME)_COMPONENTS := libraries/bt_audio

BT_CONFIG_DCT_H := bt_config_dct.h

GLOBAL_DEFINES := BUILDCFG \
                  WICED_USE_AUDIO \
                  TX_PACKET_POOL_SIZE=3 \
                  RX_PACKET_POOL_SIZE=32 \
                  WICED_DCT_INCLUDE_BT_CONFIG


VALID_OSNS_COMBOS  := ThreadX-NetX_Duo

VALID_PLATFORMS    := BCM9WCD1AUDIO \
                      BCM943909FCBU \
                      BCM943909B0FCBU \
                      BCM943909WCD1 \
                      BCM943909WCD1_3
