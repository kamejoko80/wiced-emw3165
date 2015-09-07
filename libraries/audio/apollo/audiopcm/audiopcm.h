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
* @file audiopcm.h
*
* audiopcm library main API defintion
*
*/
#ifndef _H_AUDIOPCM_H_
#define _H_AUDIOPCM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "audiopcm_types.h"

/** The package audiopcm is implementing the packet based error recovery for
 * Apollo/BRCM audio specification as documented in
 *
 * http://confluence.broadcom.com/display/MobileMultimedia/Apollo+Wireless+Audio+Specification
 *
 * The input of the audiopcm component are the RTP packets from the upper
 * application, taken "as-is" from the socket output.
 *
 * The audiopcm will separate audio payload from redundant data and FEC data,
 * will try to recover missing packets based on the available informations,
 * and when there are no concealment data to recover missing audio packets
 * audiopcm will trigger the audioplc (packet loss concealment) from the
 * linked audioplc component library.
 *
 * NOTE: for multi-channel audio input the audioplc is triggered only after
 *       the extraction of the playback channel (mono signal)
 *
 * The output of the audiopcm library is a single channel PCM audio stream
 * based on the setting from the audiopcm_start call.
 *
 * The audiopcm has two input queue methods, WRITE and ZERO_COPY_WRITE
 * The normal write will introduce an extra memcpy, while the zero_copy version
 * is leveraging the internal demux queue as a buffer factory, holding the input buffer
 * (during zc_init) until the buffer is released (durinc zc_complete)
 *
 * NOTE: on wiced only the normal write is implemented, since the audio packets are
 *       passed from the network and not pulled from the audiopcm internal buffer factory.
 *
 * The audiopcm component can work in two modes: with and without clock reference.
 * when clock is disabled the component is "free-running", all the packets are passed
 * into the pipeline as fast as possible, and the application (audio render) will have
 * to apply the proper timing for the final playback based on timestamps.
 *
 * Note: on WICED the only availabe mode is the free-running clock, there is no
 *       support for the internal clock management.
 *
 * DATA_FLOW_MODEL:
 * The audiopcm component can work in PUSH(in)/PUSH(out) or PULL(in)/PULL(out) based
 * on which IN/OUT callbacks are set. Currenlty (and especially for WICED) the model
 * is fixed to be PUSH/PUSH which is the optimal structure for free-clock settings
 * since this will not affect the internal queues other than front/back pressure.
 */

/* ******************************************************************************************************** */
/* AudioPCM library data flow chart                                                                         */
/* ******************************************************************************************************** */

/*
 *                     audio_rtp_fifo
 *                  {AUDIO_RTP_QUEUE_SIZE}
 *                     +------------+
 * input_push_pkt ---> | | | | | .. |
 * (main app. call)    +------------+
 *                           |
 *                           |
 *                           V
 *                    <<< rtp_loop thread >>>
 *                    does: FEC/CPY recovery)
 *                    does: lpcm_demux
 *                           |                   audio_pkt_fifo
 *                           |                {AUDIO_PKT_QUEUE_SIZE}
 *                           | (descriptors)    +------------+
 *                           +----------------> | | | | | ...|
 *                           |                  +------------+
 *                           |
 *                           |
 *                           |            audio_sample_fifo(double buffer)
 *                           |               2x{AUDIO_SAMPLE_QUEUE_SIZE}
 *                           | (samples)        +------------+            <<< dsp_loop_thread >>>
 *                           +----------------> | | | | | ...|  <-------> does: audio PLC on samples
 *                                              +------------+            note: runs "in place" on
 *                                              | | | | | ...|                  audio_sample_queue
 *                                              +------------+                  filling output gaps
 *                                                    |
 *                                                    |
 *                                                    V
 *                                        <<< output_loop_noclk thread >>>
 *                                        does: audio_pcm_push callback
 *                                              to the mediaplayer app.
 *                                        uses: both audio_pkt AND audio_sample queues!
 *                                        depends: on the dsp_loop thread to advance on
 *                                                 inside the audio_sample queue.
 */

/* ******************************************************************************************************** */
/* Audio PCM formats and channel definition                                                                 */
/* ******************************************************************************************************** */

/*
 * This is the definition of supported PCM rtp payload formats.
 * note: currently only PCM/16bit is supported.
 */
enum
{
    /* Major formats. */
    APCM_FORMAT_WAV             = 0x010000,     /* Microsoft WAV format (little endian default). */

