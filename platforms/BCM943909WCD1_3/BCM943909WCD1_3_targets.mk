#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

.PHONY: bootloader download_bootloader no_dct download_dct download

BOOTLOADER_TARGET := waf.bootloader-NoOS-NoNS-$(PLATFORM_FULL)-$(BUS)
BOOTLOADER_LINK_FILE := $(BUILD_DIR)/$(call CONV_COMP,$(subst .,/,$(BOOTLOADER_TARGET)))/binary/$(call CONV_COMP,$(subst .,/,$(BOOTLOADER_TARGET)))$(LINK_OUTPUT_SUFFIX)
BOOTLOADER_OUT_FILE  := $(BOOTLOADER_LINK_FILE:$(LINK_OUTPUT_SUFFIX)=$(FINAL_OUTPUT_SUFFIX))
BOOTLOADER_TRX_FILE  := $(BOOTLOADER_LINK_FILE:$(LINK_OUTPUT_SUFFIX)=.trx$(FINAL_OUTPUT_SUFFIX))
BOOTLOADER_LOG_FILE  ?= $(BUILD_DIR)/bootloader.log
SFLASH_LOG_FILE      ?= $(BUILD_DIR)/sflash_writer.log
GENERATED_MAC_FILE   := $(SOURCE_ROOT)generated_mac_address.txt
TRX_CREATOR          := $(SOURCE_ROOT)WICED/platform/MCU/BCM4390x/make_trx.pl
MAC_GENERATOR        := $(TOOLS_ROOT)/mac_generator/mac_generator.pl

TINY_BOOTLOADER_TARGET := waf.tiny_bootloader-NoOS-NoNS-$(PLATFORM_FULL)-$(BUS)
TINY_BOOTLOADER_LOG_FILE ?= $(BUILD_DIR)/tiny_bootloader.log
TINY_BOOTLOADER_BIN_FILE := $(BUILD_DIR)/$(call CONV_COMP,$(subst .,/,$(TINY_BOOTLOADER_TARGET)))/binary/$(call CONV_COMP,$(subst .,/,$(TINY_BOOTLOADER_TARGET))).bin
TINY_BOOTLOADER_BIN2C_FILE := $(OUTPUT_DIR)/tiny_bootloader_bin2c.c
TINY_BOOTLOADER_BIN2C_OBJ  := $(OUTPUT_DIR)/tiny_bootloader_bin2c.o
TINY_BOOTLOADER_BIN2C_ARRAY_NAME := tinybl_bin

SFLASH_APP_TARGET := waf.sflash_write-NoOS-NoNS-$(PLATFORM_FULL)-$(BUS)
SFLASH_APP_OUTFILE := $(BUILD_DIR)/$(call CONV_COMP,$(subst .,/,$(SFLASH_APP_TARGET)))/binary/$(call CONV_COMP,$(subst .,/,$(SFLASH_APP_TARGET)))


ifneq ($(filter download, $(MAKECMDGOALS)),)
ifeq ($(filter download_apps, $(MAKECMDGOALS)),)
#Overiding user options for any thing other than the main images
FR_APP  	:=
DCT_IMAGE 	:=
OTA_APP 	:=
WIFI_FIRMWARE :=
APP1		:=
APP2		:=
endif
endif

SFLASH_APP_BCM4390 := 43909
SFLASH_APP_PLATFROM_BUS := $(subst /,_,$(PLATFORM_FULL)-$(BUS))

SFLASH_DCT_LOC:= 0x00008000

# this must be zero as defined in the ROM bootloader
SFLASH_BOOTLOADER_LOC := 0x00000000

SFLASH_FS_LOC := 0x00010000


OPENOCD_LOG_FILE ?= build/openocd_log.txt
DOWNLOAD_LOG := >> $(OPENOCD_LOG_FILE)

ifneq ($(VERBOSE),1)
BOOTLOADER_REDIRECT	= > $(BOOTLOADER_LOG_FILE)
TINY_BOOTLOADER_REDIRECT = > $(TINY_BOOTLOADER_LOG_FILE)
SFLASH_REDIRECT	= > $(SFLASH_LOG_FILE)
endif


ifeq (,$(and $(OPENOCD_PATH),$(OPENOCD_FULL_NAME)))
	$(error Path to OpenOCD has not been set using OPENOCD_PATH and OPENOCD_FULL_NAME)
endif



#APPS LOOK UP TABLE PARAMS
APPS_LUT_DOWNLOAD_DEP :=

# If the current build string is building the bootloader, don't recurse to build another bootloader
ifneq (,$(findstring waf/bootloader, $(BUILD_STRING)))
NO_BOOTLOADER_REQUIRED:=1
endif

