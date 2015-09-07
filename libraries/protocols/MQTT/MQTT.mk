#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Lib_MQTT

$(NAME)_SOURCES := MQTTClient/src/MQTTClient.c \
				   MQTTClient/src/MQTTWiced.c \
                   MQTTPacket/src/MQTTConnectClient.c \
                   MQTTPacket/src/MQTTConnectServer.c \
                   MQTTPacket/src/MQTTDeserializePublish.c \
                   MQTTPacket/src/MQTTFormat.c \
                   MQTTPacket/src/MQTTPacket.c \
                   MQTTPacket/src/MQTTSerializePublish.c \
                   MQTTPacket/src/MQTTSubscribeClient.c \
                   MQTTPacket/src/MQTTSubscribeServer.c \
                   MQTTPacket/src/MQTTUnsubscribeClient.c \
                   MQTTPacket/src/MQTTUnsubscribeServer.c

GLOBAL_INCLUDES := MQTTClient/src \
                   MQTTPacket/src

#$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)
