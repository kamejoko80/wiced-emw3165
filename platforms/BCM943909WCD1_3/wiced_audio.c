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
 */

#include "wiced_rtos.h"
#include "platform_i2s.h"
#include "wiced_audio.h"
#include "wwd_assert.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "wm8533_dac.h"
#include "ak4954.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define wiced_required( X, LABEL ) \
        do                         \
        {                          \
            if( !( X ) )           \
            {                      \
                goto LABEL;        \
            }                      \
        }   while( 0 )

#define wiced_required_action( X, LABEL, ACTION ) \
        do                                        \
        {                                         \
            if( !( X ) )                          \
            {                                     \
                { ACTION; }                       \
                goto LABEL;                       \
            }                                     \
        }   while( 0 )

/******************************************************
 *                    Constants
 ******************************************************/
#define MAX_AUDIO_SESSIONS               ( 2 )
#define DEFAULT_AUDIO_DEVICE_PERIOD_SIZE ( 1024 )
#define MAX_AUDIO_BUFFER_SIZE            WICED_AUDIO_BUFFER_ARRAY_DIM_SIZEOF(12, DEFAULT_AUDIO_DEVICE_PERIOD_SIZE)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    /* Audio ring buffer. */
    wiced_audio_buffer_header_t *buffer;
    /* Start 0-index into the period. */
    uint16_t head;
    /* Bytes pending playback/capture transfer..
     * The inverse represents the bytes available to write (playback)
     * or read (captured).
    */
    uint16_t count;
    /* Total number of audio bytes in the buffer. */
    uint16_t length;
} audio_buffer_t;


/******************************************************
 *                    Structures
 ******************************************************/

struct wiced_audio_session_t
{
    int                             i2s_id, i2s_direction;
    wiced_bool_t                    i2s_running;
    wiced_semaphore_t               available_periods;
    uint16_t                        num_periods_requested;
    uint8_t                         frame_size;
    audio_buffer_t                  audio_buffer;
    uint16_t                        period_size;
    wiced_audio_device_interface_t* audio_dev;
    wiced_mutex_t                   session_lock;
    wiced_bool_t                    underrun_occurred;
    wiced_bool_t                    is_initialised;
};

/******************************************************
 *               Variables Definitions
 ******************************************************/
static audio_device_class_t audio_dev_class =
{
    /* By default there are no devices registered */
    .device_count = 0,
};


struct wiced_audio_session_t    sessions[MAX_AUDIO_SESSIONS];
extern const platform_i2s_t     i2s_interfaces[];
uint8_t                         wiced_audio_buffer[MAX_AUDIO_BUFFER_SIZE];

#define lock_session(sh)        wiced_rtos_lock_mutex(&(sh)->session_lock)
#define unlock_session(sh)      wiced_rtos_unlock_mutex(&(sh)->session_lock)

extern wm8533_device_data_t wm8533;
extern ak4954_device_data_t ak4954_dac;
extern ak4954_device_data_t ak4954_adc;

/******************************************************
 *               Function Declarations
 ******************************************************/
static wiced_audio_device_interface_t* search_audio_device( const char* device_name );

/******************************************************
 *               Function Definitions
 ******************************************************/


static wiced_audio_device_interface_t* search_audio_device( const char* device_name )
{
    int i = 0;
    /* Search device in the list of already registered */
    for ( i = 0; i < audio_dev_class.device_count; i++ )
    {
        if ( strncmp( device_name, audio_dev_class.audio_devices[i].name, strlen( device_name ) ) )
        {
            continue;
        }
        else
        {
            return &audio_dev_class.audio_devices[i];
        }
    }
    return 0;
}

static wiced_audio_session_ref wiced_audio_get_available_session(void)
{
    int i = 0;

    for( i = 0; i < MAX_AUDIO_SESSIONS ; i++ )
    {
        if( sessions[i].is_initialised == WICED_FALSE && sessions[i].audio_dev == NULL )
        {
            return &sessions[i];
        }
    }
    return NULL;
}

