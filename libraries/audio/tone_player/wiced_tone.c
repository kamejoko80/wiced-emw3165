/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**  \file    wiced_tone.c
        \brief  Implementation of APIs in #include "wiced_tone.h".
*/

#include "wiced.h"
#include "wiced_audio.h"

typedef struct
{
    uint8_t* data;
    uint32_t length;
    wiced_audio_config_t audio_info;
    uint8_t repeat;
    uint32_t cur_idx;
}tone_data_t;

#define NR_USECONDS_IN_SECONDS      (1000*1000)
#define NR_MSECONDS_IN_SECONDS      (1000)
#define TONE_PLAYER_THREAD_PRI             10
#define TONE_PLAYER_THREAD_NAME            ((const char*) "TONE_THREAD")
#define TONE_PLAYER_THREAD_STACK_SIZE      0x1000
#define TONE_PERIOD_SIZE                   (1*1024)
#define TONE_BUFFER_SIZE                   6*TONE_PERIOD_SIZE
#define TX_START_THRESHOLD                 (6*TONE_PERIOD_SIZE)
#define EXTRA_MILLIS                       (10)
#define TONE_DATA_LEN                      45

#define TONE_PLAYER_CLEANUP() \
                                do { \
                                    wiced_rtos_lock_mutex(p_session_init_mutex); \
                                    is_session_initialized = 0; \
                                    wiced_audio_stop(session); \
                                    wiced_audio_deinit(session); \
                                    wiced_rtos_deinit_semaphore(&tone_complete_semaphore); \
                                    if(args) \
                                        memset(args, 0, sizeof(tone_data_t)); \
                                    wiced_rtos_unlock_mutex(p_session_init_mutex); \
                                    destroy_session_init_mutex(); \
                                }while(0)


static const int16_t            tone_data[TONE_DATA_LEN] =
{   /* sin 980Hz */
    0xffff,0xe42,0x1c38,0x29a8,0x3643,0x41d1,0x4c1b,0x54e3,
    0x5c0b,0x6162,0x64d9,0x6655,0x65d9,0x635a,0x5ef2,0x58af,
    0x50af,0x4725,0x3c2e,0x3015,0x2304,0x154c,0x723,0xf8dd,
    0xeab5,0xdcf9,0xcfef,0xc3cf,0xb8dc,0xaf52,0xa74e,0xa112,
    0x9ca2,0x9a2b,0x99a7,0x9b2b,0x9e99,0xa3fa,0xab19,0xb3e9,
    0xbe2b,0xc9c0,0xd656,0xe3c9,0xf1be
};

wiced_thread_t                     tone_player_thread_id;
static wiced_audio_session_ref     session;
static wiced_semaphore_t           tone_complete_semaphore;
static uint8_t                     is_session_initialized;
static wiced_mutex_t*              p_session_init_mutex;
static tone_data_t *args = NULL;
uint8_t tone_player_thread_stack[TONE_PLAYER_THREAD_STACK_SIZE] __attribute__((section (".ccm")));


static void wiced_tone_player_task(uint32_t context);

static void destroy_session_init_mutex(void);


wiced_result_t wiced_tone_play(uint8_t* data, uint32_t length, wiced_audio_config_t* audio_info, uint8_t repeat)
{
    wiced_result_t ret;

    if(data && !audio_info)
        return WICED_BADARG;

    if(data && !length)
        return WICED_BADARG;

    if(!args)
        args = (tone_data_t*)malloc(sizeof(tone_data_t));
    if(!args)
        return WICED_OUT_OF_HEAP_SPACE;

    wiced_rtos_delete_thread(&tone_player_thread_id);
    args->data = data;
    args->length = length;
    if(audio_info)
        memcpy(&args->audio_info, audio_info, sizeof(wiced_audio_config_t));
    else
    {
        args->audio_info.bits_per_sample = 16;
        args->audio_info.channels = 2;
        args->audio_info.sample_rate = 44100;
    }
    args->repeat = repeat;

    ret = wiced_rtos_create_thread_with_stack(&tone_player_thread_id, TONE_PLAYER_THREAD_PRI, TONE_PLAYER_THREAD_NAME, wiced_tone_player_task,
                                                                    tone_player_thread_stack, TONE_PLAYER_THREAD_STACK_SIZE, (void*)args);
    WPRINT_APP_INFO(("[TONE] wiced_tone_play return = %d\n", ret));
    return ret;

}


