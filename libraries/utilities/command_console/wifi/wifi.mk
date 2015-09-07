#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_command_console_wifi

$(NAME)_SOURCES := command_console_wifi.c \
                   $(RTOS)_$(NETWORK)_wifi.c

GLOBAL_INCLUDES := .
