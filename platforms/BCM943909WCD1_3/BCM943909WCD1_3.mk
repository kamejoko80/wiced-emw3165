#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Platform_BCM943909WCD1_3

$(NAME)_SOURCES := platform.c

ifdef BUILD_ROM
$(NAME)_SOURCES += ROM_build/reference.S
$(NAME)_COMPONENTS += BESL
$(NAME)_COMPONENTS += WICED/RTOS/ThreadX/WICED
RTOS:=ThreadX
NETWORK:=NetX_Duo

endif

# FreeRTOS is not yet supported for Cortex-R4
VALID_OSNS_COMBOS := ThreadX-NetX ThreadX-NetX_Duo NoOS-NoNS

# BCM94390WCD1_3-specific make targets
EXTRA_TARGET_MAKEFILES +=$(SOURCE_ROOT)platforms/BCM943909WCD1_3/BCM943909WCD1_3_targets.mk

WLAN_CHIP            := 43909
WLAN_CHIP_REVISION   := B0
HOST_MCU_FAMILY      := BCM4390x
HOST_MCU_VARIANT     := BCM43909
HOST_MCU_PART_NUMBER := BCM43909KRFBG

# BCM943909WCD1 rev 3 includes BCM20707 BT chip
BT_CHIP          := 20707
BT_CHIP_REVISION := A1

GLOBAL_DEFINES += $$(if $$(NO_CRLF_STDIO_REPLACEMENT),,CRLF_STDIO_REPLACEMENT)

ifndef USES_BOOTLOADER_OTA
USES_BOOTLOADER_OTA :=1
endif

INTERNAL_MEMORY_RESOURCES =

$(NAME)_COMPONENTS += filesystems/wicedfs

# Uses spi_flash interface but implements own functions
GLOBAL_INCLUDES += ../../Library/drivers/spi_flash

#GLOBAL_DEFINES += PLATFORM_HAS_OTP


# Define default RTOS, bus, and network stack.
ifndef RTOS
RTOS:=ThreadX
$(NAME)_COMPONENTS += ThreadX
endif

VALID_BUSES :=SoC.43909

ifndef BUS
BUS:=SoC/43909
endif

ifndef NETWORK
NETWORK:=NetX_Duo
$(NAME)_COMPONENTS += NetX_Duo
endif

GLOBAL_INCLUDES  += .
GLOBAL_INCLUDES  += ../../WICED/platform/include
GLOBAL_INCLUDES  += ../../libraries/bluetooth/include

$(NAME)_LINK_FILES :=

GLOBAL_DEFINES += WICED_DISABLE_MCU_POWERSAVE
GLOBAL_DEFINES += APP_NAME=$$(SLASH_QUOTE_START)$(APP)$$(SLASH_QUOTE_END)
GLOBAL_DEFINES += WICED_DISABLE_CONFIG_TLS
GLOBAL_DEFINES += SFLASH_SUPPORT_MACRONIX_PARTS
## FIXME: need to uncomment these with a proper solution post-43909 Alpha.
#GLOBAL_DEFINES += PLATFORM_BCM943909WCD1_3
#GLOBAL_DEFINES += WICED_BT_USE_RESET_PIN
#GLOBAL_DEFINES += GSIO_UART
GLOBAL_DEFINES += WICED_DCT_INCLUDE_BT_CONFIG

ifeq ($(BUS),SoC/43909)
WIFI_IMAGE_DOWNLOAD := buffered
else
$(error This platform only supports SoC/43909 bus protocol)
endif

#PRE_APP_BUILDS := generated_mac_address.txt

# WICED APPS
# APP0 and FILESYSTEM_IMAGE are reserved main app and resources file system
# FR_APP :=
# DCT_IMAGE :=
# OTA_APP :=
# FILESYSTEM_IMAGE :=
# WIFI_FIRMWARE :=
# APP0 :=
# APP1 :=
# APP2 :=

# WICED APPS LOOKUP TABLE
APPS_LUT_HEADER_LOC := 0x10000
APPS_START_SECTOR := 17

ifneq ($(APP),bootloader)
ifneq ($(APP),tiny_bootloader)
ifneq ($(APP),sflash_write)

ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))
$(NAME)_SOURCES += platform_audio.c \
                   wiced_audio.c

$(NAME)_COMPONENTS += drivers/audio/WM8533 \
                      drivers/audio/AK4954
endif

# Platform specific MIN MAX range for WM8533 DAC in decibels
GLOBAL_DEFINES += MIN_WM8533_DB_LEVEL=-53.0
GLOBAL_DEFINES += MAX_WM8533_DB_LEVEL=6.0

ifneq ($(MAIN_COMPONENT_PROCESSING),1)
$(info +-----------------------------------------------------------------------------------------------------+ )
$(info | IMPORTANT NOTES                                                                                     | )
$(info +-----------------------------------------------------------------------------------------------------+ )
$(info | Wi-Fi MAC Address                                                                                   | )
$(info |    The target Wi-Fi MAC address is defined in <WICED-SDK>/generated_mac_address.txt                 | )
$(info |    Ensure each target device has a unique address.                                                  | )
$(info +-----------------------------------------------------------------------------------------------------+ )
$(info | MCU & Wi-Fi Power Save                                                                              | )
$(info |    It is *critical* that applications using WICED Powersave API functions connect an accurate 32kHz | )
$(info |    reference clock to the sleep clock input pin of the WLAN chip. Please read the WICED Powersave   | )
$(info |    Application Note located in the documentation directory if you plan to use powersave features.   | )
$(info +-----------------------------------------------------------------------------------------------------+ )
endif
endif
endif
endif
# uncomment for non-bootloader apps to avoid adding a DCT
# NODCT := 1


# GLOBAL_DEFINES += STOP_MODE_SUPPORT PRINTF_BLOCKING
