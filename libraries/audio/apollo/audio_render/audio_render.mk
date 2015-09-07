#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_audio_render

AUDIO_RENDER_LIBRARY_NAME :=audio_render.$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a

ifneq ($(wildcard $(CURDIR)$(AUDIO_RENDER_LIBRARY_NAME)),)
$(info Using PREBUILT:  $(AUDIO_RENDER_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY :=$(AUDIO_RENDER_LIBRARY_NAME)
else
# Build from source (Broadcom internal)
include $(CURDIR)audio_render_src.mk
endif # ifneq ($(wildcard $(CURDIR)$(AUDIO_RENDER_LIBRARY_NAME)),)

GLOBAL_INCLUDES += .

$(NAME)_CFLAGS :=

# Do not enable the timing log unless PLATFORM_DDR_BASE is defined.
#GLOBAL_DEFINES     += AUDIO_RENDER_TIMING_LOG

ifneq (,$(findstring AUDIO_RENDER_TIMING_LOG,$(GLOBAL_DEFINES)))
$(info Enabling audio render timing log.)
ifdef WICED_ENABLE_TRACEX
$(info WARNING: TraceX and timing log both enabled!)
endif

GLOBAL_DEFINES     += AUDIO_RENDER_BUFFER_DDR_OFFSET=0x0
GLOBAL_DEFINES     += AUDIO_RENDER_BUFFER_SIZE=0x200000
endif

ifdef WICED_ENABLE_TRACEX
GLOBAL_DEFINES     += AUDIO_RENDER_ENABLE_TRACE_EVENTS
endif
