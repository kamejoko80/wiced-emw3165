#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_WICED_Bluetooth_Driver_for_BCM$(BT_CHIP)$(BT_CHIP_REVISION)

GLOBAL_INCLUDES += include

# Include BTE stack as component
$(NAME)_COMPONENTS := libraries/drivers/bluetooth/BTE

$(NAME)_SOURCES := firmware/$(BT_CHIP)$(BT_CHIP_REVISION)/bt_firmware_image.c
