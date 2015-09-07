#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME               := SPI_Slave_Application

$(NAME)_COMPONENTS := libraries/drivers/spi_slave

$(NAME)_SOURCES    := spi_slave_app.c

VALID_PLATFORMS    := BCM94390WCD1 \
                      BCM94390WCD2

GLOBAL_DEFINES     += WICED_DISABLE_STDIO
