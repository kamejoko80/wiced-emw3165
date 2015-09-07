#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := RESTful_Smart_Server_Application

$(NAME)_SOURCES    := restful_smart_server_app.c \
                      wiced_bt_cfg.c

$(NAME)_COMPONENTS += daemons/bt_internet_gateway/restful_smart_server

VALID_PLATFORMS    := BCM9WCDPLUS114 \
                      BCM943341WCD1 \
                      BCM943909WCD1 \
                      BCM943909WCD1_3 \
                      BCM9WCD1AUDIO \
                      BCM943438WLPTH_2

WIFI_CONFIG_DCT_H  := wifi_config_dct.h
