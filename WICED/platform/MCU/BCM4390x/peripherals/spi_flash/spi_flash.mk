#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := SPI_Flash_43909_Library_$(PLATFORM)

$(NAME)_SOURCES := spi_flash.c

# Add one or more of the following defines to your platform makefile :
#                   SFLASH_SUPPORT_MACRONIX_PARTS
#                   SFLASH_SUPPORT_SST_PARTS
#                   SFLASH_SUPPORT_EON_PARTS

GLOBAL_INCLUDES := ../../../../../../libraries/drivers/spi_flash

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

$(eval $(call PLATFORM_LOCAL_DEFINES_INCLUDES_43909, ../..))