    /* Subtypes from here on. */
    APCM_FORMAT_PCM_S8          = 0x0001,       /* Signed 8 bit data */
    APCM_FORMAT_PCM_16          = 0x0002,       /* Signed 16 bit data */
    APCM_FORMAT_PCM_24          = 0x0003,       /* Signed 24 bit data */
    APCM_FORMAT_PCM_32          = 0x0004,       /* Signed 32 bit data */
    APCM_FORMAT_PCM_U8          = 0x0005,       /* Unsigned 8 bit data (WAV and RAW only) */
    APCM_FORMAT_FLOAT           = 0x0006,       /* 32 bit float data */
    APCM_FORMAT_DOUBLE          = 0x0007,       /* 64 bit float data */

    /* Endian-ness options. */
    APCM_ENDIAN_FILE            = 0x00000000,   /* Default file endian-ness. */
    APCM_ENDIAN_LITTLE          = 0x10000000,   /* Force little endian-ness. */
    APCM_ENDIAN_BIG             = 0x20000000,   /* Force big endian-ness. */
    APCM_ENDIAN_CPU             = 0x30000000,   /* Force CPU endian-ness. */
    /**/
    APCM_FORMAT_SUBMASK         = 0x0000FFFF,
    APCM_FORMAT_TYPEMASK        = 0x0FFF0000,
    APCM_FORMAT_ENDMASK         = 0x30000000,
    APCM_FORMAT_UNKNOWN         = 0x1FFFFFFF
};


/*
 * This is the definition of supported channel type.
 * This is used to define which channel the audiopcm should extract
 * from the multi channel payload PCM format.
 */
enum {
    APCM_CHANNEL_MAP_NONE  = 0,          /* None or undefined    */
    APCM_CHANNEL_MAP_FL    = (1 << 0),   /* Front Left           */
    APCM_CHANNEL_MAP_FR    = (1 << 1),   /* Front Right          */
    APCM_CHANNEL_MAP_FC    = (1 << 2),   /* Front Center         */
    APCM_CHANNEL_MAP_LFE1  = (1 << 3),   /* LFE-1                */
    APCM_CHANNEL_MAP_BL    = (1 << 4),   /* Back Left            */
    APCM_CHANNEL_MAP_BR    = (1 << 5),   /* Back Right           */
    APCM_CHANNEL_MAP_FLC   = (1 << 6),   /* Front Left Center    */
    APCM_CHANNEL_MAP_FRC   = (1 << 7),   /* Front Right Center   */
    APCM_CHANNEL_MAP_BC    = (1 << 8),   /* Back Center          */
    APCM_CHANNEL_MAP_LFE2  = (1 << 9),   /* LFE-2                */
    APCM_CHANNEL_MAP_SIL   = (1 << 10),  /* Side Left            */
    APCM_CHANNEL_MAP_SIR   = (1 << 11),  /* Side Right           */
    APCM_CHANNEL_MAP_TPFL  = (1 << 12),  /* Top Front Left       */
    APCM_CHANNEL_MAP_TPFR  = (1 << 13),  /* Top Front Right      */
    APCM_CHANNEL_MAP_TPFC  = (1 << 14),  /* Top Front Center     */
    APCM_CHANNEL_MAP_TPC   = (1 << 15),  /* Top Center           */
    APCM_CHANNEL_MAP_TPBL  = (1 << 16),  /* Top Back Left        */
    APCM_CHANNEL_MAP_TPBR  = (1 << 17),  /* Top Back Right       */
    APCM_CHANNEL_MAP_TPSIL = (1 << 18),  /* Top Side Left        */
    APCM_CHANNEL_MAP_TPSIR = (1 << 19),  /* Top Side Right       */
    APCM_CHANNEL_MAP_TPBC  = (1 << 20),  /* Top Back Center      */
    APCM_CHANNEL_MAP_BTFC  = (1 << 21),  /* Bottom Front Center  */
    APCM_CHANNEL_MAP_BTFL  = (1 << 22),  /* Bottom Front Left    */
    APCM_CHANNEL_MAP_BTFR  = (1 << 23),  /* Bottom Front Right   */
    APCM_CHANNEL_ZERO_024  = (1 << 24),  /* reserved: zero bit   */
    APCM_CHANNEL_ZERO_025  = (1 << 25),  /* reserved: zero bit   */
    APCM_CHANNEL_ZERO_026  = (1 << 26),  /* reserved: zero bit   */
    APCM_CHANNEL_ZERO_027  = (1 << 27),  /* reserved: zero bit   */
    APCM_CHANNEL_ZERO_028  = (1 << 28),  /* reserved: zero bit   */
    APCM_CHANNEL_ZERO_029  = (1 << 29),  /* reserved: zero bit   */
    APCM_CHANNEL_ZERO_030  = (1 << 30),  /* reserved: zero bit   */
    APCM_CHANNEL_ZERO_031  = (1 << 31),  /* reserved: zero bit   */
    /* aliases */
    APCM_CHANNEL_MAP_L     = APCM_CHANNEL_MAP_FL,
    APCM_CHANNEL_MAP_MONO  = APCM_CHANNEL_MAP_FL,
    APCM_CHANNEL_MAP_R     = APCM_CHANNEL_MAP_FR,
    APCM_CHANNEL_MAP_C     = APCM_CHANNEL_MAP_FC,
    APCM_CHANNEL_MAP_LFE   = APCM_CHANNEL_MAP_LFE1,
    APCM_CHANNEL_MAP_MAX   = APCM_CHANNEL_MAP_BTFR
};



