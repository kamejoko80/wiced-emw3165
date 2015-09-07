#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME               := SPI_Master_Example_Application

$(NAME)_COMPONENTS := libraries/drivers/spi_slave/master

$(NAME)_SOURCES    := spi_master_app.c

VALID_PLATFORMS    := BCM943362WCD4
