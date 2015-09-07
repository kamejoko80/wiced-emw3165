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

my $filename = "rom_use_orig.ld";

#open the file
open my $INFILE, $filename or die "cant open " . $filename;
@file = <$INFILE>;
close $INFILE;


my %rom_symbol_list1 = ( );
my %rom_symbol_list2 = ( );

foreach $line (@file)
{
    if ( $line =~ m/^(\S+) = (0x\S+);\r?$/gi )
    {
        $rom_symbol_list1{$2} = $1;
    }
    else
    {
        print "ERROR : $line\n";
    }
}

print "JUMP TABLE PART 1\n\n";
foreach $address (sort keys %rom_symbol_list1)
{
    if ( hex( $address ) >= 0x0069F000 )
    {
        printf( "ROM_JUMPS_%-45s_jump (rwx) : ORIGIN = 0x%08x, LENGTH = 16\n", $rom_symbol_list1{$address}, hex($address));
    }
}

print "\n\nJUMP TABLE PART 2\n\n";
foreach $address (sort keys %rom_symbol_list1)
{
    if ( hex( $address ) >= 0x0069F000 )
    {
        printf( ".rom_jump_table_%s_jump :\n",  $rom_symbol_list1{$address} );
        printf( "{\n" );
        printf( "    *(*.%s_jump %s_jump)\n",  $rom_symbol_list1{$address} );
        printf( "} > ROM_JUMPS_%s_jump AT > ROM_JUMPS_%s_jump\n\n", $rom_symbol_list1{$address}, $rom_symbol_list1{$address} );
    }
}


print "\n\nJUMP FUNCTIONS\n\n";
foreach $address (sort keys %rom_symbol_list1)
{
    if ( hex( $address ) >= 0x0069F000 )
    {
        printf( "    .arm\n" );
        printf( "    .section .text.%s_jump, \"ax\"\n", $rom_symbol_list1{$address} );
        printf( "    .align 4\n" );
        printf( "    .global %s\n", $rom_symbol_list1{$address} );
        printf( "    .global %s_jump\n", $rom_symbol_list1{$address} );
        printf( "%s_jump:\n", $rom_symbol_list1{$address} );
        printf( "    B %s\n", $rom_symbol_list1{$address} );
        printf( "\n" );
    }
}


print "\n\nROM GLOBAL VARIABLES\n\n";
foreach $address (sort keys %rom_symbol_list1)
{
    if ( ( hex( $address ) >= 0x0069D000 ) && ( hex( $address ) < 0x0069F000 ) )
    {
        printf( "%s = %s;\n", $rom_symbol_list1{$address}, $address );
    }
}


print "\n\nROM FUNCTIONS\n\n";
foreach $address (sort keys %rom_symbol_list1)
{
    if ( hex( $address ) < 0x0069D000 )
    {
        printf( "%s = %s;\n", $rom_symbol_list1{$address}, $address );
    }
}
