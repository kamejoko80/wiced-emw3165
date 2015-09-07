#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_command_console

GLOBAL_INCLUDES := .

$(NAME)_SOURCES := command_console.c

ifeq ($(CONSOLE_ENABLE_WL),1)
$(NAME)_COMPONENTS += test/wl_tool
GLOBAL_DEFINES     += CONSOLE_ENABLE_WL
endif

#include $(CURDIR)trace/trace.mk