# Bootloader is not needed when debugger downloads to RAM
ifneq (download,$(findstring download,$(MAKECMDGOALS)))
NO_BOOTLOADER_REQUIRED:=1
endif

# Do not include $(TINY_BOOTLOADER_BIN2C_OBJ) if building bootloader/tiny_bootloader/sflash_write
ifneq (,$(findstring waf/bootloader, $(BUILD_STRING))$(findstring waf/tiny_bootloader, $(BUILD_STRING))$(findstring waf/sflash_write, $(BUILD_STRING)))
NO_TINY_BOOTLOADER_REQUIRED:=1
endif

APP0                    := $(STRIPPED_LINK_OUTPUT_FILE)

# Bootloader is not needed when asked from command line
ifeq (no_tinybl,$(findstring no_tinybl,$(MAKECMDGOALS)))
NO_TINY_BOOTLOADER_REQUIRED:=1
endif
no_tinybl:
	@

# Tiny Bootloader Targets
ifneq (1,$(NO_TINY_BOOTLOADER_REQUIRED))
LINK_LIBS += $(TINY_BOOTLOADER_BIN2C_OBJ)
$(TINY_BOOTLOADER_BIN2C_OBJ):
	$(ECHO) Building Tiny Bootloader
	$(MAKE) -r -f $(SOURCE_ROOT)Makefile $(TINY_BOOTLOADER_TARGET) -I$(OUTPUT_DIR) SUB_BUILD=tiny_bootloader $(TINY_BOOTLOADER_REDIRECT)
	$(BIN2C) $(TINY_BOOTLOADER_BIN_FILE) $(TINY_BOOTLOADER_BIN2C_FILE) $(TINY_BOOTLOADER_BIN2C_ARRAY_NAME)
	$(CC) $(CPU_CFLAGS) $(COMPILER_SPECIFIC_COMP_ONLY_FLAG) $(TINY_BOOTLOADER_BIN2C_FILE) $(WICED_SDK_DEFINES) $(WICED_SDK_INCLUDES) $(COMPILER_SPECIFIC_DEBUG_CFLAGS)  $(call ADD_COMPILER_SPECIFIC_STANDARD_CFLAGS, ) -o $(TINY_BOOTLOADER_BIN2C_OBJ)
	$(ECHO) Finished Building Tiny Bootloader
endif

# Bootloader Targets
ifeq (1,$(NO_BOOTLOADER_REQUIRED))
bootloader:
	@:

download_bootloader:
	@:

copy_bootloader_output_for_eclipse:
	@:

else
ifeq (1,$(NO_BUILD_BOOTLOADER))
bootloader:
	$(QUIET)$(ECHO) Skipping building bootloader due to commandline spec

download_bootloader:
	@:

copy_bootloader_output_for_eclipse:
	@:

else
APPS_LUT_DOWNLOAD_DEP += download_bootloader
bootloader:
	$(QUIET)$(ECHO) Building Bootloader
	$(QUIET)$(MAKE) -r -f $(SOURCE_ROOT)Makefile $(BOOTLOADER_TARGET) -I$(OUTPUT_DIR)  SFLASH= EXTERNAL_WICED_GLOBAL_DEFINES=$(EXTERNAL_WICED_GLOBAL_DEFINES) SUB_BUILD=bootloader $(BOOTLOADER_REDIRECT)
	$(QUIET)$(PERL) $(TRX_CREATOR) $(BOOTLOADER_LINK_FILE) $(BOOTLOADER_TRX_FILE)
	$(QUIET)$(ECHO) Finished Building Bootloader
	$(QUIET)$(ECHO_BLANK_LINE)

download_bootloader: bootloader display_map_summary download_dct
	$(QUIET)$(ECHO) Downloading Bootloader ...
	$(QUIET)$(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_OPENOCD).cfg -f apps/waf/sflash_write/sflash_write.tcl -c "sflash_write_file $(BOOTLOADER_TRX_FILE) $(SFLASH_BOOTLOADER_LOC) $(subst /,_,$(PLATFORM_FULL)-$(BUS)) 1 43909" -c shutdown $(DOWNLOAD_LOG) 2>&1

copy_bootloader_output_for_eclipse: build_done
	$(QUIET)$(call MKDIR, $(BUILD_DIR)/eclipse_debug/)
	$(QUIET)$(CP) $(BOOTLOADER_LINK_FILE) $(BUILD_DIR)/eclipse_debug/last_bootloader.elf

endif
endif



# DCT Targets
ifneq (no_dct,$(findstring no_dct,$(MAKECMDGOALS)))
APPS_LUT_DOWNLOAD_DEP += download_dct
download_dct: $(FINAL_DCT_FILE) display_map_summary sflash_write_app
	$(QUIET)$(ECHO) Downloading DCT ...
	$(QUIET)$(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_OPENOCD).cfg -f apps/waf/sflash_write/sflash_write.tcl -c "sflash_write_file $(FINAL_DCT_FILE) $(SFLASH_DCT_LOC) $(subst /,_,$(PLATFORM_FULL)-$(BUS)) 0 43909" -c shutdown $(DOWNLOAD_LOG) 2>&1

