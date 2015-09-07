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
scriptname=${0##*/}
scriptdir=${0%*$scriptname}

# Rerun the script with environment cleared out
if [ "$1" != "ENVCLEARED" ]; then
    exec -c "$scriptdir$scriptname" ENVCLEARED "$PATH" "$HOME" "$@"
    exit
else
    shift
    export SAVED_PATH=$1
    shift
    export SAVED_HOME=$1
    shift
fi
# Delete some automatic bash environment variables
unset PATH
unset TERM
unset PWD
unset OLDPWD
unset SHLVL


linux32unamestr=`"${scriptdir}tools/common/Linux32/uname" 2> /dev/null`
linux64unamestr=`"${scriptdir}tools/common/Linux64/uname" 2> /dev/null`
linux64unamestr='Linux'
osxunamestr=`"${scriptdir}tools/common/OSX/uname"  2> /dev/null`
win32unamestr=`"${scriptdir}tools/common/Win32/uname.exe" -o 2> /dev/null`

if [ "$linux32unamestr" == "Linux" ] || [ "$linux64unamestr" == "Linux" ]; then
# Linux
linuxuname64str=`${scriptdir}tools/common/Linux64/uname -m 2> /dev/null`
if [ "$linuxuname64str" == "x86_64" ]; then

# Linux64
#echo Host is Linux64
"${scriptdir}tools/common/Linux64/make" "$@" HOST_OS=Linux64

else

# Linux32
#echo Host is Linux32
"${scriptdir}tools/common/Linux32/make" "$@" HOST_OS=Linux32

fi
elif [[ "$osxunamestr" == *Darwin* ]]; then

#OSX
#echo Host is OSX
"${scriptdir}tools/common/OSX/make" "$@" HOST_OS=OSX

elif [ "${win32unamestr}" == "MinGW" ]; then

#MinGW / Cygwin
#echo Host is MinGW
"${scriptdir}tools/common/Win32/make.exe" "$@" HOST_OS=Win32

else
echo Unknown host
echo Linux32 uname: \"${linux32unamestr}\"
echo Linux64 uname: \"${linux32unamestr}\"
echo OSX uname: \"${osxunamestr}\"
echo Win32 uname: \"${win32unamestr}\"
fi

