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
# Download OpenOCD Tar ball
#
###################################################################################################

# Fetch OpenOCD - clone it then zip the cloned directory for later use
if [ ! -e ./$DOWNLOAD_DIR/openocd-$OPENOCD_REPO_HASH.tar.gz ]; then
    echo "Downloading OpenOCD"
    cd ./$DOWNLOAD_DIR

    # OpenOCD has some utf-8 filenames and MinGW cannot delete them.
    # Use the Windows command console to delete the files (does not delete the empty directories).
    if [ "$OSTYPE" == "msys" ]; then
        cmd /c "del /S /F /Q openocd"
    fi

    rm -rf ./openocd/

    git clone $OPENOCD_REPO_URL
    cd openocd
    git config --local user.email broadcom_wiced@broadcom.local
    git config --local user.name  "Broadcom Wiced"
    git checkout  $OPENOCD_REPO_HASH
    git submodule init
    git submodule update
    cd ..

    # Openocd has some nasty UTF-8 unicode filenames in the tcl scripts that create trouble in some msys environments
    # Consequently, only tar the .git directory then later do a "git reset" to recover files
    # --warning=no-file-changed is required due to corporate spyware interference on Windows platforms
    if [ "$OSTYPE" == "msys" ]; then
        tar --warning=no-file-changed -zcvf openocd-$OPENOCD_REPO_HASH.tar.gz openocd/.git
    else
        tar -zcvf openocd-$OPENOCD_REPO_HASH.tar.gz openocd/.git
    fi

    # OpenOCD has some utf-8 filenames and MinGW cannot delete them.
    # Use the Windows command console to delete the files (does not delete the empty directories).
    if [ "$OSTYPE" == "msys" ]; then
        cmd /c "del /S /F /Q openocd"
    fi

    rm -rf openocd/

    cd ..
fi


###################################################################################################
#
# Build OpenOCD
#
###################################################################################################


# Extract OpenOCD
echo "Extracting OpenOCD"
rm -rf ./openocd/
tar -zxvf ./$DOWNLOAD_DIR/openocd-$OPENOCD_REPO_HASH.tar.gz

# Checkout the OpenOCD code
echo "Checking out OpenOCD code"
cd openocd
git reset --hard
git checkout $OPENOCD_REPO_HASH
git branch -D Broadcom_only || true
git checkout -b Broadcom_only
#git submodule init
#git submodule update

# Patching OpenOCD
echo "Patching OpenOCD"

git am ../patches/0001-Add-verify_image_checksum-command.patch
git am ../patches/0002-Add-hack-to-turn-off-Control-C-handling-in-windows-t.patch
git am ../patches/0003-FT2232-driver-simplification.patch
git am ../patches/0004-Add-BCM9WCD1EVAL1-device-to-FT2232-driver.patch
git am ../patches/0005-Fix-printf-format-errors-in-MinGW-32-bit.patch
git am ../patches/0006-Change-trace-command-to-avoid-clash-with-TCL-trace-c.patch
git am ../patches/0007-Add-more-logging-for-GDB-arm-mode-error.patch
git am ../patches/0008-Add-initial-functions-for-memory-map-support.patch
git am ../patches/0009-Fix-ThreadX-FreeRTOS-current-execution-when-there-ar.patch
git am ../patches/0010-RTOS-add-ability-to-wipe-RTOS-before-use-to-avoid-ol.patch
git am ../patches/0011-Fix-up-OpenOCD-configuration.patch
git am ../patches/0012-Add-support-for-at91sam4sa16b.patch
git am ../patches/0013-RTOS-Fix-ThreadX-for-Cortex-R4.patch
git am ../patches/0014-Cortex-A-R-Implement-reset-properly-allowing-connect.patch
git am ../patches/0015-Cortex-A-R-Allow-interrupt-disable-during-single-ste.patch
git am ../patches/0016-Cortex-A-R-Add-debug-description-of-reason-for-halt.patch
git am ../patches/0017-Reset-Remove-connect-under-reset-if-for-init-where-r.patch
git am ../patches/0018-ARM-ADIv5-Ensure-that-Debug-System-Power-up-requests.patch
git am ../patches/0019-gdb_server-Add-check-for-malloc-fail.patch
git am ../patches/0020-gdb_server-Add-thread-update-when-running-gdb_sig_ha.patch
git am ../patches/0021-Add-Logging.patch
git am ../patches/0022-Helpers-Replace-deprecated-usleep-with-nanosleep-whe.patch
git am ../patches/0023-Added-support-for-new-ATMEL-MCU-Variant-SAM4SD32B.patch
git am ../patches/0024-Cortex-M-Add-disconnect-function-to-allow-targets-to.patch
git am ../patches/0025-Cortex-R-Fix-unaligned-reading-writing-from-memory-u.patch
git am ../patches/0026-Documentation-remove-raggedright-not-supported-by-Mi.patch
git am ../patches/0027-TCL-Delete-files-with-unicode-names-that-MinGW-can-t.patch
git am ../patches/0028-Fix-up-RTOS-implementation-for-FreeRTOS-ThreadX.patch
git am ../patches/0029-Cortex-A-Revert-changes-which-break-the-sflash-write.patch

cd ..

if [ "$DEBUG_OPENOCD" == "yes" ]; then
    export EXTRA_CFLAGS=-"g -O0"
fi

if [ "$OSTYPE" == "msys" ]; then
    export EXTRA_OPENOCD_CFGOPTS="--enable-parport-giveio"
