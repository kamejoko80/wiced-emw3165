#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_audiopcm

GLOBAL_INCLUDES += .

AUDIOPCM_LIBRARY_NAME :=audiopcm.$(RTOS).$(NETWORK).$(HOST_ARCH).$(BUILD_TYPE).a


ifneq ($(wildcard $(CURDIR)$(AUDIOPCM_LIBRARY_NAME)),)

# Use the available prebuilt
$(info Using PREBUILT:  $(AUDIOPCM_LIBRARY_NAME))
$(NAME)_PREBUILT_LIBRARY := $(AUDIOPCM_LIBRARY_NAME)

else

# Build from source (Broadcom internal)
$(info Building SRC:  $(AUDIOPCM_LIBRARY_NAME))
include $(CURDIR)audiopcm_src.mk

endif
