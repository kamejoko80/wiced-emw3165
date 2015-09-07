#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_DHCP_Server

$(NAME)_SOURCES := dhcp_server.c
GLOBAL_INCLUDES := .

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)




$(NAME)_UNIT_TEST_SOURCES := unit/dhcp_server_unit.cpp unit/dhcp_server_test_content.c
MOCKED_FUNCTIONS += wiced_udp_receive wiced_udp_send