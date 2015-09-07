#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_iperf

$(NAME)_SOURCES := iperf.c

$(NAME)_COMPONENTS := test/iperf
$(NAME)_COMPONENTS += utilities/command_console

ifeq ($(PLATFORM),BCM943362WCD2)
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=5 RX_PACKET_POOL_SIZE=5
GLOBAL_DEFINES += PBUF_POOL_TX_SIZE=5 PBUF_POOL_RX_SIZE=5
else
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=18 RX_PACKET_POOL_SIZE=18
GLOBAL_DEFINES += PBUF_POOL_TX_SIZE=10 PBUF_POOL_RX_SIZE=10
endif

WIFI_CONFIG_DCT_H := wifi_config_dct.h
