#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_audioplc

GLOBAL_INCLUDES += .
GLOBAL_INCLUDES += public

AUDIOPLC_LIBRARY_NAME :=audioplc.$(RTOS).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(AUDIOPLC_LIBRARY_NAME)),)
$(info Using PREBUILT:  $(AUDIOPLC_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY := $(AUDIOPLC_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)audioplc_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(AUDIOPLC_LIBRARY_NAME)),)