/*
 * Default paramters for audiopcm component
 */
#define APCM_DEFAULT_AUDIO_TARGET_LATENCY_MS (80)
#define APCM_DEFAULT_AUDIO_JITTER_BUFFER_MS (70)
#define APCM_DEFAULT_AUDIO_RTT_MS (6)


/* ******************************************************************************************************** */
/* SYSTEM                                                                                                   */
/* ******************************************************************************************************** */

/** audio_new: constructor
 * @param err if NULL will be ignored
 * @param audiopcm will return the handle to the component once created
 * @param logging_level from LOG_LOW to LOG_MAX (see logger.h)
 * @return return value is ZERO on success, NON zero on error.
 * The demuxer handle is passed to the input dmx param on success.
 */
int8_t audiopcm_new(int8_t * err, void **audiopcm,
                    int logging_level, uint64_t clk_zero);

/** audio_start: will start the component by creating the internal processing threads
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 * @param target_audio_fmt expected audio format of the input multi-channel PCM stream
 * @param target_audio_ch specific audio channel map to be extracted by the audiopcm component (max 2 channels)
 * @param target_audio_latency_ms total end-to-end latency required (msec)
 * @param target_jitter_buffer_ms the fraction of the target_audio_latency to be used for jitter control (msec)
 * @param rtt (msec) max round trip time expected on the network link.
 */
int8_t audiopcm_start(int8_t *err, void *audiopcm,
                      uint32_t target_audio_fmt, uint32_t target_audio_ch,
                      uint32_t target_audio_latency_ms, uint32_t target_jitter_buffer_ms, uint32_t rtt_ms);

/** audio_stop: will stop the component without destroying the internal processing threads and queues
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_stop(int8_t *err, void *audiopcm);

/** audio_destroy: will destroy the component (audiopcm must be stopped first!)
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_destroy(int8_t * err, void **audiopcm);


/* ******************************************************************************************************** */
/* SETTERS                                                                                                  */
/* ******************************************************************************************************** */

/** audiopcm_set_label: will set a component label, used on LOG_MSG (must be set before audiopcm_start)
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_set_label(int8_t * err, void *audiopcm, char *label);

/** audiopcm_set_pcl: turn on/off the AudioPLC sub-component (must be set before audiopcm_start)
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_set_plc(int8_t * err, void *audiopcm, uint8_t enable_plc);

/** audiopcm_set_clock: turn on/off clock support (ONLY OFF is supported, must be set before audiopcm_start)
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_set_clock(int8_t * err, void *audiopcm, uint8_t enable_clock);

/** audiopcm_set_log_verbosity: set the log level for LOG_MSG verbosity
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_set_log_verbosity(int8_t * err, void *audiopcm, uint8_t lvl);

/** audiopcm_set_userid: set the user id of the audiopcm component, useful for debug when multiple
 *                       when multiple audiopcm components are created.
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_set_userid(int8_t * err, void *audiopcm, uint8_t userid);

/** audiopcm_set_output_byteswap: RESERVED
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_set_output_byteswap(int8_t *err, void *audiopcm, uint8_t boolean_flag);

/** audiopcm_set_label_statistics_filename: set the output stats filename (NOT supported on WICED)
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_set_statistics_filename(int8_t * err, void *audiopcm, char *filename);

/** audiopcm_set_output_filename: set the output dumps base filename (NOT supported on WICED)
 * @param err if NULL will be ignored
 * @param audiopcm handle to a valid audiopcm component
 */
int8_t audiopcm_set_output_filename(int8_t * err, void *audiopcm, char * file);

/** audiopcm_set_target_match_map_bypass: force audioplayback if target channel does not match channel map.
 * audio will fall back on MONO (FL) channel.
 * @param err if NULL will be ignored
 * @param boolean_flag value 1 will force audio playback on channel mismatch
 */
