#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := WWD_NoOS_Interface

GLOBAL_INCLUDES := .
$(NAME)_SOURCES  := wwd_rtos.c

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

ifneq ($(filter $(HOST_ARCH), ARM_CM3 ARM_CM4),)
NOOS_ARCH:=Cortex_M3_M4
else
ifneq ($(filter $(HOST_ARCH), ARM_CR4),)
NOOS_ARCH:=Cortex_R4
endif
endif

GLOBAL_INCLUDES += $(NOOS_ARCH)

GLOBAL_INCLUDES += $(NOOS_ARCH)

$(NAME)_SOURCES  += $(NOOS_ARCH)/noos.c

$(NAME)_CHECK_HEADERS := \
                         wwd_rtos.h