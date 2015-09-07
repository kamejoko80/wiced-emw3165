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

if (! $ARGV == 8)
{
    print "Usage ./apds_dct_header.pl  FactoryResetApp OTAApp DCTImage App0 App1 App2 App3 App4";
    exit;
}

my $FR_LOCATION = 0;

print "#ifndef APPS_LOCATION_H_\n";
print "#define APPS_LOCATION_H_\n";

#print "/* $ARGV[0] $ARGV[1] $ARGV[2] $ARGV[3] $ARGV[4] $ARGV[5] $ARGV[6] $ARGV[7] */\n";

my $CURR_LOCATION = 0;
if ($ARGV[0] eq '-')
{
    $FR_SIZE = 0;
    print "#define        DCT_FR_APP_LOCATION_ID    ( NONE )\n";
}
else
{
    $FR_SIZE = -s $ARGV[0];
    print "#define        DCT_FR_APP_LOCATION_ID    ( EXTERNAL_FIXED_LOCATION )\n";
}
print "#define        DCT_FR_APP_LOCATION       ( $CURR_LOCATION )\n";
$CURR_LOCATION += $FR_SIZE;

my $DCT_LOCATION = $CURR_LOCATION;
if ($ARGV[1] eq '-')
{
    $DCT_SIZE = 0;
    print "#define        DCT_DCT_IMAGE_LOCATION_ID    ( NONE )\n";
}
else
{
    $DCT_SIZE = 25600;
    print "#define        DCT_DCT_IMAGE_LOCATION_ID    ( EXTERNAL_FIXED_LOCATION )\n";
}
print "#define        DCT_DCT_IMAGE_LOCATION       ( $CURR_LOCATION )\n";
$CURR_LOCATION += $DCT_SIZE;

my $OTA_LOCATION = $CURR_LOCATION;
if ($ARGV[2] eq '-')
{
    $OTA_SIZE = 0;
    print "#define        DCT_OTA_APP_LOCATION_ID    ( NONE )\n";
}
else
{
    $OTA_SIZE = -s $ARGV[2];
    print "#define        DCT_OTA_APP_LOCATION_ID    ( EXTERNAL_FIXED_LOCATION )\n";
}
print "#define        DCT_OTA_APP_LOCATION       ( $CURR_LOCATION )\n";
$CURR_LOCATION += $OTA_SIZE;

my $APP0_LOCATION = $CURR_LOCATION;
if ($ARGV[3] eq '-')
{
    $APP0_SIZE = 0;
    print "#define        DCT_APP0_LOCATION_ID    ( NONE )\n";
}
else
{
    $APP0_SIZE = -s $ARGV[3];
    print "#define        DCT_APP0_LOCATION_ID    ( EXTERNAL_FIXED_LOCATION )\n";
}
print "#define        DCT_APP0_LOCATION       ( $CURR_LOCATION )\n";
$CURR_LOCATION += $APP0_SIZE;

my $APP1_LOCATION = $CURR_LOCATION;
if ($ARGV[4] eq '-')
{
    $APP1_SIZE = 0;
    print "#define        DCT_APP1_LOCATION_ID    ( NONE )\n";
}
else
{
    $APP1_SIZE = -s $ARGV[4];
    print "#define        DCT_APP1_LOCATION_ID    ( EXTERNAL_FIXED_LOCATION )\n";
}
print "#define        DCT_APP1_LOCATION       ( $CURR_LOCATION )\n";
$CURR_LOCATION += $APP1_SIZE;

my $APP2_LOCATION = $CURR_LOCATION;
if ($ARGV[5] eq '-')
{
    $APP2_SIZE = 0;
    print "#define        DCT_APP2_LOCATION_ID    ( NONE )\n";
}
else
{
    $APP2_SIZE = -s $ARGV[5];
    print "#define        DCT_APP2_LOCATION_ID    ( EXTERNAL_FIXED_LOCATION )\n";
}
print "#define        DCT_APP2_LOCATION       ( $CURR_LOCATION )\n";
$CURR_LOCATION += $APP2_SIZE;

my $APP3_LOCATION = $CURR_LOCATION;
if ($ARGV[6] eq '-')
{
    $APP3_SIZE = 0;
    print "#define        DCT_APP3_LOCATION_ID    ( NONE )\n";
}
else
{
    $APP3_SIZE = -s $ARGV[6];
    print "#define        DCT_APP3_LOCATION_ID    ( EXTERNAL_FIXED_LOCATION )\n";
}
print "#define        DCT_APP3_LOCATION       ( $CURR_LOCATION )\n";
$CURR_LOCATION += $APP3_SIZE;

my $APP4_LOCATION = $CURR_LOCATION;
if ($ARGV[7] eq '-')
{
    $APP4_SIZE = 0;
    print "#define        DCT_APP4_LOCATION_ID    ( NONE )\n";
}
else
{
    $APP4_SIZE = -s $ARGV[7];
    print "#define        DCT_APP4_LOCATION_ID    ( EXTERNAL_FIXED_LOCATION )\n";
}
print "#define        DCT_APP4_LOCATION       ( $CURR_LOCATION )\n";

print "#endif /* APPS_LOCATION_H_ */\n";