static wiced_result_t wiced_audio_check_device_is_available_internal( wiced_audio_device_interface_t* audio )
{
    int i = 0;

    /* Search the table of the sessions which are initialised and check which
     * audio device is attached to them. Audio device is considered as available
     * when is not attached to any of the audio sessions which passed through the initialisation
     * stage already */
    for( i = 0; i < MAX_AUDIO_SESSIONS ; i++ )
    {
        if( sessions[i].is_initialised == WICED_TRUE )
        {
            if( sessions[i].audio_dev == audio )
            {
                return WICED_FALSE;
            }
        }
    }
    return WICED_TRUE;
}

/* Bytes available to write (playback) or read (capture). */
static uint16_t wiced_audio_buffer_get_available_bytes(wiced_audio_session_ref sh)
{
    wiced_assert("data available exceeds buffer size", sh->audio_buffer.count <= sh->audio_buffer.length);
    return sh->audio_buffer.length - sh->audio_buffer.count;
}

wiced_result_t wiced_audio_init( const char* device_name, wiced_audio_session_ref* sh, uint16_t period_size )
{
    wiced_audio_device_interface_t* audio     = NULL;
    wiced_bool_t                    available = WICED_FALSE;
    wiced_result_t                  result    = WICED_SUCCESS;
    int                             attempts = 0;
    wiced_audio_session_ref         session  = NULL;

    /* Period size cannot exceed device limit. */
    wiced_required_action(period_size <= I2S_PERIOD_BYTES_MAX, exit2, result = WICED_BADARG);

    /* Check whether platform has registered at least one audio device */
    wiced_required_action(audio_dev_class.device_count != 0, exit2, result = WICED_ERROR );

    /* Check whether this audio device is not used by any of the current audio sessions */
    audio = search_audio_device( device_name );
    wiced_required_action(audio != NULL, exit2, result = WICED_ERROR );

    available = wiced_audio_check_device_is_available_internal(audio);
    wiced_required_action(available == WICED_TRUE, exit, result = WICED_ERROR );

    do
    {
        result = audio->audio_device_init( audio->audio_device_driver_specific );
    } while (result != WICED_SUCCESS && ++attempts < 50 && (wiced_rtos_delay_milliseconds(1) == WICED_SUCCESS) );

    wiced_required_action(result == WICED_SUCCESS, exit2, result = WICED_ERROR );

    wiced_assert("Cant initialise audio device", result == WICED_SUCCESS);

    /* Get an available session and prepare to use it for the audio device. */
    session = wiced_audio_get_available_session();
    wiced_required_action( session != NULL, exit, result = WICED_ERROR );

    result = wiced_rtos_init_mutex(&session->session_lock);
    wiced_required_action(result == WICED_SUCCESS, exit, result = result );
    result = wiced_rtos_init_semaphore(&session->available_periods);
    wiced_required_action(result == WICED_SUCCESS, exit, result = result );

    *sh = session;
    session->audio_buffer.buffer = NULL;
    session->audio_buffer.length = 0;
    session->audio_dev = audio;
    if( period_size != 0 )
    {
        session->period_size = period_size;
    }
    else
    {
        session->period_size = DEFAULT_AUDIO_DEVICE_PERIOD_SIZE;
    }
    session->underrun_occurred = WICED_FALSE;
    session->is_initialised    = WICED_TRUE;
exit:
    if(result)
    {
        result = audio->audio_device_deinit( audio->audio_device_driver_specific );
        wiced_rtos_deinit_mutex(&session->session_lock);
        wiced_rtos_deinit_semaphore(&session->available_periods);
    }
exit2:
    return result;
}

