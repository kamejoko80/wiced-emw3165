#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME = Lib_device_configuration

CERTIFICATE := $(SOURCE_ROOT)resources/certificates/brcm_demo_server_cert.cer
PRIVATE_KEY := $(SOURCE_ROOT)resources/certificates/brcm_demo_server_cert_key.key

$(NAME)_SOURCES += device_configuration.c \
                   device_configuration_http_content.c

$(NAME)_RESOURCES := images/brcmlogo.png \
                     images/brcmlogo_line.png \
                     images/favicon.ico \
                     images/scan_icon.png \
                     images/wps_icon.png \
                     images/64_0bars.png \
                     images/64_1bars.png \
                     images/64_2bars.png \
                     images/64_3bars.png \
                     images/64_4bars.png \
                     images/64_5bars.png \
                     images/tick.png \
                     images/cross.png \
                     images/lock.png \
                     images/progress.gif \
                     scripts/general_ajax_script.js \
                     scripts/wpad.dat \
                     config/device_settings.html \
                     config/scan_page_outer.html \
                     config/redirect.html \
                     styles/buttons.css \
                     styles/border_radius.htc

$(NAME)_COMPONENTS += daemons/simple_HTTP_server \
                      daemons/DNS_redirect \
                      protocols/DNS
