#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_amqps

$(NAME)_SOURCES := amqps.c \

$(NAME)_COMPONENTS := protocols/AMQP

$(NAME)_RESOURCES  := apps/amqps/amqps_client_cert.cer \
                      apps/amqps/amqps_client_key.key \
                      apps/amqps/amqps_root_cacert.cer