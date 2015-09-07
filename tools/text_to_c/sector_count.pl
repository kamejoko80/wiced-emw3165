#!/usr/bin/perl

#
# Copyright 2015, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
#use File:stat;

if (! $ARGV == 3)
{
    print "Usage ./apds_dct_header.pl  Filename Current_Sector BLOCK_SIZE";
    exit;
}
if (! -e $ARGV[0])
{
    $filesize = "33";
}
else
{
    $filesize = -s $ARGV[0];
}
$sector_count = int($filesize / int($ARGV[2]));
$final_sector = $ARGV[1] + $sector_count + 1;
print ("$final_sector");