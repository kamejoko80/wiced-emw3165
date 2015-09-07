#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_OTA_tftp

$(NAME)_SOURCES := tftp.c

$(NAME)_COMPONENTS := daemons/tftp

WIFI_CONFIG_DCT_H := wifi_config_dct.h

#Set factory reset application to be this same application.
FR_APP    := $(OUTPUT_DIR)/binary/$(CLEANED_BUILD_STRING).stripped.elf
DCT_IMAGE := $(OUTPUT_DIR)/DCT.stripped.elf