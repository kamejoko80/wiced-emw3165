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

#include "wiced_audio.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * Configuration parameters for starting audio render.
 */

typedef struct wiced_audio_render_params_s {
    uint32_t buffer_nodes;                  /* Number of buffer nodes for audio render to allocate      */
    uint32_t buffer_ms;                     /* Buffering (pre-roll) time that audio render should use   */
    uint32_t threshold_ms;                  /* Threshold in ms for adding silence/dropping audio frames */
    int      clock_enable;                  /* 0 = disable (blind push), 1 = enable */
    wiced_audio_config_t audio_config;
} wiced_audio_render_params_t;

/**
 * Audio buffer structure.
 */

typedef struct wiced_audio_render_buf_s {
    int64_t  pts;
    uint8_t* data_buf;
    uint32_t data_offset;
    uint32_t data_length;
    void*    opaque;
} wiced_audio_render_buf_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/**
 * Callback for releasing audio buffers.
 */

typedef wiced_result_t (*wiced_audio_render_buf_release_cb_t)(wiced_audio_render_buf_t* buf, void* session_ptr);

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

typedef struct wiced_audio_render_s *wiced_audio_render_ref;

/** Initialize the audio render library.
 *
 * @param[in] params      : Pointer to the audio configuration parameters.
 * @param[in] cb          : Audio buffer release callback.
 * @paran[in] session_ptr : Application session pointer passed back in buffer release callback.
 *
 * @return Pointer to the audio render instance or NULL
 */
wiced_audio_render_ref wiced_audio_render_init(wiced_audio_render_params_t*        params,
                                               wiced_audio_render_buf_release_cb_t cb,
                                               void*                               session_ptr);

/** Deinitialize the audio render library.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_deinit(wiced_audio_render_ref audio);

/** Put the audio render in play state.
 * @note this functionality still needs to be implemented.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_play(wiced_audio_render_ref audio);

/** Put the audio render in pause state.
 * @note this functionality still needs to be implemented.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_pause(wiced_audio_render_ref audio);

/** Flush the audio render.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_flush(wiced_audio_render_ref audio);

/** Stop the audio render playback.
 * @note this functionality still needs to be implemented.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_stop(wiced_audio_render_ref audio);

/** Push an audio buffer to the audio render library.
 * @note The audio render becomes owner of the buffer pointed to
 * by the data_buf member until it is release via the buffer release
 * callback.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 * @param[in] buf    : Pointer to the audio buffer.
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_push(wiced_audio_render_ref audio, wiced_audio_render_buf_t* buf);

/** Set the output volume for the audio render library.
 *
 * @param[in] audio  : Pointer to the audio render instance.
 * @param[in] volume : The new volume (0-100)
 *
 * @return    Status of the operation.
 */
wiced_result_t wiced_audio_render_set_volume(wiced_audio_render_ref audio, uint8_t volume);


#ifdef __cplusplus
} /* extern "C" */
#endif