else
download_dct:
	@:

no_dct:
	$(QUIET)$(ECHO) DCT unmodified

endif


run: $(SHOULD_I_WAIT_FOR_DOWNLOAD)
	$(QUIET)$(ECHO) Resetting target
	$(QUIET)$(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -c "log_output $(OPENOCD_LOG_FILE)" -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_OPENOCD).cfg  -f $(OPENOCD_PATH)$(HOST_OPENOCD)_gdb_jtag.cfg -c "resume" -c shutdown $(DOWNLOAD_LOG) 2>&1 && $(ECHO) Target running

copy_output_for_eclipse: build_done copy_bootloader_output_for_eclipse
	$(QUIET)$(call MKDIR, $(BUILD_DIR)/eclipse_debug/)
	$(QUIET)$(CP) $(LINK_OUTPUT_FILE) $(BUILD_DIR)/eclipse_debug/last_built.elf



debug: $(BUILD_STRING) $(SHOULD_I_WAIT_FOR_DOWNLOAD)
	$(QUIET)$(GDB_COMMAND) $(LINK_OUTPUT_FILE) -x .gdbinit_attach


$(GENERATED_MAC_FILE): $(MAC_GENERATOR)
	$(QUIET)$(PERL) $<  > $@

EXTRA_PRE_BUILD_TARGETS  += $(GENERATED_MAC_FILE) $(SFLASH_APP_DEPENDENCY) bootloader
EXTRA_POST_BUILD_TARGETS += copy_output_for_eclipse $(FS_IMAGE)  .gdbinit_platform

STAGING_DIR := $(OUTPUT_DIR)/resources/Staging/
FS_IMAGE    := $(OUTPUT_DIR)/filesystem.bin
FILESYSTEM_IMAGE := $(FS_IMAGE)

$(FS_IMAGE): $(STRIPPED_LINK_OUTPUT_FILE) display_map_summary $(STAGING_DIR).d
	$(QUIET)$(ECHO) Creating Filesystem
	$(QUIET)$(CP) $(STRIPPED_LINK_OUTPUT_FILE) $(STAGING_DIR)app.elf
	$(QUIET)$(COMMON_TOOLS_PATH)mk_wicedfs32 $(FS_IMAGE) $(STAGING_DIR)




ifdef BUILD_ROM

BESL_PREBUILT_FILE            := $(SOURCE_ROOT)WICED/security/BESL/BESL.ARM_CR4.release.a
ROM_LINK_OUTPUT_FILE          := $(SOURCE_ROOT)build/rom.elf
ROM_BINARY_FILE               := $(SOURCE_ROOT)build/rom.bin
ROM_LINK_C_FILE               := $(SOURCE_ROOT)build/rom.c
ROM_USE_NM_FILE               := $(SOURCE_ROOT)build/rom_use.nm
ROM_USE_LD_FILE               := $(SOURCE_ROOT)build/rom_use.ld

ROM_MAP_OUTPUT_FILE           :=$(ROM_LINK_OUTPUT_FILE:.elf=.map)
ROMLINK_OPTS_FILE             :=$(ROM_LINK_OUTPUT_FILE:.elf=.opts)
ROM_LINK_SCRIPT               := platforms/BCM43909/ROM_Build/rom.ld
ROM_LINK_LIBS := $(OUTPUT_DIR)/modules/platforms/BCM43909/ROM_build/reference.o \
                 $(OUTPUT_DIR)/libraries/NetX_Duo.a  \
                 $(OUTPUT_DIR)/libraries/ThreadX.a \
                 $(BESL_PREBUILT_FILE) \
                 $(TOOLS_ROOT)/ARM_GNU/lib/libc.a \
                 $(TOOLS_ROOT)/ARM_GNU/lib/libgcc.a

  #Supplicant_besl.a
#ROM_LINK_LIBS := NetX_Duo.a  ThreadX.a
#ROM_LINK_STDLIBS := $(TOOLS_ROOT)/ARM_GNU/lib/libc.a \
#                    $(TOOLS_ROOT)/ARM_GNU/lib/libgcc.a
#ROM_LINK_LIBS := $(addprefix $(OUTPUT_DIR)/libraries/, $(ROM_LINK_LIBS))
#ROM_LINK_LIBS += $(BESL_PREBUILT_FILE)
#ROM_BOOT_SRC := reference.S
#boot.c jumps.c


