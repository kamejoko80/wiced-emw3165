#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Gedday

GEDDAY_LIBRARY_NAME :=Gedday.$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(GEDDAY_LIBRARY_NAME)),)
$(NAME)_PREBUILT_LIBRARY :=$(GEDDAY_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)Gedday_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(GEDDAY_LIBRARY_NAME)),)

GLOBAL_INCLUDES := .

ifeq (NETWORK,LwIP)
GLOBAL_DEFINES += LWIP_AUTOIP=1
endif

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)