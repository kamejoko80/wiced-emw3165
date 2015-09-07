#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_temp_control

$(NAME)_SOURCES    := temp_control.c

$(NAME)_COMPONENTS := daemons/HTTP_server \
                      daemons/device_configuration \
                      protocols/Xively \
                      drivers/sensors/NCP18XH103J03RB \
                      protocols/SNTP \
                      daemons/Gedday \
                      inputs/gpio_keypad

$(NAME)_RESOURCES  := apps/temp_control/main.html \
                      apps/temp_control/data.html \
                      images/brcmlogo.png \
                      images/brcmlogo_line.png \
                      images/favicon.ico \
                      styles/buttons.css \
                      scripts/general_ajax_script.js \
                      scripts/wpad.dat

GLOBAL_DEFINES     := USE_SELF_SIGNED_TLS_CERT

APPLICATION_DCT    := temp_control_dct.c

WIFI_CONFIG_DCT_H  := wifi_config_dct.h

VALID_PLATFORMS    := BCM943362WCD4 \
                      BCM943362WCD6 \
                      BCM943362WCD8 \
                      BCM9WCDPLUS114