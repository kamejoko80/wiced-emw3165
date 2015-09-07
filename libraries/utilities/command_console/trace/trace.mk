#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
NAME := Lib_command_console_trace

$(NAME)_SOURCES += trace.c \
                   trace_hook.c

#==============================================================================
# Configuration
#==============================================================================
TRACE_BUFFERED       := 1
TRACE_BUFFERED_PRINT := 1
TRACE_GPIO           := 1


ifneq ($(wildcard $(CURDIR)trace/$(RTOS)_trace.h),)
# Make sure to include this trace_macros.h first (to ensure the trace macros are defined accordingly)
GLOBAL_CFLAGS   += -include $(CURDIR)trace/$(RTOS)_trace.h
GLOBAL_CXXFLAGS += -include $(CURDIR)trace/$(RTOS)_trace.h

ifeq (FreeRTOS, $(RTOS))
# Needed to store useful tracing information
GLOBAL_DEFINES  += configUSE_TRACE_FACILITY=1
endif

#==============================================================================
# Defines and sources
#==============================================================================
ifeq (1, $(TRACE_BUFFERED))
ifneq ($(wildcard $(CURDIR)trace/gpio/$(HOST_MCU_FAMILY)_gpio_trace.c),)
GLOBAL_CFLAGS   += -include $(CURDIR)trace/clocktimer/ARM_CM3_DWT.h
GLOBAL_CXXFLAGS += -include $(CURDIR)trace/clocktimer/ARM_CM3_DWT.h

$(NAME)_DEFINES += TRACE_ENABLE_BUFFERED
$(NAME)_SOURCES += buffered/buffered_trace.c
$(NAME)_SOURCES += clocktimer/$(HOST_ARCH)_DWT.c

ifeq (1, $(TRACE_BUFFERED_PRINT))
$(NAME)_DEFINES += TRACE_ENABLE_BUFFERED_PRINT
$(NAME)_SOURCES += buffered/print/buffered_trace_print.c
endif

ifeq (1, $(TRACE_GPIO))
ifneq ($(wildcard $(CURDIR)trace/gpio/$(HOST_MCU_FAMILY)_gpio_trace.c),)
$(NAME)_DEFINES += TRACE_ENABLE_GPIO
$(NAME)_SOURCES += gpio/gpio_trace.c
# Note that we need to use delayed variable expansion here.
# See: http://www.bell-labs.com/project/nmake/newsletters/issue025.html
$(NAME)_SOURCES += gpio/$(HOST_MCU_FAMILY)_gpio_trace.c
else
$(info GPIO tracing not implemented for MCU $(HOST_MCU_FAMILY) - GPIO tracing will be disabled )
endif
endif

else
$(info Clocktimer tracing not implemented for Host Architecture $(HOST_ARCH) - Buffered tracing will be disabled )
endif
endif

else
$(info Tracing not implemented for RTOS $(RTOS) - tracing will be disabled)
endif