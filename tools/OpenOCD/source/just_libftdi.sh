#!/bin/bash

#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

if [ "$PREREQUISITES_DONE" != "yes" ]; then
source ./prerequisites.sh
fi

###################################################################################################
#
# Download LibFTDI Tar ball
#
###################################################################################################


# Download libFTDI
if [ ! -e ./$DOWNLOAD_DIR/$LIBFTDI_FILENAME ]; then
    echo "Downloading libFTDI"
    cd $DOWNLOAD_DIR
    $WGET $LIBFTDI_URL
    cd ..
fi


###################################################################################################
#
# Build LibFTDI
#
###################################################################################################

# Extract libFTDI
echo "Extracting libFTDI"
rm -rf ./libftdi-$LIBFTDI_VER/
tar -zxvf ./$DOWNLOAD_DIR/$LIBFTDI_FILENAME

# Build libFTDI
echo "Building libFTDI"
rm -rf libftdi-build libftdi-install
mkdir -p libftdi-build
mkdir -p libftdi-install

if [ "$OSTYPE" == "msys" ]; then
    cp libusb-config-msys libftdi-build/libusb-config
fi
cd libftdi-build
../libftdi-$LIBFTDI_VER/configure --prefix=`pwd`/../libftdi-install/ --enable-shared=no
make CFLAGS="-g -L`pwd`/../libusb-win32-src-$LIBUSB_WIN32_VER/ -L`pwd`/../libusb-compat-install/lib/ -lusb -I`pwd`/../libusb-win32-src-$LIBUSB_WIN32_VER/src -I`pwd`/../libusb-compat-install/include/ -I/opt/local/include $EXTRA_CFLAGS" install
cd ..


