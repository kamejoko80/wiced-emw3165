#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_Manufacturing_Test_for_BCM$(BT_CHIP)$(BT_CHIP_REVISION)

################################################################################
# Default settings                                                             #
################################################################################

ifndef BT_TRANSPORT_BUS
BT_TRANSPORT_BUS := UART
endif

################################################################################
# Add defines for specific variants and check variants for validity            #
################################################################################

ifndef BT_CHIP
$(error ERROR: BT_CHIP is undefined!)
endif

ifndef BT_CHIP_REVISION
$(error ERROR: BT_CHIP_REVISION is undefined!)
endif

ifndef BT_TRANSPORT_BUS
$(error ERROR: BT_TRANSPORT_BUS is undefined!)
endif

################################################################################
# Specify global include directories                                           #
################################################################################

GLOBAL_INCLUDES  := . \
                    ../include \
                    internal/bus \
                    internal/firmware \
                    internal/packet \
                    internal/transport/driver \
                    internal/transport/HCI \
                    internal/transport/thread

$(NAME)_SOURCES  += bt_mfg_test.c \
                    internal/bus/$(BT_TRANSPORT_BUS)/bt_bus.c \
                    internal/transport/driver/$(BT_TRANSPORT_BUS)/bt_transport_driver_receive.c \
                    internal/transport/driver/$(BT_TRANSPORT_BUS)/bt_transport_driver.c \
                    internal/transport/thread/bt_transport_thread.c \
                    internal/packet/bt_packet.c \
                    internal/firmware/bt_firmware.c \
                    ../firmware/$(BT_CHIP)$(BT_CHIP_REVISION)/bt_firmware_image.c

$(NAME)_COMPONENTS += utilities/linked_list