#ROM_BOOT_OUTPUT := $(addprefix $(OUTPUT_DIR)/Modules/$(call GET_BARE_LOCATION,WICED),$(ROM_BOOT_SRC:.S=.o))


PLATFORM_POST_BUILD_TARGETS += $(ROM_LINK_OUTPUT_FILE) $(ROM_USE_LD_FILE) $(ROM_LINK_C_FILE)


ROM_LDFLAGS := -Wl,--cref -Wl,-O3 -mcpu=cortex-r4 -mlittle-endian -nostartfiles -nostdlib
WHOLE_ARCHIVE := -Wl,--whole-archive
NO_WHOLE_ARCHIVE := -Wl,--no-whole-archive

$(ROMLINK_OPTS_FILE): $(MAKEFILES_PATH)/wiced_elf.mk
	$(QUIET)$(call WRITE_FILE_CREATE, $@ ,$(ROM_LDFLAGS) $(call COMPILER_SPECIFIC_LINK_SCRIPT,$(ROM_LINK_SCRIPT)) $(call COMPILER_SPECIFIC_LINK_MAP,$(ROM_MAP_OUTPUT_FILE))  $(NO_WHOLE_ARCHIVE) $(ROM_LINK_LIBS) )



$(BESL_PREBUILT_FILE):
	$(QUIET)$(MAKE) -C WICED/security/BESL/ HOST_ARCH=ARM_CR4

$(ROM_LINK_OUTPUT_FILE): build_done $(ROM_LINK_SCRIPT) $(ROMLINK_OPTS_FILE) $(BESL_PREBUILT_FILE)
	$(QUIET)$(ECHO) Making $@
	$(QUIET)$(LINKER) -o  $@ $(OPTIONS_IN_FILE_OPTION)$(ROMLINK_OPTS_FILE)

$(ROM_BINARY_FILE): $(ROM_LINK_OUTPUT_FILE)
	$(QUIET)$(OBJCOPY) -O binary -R .eh_frame -R .init -R .fini -R .comment -R .ARM.attributes $< $@

$(ROM_LINK_C_FILE): $(ROM_BINARY_FILE)
	$(QUIET)$(PERL) $(SOURCE_ROOT)tools/text_to_c/bin_to_c.pl apps_rom_image $< > $@


$(ROM_USE_NM_FILE): $(ROM_LINK_OUTPUT_FILE)
	$(QUIET)"$(TOOLCHAIN_PATH)$(TOOLCHAIN_PREFIX)nm$(EXECUTABLE_SUFFIX)"  $< > $@


$(ROM_USE_LD_FILE): $(ROM_USE_NM_FILE)
	$(QUIET)$(PERL) $(SOURCE_ROOT)platforms/BCM43909/nm_to_ld.pl $(ROM_USE_NM_FILE) > $(ROM_USE_LD_FILE)

endif


download: APPS_LUT_DOWNLOAD $(STRIPPED_LINK_OUTPUT_FILE) display_map_summary download_bootloader $(if $(findstring no_dct,$(MAKECMDGOALS)),,download_dct)

ifneq (no_dct,$(findstring no_dct,$(MAKECMDGOALS)))
FS_DEP := download_dct
else
FS_DEP := download_bootloader
endif

download_filesystem: $(FS_IMAGE) display_map_summary  $(FS_DEP)
	$(QUIET)$(ECHO) Downloading filesystem ...
	$(QUIET)$(call CONV_SLASHES,$(OPENOCD_FULL_NAME)) -f $(OPENOCD_PATH)$(JTAG).cfg -f $(OPENOCD_PATH)$(HOST_OPENOCD).cfg -f apps/waf/sflash_write/sflash_write.tcl -c "sflash_write_file $(FS_IMAGE) $(SFLASH_FS_LOC) $(subst /,_,$(PLATFORM_FULL)-$(BUS)) 0 43909" -c shutdown $(DOWNLOAD_LOG) 2>&1



.gdbinit_platform:
ifeq ($(SUB_BUILD),)
ifneq ($(wildcard WICED/platform/MCU/BCM4390x/A0_apps_rom/A0flashbl.elf),)
	$(QUIET)$(ECHO) add-sym WICED/platform/MCU/BCM4390x/A0_apps_rom/A0flashbl.elf 0x0000000 >> $@
	$(QUIET)$(ECHO) set substitute-path /home/ehunter/tmpbis/BIS120RC4ROM_REL_7_17_4  ./WICED/platform/MCU/BCM4390x/A0_apps_rom/BIS120RC4ROM_REL_7_17_4 >> $@
endif
ifneq (1,$(NO_BOOTLOADER_REQUIRED))
	$(QUIET)$(ECHO) add-sym $(BOOTLOADER_LINK_FILE) 0x00000000 >> $@
endif
endif
