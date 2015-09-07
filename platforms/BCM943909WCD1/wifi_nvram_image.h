/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 * BCM943909 variables
 */

#ifndef INCLUDED_NVRAM_IMAGE_H_
#define INCLUDED_NVRAM_IMAGE_H_

#include <string.h>
#include <stdint.h>
#include "../generated_mac_address.txt"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Character array of NVRAM image
 */

// Sample variables file for BCM943909
static const char wifi_nvram_image[] =
        "sromrev=11"                                                         "\x00"
        "vendid=0x14e4"                                                      "\x00"
        "devid=0x43d0"                                                       "\x00"
        NVRAM_GENERATED_MAC_ADDRESS                                          "\x00"
        "nocrc=1"                                                            "\x00"
        "boardtype=0x0755"                                                   "\x00"
        "boardrev=0x1202"                                                    "\x00"
        "xtalfreq=37400"                                                     "\x00"
        "boardflags=0xa00"                                                   "\x00"
        "boardflags2=0x2000"                                                 "\x00"
        "boardflags3=0x08100100"                                             "\x00"
        "rxgains2gelnagaina0=0"                                              "\x00"
        "rxgains2gtrisoa0=0"                                                 "\x00"
        "rxgains2gtrelnabypa0=0"                                             "\x00"
        "rxgains5gelnagaina0=0"                                              "\x00"
        "rxgains5gtrisoa0=0"                                                 "\x00"
        "rxgains5gtrelnabypa0=0"                                             "\x00"
        "pdgain5g=5"                                                         "\x00"
        "pdgain2g=5"                                                         "\x00"
        "rxchain=1"                                                          "\x00"
        "txchain=1"                                                          "\x00"
        "aa2g=1"                                                             "\x00"
        "aa5g=1"                                                             "\x00"
        "tssipos5g=1"                                                        "\x00"
        "tssipos2g=1"                                                        "\x00"
        "femctrl=0"                                                          "\x00"
        "pa2ga0=-156,5865,-667"                                              "\x00"
        "pa5ga0=-181,5548,-667,-179,5526,-662,-174,5469,-649,-172,5534,-656"        "\x00"
        "pdoffset2g40ma0=0xa"                                                "\x00"
        "pdoffset40ma0=0xaab9"                                               "\x00"
        "pdoffset80ma0=0xba88"                                               "\x00"
        "extpagain5g=2"                                                      "\x00"
        "extpagain2g=2"                                                      "\x00"
        "maxp2ga0=0x48"                                                      "\x00"
        "maxp5ga0=0x48,0x48,0x48,0x48"                                       "\x00"
        "cckbw202gpo=0"                                                      "\x00"
        "cckbw20ul2gpo=0"                                                    "\x00"
        "mcsbw202gpo=0x03000000"                                             "\x00"
        "mcsbw402gpo=0x90200000"                                             "\x00"
        "mcsbw205glpo=0x0c860200"                                            "\x00"
        "mcsbw405glpo=0xc0800400"                                            "\x00"
        "mcsbw805glpo=0xc0800400"                                            "\x00"
        "mcsbw205gmpo=0x0c860200"                                            "\x00"
        "mcsbw405gmpo=0xc0800400"                                            "\x00"
        "mcsbw805gmpo=0xc0800400"                                            "\x00"
        "mcsbw205ghpo=0x0c860200"                                            "\x00"
        "mcsbw405ghpo=0xc0800400"                                            "\x00"
        "mcsbw805ghpo=0xc0800400"                                            "\x00"
        "dot11agofdmhrbw202gpo=0x4321"                                       "\x00"
        "ofdmlrbw202gpo=0x0011"                                              "\x00"
        "swctrlmap_2g=0x00002111,0x00002212,0x00002212,0x000000,0x3ff"       "\x00"
        "swctrlmap_5g=0x00002414,0x00002818,0x00002818,0x000000,0x3ff"       "\x00"
        "swctrlmapext_5g=0x00000000,0x00000000,0x00000000,0x000000,0x000"    "\x00"
        "swctrlmapext_2g=0x00000000,0x00000000,0x00000000,0x000000,0x000"    "\x00"
        "itrsw=1"                                                            "\x00"
        "rssi_delta_5gh=-24,-24,-24,-24,-32,-32,-32,-32,-24,-24,-24,-24"     "\x00"
        "rssi_delta_5gmu=-24,-24,-24,-24,-32,-32,-32,-32,-24,-24,-24,-24"    "\x00"
        "rssi_delta_5gml=-24,-24,-24,-24,-32,-32,-32,-32,-24,-24,-24,-24"    "\x00"
        "rssi_delta_5gl=-24,-24,-24,-24,-32,-32,-32,-32,-24,-24,-24,-24"     "\x00"
        "rssi_delta_2gb0=-24,0,0,0,-36,0,0,0"                                "\x00"
        "rssi_delta_2gb1=0,0,0,0,0,0,0,0"                                    "\x00"
        "rssi_delta_2gb2=0,0,0,0,0,0,0,0"                                    "\x00"
        "rssi_delta_2gb3=0,0,0,0,0,0,0,0"                                    "\x00"
        "rssi_delta_2gb4=0,0,0,0,0,0,0,0"                                    "\x00"
        "rssi_cal_freq_grp_2g=0x0,0x00,0x00,0x08,0x00,0x00,0x00"             "\x00"
        "rssi_cal_rev=1"                                                     "\x00"
        "rxgaincal_rssical=1"                                                "\x00"
        "rssi_qdB_en=1"                                                      "\x00"
        "fdss_level_2g=2,2"                                                  "\x00"
        "fdss_level_5g=2,2"                                                  "\x00"
        "\x00\x00";
#ifdef __cplusplus
} /*extern "C" */
#endif


#else /* ifndef INCLUDED_NVRAM_IMAGE_H_ */

#error Wi-Fi NVRAM image included twice

#endif /* ifndef INCLUDED_NVRAM_IMAGE_H_ */
