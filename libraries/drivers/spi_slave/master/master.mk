#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_SPI_Master_Library

$(NAME)_SOURCES := spi_master.c

$(NAME)_CFLAGS  := $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)

GLOBAL_INCLUDES += .