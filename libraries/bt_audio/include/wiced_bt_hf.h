/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef WICED_BT_HF_H
#define WICED_BT_HF_H

#ifdef __cplusplus
extern "C" {
#endif

/**  \file    wiced_bt_hf.h
        \brief  Header file for wiced_bt Hands Free functions accessed from application.
*/

/** \defgroup bthf WICED BT HF
*
* @{
*/

/** \brief MIN AVK volume level (MUTE) */
#define WICED_BT_HF_VOLUME_LEVEL_MIN   0

/** \brief MAX AVK volume level */
#define WICED_BT_HF_VOLUME_LEVEL_MAX   15


/** \brief   Bluetooth HF device connection states */
typedef enum {
    WICED_BT_HF_CONN_STATE_DISCONNECTED = 0,
    WICED_BT_HF_CONN_STATE_CONNECTING,
    WICED_BT_HF_CONN_STATE_CONNECTED,
    WICED_BT_HF_CONN_STATE_SLC_CONNECTED,
    WICED_BT_HF_CONN_STATE_DISCONNECTING
} wiced_bt_hf_conn_state_t;

/** \brief Bluetooth Hf device datapath states */
typedef enum {
    WICED_BT_HF_AUDIO_STATE_DISCONNECTED = 0,
    WICED_BT_HF_AUDIO_STATE_CONNECTION_WAITING,
    WICED_BT_HF_AUDIO_STATE_CONNECTING,
    WICED_BT_HF_AUDIO_STATE_CONNECTED
} wiced_bt_hf_audio_state_t;

/** \brief AG's Service Availability indicator */
typedef enum
{
    WICED_BT_HF_SERVICE_NOT_AVAILABLE = 0,
    WICED_BT_HF_SERVICE_AVAILABLE
} wiced_bt_hf_service_t;

/** \brief AG's Roaming status indicator */
typedef enum
{
    WICED_BT_HF_SERVICE_TYPE_HOME = 0,
    WICED_BT_HF_SERVICE_TYPE_ROAMING
} wiced_bt_hf_service_type_t;

/** \brief States of a call during setup procedure*/
typedef enum {
    WICED_BT_HF_CALL_SETUP_STATE_IDLE = 0,
    WICED_BT_HF_CALL_SETUP_STATE_HELD,
    WICED_BT_HF_CALL_SETUP_STATE_DIALING,
    WICED_BT_HF_CALL_SETUP_STATE_ALERTING,
    WICED_BT_HF_CALL_SETUP_STATE_INCOMING,
    WICED_BT_HF_CALL_SETUP_STATE_WAITING
} wiced_bt_hf_call_setup_state_t;

/** \brief In-band ring tone setting in AG*/
typedef enum {
    WICED_BT_HF_INBAND_RING_STATE_OFF = 0,
    WICED_BT_HF_INBAND_RING_STATE_ON
} wiced_bt_hf_inband_ring_status_t;


/** \brief HF connection status notification callback.
*
*   The library issues this call back to notify change in BT HF connection state.
*
*   \param state  HF connection state
*   \param bd_addr  remote device address
*   \param peer_features  AG supported features
*   \param local_features  local (HF Device) supported features
*
**/
typedef void (* wiced_bt_hf_connection_state_callback)(wiced_bt_hf_conn_state_t state,
                                                                wiced_bt_bdaddr_t *bd_addr,
                                                                int peer_features,
                                                                int local_features);



/** \brief HF SCO connection satutus notification callback.
*
*   The library issues this call back to notify change in SCO connection state.
*   when the parameter state is set to WICED_BT_HF_AUDIO_STATE_CONNECTION_WAITING
*   the BT library expects the Application to call wiced_bt_hf_audio_connection_response() API
*   to confirm or reject the connection request.
*
*   \param state  SCO connection state
*   \param bd_addr  remote device address
*
**/
typedef void (* wiced_bt_hf_audio_state_callback)(wiced_bt_hf_audio_state_t state,
                                                              wiced_bt_bdaddr_t *bd_addr);


/** \brief AG's device status notification callback.
*
*   The library issues this call back to notify change in AG's indicator status.
*
*   \param svc  Service availability indicator
*   \param svc_type  Roaming indicator
*   \param signal  Signal strength in the range 0 - 5
*   \param batt_chg  battery charge indicator of AG in the range 0 - 5
*
**/
typedef void (*wiced_bt_hf_device_status_ind_callback)(wiced_bt_hf_service_t svc,
                                                                wiced_bt_hf_service_type_t svc_type,
                                                                int signal, int batt_chg);


/** \brief Call status notification callback.
*
*   The library issues this call back to notify change in BT HF connection state.
*
*   \param num_active   1 if a call is active, else 0
*   \param num_held  number of held calls
*   \param call_setup  indicates if a call setup is in progress
*
**/
typedef void (*wiced_bt_hf_call_status_ind_callback)(int num_active, int num_held,
                                                            wiced_bt_hf_call_setup_state_t call_setup);


/** \brief RING indication callback.
*
*   The library issues this call back to notify RING indication from AG when there is an incomig call.
*
**/
typedef void (*wiced_bt_hf_ring_callback)(void);


/** \brief Caller ID indication callback.
*
*   The library issues this call back to notify CLIP indication from AG when there is an incoming call.
*
*   \param clip_str  NULL terminated string containing caller ID of the caller.
*
**/
typedef void (*wiced_bt_hf_clip_callback)(char* clip_str);

/** \brief Peer's inband ring tone status indication callback.
*
*   The library issues this call back to notify inband ring tone enable/disable on AG.
*
*   \param status  indicates if inband ring tone is enabled or not, on AG.
*
**/
typedef void (* wiced_bt_hf_inband_ring_status_callback)(wiced_bt_hf_inband_ring_status_t status);


/**    \brief     struct used to pass HF callbacks to library.
*
*    Application defines handler functions to handle BT HF events
*    following the prototype defined  here and register them with BT
*    library thru wiced_bt_hf_init() API.
**/
typedef struct
{
    wiced_bt_hf_connection_state_callback  connection_state_cb; //!<Callback function pointer
    wiced_bt_hf_audio_state_callback audio_state_cb; //!<Callback function pointer
    wiced_bt_hf_device_status_ind_callback device_status_ind_cb; //!<Callback function pointer
    wiced_bt_hf_call_status_ind_callback call_status_ind_cb; //!<Callback function pointer
    wiced_bt_hf_ring_callback ring_cb; //!<Callback function pointer
    wiced_bt_hf_clip_callback clip_cb; //!<CLIP Callback function pointer
    wiced_bt_hf_inband_ring_status_callback inband_ring_status_cb; //!<Inband ring status Callback function pointer
}wiced_bt_hf_callbacks_t;


/** \brief Initialize bluetooth HF module.
*
*    This function initializes the Bluetooth HF profile related modules.
*
*   \param callbacks    structure containing pointer to functions that will be called from
*                                WICED bt library to convey HF events to applicaiton.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_hf_init(wiced_bt_hf_callbacks_t* callbacks );


/** \brief Initiate an outgoing call.
*
*    This function initiates an outgoing call thru the connected AG.
*
*   \param num    NUL terminated string or NULL ptr.
*                          place an outgoing call on given number in num.
*                          if NULL, initiates re-dial of last dialled number.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_hf_dial(unsigned char *num);


/** \brief Answer an incoming call.
*
*    This function answers an incoming call.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_hf_answer(void);


/** \brief Hangup a call.
*
*    This function Ends an ongoing call, or rejects an incoming call,
*    or aborts an outgoing call being attempted.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_hf_hangup(void);


/** \brief Accept or reject an audio connection establishment request from peer device.
*
*    This function accepts or rejects an audio connection establishment request ,
*    from a peer device.
*
*   \param  accept  if TRUE the connection is accepted, if FALSE it is rejected
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_hf_audio_connection_response(wiced_bool_t accept);


/** \brief Set the voice playback volume.
*
*    This function is called by the applicaiton when it wants to modify the volume on HF.
*
*   \param vol_level  Integer value indicating the volume level to be set.
*                              Can be any value between (and including)
*                              WICED_BT_HF_VOLUME_LEVEL_MIN and
*                              WICED_BT_HF_VOLUME_LEVEL_MAX.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_hf_update_volume(unsigned char vol_level);


/** \brief Retrieve voice playback volume.
*
*    This function is called by the applicaiton when it wants get the current playback volume level.
*
*   \param *vol_level   A non NULL pointer to UINT8 variable. The current volume level is returned
*                                 by the library. The level is an integer between 0 and maximum (inclusive)
*                                 possible volume level as returned by wiced_bt_hf_get_max_volume().
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_hf_get_current_volume(unsigned char *vol_level);


/** \brief Shutdown bluetooth HF module.
*
*    This function cleans up the resources held by HF module.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_hf_cleanup(void);

/** @} */ // end of bthf

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //#ifndef WICED_BT_HF_H