wiced_result_t wiced_tone_stop( void )
{
    if(p_session_init_mutex && is_session_initialized)
    {
        wiced_rtos_lock_mutex(p_session_init_mutex);
        wiced_rtos_delete_thread(&tone_player_thread_id);
        wiced_rtos_unlock_mutex(p_session_init_mutex);
        TONE_PLAYER_CLEANUP();
    }

    WPRINT_APP_INFO(("[TONE] wiced_tone_stop return = %d\n", WICED_SUCCESS));
    return WICED_SUCCESS;
}


static wiced_result_t initialize_audio_device(const char *dev, wiced_audio_config_t *config, wiced_audio_session_ref *shout)
{
    wiced_result_t          result;
    wiced_audio_session_ref sh;

    result = WICED_SUCCESS;

    /* Initialize device. */
    if (result == WICED_SUCCESS)
    {
        result = wiced_audio_init(dev, &sh, TONE_PERIOD_SIZE);
        wiced_assert("wiced_audio_init", WICED_SUCCESS == result);
    }

    /* Allocate audio buffer. */
    if (result == WICED_SUCCESS)
    {
        result = wiced_audio_create_buffer(sh, TONE_BUFFER_SIZE, NULL, NULL);
        wiced_assert("wiced_audio_create_buffer", WICED_SUCCESS == result);
    }

    /* Configure session. */
    if (result == WICED_SUCCESS)
    {
        result = wiced_audio_configure(sh, config);
        wiced_assert("wiced_audio_configure", WICED_SUCCESS == result);
    }

    if (result == WICED_SUCCESS)
    {
        /* Copy-out. */
        *shout = sh;
    }
    else
    {
        /* Error handling. */
        if (wiced_audio_deinit(sh) != WICED_SUCCESS)
        {
            wiced_assert("wiced_audio_deinit", 0);
        }
    }
    return result;
}

static wiced_result_t create_session_init_mutex(void)
{
    wiced_result_t res = WICED_OUT_OF_HEAP_SPACE;
    p_session_init_mutex = calloc(1, sizeof(wiced_mutex_t));
    if(p_session_init_mutex != NULL)
    {
        res = wiced_rtos_init_mutex(p_session_init_mutex);
        if(res != WICED_SUCCESS)
        {
            free(p_session_init_mutex);
            p_session_init_mutex = NULL;
        }
    }
    WPRINT_APP_INFO(("[TONE] create_session_init_mutex ret = %d\n", res));
    return res;
}

static void destroy_session_init_mutex(void)
{
    if(p_session_init_mutex);
    {
        wiced_rtos_deinit_mutex(p_session_init_mutex);
        free(p_session_init_mutex);
        p_session_init_mutex = NULL;
    }
}


static wiced_result_t copy_data(tone_data_t *tone, uint8_t *buf, uint16_t nrbytes)
{
    int i;
    static int last_pos;

    if(tone->cur_idx == 0)
        last_pos = 0;

    for (i = 0; i < nrbytes/2; )
    {
        int16_t *buf16 = (int16_t *)buf;
        buf16[i++] = tone_data[last_pos];
        buf16[i++] = tone_data[last_pos];

        if (++last_pos >= sizeof(tone_data)/sizeof(tone_data[0]))
            last_pos = 0;
    }
    tone->cur_idx += nrbytes;
    return WICED_SUCCESS;
}

static wiced_result_t copy_from_mem(tone_data_t *tone, uint8_t *dst, uint32_t sz)
{
    wiced_result_t result;

    result = WICED_SUCCESS;

    /* Copy predefined data. */
    if (tone->data)
    {
        uint32_t cpy_len = (sz > tone->length)?tone->length:sz;
        memcpy(dst, &tone->data[tone->cur_idx], cpy_len);
        tone->cur_idx += cpy_len;

        if(cpy_len < sz)
            memset(&dst[cpy_len], 0, sz-cpy_len);
    }
    else
    {
        result = copy_data(tone, dst, sz);
        wiced_assert("copy_data", result == WICED_SUCCESS);
    }

    return result;
}

uint32_t get_tone_length(tone_data_t *tone)
{
    uint32_t len;
    wiced_audio_config_t *config = &tone->audio_info;

    if(tone->data)
    {
        len = tone->length;
    }
    else
    {
        if(!tone->length)
            len = 3*TX_START_THRESHOLD; //TX_START_THRESHOLD+TONE_PERIOD_SIZE; //128ms
        else
        {
            len = (((config->sample_rate * (config->bits_per_sample/8) * config->channels) * tone->length*32)/NR_MSECONDS_IN_SECONDS); //TODO:to change this
        if(len < TX_START_THRESHOLD)
            len += (TX_START_THRESHOLD-len+TX_START_THRESHOLD);
        }
    }
    return len;
}

