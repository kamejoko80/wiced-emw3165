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
# Download HIDAPI Tar ball
#
###################################################################################################


# Fetch hidapi - clone it then zip the cloned directory for later use
if [ ! -e ./$DOWNLOAD_DIR/hidapi-$HIDAPI_REPO_HASH.tar.gz ]; then
    echo "Downloading hidapi"
    cd ./$DOWNLOAD_DIR
    rm -rf ./hidapi/
    git clone $HIDAPI_REPO_URL
    cd hidapi
    git checkout  $HIDAPI_REPO_HASH
    cd ..
    tar -zcvf hidapi-$HIDAPI_REPO_HASH.tar.gz hidapi/
    rm -rf hidapi/
    cd ..
fi


###################################################################################################
#
# Build HIDAPI
#
###################################################################################################

# Extract hidapi
echo "Extracting HIDAPI"
rm -rf ./hidapi/
tar -zxvf ./$DOWNLOAD_DIR/$HIDAPI_FILENAME

# Build HIDAPI
echo "Building HIDAPI"
rm -rf hidapi-build hidapi-install
mkdir -p hidapi-build
mkdir -p hidapi-install

cd hidapi
./bootstrap
cd ../hidapi-build
../hidapi/configure --prefix=`pwd`/../hidapi-install/ --enable-shared=no CFLAGS="-g"
make install
cd ..
