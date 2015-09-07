/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "apollo_audio_dct.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define RTP_DEFAULT_PORT                5004
#define RTP_PACKET_MAX_SIZE             1500
#define RTP_BASE_HEADER_SIZE            12
#define RTP_EXT_SIZE_1_0                20
#define RTP_HEADER_SIZE                 (RTP_BASE_HEADER_SIZE + RTP_EXT_SIZE_1_0)
#define RTP_PACKET_MAX_DATA             (RTP_PACKET_MAX_SIZE - RTP_HEADER_SIZE)

#define RTP_PAYLOAD_AUDIO               98
#define RTP_PAYLOAD_AUDIO_DUP           99
#define RTP_PAYLOAD_FEC                 100
#define RTP_MAX_SEQ_NUM                 ((uint32_t)0x0000FFFF)

/*
 * RTP Audio extension header definitions.
 */

#define RTP_AUDIO_EXT_ID_LPCM           0x00010000
#define RTP_AUDIO_EXT_LENGTH            5           /* Length of extension header in 32 bit units */

#define RTP_AUDIO_EXT_MAJOR_VER         1
#define RTP_AUDIO_EXT_MINOR_VER         0
#define RTP_AUDIO_EXT_VER_MAJOR_SHIFT   28
#define RTP_AUDIO_EXT_VER_MINOR_SHIFT   24
#define RTP_AUDIO_EXT_VERSION           ((RTP_AUDIO_EXT_MAJOR_VER << RTP_AUDIO_EXT_VER_MAJOR_SHIFT) | (RTP_AUDIO_EXT_MINOR_VER << RTP_AUDIO_EXT_VER_MINOR_SHIFT))

#define RTP_AUDIO_SPATIAL_INTERLEAVING  0x00000000
#define RTP_AUDIO_TIME_INTERLEAVING     0x00200000

#define RTP_AUDIO_FEC_PRIOR             0x00100000
#define RTP_AUDIO_FEC_POST              0x00000000

#define RTP_AUDIO_SAMPLE_RATE_441       0x00000000
#define RTP_AUDIO_SAMPLE_RATE_48        0x20000000
#define RTP_AUDIO_SAMPLE_RATE_96        0x40000000
#define RTP_AUDIO_SAMPLE_RATE_192       0x60000000
#define RTP_AUDIO_SAMPLE_RATE_MASK      0xE0000000

#define RTP_AUDIO_BPS_16                0x00000000
#define RTP_AUDIO_BPS_24                0x04000000

#define RTP_AUDIO_CHANNEL_SHIFT         22

/*
 * Uint32 offsets within the RTP packet.
 */

#define RTP_AUDIO_OFFSET_BASE           0
#define RTP_AUDIO_OFFSET_TIMESTAMP      1
#define RTP_AUDIO_OFFSET_SSRC           2
#define RTP_AUDIO_OFFSET_ID             3
#define RTP_AUDIO_OFFSET_VERSION        4
#define RTP_AUDIO_OFFSET_CLOCK_LOW      5
#define RTP_AUDIO_OFFSET_CLOCK_HIGH     6
#define RTP_AUDIO_OFFSET_FORMAT         7


#define PLAYER_TAG_VALID                0x51EDBABE
#define PLAYER_TAG_INVALID              0xDEADBEEF

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum {
    PLAYER_EVENT_SHUTDOWN           = (1 << 0),
    PLAYER_EVENT_PLAY               = (1 << 1),
    PLAYER_EVENT_STOP               = (1 << 2),
    PLAYER_EVENT_AUTOSTOP           = (1 << 3)
} PLAYER_EVENTS_T;

#define PLAYER_ALL_EVENTS       (-1)

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct rtp_player_s {
    uint32_t tag;
    int rtp_done;
    int stop_received;

    wiced_event_flags_t events;

    platform_dct_network_config_t *dct_network;
    platform_dct_wifi_config_t *dct_wifi;
    apollo_audio_dct_t *dct_app;

    wiced_thread_t rtp_thread;
    wiced_thread_t *rtp_thread_ptr;

    uint16_t audio_format;
    uint16_t audio_channels;
    uint32_t audio_sample_rate;
    uint32_t audio_byte_rate;
    uint16_t audio_block_align;
    uint16_t audio_bits_per_sample;

    uint8_t audio_volume;

    int rtp_port;
    int rtcp_port;

    wiced_ip_address_t mrtp_ip_address;
    wiced_udp_socket_t rtp_socket;

    uint32_t last_seq;
    uint32_t next_seq;
    int seq_valid;
    int rtp_data_received;
    int first_rtp_data;

    uint64_t rtp_packets_received;
    uint64_t total_bytes_received;

    uint32_t rtp_seq_number;
    uint8_t rtp_payload_type;
    uint32_t rtp_ssrc;

    /* audiopcm(audioplc) support */
    void* audiopcm;
    int8_t use_audiopcm_chnum;

    wiced_audio_render_ref audio;

} rtp_player_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif
