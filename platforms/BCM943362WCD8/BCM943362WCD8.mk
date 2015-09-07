#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := Platform_BCM943362WCD8

WLAN_CHIP            := 43362
WLAN_CHIP_REVISION   := A2
HOST_MCU_FAMILY      := SAM4S

INTERNAL_MEMORY_RESOURCES = $(ALL_RESOURCES)

ifndef BUS
BUS := SDIO
endif

EXTRA_TARGET_MAKEFILES +=  $(MAKEFILES_PATH)/standard_platform_targets.mk

VALID_BUSES := SDIO

ifeq ($(BUS),SDIO)
ifneq ($(MULTI_APP_WIFI_FIRMWARE),)
# Setting some internal build parameters
WIFI_FIRMWARE           := $(MULTI_APP_WIFI_FIRMWARE)
WIFI_FIRMWARE_LOCATION 	:= WIFI_FIRMWARE_IN_MULTI_APP
GLOBAL_DEFINES          += WIFI_FIRMWARE_IN_MULTI_APP
endif
endif

# Global includes
GLOBAL_INCLUDES  := .

GLOBAL_DEFINES  += __SAM4S16B__ \
                   ARM_MATH_CM4=true \
                   CONFIG_SYSCLK_SOURCE=SYSCLK_SRC_PLLACK \
                   CONFIG_SYSCLK_PRES=SYSCLK_PRES_1 \
                   CONFIG_PLL0_SOURCE=PLL_SRC_MAINCK_XTAL \
                   CONFIG_PLL0_MUL=41 \
                   CONFIG_PLL0_DIV=4 \
                   BOARD=USER_BOARD \
                   BOARD_FREQ_SLCK_XTAL=32768 \
                   BOARD_FREQ_SLCK_BYPASS=32768 \
                   BOARD_FREQ_MAINCK_XTAL=12000000 \
                   BOARD_FREQ_MAINCK_BYPASS=12000000 \
                   BOARD_OSC_STARTUP_US=2000 \
                   BOARD_MCK=123000000 \
                   CPU_CLOCK_HZ=123000000 \
                   PERIPHERAL_CLOCK_HZ=123000000 \
                   SDIO_4_BIT \
                   TRACE_LEVEL=0 \
                   WAIT_MODE_SUPPORT \
                   WAIT_MODE_ENTER_DELAY_CYCLES=3 \
                   WAIT_MODE_EXIT_DELAY_CYCLES=34

# Components
$(NAME)_COMPONENTS += drivers/spi_flash

GLOBAL_DEFINES += $$(if $$(NO_CRLF_STDIO_REPLACEMENT),,CRLF_STDIO_REPLACEMENT)


# Source files
$(NAME)_SOURCES := platform.c

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
APPS_LUT_HEADER_LOC := 0x0000
APPS_START_SECTOR := 1

ifneq ($(APP),bootloader)
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
