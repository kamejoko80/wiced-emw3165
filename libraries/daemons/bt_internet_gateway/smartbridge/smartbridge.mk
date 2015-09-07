#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_Bluetooth_SmartBridge

GLOBAL_DEFINES     += BT_TRACE_PROTOCOL=FALSE \
                      BT_USE_TRACES=FALSE

$(NAME)_COMPONENTS += utilities/linked_list

$(NAME)_SOURCES    := smartbridge.c \
                      smartbridge_cache.c

GLOBAL_INCLUDES    := .
