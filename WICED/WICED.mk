#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = WICED

ifndef USES_BOOTLOADER_OTA
USES_BOOTLOADER_OTA :=1
endif

ifeq ($(BUILD_TYPE),debug)
#$(NAME)_COMPONENTS += test/malloc_debug
endif

# Check if the WICED API is being used
ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))

$(NAME)_SOURCES += internal/wifi.c \
                   internal/time.c \
                   internal/wiced_lib.c \
                   internal/management.c \
                   internal/system_monitor.c \
                   internal/wiced_cooee.c \
                   internal/wiced_easy_setup.c \
                   internal/waf.c

$(NAME)_INCLUDES := security/BESL/crypto \
                    security/BESL/include


$(NAME)_CHECK_HEADERS := internal/wiced_internal_api.h \
                         ../include/default_wifi_config_dct.h \
                         ../include/resource.h \
                         ../include/wiced.h \
                         ../include/wiced_defaults.h \
                         ../include/wiced_easy_setup.h \
                         ../include/wiced_framework.h \
                         ../include/wiced_management.h \
                         ../include/wiced_platform.h \
                         ../include/wiced_rtos.h \
                         ../include/wiced_security.h \
                         ../include/wiced_tcpip.h \
                         ../include/wiced_time.h \
                         ../include/wiced_utilities.h \
                         ../include/wiced_wifi.h

ifneq (NoNS,$(NETWORK))
$(NAME)_COMPONENTS += WICED/security/BESL \
                      protocols/DNS
GLOBAL_DEFINES += ADD_NETX_EAPOL_SUPPORT
endif

endif

# Add standard WICED 1.x components
$(NAME)_COMPONENTS += WICED/WWD

# Add WICEDFS as standard filesystem
$(NAME)_COMPONENTS += filesystems/wicedfs

# Add MCU component
$(NAME)_COMPONENTS += WICED/platform/MCU/$(HOST_MCU_FAMILY)

# Define the default ThreadX and FreeRTOS starting stack sizes
FreeRTOS_START_STACK := 800
ThreadX_START_STACK  := 800

GLOBAL_DEFINES += WWD_STARTUP_DELAY=10 \
                  BOOTLOADER_MAGIC_NUMBER=0x4d435242
                  APP_NAME=$$(SLASH_QUOTE)$(APP)$$(SLASH_QUOTE)

GLOBAL_INCLUDES := .

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS)