wiced_result_t wiced_audio_configure( wiced_audio_session_ref sh, wiced_audio_config_t* config )
{
    wiced_result_t result;

    UNUSED_PARAMETER( config );

    /* Set the audio device to the mode required  */
    if ( sh->audio_dev->audio_device_configure )
    {
        wiced_i2s_params_t params;
        uint32_t mclk = 0;
        wiced_i2s_t i2s_id = WICED_I2S_1;
        wiced_audio_device_channel_t i2s_direction = WICED_PLAY_CHANNEL;

        /* Init audio interface, for all codecs the audio interface will be initialised similarly */
        if (sh->audio_dev->audio_device_driver_specific == &wm8533)
        {
            i2s_id = ((wm8533_device_data_t*)sh->audio_dev->audio_device_driver_specific)->data_port;
            i2s_direction = WICED_PLAY_CHANNEL;
        }
        else if (sh->audio_dev->audio_device_driver_specific == &ak4954_dac)
        {
            i2s_id = ((ak4954_device_data_t*)sh->audio_dev->audio_device_driver_specific)->data_port;
            i2s_direction = WICED_PLAY_CHANNEL;
        }
        else if (sh->audio_dev->audio_device_driver_specific == &ak4954_adc)
        {
            i2s_id = ((ak4954_device_data_t*)sh->audio_dev->audio_device_driver_specific)->data_port;
            i2s_direction = WICED_RECORD_CHANNEL;
        }

        sh->i2s_id = i2s_id;
        sh->i2s_direction = i2s_direction;
        params.period_size =    sh->period_size;
        params.sample_rate=     config->sample_rate;
        params.bits_per_sample= config->bits_per_sample;
        params.channels =       config->channels;

        sh->frame_size = params.channels * ( config->bits_per_sample / 8 );
        wiced_assert("Bits per sample must be multiple of 8", ( config->bits_per_sample % 8 ) == 0 );

        result = wiced_i2s_init(sh, i2s_id, &params, &mclk);
        wiced_assert("Cant initialize audio interface\r\n", result == WICED_SUCCESS);
        wiced_required_action( result == WICED_SUCCESS, exit, result = result );

        /* Configure the audio codec device */
        result = sh->audio_dev->audio_device_configure( sh->audio_dev->audio_device_driver_specific, config, &mclk );
        wiced_assert("Cant initialize audio device\r\n", result == WICED_SUCCESS);

        if( sh->audio_buffer.buffer != NULL )
        {
            sh->audio_buffer.count = sh->i2s_direction == WICED_RECORD_CHANNEL ? sh->audio_buffer.length : 0;
            result = wiced_i2s_set_audio_buffer_details(i2s_id, sh->audio_buffer.buffer);
        }
        else
        {
            /* Audio buffer has not been created yet, audio interface cant work without it */
            wiced_i2s_deinit(i2s_id);
            result = WICED_ERROR;
        }
exit:
        return result;
    }
    else
    {
        return WICED_ERROR;
    }
}

/* DMA'able contiguous audio buffer.
 * "size" is the unaligned size in bytes, that is with padding added.
 */