static void wiced_tone_player_task(uint32_t context)
{
    uint8_t is_tx_started = 0, silence = 0;
    tone_data_t *args = (tone_data_t*)context;
    wiced_result_t result = WICED_SUCCESS;
    uint32_t weight;
    uint16_t remaining;
    double decibels = 0.0, min_volume_in_db = 0.0, max_volume_in_db = 0.0;

    if(!args)
        return;

    if( WICED_SUCCESS != create_session_init_mutex())
    {
        WPRINT_APP_INFO(("[TONE] mutex creation failed\n"));
        return;
    }

    wiced_rtos_lock_mutex(p_session_init_mutex);
    result = initialize_audio_device(WICED_AUDIO_1, &args->audio_info, &session);
    is_session_initialized = 1;
    wiced_rtos_unlock_mutex(p_session_init_mutex);

    if(result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("[TONE] init audio failed\n"));
        destroy_session_init_mutex();
        return;
    }

    wiced_rtos_init_semaphore(&tone_complete_semaphore);

    if ( WICED_SUCCESS == wiced_audio_get_volume_range( session, &min_volume_in_db, &max_volume_in_db ) )
    {
        decibels = (max_volume_in_db + min_volume_in_db)/2;
        wiced_audio_set_volume( session, decibels );
        //WPRINT_APP_INFO(( "[TONE] Volume changed[ Min:%.2f Max:%.2f now:%.2f ]\n",min_volume_in_db, max_volume_in_db, decibels ));
    }

    args->length = get_tone_length(args);
    WPRINT_APP_INFO(("[TONE] tone length = %u\n", (unsigned int)args->length));

    do
    {
        args->cur_idx = 0;
        while(args->cur_idx < args->length)
        {
            if (!is_tx_started)
            {
                /* Determine if we should start TX. */
                wiced_audio_get_current_buffer_weight(session, &weight);
                if (weight >= TX_START_THRESHOLD)
                {
                    result = wiced_audio_start(session);
                    if (result == WICED_SUCCESS)
                    {
                        is_tx_started = 1;
                    }
                }
            }

            if (result == WICED_SUCCESS)
            {
                /* Wait for slot in transmit buffer. */
                wiced_audio_config_t *config = &args->audio_info;
                uint32_t timeout = (((NR_USECONDS_IN_SECONDS/(config->sample_rate * config->channels)) * TONE_PERIOD_SIZE)/NR_MSECONDS_IN_SECONDS);
                result = wiced_audio_wait_buffer(session, TONE_PERIOD_SIZE, timeout);
                if (result == WICED_SUCCESS)
                {
                    /* Copy available data to transmit buffer. */
                    remaining = TONE_PERIOD_SIZE;
                    while (0 != remaining && result == WICED_SUCCESS)
                    {
                        uint8_t *buf;
                        uint16_t avail = remaining;

                        result = wiced_audio_get_buffer(session, &buf, &avail);
                        if (result == WICED_SUCCESS)
                        {
                            if(!silence)
                                copy_from_mem(args, buf, avail);
                            else
                            {
                                memset(buf, 0, avail);
                                args->cur_idx+=avail;
                            }
                            result = wiced_audio_release_buffer(session, avail);
                            remaining -= avail;
                        }
                    }
                }
            }
            if(result != WICED_SUCCESS)
            {
                args->repeat = 0;
                break;
            }
        }
        if(args->repeat)
        {
            if(!silence)
                silence = 1;
            else
            {
                args->repeat--;
                silence = 0;
            }
        }
    }while(args->repeat);

    if(result == WICED_SUCCESS)
    {
        wiced_audio_get_current_buffer_weight(session, &weight);
        if (weight > 0)
        {
            uint32_t timeout = (((NR_USECONDS_IN_SECONDS/(args->audio_info.sample_rate * args->audio_info.channels)) * weight)/NR_MSECONDS_IN_SECONDS)+EXTRA_MILLIS;
            wiced_rtos_get_semaphore(&tone_complete_semaphore,timeout);
        }
    }
    TONE_PLAYER_CLEANUP();
    WPRINT_APP_INFO(("[TONE] exit tone player loop\n"));
}

