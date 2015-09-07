#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_Gpio_Test

$(NAME)_SOURCES := gpio.c

# These do not have any LEDs/Buttons
INVALID_PLATFORMS := BCM943909WCD1 BCM943909FCBU BCM943909WCD1_3 BCM943909B0FCBU