else
    export EXTRA_OPENOCD_CFGOPTS=""
    #Note: Using --enable-buspirate --enable-zy1000-master --enable-zy1000 causes ONLY the zy1000 adapter to be supported.
fi

#if [ "$OSTYPE" == "linux-gnu" ]; then
#    export EXTRA_OPENOCD_CFGOPTS="--enable-amtjtagaccel --enable-gw16012"
#fi

if [[ "$OSTYPE" == *darwin* ]]; then
#    export EXTRA_CFLAGS="$EXTRA_CFLAGS -framework IOKit -framework CoreFoundation"
#export EXTRA_CFLAGS="$EXTRA_CFLAGS -L`pwd`/libusb-install/lib/" # -L/opt/local/lib/" # -lusb-1.0"
    export EXTRA_CFLAGS="$EXTRA_CFLAGS -L`pwd`/libusb-compat-install/lib/ -Qunused-arguments"
    export CC_VAL="gcc"
else
    # These require linux/parport.h - hence do not work on OS-X
    export EXTRA_OPENOCD_CFGOPTS="$EXTRA_OPENOCD_CFGOPTS --enable-amtjtagaccel --enable-gw16012 --enable-parport "
    export CC_VAL="gcc"
fi



# Build OpenOCD
echo "Building OpenOCD"
rm -rf openocd-build openocd-install
mkdir -p openocd-build
mkdir -p openocd-install
cd openocd
./bootstrap

cd jimtcl
git config --local user.email broadcom_wiced@broadcom.local
git config --local user.name  "Broadcom Wiced"
git reset --hard
git am ../../patches/JIM_0001-Add-variable-tracing-functionality.patch
git am ../../patches/JIM_0002-JimTCL-Define-S_IRWXG-and-S_IRWXO-for-platforms-win3.patch
cd ../..

cd openocd-build
export PKG_CONFIG_PATH="`pwd`/../hidapi-install/lib/pkgconfig:`pwd`/../libusb-install/lib/pkgconfig:`pwd`/../libusb-compat-install/lib/pkgconfig:`pwd`/../libftdi-install/lib/pkgconfig:`pwd`/../libusb-win32-src-$LIBUSB_WIN32_VER/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH=`pwd`/../libftdi-install/lib/:$LD_LIBRARY_PATH
../openocd/configure $EXTRA_OPENOCD_CFGOPTS \
                    --enable-dummy \
                    --enable-ftdi \
                    --enable-stlink \
                    --enable-ti-icdi \
                    --enable-ulink \
                    --enable-usb-blaster-2 \
                    --enable-vsllink \
                    --enable-jlink \
                    --enable-osbdm \
                    --enable-opendous \
                    --enable-aice \
                    --enable-usbprog \
                    --enable-rlink \
                    --enable-armjtagew \
                    --enable-cmsis-dap \
                    --enable-legacy-ft2232_libftdi \
                    --enable-jtag_vpi \
                    --enable-usb_blaster_libftdi \
                    --enable-ep93xx \
                    --enable-at91rm9200 \
                    --enable-bcm2835gpio \
                    --enable-presto_libftdi \
                    --enable-openjtag_ftdi \
                    --disable-option-checking  \
                    --prefix=`pwd`/../openocd-install/ \
                    --program-suffix=-all-brcm-libftdi \
                    --enable-maintainer-mode  \
                    PKG_CONFIG="pkg-config" \
                    CC="$CC_VAL" \
                    CFLAGS="-g \
                        -L`pwd`/../libusb-win32-src-$LIBUSB_WIN32_VER/ \
                        -L`pwd`/../libftdi-install/lib \
                        -I`pwd`/../libusb-win32-src-$LIBUSB_WIN32_VER/src/ \
                        -I/opt/local/include \
                        -I`pwd`/../openocd/src/jtag/drivers/hndjtag/include/ \
                        $EXTRA_CFLAGS"

# These configure options require the FTDI ftd2xx library which
# is copyright FTDI with a non-compatible license
# --enable-legacy-ft2232_ftd2xx
# --enable-usb_blaster_ftd2xx
# --enable-presto_ftd2xx
# --enable-openjtag_ftd2xx

# not compatible with --enable-zy1000
# --enable-minidriver-dummy

# Fails to compile on mingw
# --enable-remote-bitbang
# --enable-sysfsgpio
# --enable-ioutil
# --enable-oocd_trace

make
make install
cd ..

# Copying OpenOCD into install directory
echo "Copying OpenOCD into install directory"
cp openocd-install/bin/openocd-all-brcm-libftdi* $INSTALL_DIR/$HOST_TYPE/

# Strip OpenOCD
if [ ! "$DEBUG_OPENOCD" == "yes" ]; then
    echo "Stripping executable"
    for f in \
        `find ./$INSTALL_DIR/$HOST_TYPE/ -name openocd-all-brcm-libftdi*`
    do
        strip $f
    done
fi

# OSX cannot be built static, so make a script to force it to find the dynamic libs
if [[ "$OSTYPE" == *darwin* ]]; then
    mv $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi_run
    cp `which dirname` $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi_dirname

    echo "#!/bin/bash" > $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi
    echo "export DYLD_LIBRARY_PATH=\`\$0_dirname \$0\`:$DYLD_LIBRARY_PATH" >> $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi
    echo "\${0}_run \"\$@\"" >> $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi
    chmod a+x $INSTALL_DIR/$HOST_TYPE/openocd-all-brcm-libftdi
fi

echo
echo "Done! - Output is in $INSTALL_DIR"
echo

