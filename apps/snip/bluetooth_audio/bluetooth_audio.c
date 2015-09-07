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
 * Bluetooth Audio AVDT Sink Application
 *
 * The application demonstrates the following features:
 *  - Bluetooth intialization
 *  - A2DP sink
 *  - SBC Decoding
 *  - Audio playback
 *
 * Usage:
 *    On startup device will be discoverable and connectable,
 *    allowing a BT audio source to connect and stream audio.
 *
 * Notes: Currently supports 44.1kHz and 48kHz audio
 */

#include <stdlib.h>
#include "wiced.h"
#include "wiced_result.h"
#include "platform_audio.h"
#include "wiced_bt_common.h"
#include "wiced_bt_dm.h"
#include "wiced_bt_avk.h"

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
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    wiced_bt_avk_callbacks_t  audio_callbacks;
    wiced_bt_callbacks_t      device_manager_callbacks;
} bluetooth_app_callbacks_t;

/******************************************************
 *               Function Definitions
 ******************************************************/

static void device_state_handler              ( wiced_bt_state_t state, wiced_result_t status );
static void audio_connection_state_handler    ( wiced_bt_avk_connection_state_t state, wiced_bt_bdaddr_t *bd_addr );
static void audio_state_handler               ( wiced_bt_avk_audio_state_t state, wiced_bt_bdaddr_t *bd_addr );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static const bluetooth_app_callbacks_t app_callbacks =
{
    .audio_callbacks =
    {
        .connection_state_cb    = audio_connection_state_handler,
        .audio_state_cb         = audio_state_handler,
    },
    .device_manager_callbacks =
    {
        .state_changed_cb       = device_state_handler,
        .info_cb                = NULL,
        .acl_conn_state_cb      = NULL,
    },
};

static const wiced_bt_audio_pref_t audio_preferences =
{
    .dac_name_str = WICED_AUDIO_1,
    .adc_name_str = "Not applicable",
};

/******************************************************
 *               Function Definitions
 ******************************************************/


void application_start( )
{
    wiced_result_t result;

    wiced_init( );

    platform_init_audio( );

    WPRINT_APP_INFO ( ("Starting Bluetooth...\n") );

    /* Initialize BT stack and profiles */
    result = wiced_bt_init( &app_callbacks.device_manager_callbacks, &audio_preferences );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_ERROR( ( "Failed to initialize Bluetooth\n" ) );
    }
}

static void device_state_handler( wiced_bt_state_t state, wiced_result_t status )
{
    switch ( state )
    {
        case WICED_BT_STATE_ENABLED:
            WPRINT_APP_INFO( ("Bluetooth enabled\n") );
            wiced_bt_avk_init( &app_callbacks.audio_callbacks );
            break;

        case WICED_BT_STATE_DISABLED:
            WPRINT_APP_INFO( ("Bluetooth disabled\n") );
            break;

        default:
            break;
    }
}

static void audio_connection_state_handler( wiced_bt_avk_connection_state_t state, wiced_bt_bdaddr_t* device_address )
{
    switch ( state )
    {
        case WICED_BT_AVK_CONNECTION_STATE_CONNECTED:
            WPRINT_APP_INFO( ("Device connected [0x%x:0x%x:0x%x:0x%x:0x%x:0x%x]\n", device_address->address[0], device_address->address[1], device_address->address[2], device_address->address[3], device_address->address[4], device_address->address[5]) );
            break;

        case WICED_BT_AVK_CONNECTION_STATE_DISCONNECTED:
            WPRINT_APP_INFO( ("Device disconnected\n") );
            break;

        default:
            break;
    }
}

static void audio_state_handler( wiced_bt_avk_audio_state_t state, wiced_bt_bdaddr_t* device_address )
{
    switch ( state )
    {
        case WICED_BT_AVK_AUDIO_STATE_STARTED:        WPRINT_APP_INFO( ("Audio streaming started\n") );    break;
        case WICED_BT_AVK_AUDIO_STATE_REMOTE_SUSPEND: WPRINT_APP_INFO( ("Audio streaming suspended\n") );  break;
        case WICED_BT_AVK_AUDIO_STATE_STOPPED:        WPRINT_APP_INFO( ("Audio streaming stopped\n") );    break;
        default:  break;
    }
}
