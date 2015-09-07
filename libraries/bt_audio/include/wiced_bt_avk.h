/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef WICED_BT_AVK_H
#define WICED_BT_AVK_H

#ifdef __cplusplus
extern "C" {
#endif

/**  \file    wiced_bt_avk.h
        \brief  Header file for wiced_bt AVK functions accessed from application.
*/

/** \defgroup btavk WICED BT AVK
*
* @{
*/

/** \brief MIN AVK volume level (MUTE) */
#define WICED_BT_AVK_VOLUME_LEVEL_MIN   0

/** \brief MAX AVK volume level */
#define WICED_BT_AVK_VOLUME_LEVEL_MAX   127



/** \brief AV Remote control commands */
typedef enum
{
   WICED_BT_AVK_RCC_PLAY,
   WICED_BT_AVK_RCC_PAUSE,
   WICED_BT_AVK_RCC_STOP,
   WICED_BT_AVK_RCC_SKIP_FORWARD,
   WICED_BT_AVK_RCC_SKIP_BACKWARD
} wiced_bt_avk_rcc_command_t;

/** \brief AVK Connection states*/
typedef enum {
    WICED_BT_AVK_CONNECTION_STATE_DISCONNECTED = 0,
    WICED_BT_AVK_CONNECTION_STATE_CONNECTING,
    WICED_BT_AVK_CONNECTION_STATE_CONNECTED,
    WICED_BT_AVK_CONNECTION_STATE_DISCONNECTING
} wiced_bt_avk_connection_state_t;

/** \brief Bluetooth AV datapath states */
typedef enum {
    WICED_BT_AVK_AUDIO_STATE_REMOTE_SUSPEND = 0,
    WICED_BT_AVK_AUDIO_STATE_STOPPED,
    WICED_BT_AVK_AUDIO_STATE_STARTED,
} wiced_bt_avk_audio_state_t;


/** \brief AVK connection status notification callback.
*
*   The library issues this call back to notify change in BT AVK connection state.
*
*   \param state  AVK connection state
*   \param bd_addr  remote device address
**/
typedef void ( *wiced_bt_avk_connection_state_callback )( wiced_bt_avk_connection_state_t state,
                                                                             wiced_bt_bdaddr_t *bd_addr );

/** \brief AVK audio path status notification callback.
*
*   The library issues this call back to notify change in BT AVK audio connection state.
*
*   \param state  AVK audio connection state
*   \param bd_addr  remote device address
**/
typedef void ( *wiced_bt_avk_audio_state_callback )( wiced_bt_avk_audio_state_t state,
                                                                  wiced_bt_bdaddr_t *bd_addr );


/**    \brief     struct used to pass AVK callbacks to library.
*
*    Application defines handler functions to handle BT AVK events
*    following the prototype defined  here and register them with BT
*    library thru wiced_bt_avk_init() API.
**/
typedef struct
{
    wiced_bt_avk_connection_state_callback  connection_state_cb; //!<Callback function pointer
    wiced_bt_avk_audio_state_callback audio_state_cb; //!<Callback function pointer
} wiced_bt_avk_callbacks_t;


/** \brief Initialize bluetooth AV Sink module.
*
*    This function initializes the Bluetooth A2DP/AVRCP profile related modules.
*
*   \param callbacks    structure containing pointer to functions that will be called from
*                                WICED bt library to convey bt AVK events to applicaiton.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_avk_init( const wiced_bt_avk_callbacks_t* callbacks );


/** \brief Send Remote Control Command to MP.
*
*    This function is called by the applicaiton when it wants to send a pass thru remote control command
*    to the MP.
*
*   \param command    command to be sent, of type wiced_bt_avk_rcc_command_t.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_avk_send_rcc_cmd( wiced_bt_avk_rcc_command_t command );


/** \brief Set the playback volume.
*
*    This function is called by the applicaiton when it wants to modify the A2DP playback volume,
*    It also updates the absolute volume of AVK.
*
*   \param volume        Integer value indicating the volume level to be set.
*                              Can be any value between (and including)
*                              WICED_BT_AVK_VOLUME_LEVEL_MIN and
*                              WICED_BT_AVK_VOLUME_LEVEL_MAX.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_avk_update_volume( unsigned char volume );


/** \brief Retrieve playback volume.
*
*    This function is called by the applicaiton when it wants get the current playback volume level.
*
*   \param *volume         A non NULL pointer to UINT8 variable. The current volume level is returned
*                                 by the library. The level is an integer between 0 and maximum (inclusive)
*                                 possible volume level as returned by wiced_bt_avk_get_max_volume().
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_avk_get_current_volume( unsigned char *volume );


/** \brief Stop and prevent BT A2DP audio playback.
*
*   This function is called by the applicaiton when it wants A2DP playback to be stopped (if active)
*   and to prevent playback till wiced_bt_avk_allow_playback() is called.
*   This API can be called by the application when it wants to play audio from a higher priority
*   audio source like Airplay
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_avk_prevent_playback( void );


/** \brief Allow BT A2DP playback on the device.
*
*   This function allows the A2DP audio playback that was preveted by calling
*   wiced_bt_avk_prevent_playback(). It does not however start playback,
*   but allows any further requests from MP or the application to resume playback.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_avk_allow_playback( void );


/** \brief Shutdown bluetooth AV Sink module.
*
*    This function cleans up the resources held by AV SINK module.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_avk_cleanup( void );


/** @} */ // end of btavk

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //#ifndef WICED_BT_AVK_H
