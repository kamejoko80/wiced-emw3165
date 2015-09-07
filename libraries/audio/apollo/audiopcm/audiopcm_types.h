/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
/**
 * @file audiopcm_types.h
 *
 * Collection of exported defintions and component type
 *
 */
#ifndef _H_AUDIOPCM_TYPES_H_
#define _H_AUDIOPCM_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* the audio (payload data) queue size is based on
 * max sample rate and max delay that we want to handle
 * in our case let's say 200ms, 96Khz, 24bit per sample
 * 96000*24*.2=461000bit=58Kbytes so we use 64Kbytes (2^16)
 */
//#define AUDIO_SAMPLE_QUEUE_SIZE (1<<16) /* MUST be pow of 2 */

/* 48000*16*.34s = 48Kbyte (2^15) */
#define AUDIO_SAMPLE_QUEUE_SIZE (1<<15) /* MUST be pow of 2 */

/* the audio (pkt descriptor) queue size is based on
 * the size of the AUDIO_SAMPLE_QUEUE_SIZE, since we have
 * as worst case (5.1 96Khz, 24bit) 2ms of payloads per channel.
 *
 * we need to take in account the worst case: with ~1500bytes max per RTP MTU
 * and having a 6ch payload we compute the AUDIO_PKT_QUEUE size from the above
 * AUDIO_SAMPLE_QUEUE_SIZE:
 *
 * pkt_queue_size = sample_queue_size(bytes)/ (1500/6) =~ 262 slots
 *
 * we also need to keep a margin for smaller pkts and bursts, so we use 512 slots
 *
 * Note: these are very HIGH values for wide delays (10ms per pkt) and pkt loss (8-10%)
 *       once we know the REAL number from the field tests we can reduce these sizes,
 *       but it is important to keep them pow(2) to handle wraparound in a simple way.
 */

//#define AUDIO_PKT_QUEUE_SIZE (1<<9) /* MUST be pow of 2 */

// 48k, max 6ch, 340ms of sample queue --> 170pkts --> using 256
#define AUDIO_PKT_QUEUE_SIZE (1<<8) /* MUST be pow of 2 */

/* the input RTP queue is the buffer between the upper application and the RTP thread.
 * The RTP thread is the one performing FEC/CPY recovery so you will ALWAYS need
 * a minimum size equal to 2xBL (burst length) to allow lookahead in case of pkt loss.
 * This queue will sustain front pressure from the incoming data flow (skt layer) but
 * but also backpressure from the DSP/OUTPUT stages. So if the audio render is stuck
 * this queue will overflow.
 * Note: the "easy configuration" is to keep this size equal to the AUDIO_PKT_QUEUE_SIZE.
 *
 * For normal conditions this queue should be as big as the worst burst loss expected
 * from the wifi channel.
 */
#define AUDIO_RTP_QUEUE_SIZE (1<<6) /* MUST be pow of 2 */

/*
 * Note: for memory footprint there are 3 fifos to tune:
 *
 * audiopcm_rtp_t audio_rtp_fifo[AUDIO_PKT_QUEUE_SIZE];
 * which handles the "raw" incoming RTP traffic "as is" (big footprint)
 *
 * audio_pkt_t audio_pkt_fifo[AUDIO_PKT_QUEUE_SIZE];
 * which handle the extracted data flow for concealment/buffering
 * but only at audio_pkt level (small footprint)
 *
 * uint8_t audio_sample_fifo[2][AUDIO_SAMPLE_QUEUE_SIZE];
 * which handle the extracted data flow for concealment/buffering
 * at the real byte level (big footprint)
 *
 * Note: increase/reduce the size keeping the ration between them
 */


/*
 * Wireless Audio Specification does define a maximum burst length
 * of 16 for RTP contiguos audio packet loss
 * we need to double that number to allow for cases where we search
 * for bursts of cpy/fec to find the first group of data.
 */
#define AUDIO_PKT_LOSS_BURST_MAX (16*2)



/*
 * RTP protocol data and payload sizes
 */
/* standard MTU, no jumbo frames for now */
#define RTP_PKT_MTU_SIZE ((uint32_t)(1500))
/* Header + TS + SSRC */
#define RTP_PKT_HEADER_SIZE  ((uint32_t)(12))
/* Header extension for Audio LPCM specification*/
#define RTP_PKT_HEADER_EXT_AUDIO_SIZE_1_0  ((uint32_t)(20))
/* Header extension for FEC payload specification */
#define RTP_PKT_HEADER_EXT_FEC_SIZE  ((uint32_t)(8))

