#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_Config_mode

$(NAME)_SOURCES := config_mode.c

GLOBAL_DEFINES :=

APPLICATION_DCT := config_mode_dct.c

$(NAME)_COMPONENTS := daemons/device_configuration
