#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := App_Appliance

$(NAME)_SOURCES    := appliance.c

$(NAME)_COMPONENTS := daemons/Gedday \
                      daemons/HTTP_server \
                      daemons/device_configuration

$(NAME)_RESOURCES  := apps/appliance/top_web_page_top.html \
                      images/brcmlogo.png \
                      images/brcmlogo_line.png \
                      images/favicon.ico \
                      styles/buttons.css \
                      scripts/general_ajax_script.js

INVALID_PLATFORMS := BCM943362WCD4_LPCX1769 STMDiscovery411_BCM43438 BCM943362WCDA
