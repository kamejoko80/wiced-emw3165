#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = LPC18xx_Peripheral_Libraries

GLOBAL_INCLUDES :=  . \
                    inc \
                    ../../../$(HOST_ARCH)/CMSIS

$(NAME)_SOURCES := \
                    src/adc_18xx_43xx.c      \
                    src/aes_18xx_43xx.c      \
                    src/atimer_18xx_43xx.c   \
                    src/ccan_18xx_43xx.c     \
                    src/chip_18xx_43xx.c     \
                    src/clock_18xx_43xx.c    \
                    src/dac_18xx_43xx.c      \
                    src/eeprom_18xx_43xx.c   \
                    src/emc_18xx_43xx.c      \
                    src/enet_18xx_43xx.c     \
                    src/evrt_18xx_43xx.c     \
                    src/gpdma_18xx_43xx.c    \
                    src/gpio_18xx_43xx.c     \
                    src/gpiogroup_18xx_43xx.c\
                    src/i2c_18xx_43xx.c      \
                    src/i2cm_18xx_43xx.c     \
                    src/i2s_18xx_43xx.c      \
                    src/iap_18xx_43xx.c      \
                    src/lcd_18xx_43xx.c      \
                    src/otp_18xx_43xx.c      \
                    src/pinint_18xx_43xx.c   \
                    src/pmc_18xx_43xx.c      \
                    src/rgu_18xx_43xx.c      \
                    src/ring_buffer.c        \
                    src/ritimer_18xx_43xx.c  \
                    src/rtc_18xx_43xx.c      \
                    src/sct_18xx_43xx.c      \
                    src/sct_pwm_18xx_43xx.c  \
                    src/scu_18xx_43xx.c      \
                    src/sdif_18xx_43xx.c     \
                    src/sdmmc_18xx_43xx.c    \
                    src/spi_18xx_43xx.c      \
                    src/ssp_18xx_43xx.c      \
                    src/stopwatch_18xx_43xx.c\
                    src/sysinit_18xx_43xx.c  \
                    src/timer_18xx_43xx.c    \
                    src/uart_18xx_43xx.c     \
                    src/wwdt_18xx_43xx.c     \
                    src/eeprom.c             \
                    src/fpu_init.c           \
                    src/spifilib_dev_common.c \
                    src/spifilib_fam_standard_cmd.c \