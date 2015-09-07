Building OpenOCD
================

This file describes how to build the OpenOCD binary that is shipped with the WICED-SDK
It can be compiled using MinGW, Linux or Mac OS X.

Prerequisites:
    * Linux    :  Install packages "libusb-dev", "texinfo", "libtool", "automake", "binutils-dev", "cmake", "git" & "autotools"
    * Mac OS X : Install MacPorts from macports.org then install "pkgconfig" with "sudo port install pkgconfig"
    * Windows  : install MinGW from mingw.org. execute build script from MinGW bash shell


Building:
    * Open bash and go to tools/OpenOCD/source
    * ./build_all.sh


Your OpenOCD binaries will be updated and ready to use in tools/OpenOCD/<OS_TYPE>/

