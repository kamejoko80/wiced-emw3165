#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_mfg_test

$(NAME)_SOURCES := mfg_test_init.c

ifneq ($(PLATFORM),BCM943364WCD1)
$(NAME)_RESOURCES += firmware/$(WLAN_CHIP)/$(WLAN_CHIP)$(WLAN_CHIP_REVISION)-mfgtest.bin
else
# Set the WIFI firmware in multi application file system to point to firmware
MULTI_APP_WIFI_FIRMWARE   := resources/firmware/$(WLAN_CHIP)/$(WLAN_CHIP)$(WLAN_CHIP_REVISION)-mfgtest.bin
endif
NO_WIFI_FIRMWARE := YES

$(NAME)_COMPONENTS += test/wl_tool

# wl commands which dump a lot of data require big buffers.
GLOBAL_DEFINES   += WICED_PAYLOAD_MTU=8320

ifeq ($(PLATFORM),$(filter $(PLATFORM), BCM943909WCD1 BCM943909WCD1_3 ))
GLOBAL_DEFINES += TX_PACKET_POOL_SIZE=10 \
                  RX_PACKET_POOL_SIZE=10 \
                  PBUF_POOL_TX_SIZE=8 \
                  PBUF_POOL_RX_SIZE=8 \
                  WICED_ETHERNET_DESCNUM_TX=32 \
                  WICED_ETHERNET_DESCNUM_RX=8 \
                  WICED_ETHERNET_RX_PACKET_POOL_SIZE=32+WICED_ETHERNET_DESCNUM_RX
else
ifneq ($(PLATFORM),$(filter $(PLATFORM), BCM943362WCD4_LPCX1769))
GLOBAL_DEFINES   += TX_PACKET_POOL_SIZE=2 \
                    RX_PACKET_POOL_SIZE=2 \
                    PBUF_POOL_TX_SIZE=2 \
                    PBUF_POOL_RX_SIZE=2
endif
endif



# ENABLE for ethernet support
#$(NAME)_DEFINES   += MFG_TEST_ENABLE_ETHERNET_SUPPORT

INVALID_PLATFORMS := BCM943362WCD4_LPCX1769 \
                     BCM943362WCDA \
                     BCM9WCD2WLREFAD.BCM94334WLAGB \
                     BCM943909QT \
                     BCM943909B0FCBU

ifeq ($(PLATFORM),$(filter $(PLATFORM),BCM943362WCD4_LPCX1769 BCM94334WLAGB BCM943909QT BCM943909B0FCBU))
ifneq (yes,$(strip $(TESTER)))
$(error Platform not supported for Manufacturing test)
endif
endif
