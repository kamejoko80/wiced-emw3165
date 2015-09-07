#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_OTA_Server

$(NAME)_SOURCES := wiced_ota_server.c \
                   ota_server_web_page.c \
                   ota_server.c
GLOBAL_INCLUDES := .
