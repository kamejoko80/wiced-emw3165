#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := app_nfc_tag_writer

$(NAME)_SOURCES    := tag_writer.c
$(NAME)_COMPONENTS := drivers/nfc

GLOBAL_DEFINES    += WICED_DISABLE_MCU_POWERSAVE

VALID_PLATFORMS    := BCM943341WCD1