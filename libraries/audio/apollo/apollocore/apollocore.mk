#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_apollocore

APOLLOCORE_LIBRARY_NAME :=apollocore.$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(APOLLOCORE_LIBRARY_NAME)),)
$(info Using PREBUILT:  $(APOLLOCORE_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY :=$(APOLLOCORE_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)apollocore_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(APOLLOCORE_LIBRARY_NAME)),)

GLOBAL_INCLUDES += .

$(NAME)_CFLAGS :=
