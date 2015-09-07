#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := WWD_ThreadX_Interface

GLOBAL_INCLUDES := .

ifeq ($(TOOLCHAIN_NAME),IAR)
$(NAME)_SOURCES  := wwd_rtos.c \
                    low_level_init.c \
                    interrupt_handlers_IAR.s

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

$(NAME)_LINK_FILES := interrupt_handlers_IAR.o

else
$(NAME)_SOURCES  := wwd_rtos.c

ifneq ($(filter $(HOST_ARCH), ARM_CM3 ARM_CM4),)
$(NAME)_SOURCES  += CM3_CM4/low_level_init.c
GLOBAL_INCLUDES  += CM3_CM4
else
ifneq ($(filter $(HOST_ARCH), ARM_CR4),)
$(NAME)_SOURCES    += CR4/low_level_init.S
$(NAME)_SOURCES    += CR4/timer_isr.c
$(NAME)_LINK_FILES += CR4/timer_isr.o
GLOBAL_INCLUDES    += CR4
else
$(error No ThreadX low_level_init function for architecture $(HOST_ARCH))
endif
endif


$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

endif

$(NAME)_CHECK_HEADERS := \
                         wwd_rtos.h