#define RTP_PKT_PAYLOAD_SIZE ((uint32_t)(RTP_PKT_MTU_SIZE - RTP_PKT_HEADER_SIZE))
#define RTP_PKT_PAYLOAD_AUDIO_SIZE ((uint32_t)(RTP_PKT_MTU_SIZE - RTP_PKT_HEADER_SIZE - RTP_PKT_HEADER_EXT_AUDIO_SIZE))
#define RTP_PKT_PAYLOAD_FEC_SIZE ((uint32_t)(RTP_PKT_MTU_SIZE - RTP_PKT_HEADER_SIZE - RTP_PKT_HEADER_EXT_FEC_SIZE))

#define RTP_PKT_PAYLOAD_TYPE_AUDIO (98)
#define RTP_PKT_PAYLOAD_TYPE_REDUNDANT_AUDIO (99)
#define RTP_PKT_PAYLOAD_TYPE_FEC_AUDIO (100)

/* RTP PTS is 32bit wide, internally we use 64bit for PTS */
#define RTP_PKT_PTS_WRAPAROUND ((((uint64_t)1)<<32)-1)
/* RTP SN is 16bit wide */
#define RTP_PKT_SN_WRAPAROUND ((((uint16_t)1)<<16)-1)



/**
 * audiopcm buffer info
 * this is the audio info structure attached to each audio packet (audio_pkt_fifo)
 */
typedef struct audiopcm_audio_info_
{
    /* RTP timing */
    uint64_t rtp_pkt_num;
    uint64_t PTS;
    uint64_t sys_clk;
    /* audio spec versioning */
    uint16_t extension_id;
    uint16_t extension_length;
    uint8_t  version_mjr;
    uint8_t  version_min;
    /* priority info */
    uint32_t audio_format;
    uint32_t audio_samplerate;
    uint32_t audio_channel_map; /* 7.1=8ch */
    uint8_t  audio_channel_num; /* num of channels in the channel map */
    uint8_t  audio_interleave_f;
    uint8_t  rtp_fec_order_f;
    uint8_t  burst_len_sync;
    uint16_t rtp_loss_burst_len;

    /* aux info */
    uint16_t audio_frames_num;
}audiopcm_audio_info_t;


/**
 * audio packet struct
 * this is the structure of the buffers into the audio_pkt_fifo
 * this fifo just contain a touple (index+length) of a chunk of samples
 * into the audio_sample_queue (which is where LPCM samples are stored and concealed)
 */
typedef struct audio_pkt_
{
    uint64_t pkt_id;
    //uint8_t *audio_data_start;
    uint8_t refcnt;
    uint32_t audio_data_start_idx;
    uint32_t audio_data_length_frames;
    uint32_t audio_data_length_bytes;
    audiopcm_audio_info_t audio_info;

}audio_pkt_t;


/**
 * rtp packet struct for audio input
 * this is the structure for buffers of the audio_rtp_fifo,
 * which is also the main input queue of the audiopcm pipeline.
 */
typedef struct audio_rtp_
{
    uint64_t pkt_id;
    uint32_t length;
    uint8_t  data[RTP_PKT_MTU_SIZE];
    audiopcm_audio_info_t audio_info;
    /* aux info */
    uint32_t token_id; //read only, do not touch
    uint8_t rtp_parse_hint_f; //helper flag
}audiopcm_rtp_t;



/** documenting events following "5 Ws" rules
  * http://en.wikipedia.org/wiki/Five_Ws
  *
  * for now only 3 Ws (who, what, where)
  */

/**
 * audiopcm event: component ("who")
 */
typedef enum
{
    AUDIOPCM_E_CMP_UNKNOWN         = -2,
    AUDIOPCM_E_CMP_ANYTYPE         = -1,
    AUDIOPCM_E_CMP_RESERVED        = 0,
    AUDIOPCM_E_CMP_AUDIO_SYNTAX    = 1,
    AUDIOPCM_E_CMP_INPUT_THREAD    = 2,
    AUDIOPCM_E_CMP_OUTPUT_THREAD   = 3,
    AUDIOPCM_E_CMP_RTP_THREAD      = 4,
    AUDIOPCM_E_CMP_DSP_THREAD      = 5,
    AUDIOPCM_E_CMP_CLK_THREAD      = 6,
    AUDIOPCM_E_CMP_MAX = AUDIOPCM_E_CMP_CLK_THREAD+1,
}audiopcm_event_component_t;

