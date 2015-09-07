#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_RESTful_Smart_server

RESTFUL_SMART_SERVER_HOST := WICED

$(NAME)_COMPONENTS += daemons/HTTP_server \
                      daemons/bt_internet_gateway/smartbridge \
                      drivers/bluetooth

$(NAME)_SOURCES    := restful_smart_constants.c \
                      host/$(RESTFUL_SMART_SERVER_HOST)/restful_smart_server.c \
                      host/$(RESTFUL_SMART_SERVER_HOST)/restful_smart_response.c \
                      host/$(RESTFUL_SMART_SERVER_HOST)/restful_smart_ble.c

$(NAME)_INCLUDES   := . \
                      host/$(RESTFUL_SMART_SERVER_HOST)

GLOBAL_INCLUDES    := .
