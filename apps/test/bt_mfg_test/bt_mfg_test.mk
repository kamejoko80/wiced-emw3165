#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Bluetooth_Manufacturing_Test_Application

$(NAME)_SOURCES    := bt_mfg_test.c

$(NAME)_COMPONENTS += drivers/bluetooth/mfg_test

GLOBAL_DEFINES     += WICED_DISABLE_STDIO     # Disable default STDIO. Manufacturing Test uses raw UART
GLOBAL_DEFINES     += BT_BUS_RX_FIFO_SIZE=512 # Set Bluetooth UART RX FIFO size to large enough number

VALID_PLATFORMS    := BCM9WCDPLUS114 \
                      BCM943341WCD1 \
                      BCM9WCD1AUDIO \
                      BCM943909WCD1 \
                      BCM943909WCD1_3 \
                      BCM9WCD2REFAD_BCM943438WLPTH_2