int8_t audiopcm_set_target_match_map_bypass(int8_t * err, void *audiopcm, uint8_t boolean_flag);

/* ******************************************************************************************************** */
/* GETTERS                                                                                                  */
/* ******************************************************************************************************** */

/** audiopcm_get_is_valid: check if the audiopcm component is valid
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 * @return return value is ZERO on success, NON zero on error.
 */
int8_t audiopcm_get_is_valid(int8_t *err, void *audiopcm);

/** audiopcm_get_api_version: get the Apollo Audio API version implemented by the component
 * Api versioning: MJR.MIN with a flag to tell if the component is in debug mode
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 */
int8_t audiopcm_get_api_version(int8_t *err, void *audiopcm,
                                uint8_t *api_major, uint8_t *api_minor, uint8_t *is_debug);


/** audiopcm_get_api_version: get the Apollo audiopcm component status
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 * @return return value is ZERO on success, NON zero on error.
 */
int8_t audiopcm_get_status(int8_t *err, void *audiopcm, audiopcm_cstatus_t *status);


/** audiopcm_get_version: get the Audiopcm sw version implemented by the component
 * Api versioning: MJR.MIN.revision this is useful to distinguish components that have the
 *                 same Apollo API version but a different sw implementation.
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 */
int8_t audiopcm_get_version(int8_t *err, void *audiopcm,
                            uint8_t *major, uint8_t *minor, char *revision);

/** audiopcm_get_userid: get the userid of the provided audiopcm component
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 */
int8_t audiopcm_get_userid(int8_t * err, void *audiopcm, uint8_t *userid);



/* ******************************************************************************************************** */
/* TOOLS                                                                                                    */
/* ******************************************************************************************************** */

/** chnum_to_chmap: convert a channel number to a specific channel_map value as per Apollo audio specification
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 */
int8_t chnum_to_chmap(int8_t *err, int8_t chnum);

/** chmap_to_chnum: convert a channel map to a specific channel number value as per Apollo audio specification
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 */
int8_t chmap_to_chnum(int8_t *err, int8_t chmap);



/* ******************************************************************************************************** */
/* DATA QUEUES SPECIFIC */
/* ******************************************************************************************************** */

/** audiopcm_get_queue_level: retrieve the level and max size of the audiopcm input RTP queue
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 */
int8_t audiopcm_get_queue_level(int8_t *err, void *audiopcm,
                                uint32_t *audiopcm_queue_level, uint32_t *audiopcm_queue_size);

/** audiopcm_set_alert_overflow_queue: set the level on the RTP input queue to trigger the alert for OVERFLOW
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 */
int8_t audiopcm_set_alert_overflow_queue(int8_t *err,
                                         void *audiopcm,
                                         uint32_t audiopcm_queue_level);

/** audiopcm_set_alert_underfloe_queue: set the level on the RTP input queue to trigger the alert for UNDERFLOW
 * @param err if NULL will be ignored
 * @param audiopcm handle to an audiopcm component
 */
int8_t audiopcm_set_alert_underflow_queue(int8_t *err,
                                          void* *audiopcm,
                                          uint32_t audiopcm_queue_level);



/* ******************************************************************************************************** */
/* CALLBACKS                                                                                                */
/* ******************************************************************************************************** */

/* *********************************************** */
/* ALERTS, HEARTBEAT, EVENT calls                  */
/* *********************************************** */

/** public set/get method: audiopcm_set_alert
 *  set the callback to raise for an internal system event (like underflow/overflow, missing component)
 */
int8_t audiopcm_set_system_alert_CBACK(int8_t *err,
                                       void *audiopcm,
                                       void *player_app,
                                       int8_t (*sys_alert_CBACK)(
                                           int8_t *err,
                                           void *audiopcm,
                                           void *player_app,
                                           audiopcm_event_t *event,
                                           void *event_data,
                                           uint32_t event_data_size));

/** public set/get method: set_audio_event
 *  set the callback to raise an event for the AUDIO stream only (like format change, sample_cnt)
 */
int8_t audiopcm_set_audio_alert_CBACK(int8_t *err,
                                      void *audiopcm,
                                      void *player_app,
                                      int8_t (*audio_alert_CBACK)(
                                          int8_t *err,
                                          void *audiopcm,
                                          void *player_app,
                                          audiopcm_event_t *event,
                                          void *event_data,
                                          uint32_t event_data_size));

/** public set/get method: set_audiopcm_hearbeat_CBACK
 *  set the callback to request an heartbeat every (x) seconds
 */