static uint16_t wiced_audio_initialize_audio_buffer(uint16_t period_size, uint16_t size, uint8_t *buffer_start_ptr_aligned)
{
    uint16_t    count;
    uint8_t     *buf_ptr;
    uint8_t     *buf_ptr_next;
    uint8_t     *start_of_array;
    uint8_t     *end_of_array;

#if !defined(I2S_DMA_OFS_BYTES)
#define I2S_DMA_OFS_BYTES   0
#endif /* !defined(I2S_DMA_OFS_BYTES) */

    start_of_array  = buffer_start_ptr_aligned;

    buf_ptr         = start_of_array;
    buf_ptr_next    = buf_ptr + WICED_AUDIO_BUFFER_ARRAY_ELEMENT_SIZEOF(period_size);

    /* End-of-array is at the aligned pointer + the size without the alignment padding. */
    end_of_array    = buf_ptr + WICED_AUDIO_BUFFER_ROUND_DOWN(size);

    memset(start_of_array, 0, WICED_AUDIO_BUFFER_ROUND_DOWN(size));

    count = 0;

    while (buf_ptr_next <= end_of_array)
    {
        ((wiced_audio_buffer_header_t *)buf_ptr)->data_start    = buf_ptr + WICED_AUDIO_BUFFER_HEADER_SIZEOF + I2S_DMA_OFS_BYTES;
        ((wiced_audio_buffer_header_t *)buf_ptr)->data_end      = buf_ptr + WICED_AUDIO_BUFFER_HEADER_SIZEOF + I2S_DMA_OFS_BYTES + period_size;
        ((wiced_audio_buffer_header_t *)buf_ptr)->next          = (wiced_audio_buffer_header_t *)buf_ptr_next;

        buf_ptr         = buf_ptr_next;
        buf_ptr_next    = buf_ptr + WICED_AUDIO_BUFFER_ARRAY_ELEMENT_SIZEOF(period_size);

        count++;
    }

    if (count != 0)
    {
        buf_ptr = buf_ptr - WICED_AUDIO_BUFFER_ARRAY_ELEMENT_SIZEOF(period_size);
        ((wiced_audio_buffer_header_t *)buf_ptr)->next = (wiced_audio_buffer_header_t *)start_of_array;
    }

    return count;
}

/* If buffer_ptr_aligned is supplied, size must be allocated with WICED_AUDIO_BUFFER_ARRAY_BYTES; otherwise
 * size represents the period_size multiplied by some integer.
 */
wiced_result_t wiced_audio_create_buffer( wiced_audio_session_ref sh, uint16_t size, uint8_t* buffer_ptr_aligned, void*(*allocator)(uint16_t size))
{
    wiced_result_t result;

    wiced_required_action((sh->i2s_running == WICED_FALSE), exit, result = WICED_ERROR );

    /* Size of the audio buffer must be divided by the period size without the remainder */
    //wiced_required_action((size % sh->period_size) == 0, exit, result = WICED_ERROR);
    wiced_required_action(size != 0, exit, result = WICED_ERROR);
    wiced_required_action(allocator == NULL, exit, result = WICED_UNSUPPORTED);

    if( buffer_ptr_aligned != NULL )
    {
        wiced_required_action(WICED_AUDIO_BUFFER_ROUND_DOWN(size) % WICED_AUDIO_BUFFER_ARRAY_ELEMENT_SIZEOF(sh->period_size) == 0, exit, result = WICED_ERROR);
        sh->audio_buffer.buffer = (wiced_audio_buffer_header_t *)buffer_ptr_aligned;
        sh->audio_buffer.length = (WICED_AUDIO_BUFFER_ROUND_DOWN(size) / WICED_AUDIO_BUFFER_ARRAY_ELEMENT_SIZEOF(sh->period_size)) * sh->period_size;
    }
    else
    {
        /* If allocator is null and buffer ptr is null, then the buffer will be allocated in and owned by the wiced audio */
        WPRINT_LIB_INFO( ( "using shared buffer for player\n" ) );
        wiced_required_action(size % sh->period_size == 0, exit, result = WICED_ERROR);
        sh->audio_buffer.buffer = WICED_AUDIO_BUFFER_ARRAY_PTR(wiced_audio_buffer);
        sh->audio_buffer.length = size;
        size = WICED_AUDIO_BUFFER_ARRAY_DIM_SIZEOF(size / sh->period_size, sh->period_size);
        wiced_required_action(size <= sizeof(wiced_audio_buffer), exit, result = WICED_ERROR);
    }

    wiced_audio_initialize_audio_buffer(sh->period_size, size, (uint8_t *)sh->audio_buffer.buffer);

    result = WICED_SUCCESS;

    exit:
    sh->audio_buffer.head = 0;
    sh->audio_buffer.count = 0;

    return result;
}


