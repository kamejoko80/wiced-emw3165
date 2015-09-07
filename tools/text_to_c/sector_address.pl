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

if (! $ARGV == 2)
{
    print "Usage ./apds_dct_header.pl  Filename Current_Sector BLOCK_SIZE";
    exit;
}
$ADDRESS = $ARGV[0] * $ARGV[1];
print ("$ADDRESS");