int8_t audiopcm_set_heartbeat_CBACK(int8_t *err,
                                    void *audiopcm,
                                    void *player_app,
                                    uint32_t milliseconds,
                                    int8_t (*heartbeat_CBACK)(
                                        int8_t *err,
                                        void *audiopcm,
                                        void *player_app));


/* *********************************************** */
/* DATA I/O: INPUT: PULL(in) callbacks             */
/* *********************************************** */

/** public set/get method: set_input_pull callkback
 *  set the callback to let the audiopcm work in PULL(in) mode,
 *  the component interal input loop will ask the upper applifor a new buffer when needed
 */
int8_t audiopcm_set_input_pull_CBACK(int8_t *err,
                                     void *audiopcm,
                                     int8_t (*audio_output_input_pull_CBACK)(
                                         int8_t *err,
                                         void *audiopcm,
                                         void *player_app,
                                         unsigned char **buf,
                                         uint32_t *buf_size,
                                         uint32_t *buf_id,
                                         void **userdata));

/* *********************************************** */
/* DATA I/O: INPUT: PUSH(int) calls                */
/* *********************************************** */

/** audiopcm_input_push_pkt
 *  push an RTP packet to the audiopcm input. Audiopcm will handle the content internally.
 *  if needed the application should release the packet after this call.
 *  Note: for Apollo audio RTP packets the audio_info should be set to NULL,
 *  all the details on format and content will be derived by the Apollo audio header format
 *  of the RTP payload
 */
uint32_t audiopcm_input_push_pkt(int8_t *err, void *audiopcm, uint64_t rtp_pkt_num,
                                 char *buf, uint32_t buf_len, audiopcm_audio_info_t *audio_info);

/** audiopcm_input_push_pkt_zc: same as the above, but the application will reuest the packet
 *  from the internal audiopcm buffer queue directly, saving one memcpy stage.
 *
 *  Note: NOT implemented yet.
 */
int8_t audiopcm_input_push_zc_begin(int8_t *err, void *audiopcm, audiopcm_rtp_t **audio_rtp_pkt);
int8_t audiopcm_input_push_zc_end(int8_t *err, void *audiopcm, audiopcm_rtp_t *audio_rtp_pkt);



/* *********************************************** */
/* DATA I/O: PUSH(out) callbacks                   */
/* *********************************************** */

/** audiopcm_set_output_push_CBACK
 *  callback for the output stage called once PCM samples are available from the output queue
 *  of the audiopcm component to be sent to the upper application
 */
int8_t audiopcm_set_output_push_CBACK(int8_t *err,
                                      void *audiopcm,
                                      void *player_app,
                                      int8_t (*audio_output_push_CBACK)(
                                          int8_t *err,
                                          void *audiopcm,
                                          void *player_app,
                                          unsigned char *buf,
                                          uint32_t *buf_len,
                                          uint32_t *buf_id,
                                          audiopcm_audio_info_t *audio_info));

/** audiopcm_set_output_push_zc
 *  same as the above but the buffer is requested from the buffer pool of the application itself,
 *  skipping one memcpy.
 *
 *  Note: NOT implemented yet.
 */
int8_t audiopcm_set_output_push_zc_begin_CBACK(int8_t *err,
                                               void *audiopcm,
                                               void *player_app,
                                               int8_t (*audio_output_push_zc_init_CBACK)(
                                                   int8_t *err,
                                                   void *audiopcm,
                                                   void *player_app,
                                                   unsigned char **buf,
                                                   uint32_t *buf_len,
                                                   uint32_t *buf_id,
                                                   audiopcm_audio_info_t *audio_info));

int8_t audiopcm_set_output_push_zc_end_CBACK(int8_t *err,
                                             void *audiopcm,
                                             void *player_app,
                                             int8_t (*audio_push_zc_complete_CBACK)(
                                                 int8_t *err,
                                                 void *audiopcm,
                                                 void *player_app,
                                                 unsigned char *buf,
                                                 uint32_t *buf_len,
                                                 uint32_t *buf_id,
                                                 audiopcm_audio_info_t *audio_info));


/* *********************************************** */
/* DATA I/O: PUSH(out) calls                       */
/* *********************************************** */

/** audiopcm_output_pull_pkt
 * call in the context of the upper application to pull audio samples out of the
 * audiopcm output queue.
 *
 *  Note: NOT implemented yet.
 */

int8_t audiopcm_output_pull_pkt(int8_t *err, void *audiopcm,
                                char *buf, uint32_t len, unsigned int flags);



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _H_AUDIOPCM_H_ */
