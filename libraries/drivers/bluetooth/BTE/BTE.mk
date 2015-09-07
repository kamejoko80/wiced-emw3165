#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_Embedded_Stack_for_WICED

GLOBAL_INCLUDES := . \
                   ../include \
                   WICED \
                   Components/stack/include \
                   Projects/bte/main
                   
GLOBAL_DEFINES +=  BUILDCFG \
                   BLUETOOTH_BTE

ifneq ($(wildcard $(CURDIR)BTE.$(RTOS).$(NETWORK).$(HOST_ARCH).release.a),)
$(NAME)_PREBUILT_LIBRARY := BTE.$(RTOS).$(NETWORK).$(HOST_ARCH).release.a
else
# Build from source (Broadcom internal)
include $(CURDIR)BTE_src.mk
endif
