#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := BCM439x_WWD_Internal_Libraries

BCM439X_INTERNAL_LIBRARY_NAME :=BCM439X_Internal.$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(BCM439X_INTERNAL_LIBRARY_NAME)),)
$(NAME)_PREBUILT_LIBRARY := $(BCM439X_INTERNAL_LIBRARY_NAME)
else
include $(CURDIR)internal_src.mk
endif