wiced_result_t wiced_audio_set_volume( wiced_audio_session_ref sh, double volume_in_db )
{
    wiced_result_t result;

    UNUSED_PARAMETER( volume_in_db );

    result = sh->audio_dev->audio_device_set_volume( sh->audio_dev->audio_device_driver_specific, volume_in_db );
    if ( result != WICED_SUCCESS )
    {
        /* Do some error handling... */
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_audio_get_volume_range( wiced_audio_session_ref sh, double *min_volume_in_db, double *max_volume_in_db)
{
    wiced_result_t result;

    if ( sh->audio_dev->audio_device_get_volume_range )
    {
        result = sh->audio_dev->audio_device_get_volume_range( sh->audio_dev->audio_device_driver_specific, min_volume_in_db, max_volume_in_db );
        if ( result != WICED_SUCCESS )
        {
            /* Do some error handling... */
            return WICED_ERROR;
        }
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_audio_deinit( wiced_audio_session_ref sh )
{
    wiced_result_t result;

    /* Stop audio device. Initiate power-down sequence on the audio device */
    result = sh->audio_dev->audio_device_deinit( sh->audio_dev->audio_device_driver_specific );
    result = wiced_i2s_deinit (sh->i2s_id);

    wiced_rtos_deinit_mutex(&sh->session_lock);
    wiced_rtos_deinit_semaphore(&sh->available_periods);

    sh->audio_buffer.buffer = NULL;
    sh->audio_buffer.length = 0;
    sh->audio_dev           = NULL;
    sh->is_initialised      = WICED_FALSE;
    if ( result != WICED_SUCCESS )
    {
        /* Do some error handling... */
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_register_audio_device( const char* name, wiced_audio_device_interface_t* interface )
{
    static wiced_bool_t first_device = WICED_FALSE;

    UNUSED_PARAMETER(name);

    /* Add interface to the first empty slot */
    if ( first_device == WICED_FALSE )
    {
        int i = 0;

        for ( i = 0; i < MAX_NUM_AUDIO_DEVICES; i++ )
        {
            memset( &audio_dev_class.audio_devices[i], 0x00, sizeof(wiced_audio_device_interface_t) );
        }

        audio_dev_class.device_count = 0;
        first_device = WICED_TRUE;
    }

    memcpy( &audio_dev_class.audio_devices[audio_dev_class.device_count++], interface, sizeof( *interface ) );

    return WICED_SUCCESS;
}

wiced_result_t wiced_audio_get_current_hw_pointer      ( wiced_audio_session_ref sh, uint32_t* hw_pointer)
{
    wiced_result_t result;
    UNUSED_PARAMETER(result);
    if( ( hw_pointer == NULL ) || ( sh == NULL ) )
    {
        return WICED_BADARG;
    }

    result = wiced_i2s_get_current_hw_pointer (sh->i2s_id, hw_pointer);

    return result;
}

/* Get a portion of the audio buffer that is available for writing (playback) or reading (capture). */
wiced_result_t wiced_audio_get_buffer( wiced_audio_session_ref sh, uint8_t** ptr, uint16_t* size)
{
    wiced_result_t result;
    uint16_t count_available;

    *ptr = NULL;

    /* If Underrun situation occurs during playback - let the application know about it,
     *  hence application will be able to do a recovery when it happens */
    WICED_DISABLE_INTERRUPTS();

    count_available = wiced_audio_buffer_get_available_bytes(sh);
    wiced_required_action( sh->underrun_occurred == WICED_FALSE, exit, result = WICED_ERROR);
    wiced_required_action( count_available > 0, exit, result = WICED_WWD_BUFFER_UNAVAILABLE_TEMPORARY);

    wiced_assert("size must be a non-zero value", (*size > 0));
    wiced_required_action( *size != 0, exit, result = WICED_BADARG );

    if ( (sh->audio_buffer.head + *size) > sh->period_size)
    {
        wiced_assert("head didn't wrap!", sh->audio_buffer.head < sh->period_size);

        /* Give everything which is below the top edge of the period buffer */
        *ptr = &sh->audio_buffer.buffer->data_start[sh->audio_buffer.head];

        /* Cap the requested size */
        *size = (uint16_t)(sh->period_size - sh->audio_buffer.head);
    }
    else if( (sh->audio_buffer.head + *size) == sh->period_size )
    {
        *ptr = &sh->audio_buffer.buffer->data_start[sh->audio_buffer.head];
        *size = (uint16_t)(sh->period_size - sh->audio_buffer.head);
    }
    else /* (sh->audio_buffer.head + *size) < sh->audio_buffer.length */
    {
        /* Do not update size since audio buffer has enough free space left */
        *ptr = &sh->audio_buffer.buffer->data_start[sh->audio_buffer.head];
    }

    /* Cap to what's really available. */
    if (*size > count_available)
        *size = count_available;

    result = WICED_SUCCESS;
exit:
    WICED_ENABLE_INTERRUPTS();
    return result;
}

wiced_result_t wiced_audio_get_latency ( wiced_audio_session_ref sh, uint32_t* latency )
{
    if( sh->audio_buffer.buffer != NULL && latency != NULL )
    {
        *latency = sh->audio_buffer.length / sh->frame_size;
        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}


wiced_result_t wiced_audio_stop( wiced_audio_session_ref sh )
{
    wiced_result_t result = WICED_SUCCESS;
    if( sh->i2s_running == WICED_TRUE )
    {
        result = wiced_i2s_stop(sh->i2s_id);
        wiced_required( result == WICED_SUCCESS, exit );
        result = wiced_i2s_set_audio_buffer_details(sh->i2s_id, sh->audio_buffer.buffer);
        wiced_required( result == WICED_SUCCESS, exit );
        sh->i2s_running = WICED_FALSE;
    }
    sh->audio_buffer.head = 0;
    sh->audio_buffer.count = sh->i2s_direction == WICED_RECORD_CHANNEL ? sh->audio_buffer.length : 0;
    sh->underrun_occurred = WICED_FALSE;
    sh->num_periods_requested = 0;

    result = wiced_rtos_deinit_semaphore(&sh->available_periods);
    result = wiced_rtos_init_semaphore(&sh->available_periods);
exit:
    return result;
}


wiced_result_t wiced_audio_start( wiced_audio_session_ref sh )
{
    wiced_result_t result;
    if( sh->i2s_running == WICED_FALSE )
    {
        if( sh->audio_buffer.count/sh->period_size >=1 )
        {
            if( sh->audio_dev->audio_device_start_streaming != NULL )
            {
                result = sh->audio_dev->audio_device_start_streaming( sh->audio_dev->audio_device_driver_specific );
            }
            result = wiced_i2s_start(sh->i2s_id);
            if( result == WICED_SUCCESS )
            {
                sh->i2s_running = WICED_TRUE;
                return WICED_SUCCESS;
            }
            else
            {
                return result;
            }
        }
        else
        {
            return WICED_ERROR;
        }
    }
    else
    {
        return WICED_ERROR;
    }
}

wiced_result_t wiced_audio_release_buffer(wiced_audio_session_ref sh, uint16_t size)
{
    wiced_result_t result;
    uint16_t available;
    uint16_t count;

    /* Make sure that an application knows about "underrun" situation ,
     * so that it will be able to do a recovery when it happens */
    WICED_DISABLE_INTERRUPTS();
    wiced_required_action( sh->underrun_occurred == WICED_FALSE, exit, result = WICED_ERROR );
    available = wiced_audio_buffer_get_available_bytes(sh);

    wiced_required_action( available >= size, exit, result = WICED_WWD_BUFFER_UNAVAILABLE_TEMPORARY );
    wiced_required_action( ( (sh->audio_buffer.count + size) <= sh->audio_buffer.length ), exit, result = WICED_WWD_BUFFER_UNAVAILABLE_TEMPORARY) ;

    count = (sh->audio_buffer.head + size) / sh->period_size;

    /* Move ahead in the period ring buffer. */
    while (count > 0)
    {
        sh->audio_buffer.buffer = sh->audio_buffer.buffer->next;
        count--;
    }

    sh->audio_buffer.head = (uint16_t)(( sh->audio_buffer.head + size ) % ( sh->period_size ));
    sh->audio_buffer.count = (uint16_t)( sh->audio_buffer.count + size);

    WICED_ENABLE_INTERRUPTS();
    result = WICED_SUCCESS;
exit:
    if( result != WICED_SUCCESS )
    {
        WICED_ENABLE_INTERRUPTS();
    }

    return result;
}


wiced_result_t wiced_audio_buffer_platform_event(wiced_audio_session_ref sh, wiced_audio_platform_event_t event)
{
    switch(event)
    {
        case WICED_AUDIO_PERIOD_ELAPSED:
            sh->audio_buffer.count = (uint16_t)(sh->audio_buffer.count- sh->period_size);
            if( sh->num_periods_requested > 0 )
            {
                sh->num_periods_requested--;
                if( sh->num_periods_requested == 0 )
                {
                    wiced_rtos_set_semaphore(&sh->available_periods);
                }
            }
        break;

        case WICED_AUDIO_UNDERRUN:
            sh->underrun_occurred = WICED_TRUE;
            /* No use having the the requester wait to find out about the xrun, so
             * go ahead and signal.
             */
            if ( sh->num_periods_requested != 0)
            {
                wiced_rtos_set_semaphore(&sh->available_periods);
            }
        break;

        default:
            break;
    }
    return WICED_SUCCESS;
}



uint16_t wiced_audio_buffer_platform_get_periods(wiced_audio_session_ref sh)
{
    if( sh->audio_buffer.count >= sh->period_size )
    {
        return sh->audio_buffer.count / sh->period_size;
    }
    else
    {
        return 0;
    }
}


wiced_result_t wiced_audio_get_current_buffer_weight( wiced_audio_session_ref sh, uint32_t* weight )
{
    WICED_DISABLE_INTERRUPTS();

    /* Return how many bytes are in the buffer already */
    *weight = sh->audio_buffer.count;
    WICED_ENABLE_INTERRUPTS();


    return WICED_SUCCESS;
}

extern uint64_t audio_host_time;
/* wait till platform moves in the buffer to the position where the requested size gets available */
wiced_result_t wiced_audio_wait_buffer( wiced_audio_session_ref sh, uint16_t size, uint32_t timeout )
{
    wiced_result_t result;
    uint16_t available;


    /* Can't request bigger than the buffer. */
    wiced_required_action( size <= sh->audio_buffer.length, exit, result = WICED_ERROR);

    WICED_DISABLE_INTERRUPTS();

    /* Make sure that application knows about "underrun" condition,
     * so that it will be able to do a recovery when it happens */
    wiced_required_action( sh->underrun_occurred == WICED_FALSE, exit, result = WICED_ERROR);

    available = wiced_audio_buffer_get_available_bytes(sh);

    /* Don't wait if we have enough bytes in the audio buffer already */
    wiced_required_action( available < size, exit, result = WICED_SUCCESS );
    sh->num_periods_requested = size / sh->period_size - available / sh->period_size;
    if( (size % sh->period_size) > 0 )
    {
        sh->num_periods_requested++;
    }

    WICED_ENABLE_INTERRUPTS();
    result = wiced_rtos_get_semaphore( &sh->available_periods, timeout);
    if( result == WICED_SUCCESS )
    {
        if( sh->underrun_occurred == WICED_TRUE )
        {
            result = WICED_ERROR;
        }
    }
    if( result == WICED_SUCCESS )
    {
        wiced_assert("requested periods number must be decremented by the audio interface down to 0", (sh->num_periods_requested == 0));
        if( sh->num_periods_requested != 0 )
        {
            WICED_TRIGGER_BREAKPOINT();
        }
    }

    return result;

exit:
    WICED_ENABLE_INTERRUPTS();
    //unlock_session(sh);
    return result;
}