/**
 * audiopcm event: object ("where")
 */
typedef enum
{
    AUDIOPCM_E_OBJ_UNKNOWN                   = -2,
    AUDIOPCM_E_OBJ_ANYTYPE                   = -1,
    AUDIOPCM_E_OBJ_RESERVED                  = 0,
    AUDIOPCM_E_OBJ_AUDIO_STREAM              = 1,
    AUDIOPCM_E_OBJ_AUDIO_STREAM_SAMPLERATE   = 2,
    AUDIOPCM_E_OBJ_AUDIO_STREAM_BITPERSAMPLE = 3,
    AUDIOPCM_E_OBJ_AUDIO_STREAM_BURSTLENGHT  = 4,
    AUDIOPCM_E_OBJ_AUDIO_RTP_BUF             = 5,
    AUDIOPCM_E_OBJ_AUDIO_RTP_QUEUE           = 6,
    AUDIOPCM_E_OBJ_AUDIO_PKT_BUF             = 7,
    AUDIOPCM_E_OBJ_AUDIO_PKT_QUEUE           = 8,
    AUDIOPCM_E_OBJ_AUDIO_SAMPLE_QUEUE        = 9,
    AUDIOPCM_E_OBJ_AUDIO_STEREO_BUF          = 10,
    AUDIOPCM_E_OBJ_AUDIO_RENDER_CBAK         = 11,
    AUDIOPCM_E_OBJ_MAX = AUDIOPCM_E_OBJ_AUDIO_RENDER_CBAK+1,
}audiopcm_event_object_t;

/**
 * audiopcm event: info ("what")
 */
typedef enum
{
    AUDIOPCM_E_INFO_UNKNOWN             = -2,
    AUDIOPCM_E_INFO_ANYTYPE             = -1,
    AUDIOPCM_E_INFO_RESERVED            = 0,
    AUDIOPCM_E_INFO_ILLEGAL             = 1,
    AUDIOPCM_E_INFO_OVERFOW             = 2,
    AUDIOPCM_E_INFO_UNDERFLOW           = 3,
    AUDIOPCM_E_INFO_CHANGE              = 4,
    AUDIOPCM_E_INFO_INVALID             = 5,
    AUDIOPCM_E_INFO_DISCONTINUITY       = 6,
    AUDIOPCM_E_INFO_BIG_GAP             = 7,
    AUDIOPCM_E_INFO_BL_SYNC_FAIL        = 8,
    AUDIOPCM_E_INFO_BL_SYNC_MISMATCH    = 9,
    AUDIOPCM_E_INFO_PLC_INIT_FAIL       = 10,
    AUDIOPCM_E_INFO_PLC_DECODE_FAIL     = 11,
    AUDIOPCM_EVENT_MAX,
}audiopcm_event_info_t;

/**
 * audiopcm event: who/where/what
 */
typedef struct audiopcm_event_
{
    uint32_t id;
    audiopcm_event_component_t comp;
    audiopcm_event_object_t obj;
    audiopcm_event_info_t info;
}audiopcm_event_t;



/**
 * component type (ctype)
 */
typedef enum
{
    AUDIOPCM_CTYPE_UNKNOWN = -2,
    AUDIOPCM_CTYPE_ANYTYPE = -1,
    AUDIOPCM_CTYPE_RESERVED = 0,
    AUDIOPCM_CTYPE_SINGLE_THREAD = 1,
    AUDIOPCM_CTYPE_MULTI_THREAD = 2,
    AUDIOPCM_CTYPE_MAX
}audiopcm_ctype_t;


/**
 * component status (cstatus)
 */
typedef enum
{
    AUDIOPCM_CSTATUS_UNKNOWN = -2,
    AUDIOPCM_CSTATUS_ANYTYPE = -1,
    AUDIOPCM_CSTATUS_RESERVED = 0,
    AUDIOPCM_CSTATUS_CREATED = 1,
    AUDIOPCM_CSTATUS_READY   = 2,
    AUDIOPCM_CSTATUS_RUNNED = 4,
    AUDIOPCM_CSTATUS_STOPPED = 8,
    AUDIOPCM_CSTATUS_FLUSHED = 9,
    /**/
    AUDIOPCM_CSTATUS_MAX
}audiopcm_cstatus_t;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _H_AUDIOPCM_TYPES_H_ */
