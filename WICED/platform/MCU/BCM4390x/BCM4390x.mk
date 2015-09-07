#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = MCU_BCM4390x

HOST_ARCH  := ARM_CR4

# Host MCU alias for OpenOCD
HOST_OPENOCD := BCM4390x

VALID_BUSES := SoC/43909

GLOBAL_INCLUDES :=  . \
                    .. \
                    ../../$(HOST_ARCH) \
                    WAF \
                    peripherals \
                    peripherals/include \

ifdef BUILD_ROM
$(NAME)_COMPONENTS += WICED/platform/MCU/BCM4390x/ROM_build
endif

# $(1) is the relative path to the platform directory
define PLATFORM_LOCAL_DEFINES_INCLUDES_43909
$(NAME)_INCLUDES += $(1)/peripherals/include

$(NAME)_DEFINES += BCMDRIVER \
                   BCM_WICED \
                   BCM_CPU_PREFETCH \
                   BCMCHIPID=BCM43909_CHIP_ID \
                   UNRELEASEDCHIP \
                   BCMM2MDEV_ENABLED \
                   CFG_GMAC

ifeq ($(WLAN_CHIP_REVISION),A0)
$(NAME)_DEFINES += BCMCHIPREV=0
endif
ifeq ($(WLAN_CHIP_REVISION),B0)
$(NAME)_DEFINES += BCMCHIPREV=1
endif
endef

$(eval $(call PLATFORM_LOCAL_DEFINES_INCLUDES_43909, .))

$(NAME)_COMPONENTS  += $(TOOLCHAIN_NAME)
$(NAME)_COMPONENTS  += MCU/BCM4390x/peripherals
$(NAME)_COMPONENTS  += utilities/ring_buffer

GLOBAL_CFLAGS   += $$(CPU_CFLAGS)    $$(ENDIAN_CFLAGS_LITTLE)
GLOBAL_CXXFLAGS += $$(CPU_CXXFLAGS)  $$(ENDIAN_CXXFLAGS_LITTLE)
GLOBAL_ASMFLAGS += $$(CPU_ASMFLAGS)  $$(ENDIAN_ASMFLAGS_LITTLE)
GLOBAL_LDFLAGS  += $$(CPU_LDFLAGS)   $$(ENDIAN_LDFLAGS_LITTLE)

ifndef $(RTOS)_SYS_STACK
$(RTOS)_SYS_STACK=0
endif
ifndef $(RTOS)_FIQ_STACK
$(RTOS)_FIQ_STACK=0
endif
ifndef $(RTOS)_IRQ_STACK
$(RTOS)_IRQ_STACK=1024
endif

ifeq ($(TOOLCHAIN_NAME),GCC)
GLOBAL_LDFLAGS  += -nostartfiles
GLOBAL_LDFLAGS  += -Wl,--defsym,START_STACK_SIZE=$($(RTOS)_START_STACK) \
                   -Wl,--defsym,FIQ_STACK_SIZE=$($(RTOS)_FIQ_STACK) \
                   -Wl,--defsym,IRQ_STACK_SIZE=$($(RTOS)_IRQ_STACK) \
                   -Wl,--defsym,SYS_STACK_SIZE=$($(RTOS)_SYS_STACK)
GLOBAL_ASMFLAGS += --defsym SYS_STACK_SIZE=$($(RTOS)_SYS_STACK) \
                   --defsym FIQ_STACK_SIZE=$($(RTOS)_FIQ_STACK) \
                   --defsym IRQ_STACK_SIZE=$($(RTOS)_IRQ_STACK)
endif


$(NAME)_SOURCES := vector_table_$(TOOLCHAIN_NAME).S \
                   start_$(TOOLCHAIN_NAME).S \
                   BCM4390x_platform.c \
                   wwd_platform.c \
                   wwd_bus.c \
                   WAF/waf_platform.c \
                   ../platform_resource.c \
                   ../platform_stdio.c \
                   ../wiced_platform_common.c \
                   ../wiced_apps_common.c \
                   ../wiced_waf_common.c \
                   ../wiced_dct_external_common.c \
                   ../../$(HOST_ARCH)/crt0_$(TOOLCHAIN_NAME).c \
                   ../../$(HOST_ARCH)/platform_cache.c \
                   ../../$(HOST_ARCH)/platform_cache_asm.S \
                   platform_unhandled_isr.c \
                   platform_vector_table.c \
                   platform_filesystem.c \
                   platform_tick.c \
                   platform_chipcommon.c \
                   platform_deep_sleep.c \
                   ../platform_nsclock.c \
                   ../../$(HOST_ARCH)/exception_handlers.c

#                   HardFault_handler.c \
#                   wwd_platform.c \
#                   $(BUS)/wwd_bus.c \
#                   gpio_irq.c \
#                   watchdog.c \

ifndef BUILD_ROM
$(NAME)_SOURCES += rom_jump_functions.S
endif

# These need to be forced into the final ELF since they are not referenced otherwise
$(NAME)_LINK_FILES := ../../$(HOST_ARCH)/crt0_$(TOOLCHAIN_NAME).o \
                      vector_table_$(TOOLCHAIN_NAME).o \
                      start_$(TOOLCHAIN_NAME).o

#                      ../../$(HOST_ARCH)/hardfault_handler.o

$(NAME)_CFLAGS  = $(COMPILER_SPECIFIC_PEDANTIC_CFLAGS) -I$(SOURCE_ROOT)./

# Add maximum and default watchdog timeouts to definitions. Warning: Do not change MAX_WATCHDOG_TIMEOUT_SECONDS
MAX_WATCHDOG_TIMEOUT_SECONDS    = 22
MAX_WAKE_FROM_SLEEP_DELAY_TICKS = 500
GLOBAL_DEFINES += MAX_WATCHDOG_TIMEOUT_SECONDS=$(MAX_WATCHDOG_TIMEOUT_SECONDS) MAX_WAKE_FROM_SLEEP_DELAY_TICKS=$(MAX_WAKE_FROM_SLEEP_DELAY_TICKS)

# Tell that platform has data cache and set cache line size
GLOBAL_DEFINES += PLATFORM_L1_CACHE_SHIFT=5

# Tell that platform supports deep sleep
GLOBAL_DEFINES += PLATFORM_DEEP_SLEEP

# Need to be removed after WLAN firmware start to use SR
GLOBAL_DEFINES += PLATFORM_WLAN_POWERSAVE=0

# Tell that platform has special WLAN features
GLOBAL_DEFINES += BCM43909

# ARM DSP/SIMD instruction set support:
# use this define to embed specific instructions
# related to the ARM DSP set.
# If further defines are needed for specific sub-set (CR4, floating point)
# make sure you protect them under this global define.
GLOBAL_DEFINES += ENABLE_ARM_DSP_INSTRUCTIONS

# DCT linker script
DCT_LINK_SCRIPT += $(TOOLCHAIN_NAME)/dct$(LINK_SCRIPT_SUFFIX)

ifeq ($(APP),bootloader)
####################################################################################
# Building bootloader
####################################################################################
DEFAULT_LINK_SCRIPT += $(TOOLCHAIN_NAME)/bootloader$(LINK_SCRIPT_SUFFIX)
GLOBAL_DEFINES      += bootloader_ota

else
ifeq ($(APP),tiny_bootloader)
####################################################################################
# Building tiny bootloader
####################################################################################

$(NAME)_SOURCES := $(filter-out vector_table_$(TOOLCHAIN_NAME).S, $($(NAME)_SOURCES))
$(NAME)_LINK_FILES := $(filter-out vector_table_$(TOOLCHAIN_NAME).o, $($(NAME)_LINK_FILES))
DEFAULT_LINK_SCRIPT += $(TOOLCHAIN_NAME)/tiny_bootloader$(LINK_SCRIPT_SUFFIX)
GLOBAL_DEFINES      += TINY_BOOTLOADER

else
ifneq ($(filter ota_upgrade sflash_write, $(APP)),)
####################################################################################
# Building sflash_write OR ota_upgrade
####################################################################################

PRE_APP_BUILDS      += bootloader
WIFI_IMAGE_DOWNLOAD := buffered
DEFAULT_LINK_SCRIPT := $(TOOLCHAIN_NAME)/app_without_rom$(LINK_SCRIPT_SUFFIX)
GLOBAL_DEFINES      += WICED_DISABLE_BOOTLOADER
GLOBAL_DEFINES      += __JTAG_FLASH_WRITER_DATA_BUFFER_SIZE__=32768
ifeq ($(TOOLCHAIN_NAME),IAR)
GLOBAL_LDFLAGS      += --config_def __JTAG_FLASH_WRITER_DATA_BUFFER_SIZE__=16384
endif

else

####################################################################################
# Building a stand-alone application
####################################################################################
DEFAULT_LINK_SCRIPT := $(TOOLCHAIN_NAME)/app_without_rom$(LINK_SCRIPT_SUFFIX)
#DEFAULT_LINK_SCRIPT := $(TOOLCHAIN_NAME)/app_with_rom$(LINK_SCRIPT_SUFFIX)


endif # APP=ota_upgrade OR sflash_write
endif # APP=bootloader
endif
