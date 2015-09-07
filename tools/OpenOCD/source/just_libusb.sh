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
# Download LibUSBTar balls
#
###################################################################################################


# Windows Specific
if [ "$OSTYPE" == "msys" ]; then

    # Download libusb 1.x on Windows
    if [ ! -e ./$DOWNLOAD_DIR/$LIBUSB_FILENAME ]; then
        echo "Downloading libUSB"
    cd $DOWNLOAD_DIR
        $WGET $LIBUSB_URL
    cd ..
    fi

    # Download libusb 0.x on Windows
    if [ ! -e ./$DOWNLOAD_DIR/$LIBUSB_WIN32_FILENAME ]
    then
        echo "Downloading libUSB-win32"
    cd $DOWNLOAD_DIR
        $WGET $LIBUSB_WIN32_URL
    cd ..
    fi

fi


# Fetch libusb and libusb-compat on OS-X
if [[ "$OSTYPE" == *darwin* ]]; then
    if [ ! -e ./$DOWNLOAD_DIR/$LIBUSB_FILENAME ]; then
        echo "Downloading libUSB"
                cd $DOWNLOAD_DIR
        $WGET $LIBUSB_URL
                cd ..
    fi
    if [ ! -e ./$DOWNLOAD_DIR/$LIBUSB_COMPAT_FILENAME ]; then
        echo "Downloading libUSB-compat"
                cd $DOWNLOAD_DIR
        $WGET $LIBUSB_COMPAT_URL
                cd ..
    fi
fi



###################################################################################################
#
# Build LibUSB
#
###################################################################################################


# Build libusb on Windows
if [ "$OSTYPE" == "msys" ]; then

    echo "Unzipping libUSB 0.x  (libUSB-win32)"

    rm -rf ./libusb-win32-src-$LIBUSB_WIN32_VER/

    # Unzip fails with segmentation violation first time when used in long directory path for some reason
    unzip -o ./$DOWNLOAD_DIR/$LIBUSB_WIN32_FILENAME || unzip -o ./$DOWNLOAD_DIR/$LIBUSB_WIN32_FILENAME

    echo "Patching libUSB 0.x  (libUSB-win32)"
    patch --ignore-whitespace -p1 -N -d libusb-win32-src-$LIBUSB_WIN32_VER < patches/libusb-win32-src-$LIBUSB_WIN32_VER.patch

    echo "Building libUSB 0.x  (libUSB-win32)"
    CFLAGS="-g $EXTRA_CFLAGS" make -C ./libusb-win32-src-$LIBUSB_WIN32_VER/ static_lib

    # Create a package config file for LibUSB-Win32
    mkdir -p ./libusb-win32-src-$LIBUSB_WIN32_VER/pkgconfig/
    echo "Name: libusb" > ./libusb-win32-src-$LIBUSB_WIN32_VER/pkgconfig/libusb.pc
    echo "Description: Legacy C API for USB device access from Windows userspace" >> ./libusb-win32-src-$LIBUSB_WIN32_VER/pkgconfig/libusb.pc
    echo "Version: win32-1.0.18" >> ./libusb-win32-src-$LIBUSB_WIN32_VER/pkgconfig/libusb.pc
    echo "Libs: -lusb" >> ./libusb-win32-src-$LIBUSB_WIN32_VER/pkgconfig/libusb.pc
    echo "Libs.private: " >> ./libusb-win32-src-$LIBUSB_WIN32_VER/pkgconfig/libusb.pc
    echo "Cflags:" >> ./libusb-win32-src-$LIBUSB_WIN32_VER/pkgconfig/libusb.pc

    cp ./libusb-win32-src-$LIBUSB_WIN32_VER/src/lusb0_usb.h ./libusb-win32-src-$LIBUSB_WIN32_VER/src/usb.h


    echo "Unzipping libUSB 1.x"
    rm -rf ./libusb-$LIBUSB_VER/
    tar -jxvf ./$DOWNLOAD_DIR/$LIBUSB_FILENAME

    echo "Patching libUSB 1.x"
    patch --ignore-whitespace -p1 -N -d libusb-$LIBUSB_VER < patches/libusb-${LIBUSB_VER}-MinGW.patch

    mkdir -p libusb-build
    mkdir -p libusb-install
    cd libusb-build
    echo "Building libUSB 1.x"
    ../libusb-$LIBUSB_VER/configure --prefix=`pwd`/../libusb-install/  --enable-shared=no --enable-static=yes CFLAGS="-g $EXTRA_CFLAGS"
    make install
    cd ..
fi

# Build libusb & libusb-compat on OS-X
if [[ "$OSTYPE" == *darwin* ]]; then
    echo "Unzipping libUSB"
    rm -rf ./libusb-$LIBUSB_VER/
    tar -jxvf ./$DOWNLOAD_DIR/$LIBUSB_FILENAME

    echo "Building libUSB"
    mkdir -p libusb-build
    mkdir -p libusb-install
    cd libusb-build
    ../libusb-$LIBUSB_VER/configure --prefix=`pwd`/../libusb-install/ --enable-static=no
    make install
    cd ..

    export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:`pwd`/libusb-install/lib/pkgconfig/

    echo "Unzipping libUSB-compat"
    rm -rf ./libusb-$LIBUSB_COMPAT_VER/
    tar -jxvf ./$DOWNLOAD_DIR/$LIBUSB_COMPAT_FILENAME

    echo "Building libUSB-compat"
    mkdir -p libusb-compat-build
    mkdir -p libusb-compat-install
    cd libusb-compat-build
    ../libusb-compat-$LIBUSB_COMPAT_VER/configure --prefix=`pwd`/../libusb-compat-install/ --enable-static=no CFLAGS="-g -I`pwd`/libusb-install/include/libusb-1.0 -L`pwd`/libusb-install/lib/"
    make install
    cd ..
    export EXTRA_CFLAGS="$EXTRA_CFLAGS -I`pwd`/libusb-compat-install/include/ -I`pwd`/libusb-install/include/libusb-1.0" # -L`pwd`/libusb-compat-install/lib/" # -L`pwd`/libusb-install/lib/" # -lusb-1.0"
    export PATH=../libusb-compat-install/bin/:$PATH
    cp libusb-install/lib/libusb-1.0.0.dylib $INSTALL_DIR/$HOST_TYPE/
    cp libusb-compat-install/lib/libusb-0.1.4.dylib $INSTALL_DIR/$HOST_TYPE/
fi


