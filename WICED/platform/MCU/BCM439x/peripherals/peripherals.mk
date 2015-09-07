#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := BCM439x_Peripheral_Drivers

BCM439X_PERIPHERAL_LIBRARY_NAME :=BCM439X_Peripheral.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(BCM439X_PERIPHERAL_LIBRARY_NAME)),)
$(NAME)_PREBUILT_LIBRARY := $(BCM439X_PERIPHERAL_LIBRARY_NAME)
else
include $(CURDIR)peripherals_src.mk
endif
