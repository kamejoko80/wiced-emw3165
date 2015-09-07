#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_sep2_client

#App
$(NAME)_SOURCES := sep2_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_dcap_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_der_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_drlc_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_edev_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_edinfo_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_flow_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_meter_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_meter_mirror_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_msg_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_price_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_time_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_client.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_tables.c \
		   ../../../../../gridwiz_sow4a/release/apps/client/app_utils.c

#KAL
$(NAME)_SOURCES += ../../../../../gridwiz_sow4a/release/platform/kal/kbp_memory.c \
		   ../../../../../gridwiz_sow4a/release/platform/kal/kbp_message.c \
		   ../../../../../gridwiz_sow4a/release/platform/kal/kbp_misc.c \
		   ../../../../../gridwiz_sow4a/release/platform/kal/kbp_semaphore.c \
		   ../../../../../gridwiz_sow4a/release/platform/kal/kbp_task.c \
		   ../../../../../gridwiz_sow4a/release/platform/kal/kbp_timebase.c \
		   ../../../../../gridwiz_sow4a/release/platform/kal/kbp_timer.c \
		   ../../../../../gridwiz_sow4a/release/platform/kal/kup_threadx.c \
		   ../../../../../gridwiz_sow4a/release/platform/ports/psal_port_gridwiz.c
#           ../../../../../gridwiz_sow4a/release/platform/kal/kbp_alarm.c 
		   
#ISAL
$(NAME)_SOURCES += ../../../../../gridwiz_sow4a/release/platform/isal/isal_port_netx.c
#ISAL_INC  = -I release/platform/include -I $(WICED_WICED)/RTOS/ThreadX/ver5.6 -I $(WICED_WICED)/Network/NetX_Duo/ver5.7_sp2 -I $(WICED_WICED)/RTOS/ThreadX/ver5.6/Cortex_M3_M4/GCC

$(NAME)_INCLUDES := -I ../../../../../gridwiz_sow4a/release/include -I ../../../../../gridwiz_sow4a/release/platform/include -I ../../../../../gridwiz_sow4a/release/apps/include -I ../../../../../gridwiz_sow4a/release/platform/kal

$(NAME)_CFLAGS   := -Wno-missing-braces -Wno-error=missing-braces -std=gnu99 \
					-DWICED_DISABLE_MCU_POWERSAVE -DIPV4_CLIENT

ifneq (,$(findstring IPV6_CLIENT, $($(NAME)_CFLAGS)))
# These modules may only be enabled if IPV6_CLIENT is enabled
$(NAME)_LINK_FILES := ../../../../../../../../gridwiz_sow4a/release/lib/g2hsep2_client6.o
else
$(NAME)_LINK_FILES := ../../../../../../../../gridwiz_sow4a/release/lib/g2hsep2_client4.o
endif

					

WIFI_CONFIG_DCT_H := wifi_config_dct.h
