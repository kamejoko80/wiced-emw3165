#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = 43909_Peripheral_Drivers

$(eval $(call PLATFORM_LOCAL_DEFINES_INCLUDES_43909, ..))

$(NAME)_SOURCES := platform_watchdog.c      \
                   platform_ddr.c	    \
                   platform_chipcontrol.c   \
                   platform_m2m.c           \
                   platform_pinmux.c        \
                   platform_spi_i2c.c       \
                   platform_pwm.c           \
                   platform_mcu_powersave.c \
                   platform_backplane.c     \
                   platform_i2s.c           \
                   platform_rtc.c           \
                   platform_hib.c           \
                   platform_otp.c           \
                   platform_8021as_clock.c  \
                   platform_ascu.c

$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/spi_flash
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/uart
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/shared
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/crypto/tiny_crypto

ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))
$(NAME)_COMPONENTS += MCU/BCM4390x/peripherals/ethernet
endif
