/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef WICED_BT_DM_H
#define WICED_BT_DM_H

#ifdef __cplusplus
extern "C" {
#endif

/**  \file    wiced_bt.h
        \brief  API header file for wiced_bt DM functions accessed from application.
*/

/** \defgroup btdm WICED BT DM
*
* @{
*/

/** \brief Bluetooth Device Visibility Modes*/
typedef enum {
    WICED_BT_SCAN_MODE_NONE,
    WICED_BT_SCAN_MODE_CONNECTABLE,
    WICED_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE
}wiced_bt_scan_mode_t;

/**\brief Bluetooth Device State */
typedef enum {
    WICED_BT_STATE_ENABLED,
    WICED_BT_STATE_DISABLED
}wiced_bt_state_t;

/**\brief Bluetooth Device connection State */
typedef enum {
    WICED_BT_STATE_CONNECTED,
    WICED_BT_STATE_DISCONNECTED
}wiced_bt_conn_state_t;

/** \brief Bluetooth Error Status */
typedef enum {
    WICED_BT_STATUS_SUCCESS,
    WICED_BT_STATUS_UNKNOWN_ERROR,
    WICED_BT_STATUS_CLOSED,
    WICED_BT_STATUS_LINK_LOSS,

    WICED_BT_STATUS_NOT_READY,
    WICED_BT_STATUS_NOMEM,
    WICED_BT_STATUS_BUSY,
    WICED_BT_STATUS_UNSUPPORTED,
    WICED_BT_STATUS_PARAM_INVALID,
    WICED_BT_STATUS_UNHANDLED,
    WICED_BT_STATUS_AUTH_FAILURE,
    WICED_BT_STATUS_RMT_DEV_DOWN,
    WICED_BT_STATUS_AUTH_REJECTED,
    WICED_BT_STATUS_AUTH_TIMEOUT
} wiced_bt_status_t;

/** \brief Bluetooth Device info data structure */
typedef struct
{
   wiced_bt_bdaddr_t bd_addr; //!<Local bluetooth device address
   wiced_bt_bdname_t bd_name; //!<Local bluetooth device name
   wiced_bt_scan_mode_t scan_mode; //!<device scan mode
} wiced_bt_info_t;

/** \brief Bluetooth Connection status event data */
typedef struct
{
    wiced_bt_bdaddr_t bd_addr; //!<Bluetooth address of the remote connected/disconnected phone
    wiced_bt_conn_state_t state; //!<Status of the connection/disconnection
    wiced_bt_status_t status; //!<reason for disconnection
} wiced_bt_connection_event_data_t;


/** \brief Bluetooth Enable/Disable callback.
*
*   The call back notifies the current state of BT device.
*
*   \param state Current BT device state
*
**/
typedef void ( *wiced_bt_state_changed_callback )( wiced_bt_state_t state, wiced_result_t status );

/** \brief BT device info callback.
*
*   The call back notifies change in BT device info.
*
*   \param device_info BT device info
**/
typedef void ( *wiced_bt_info_callback )( wiced_bt_info_t* device_info );

/** \brief Connection satutus notification callback.
*
*   The call back notifies change in ACL connection state.
*
*   \param device_info BT device info
**/
typedef void ( *wiced_bt_acl_connection_state_callback )( wiced_bt_connection_event_data_t* device_info );


/**    \brief     struct used to pass callbacks to library.
*
*    Application defines handler functions to handle BT library events
*    following the prototype defined  here and register them with BT
*    library thru wiced_bt_init() API.
**/
typedef struct
{
    wiced_bt_state_changed_callback state_changed_cb;       //!<Callback function pointer
    wiced_bt_info_callback info_cb;                         //!<Callback function pointer
    wiced_bt_acl_connection_state_callback acl_conn_state_cb;   //!<Callback function pointer
}wiced_bt_callbacks_t;

/**    \brief     struct used to pass application prefferences/platform specific info to library.
*
*    ADC and DAC device names.
**/
typedef struct
{
    char* adc_name_str;
    char* dac_name_str;
}wiced_bt_audio_pref_t;

/** \brief Initialize bluetooth library.
*
*    This API should be the first BT library API called by an application.
*
*    This function asynchronously initializes the bluetooth library components
*    and enables bluetooth.
*    After initialization is done, the library calls state_changed_cb with state
*    WICED_BT_STATE_ENABLED.
*    The Application shall call any other API only after receiving state_changed_cb
*    callback with ENABLED state.
*
*   \param callbacks    structure containing pointer to functions that will be called from
*                                WICED bt library to convey bt DM events to applicaiton.
*   \param pref           structure to set app preferences to be used in library.
*
*   \return         WICED_SUCCESS  : on success;
*                       WICED_ERROR      : if an error occurred
**/
wiced_result_t wiced_bt_init( const wiced_bt_callbacks_t* callbacks, const wiced_bt_audio_pref_t* pref );


/**      \brief    Shuts down bluetooth services.
*
*      This function tears down any Bluetooth connections, and shuts down the services and Bluetooth.
*      The device shall no longer be available for Bluetooth operations.
*
*      \return WICED_SUCCESS : on success;
*                  WICED_ERROR     : if an error occurred
*/
wiced_result_t wiced_bt_cleanup( void );


/** \brief Gets local BT device info
*
* This function fetches the local device information such as Bluetooth address, Bluetooth device
* name and current scan mode.
*
*  \return    WICED_SUCCESS : on success;
*            WICED_ERROR   : if an error occurred
*/
wiced_result_t wiced_bt_get_device_info( void );


/** \brief Sets the Bluetooth adapter scan mode
*
* This function allows the application to configure the adapter's scan mode.
* By default, upon enable, the adapter shall be in connectable mode.
*
* @param mode Desired scan mode defined by bt_scan_mode_t
*
* @return    WICED_SUCCESS : on success;
*                 WICED_ERROR   : if an error occurred
*/
wiced_result_t wiced_bt_set_scan_mode( wiced_bt_scan_mode_t mode );


/** \brief Initiate connection to the remote device
*
* This function initates connection to the remote paired device. All the profiles that are
* initialized in the bt library are connected.
*
* @param bd_addr Remote Bluetooth address of the phone. If NULL, connection will be
*                           initiated to the last connected device
*
* @return    WICED_SUCCESS : on success;
*            WICED_ERROR   : if an error occurred
*/
 wiced_result_t wiced_bt_connect( const wiced_bt_bdaddr_t *bd_addr );


/** \brief Disconnect from the remote device
*
* This function disconnects from the remote device.
*
* @return    WICED_SUCCESS : on success;
*            WICED_ERROR   : if an error occurred
*/
wiced_result_t wiced_bt_disconnect( void );


/** \brief Delete info of all remote devices stored in NV
*
*  Dletes information (BD Address, link key, etc) of all the paired peer devices stored in NV
*
* @return    WICED_SUCCESS : on success;
*            WICED_ERROR   : if an error occurred
*/
wiced_result_t wiced_bt_delete_paired_devices_info( void );

/** @} */ // end of btdm

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //#ifndef WICED_BT_DM_H

