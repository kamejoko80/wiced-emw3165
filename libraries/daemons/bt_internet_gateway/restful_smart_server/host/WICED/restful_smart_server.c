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
#include "smartbridge.h"
#include "smartbridge_gatt.h"
#include "restful_smart_server.h"
#include "restful_smart_ble.h"
#include "restful_smart_response.h"
#include "http_server.h"
#include "wiced_bt_dev.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define STRING_GAP                         "gap"
#define STRING_GATT                        "gatt"
#define STRING_MANAGEMENT                  "management"
#define STRING_NODES                       "nodes"
#define STRING_SERVICES                    "services"
#define STRING_CHARACTERISTICS             "characteristics"
#define STRING_DESCRIPTORS                 "descriptors"
#define STRING_VALUE                       "value"
#define STRING_PRIMARY_EQUAL_1             "primary=1"
#define STRING_UUID_EQUAL                  "uuid="
#define STRING_MULTIPLE_EQUAL_1            "multiple=1"
#define STRING_START_EQUAL                 "start="
#define STRING_END_EQUAL                   "end="
#define STRING_INDICATE_EQUAL_1            "indicate=1"
#define STRING_INDICATE_EQUAL_0            "indicate=0"
#define STRING_NOTIFY_EQUAL_1              "notify=1"
#define STRING_NOTIFY_EQUAL_0              "notify=0"
#define STRING_LONG_EQUAL_1                "long=1"
#define STRING_RELIABLE_EQUAL_1            "reliable=1"
#define STRING_EVENT_EQUAL_1               "event=1"
#define STRING_NO_RESPONSE_EQUAL_1         "noresponse=1"
#define STRING_BOND_EQUAL_0                "bond=0"
#define STRING_BOND_EQUAL_1                "bond=1"
#define STRING_LEGACY_OOB                  "legacy-oob=1"
#define STRING_OOB                         "oob=1"
#define STRING_IO_CAPABILITY_EQUAL         "io-capability="
#define STRING_DISPLAY_ONLY                "DisplayOnly"
#define STRING_DISPLAY_YES_NO              "DisplayYesNo"
#define STRING_KEYBOARD_ONLY               "KeyboardOnly"
#define STRING_NO_INPUT_NO_OUTPUT          "NoInputNoOutput"
#define STRING_KEYBOARD_DISPLAY            "KeyboardDisplay"
#define STRING_PAIRING_ID_EQUAL            "pairing-id="
#define STRING_TK_EQUAL                    "tk="
#define STRING_PASSKEY_EQUAL               "passkey="
#define STRING_BDADDRB_EQUAL               "bdaddrb="
#define STRING_RB_EQUAL                    "rb="
#define STRING_CB_EQUAL                    "cb="
#define STRING_CONFIRMED_EQUAL             "confirmed="
#define STRING_PARING_ABORT_EQUAL_1        "pairing-abort=1"
#define STRING_PASSIVE_EQUAL_1             "passive=1"
#define STRING_ACTIVE_EQUAL_1              "active=1"
#define STRING_ENABLE_EQUAL_1              "enable=1"
#define STRING_ENABLE_EQUAL_0              "enable=0"
#define STRING_CONNECT_EQUAL_1             "connect=1"
#define STRING_NAME_EQUAL_1                "name=1"

#define HTTP_GET                           0x00000001 /* GET */
#define HTTP_PUT                           0x00000002 /* PUT */
#define SLASH_MANAGEMENT                   0x00000004 /* /management */
#define SLASH_GATT                         0x00000010 /* /gatt */
#define SLASH_GAP                          0x00000020 /* /gap */
#define SLASH_NODES                        0x00000040 /* /nodes */
#define SLASH_NODE_HANDLE                  0x00000080 /* /<node> */
#define SLASH_SERVICES                     0x00000100 /* /services */
#define SLASH_SERVICE_HANDLE               0x00000200 /* /<service> */
#define SLASH_CHARACTERISTICS              0x00000400 /* /characteristics */
#define SLASH_CHARACTERISTIC_HANDLE        0x00000800 /* /<characteristic> */
#define SLASH_DESCRIPTORS                  0x00001000 /* /descriptors */
#define SLASH_DESCRIPTOR_HANDLE            0x00002000 /* /<descriptor> */
#define SLASH_VALUE                        0x00004000 /* /value */
#define SLASH_VALUE_CONTENT                0x00008000 /* /<value> */

/* GATT queries */
#define QUERY_PRIMARY_EQUAL_1              0x00010000 /* ?primary=1 */
#define QUERY_UUID_EQUAL                   0x00020000 /* ?uuid= */
#define QUERY_MULTIPLE_EQUAL_1             0x00040000 /* ?multiple=1 */
#define QUERY_START_EQUAL                  0x00080000 /* ?start= */
#define QUERY_END_EQUAL                    0x00100000 /* ?end= */
#define QUERY_INDICATE_EQUAL_1             0x00200000 /* ?indicate=1 */
#define QUERY_INDICATE_EQUAL_0             0x00400000 /* ?indicate=0 */
#define QUERY_NOTIFY_EQUAL_1               0x00800000 /* ?notify=1 */
#define QUERY_NOTIFY_EQUAL_0               0x01000000 /* ?notify=0 */
#define QUERY_LONG_EQUAL_1                 0x02000000 /* ?long=1 */
#define QUERY_RELIABLE_EQUAL_1             0x04000000 /* ?reliable=1 */
#define QUERY_EVENT_EQUAL_1                0x08000000 /* ?event=1 */
#define QUERY_NO_RESPONSE_EQUAL_1          0x10000000 /* ?noresponse=1 */

/* Management queries */
#define QUERY_BOND_EQUAL_0                 0x20000000 /* ?bond=0 */
#define QUERY_BOND_EQUAL_1                 0x40000000 /* ?bond=1 */
#define QUERY_PAIRING_ID_EQUAL             0x80000000 /* ?pairing-id= */
#define QUERY_PAIRING_ABORT_EQUAL_1        0xffff0000 /* ?pairing-abort=1 */

/* GAP queries */
#define QUERY_PASSIVE_EQUAL_1              0x00000100 /* ?passive=1 */
#define QUERY_ACTIVE_EQUAL_1               0x00000200 /* ?active=1 */
#define QUERY_ENABLE_EQUAL_1               0x00000400 /* ?enable=1 */
#define QUERY_ENABLE_EQUAL_0               0x00000800 /* ?enable=0 */
#define QUERY_CONNECT_EQUAL_1              0x00001000 /* ?connect=1 */
#define QUERY_NAME_EQUAL_1                 0x00002000 /* ?name=1 */

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    PATH_ROOT_DEPTH,
    PATH_DEPTH_1,
    PATH_DEPTH_2,
    PATH_DEPTH_3,
    PATH_DEPTH_4,
    PATH_DEPTH_5,
    PATH_DEPTH_6,
} path_depth_t;

typedef enum
{
    REST_API_UNKNOWN                                      = ( 0 ),

    /* REST GATT APIs */
    REST_API_GET_AVAILABLE_NODES                          = ( HTTP_GET | SLASH_GATT | SLASH_NODES ),
    REST_API_READ_SPECIFIC_NODE                           = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE ),
    REST_API_DISCOVER_ALL_SERVICES                        = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_SERVICES ),
    REST_API_DISCOVER_ALL_PRIMARY_SERVICES                = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_SERVICES | QUERY_PRIMARY_EQUAL_1 ),
    REST_API_DISCOVER_PRIMARY_SERVICES_BY_UUID            = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_SERVICES | QUERY_PRIMARY_EQUAL_1 | QUERY_UUID_EQUAL ),
    REST_API_READ_SPECIFIC_SERVICE                        = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_SERVICES | SLASH_SERVICE_HANDLE ),
    REST_API_DISCOVER_ALL_CHARACTERISTICS_OF_A_SERVICE    = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_SERVICES | SLASH_SERVICE_HANDLE | SLASH_CHARACTERISTICS ),
    REST_API_DISCOVER_CHARACTERISTICS_BY_UUID             = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | QUERY_UUID_EQUAL ),
    REST_API_READ_MULTIPLE_CHARACTERISTIC_VALUES          = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_VALUE | QUERY_MULTIPLE_EQUAL_1 ),
    REST_API_READ_CHARACTERISTIC_VALUES_BY_UUID           = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_VALUE | QUERY_UUID_EQUAL | QUERY_START_EQUAL | QUERY_END_EQUAL ),
    REST_API_RELIABLE_WRITE_CHARACTERISTIC_VALUES         = ( HTTP_PUT | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_VALUE | QUERY_RELIABLE_EQUAL_1 ),
    REST_API_READ_SPECIFC_CHARACTERISTIC                  = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE ),
    REST_API_READ_CHARACTERISTIC_VALUE                    = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_VALUE ),
    REST_API_READ_LONG_CHARACTERISTIC_VALUE               = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_VALUE | QUERY_LONG_EQUAL_1 ),
    REST_API_READ_CACHED_INDICATED_VALUE                  = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_VALUE | QUERY_INDICATE_EQUAL_1 ),
    REST_API_READ_CACHED_NOTIFIED_VALUE                   = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_VALUE | QUERY_NOTIFY_EQUAL_1 ),
    REST_API_SUBCSRIBE_INDICATION_EVENT                   = ( HTTP_PUT | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_VALUE | QUERY_INDICATE_EQUAL_1 | QUERY_EVENT_EQUAL_1 ),
    REST_API_SUBSCRIBE_NOTIFICATION_EVENT                 = ( HTTP_PUT | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_VALUE | QUERY_NOTIFY_EQUAL_1   | QUERY_EVENT_EQUAL_1 ),
    REST_API_WRITE_CHARACTERISTIC_VALUE                   = ( HTTP_PUT | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_VALUE | SLASH_VALUE_CONTENT ),
    REST_API_WRITE_LONG_CHARACTERISTIC_VALUE              = ( HTTP_PUT | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_VALUE | SLASH_VALUE_CONTENT | QUERY_LONG_EQUAL_1 ),
    REST_API_WRITE_CHARACTERISTIC_VALUE_WITHOUT_RESPONSE  = ( HTTP_PUT | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_VALUE | SLASH_VALUE_CONTENT | QUERY_NO_RESPONSE_EQUAL_1 ),
    REST_API_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS      = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_CHARACTERISTICS | SLASH_CHARACTERISTIC_HANDLE | SLASH_DESCRIPTORS ),
    REST_API_READ_CHARACTERISTIC_DESCRIPTOR               = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_DESCRIPTORS | SLASH_DESCRIPTOR_HANDLE ),
    REST_API_READ_CHARACTERISTIC_DESCRIPTOR_VALUE         = ( HTTP_GET | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_DESCRIPTORS | SLASH_DESCRIPTOR_HANDLE | SLASH_VALUE ),
    REST_API_WRITE_CHARACTERISTIC_DESCRIPTOR_VALUE        = ( HTTP_PUT | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_DESCRIPTORS | SLASH_DESCRIPTOR_HANDLE | SLASH_VALUE | SLASH_VALUE_CONTENT ),
    REST_API_WRITE_LONG_CHARACTERISTIC_DESCRIPTOR_VALUE   = ( HTTP_PUT | SLASH_GATT | SLASH_NODES | SLASH_NODE_HANDLE | SLASH_DESCRIPTORS | SLASH_DESCRIPTOR_HANDLE | SLASH_VALUE | SLASH_VALUE_CONTENT | QUERY_LONG_EQUAL_1 ),

    /* REST Management APIs */
    REST_API_BONDED_NODES                                 = ( HTTP_GET | SLASH_MANAGEMENT | SLASH_NODES ),
    REST_API_READ_SPECIFIC_BOND_DATA                      = ( HTTP_GET | SLASH_MANAGEMENT | SLASH_NODES | SLASH_NODE_HANDLE ),
    REST_API_REMOVE_BOND                                  = ( HTTP_PUT | SLASH_MANAGEMENT | SLASH_NODES | SLASH_NODE_HANDLE | QUERY_BOND_EQUAL_0 ),
    REST_API_CONFIGURE_BOND                               = ( HTTP_PUT | SLASH_MANAGEMENT | SLASH_NODES | SLASH_NODE_HANDLE | QUERY_BOND_EQUAL_1 ),
    REST_API_CONFIGURE_PAIRING                            = ( HTTP_PUT | SLASH_MANAGEMENT | SLASH_NODES | SLASH_NODE_HANDLE | QUERY_PAIRING_ID_EQUAL ),
    REST_API_PAIRING_ABORT                                = ( HTTP_PUT | SLASH_MANAGEMENT | SLASH_NODES | SLASH_NODE_HANDLE | QUERY_PAIRING_ABORT_EQUAL_1 ),

    /* REST GAP APIs */
    REST_API_PASSIVE_SCAN                                 = ( HTTP_GET | SLASH_GAP | SLASH_NODES | QUERY_PASSIVE_EQUAL_1 ),
    REST_API_ACTIVE_SCAN                                  = ( HTTP_GET | SLASH_GAP | SLASH_NODES | QUERY_ACTIVE_EQUAL_1 ),
    REST_API_ENABLED_NODES                                = ( HTTP_GET | SLASH_GAP | SLASH_NODES | QUERY_ENABLE_EQUAL_1 ),
    REST_API_ENABLED_NODE_DATA                            = ( HTTP_GET | SLASH_GAP | SLASH_NODES | SLASH_NODE_HANDLE ),
    REST_API_ENABLE_AND_CONNECT                           = ( HTTP_PUT | SLASH_GAP | SLASH_NODES | SLASH_NODE_HANDLE | QUERY_CONNECT_EQUAL_1 | QUERY_ENABLE_EQUAL_1 ),
    REST_API_REMOVE_NODE                                  = ( HTTP_PUT | SLASH_GAP | SLASH_NODES | SLASH_NODE_HANDLE | QUERY_ENABLE_EQUAL_0 ),
    REST_API_DISCOVER_NODE_NAME                           = ( HTTP_GET | SLASH_GAP | SLASH_NODES | SLASH_NODE_HANDLE | QUERY_NAME_EQUAL_1 ),


    REST_API_FORCE_32BIT                                  = ( 0x7fffffff )
} rest_api_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint32_t query_bit;
    char*    query_string;
} bt_rest_api_query_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

static int32_t process_restful_api_request( const char* url_parameters, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static wiced_http_server_t http_server;

static START_OF_HTTP_PAGE_DATABASE(web_pages)
    { "/",  "application/json",  WICED_RAW_DYNAMIC_URL_CONTENT, .url_content.dynamic_data = { process_restful_api_request, 0 }, },
END_OF_HTTP_PAGE_DATABASE();

static const bt_rest_api_query_t query_table[] =
{
    [0]  = { .query_bit = QUERY_PRIMARY_EQUAL_1,       .query_string = STRING_PRIMARY_EQUAL_1      },
    [1]  = { .query_bit = QUERY_UUID_EQUAL,            .query_string = STRING_UUID_EQUAL           },
    [2]  = { .query_bit = QUERY_MULTIPLE_EQUAL_1,      .query_string = STRING_MULTIPLE_EQUAL_1     },
    [3]  = { .query_bit = QUERY_START_EQUAL,           .query_string = STRING_START_EQUAL          },
    [4]  = { .query_bit = QUERY_END_EQUAL,             .query_string = STRING_END_EQUAL            },
    [5]  = { .query_bit = QUERY_INDICATE_EQUAL_1,      .query_string = STRING_INDICATE_EQUAL_1     },
    [6]  = { .query_bit = QUERY_INDICATE_EQUAL_0,      .query_string = STRING_INDICATE_EQUAL_0     },
    [7]  = { .query_bit = QUERY_NOTIFY_EQUAL_1,        .query_string = STRING_NOTIFY_EQUAL_1       },
    [8]  = { .query_bit = QUERY_NOTIFY_EQUAL_0,        .query_string = STRING_NOTIFY_EQUAL_0       },
    [9]  = { .query_bit = QUERY_LONG_EQUAL_1,          .query_string = STRING_LONG_EQUAL_1         },
    [10] = { .query_bit = QUERY_RELIABLE_EQUAL_1,      .query_string = STRING_RELIABLE_EQUAL_1     },
    [11] = { .query_bit = QUERY_EVENT_EQUAL_1,         .query_string = STRING_EVENT_EQUAL_1        },
    [12] = { .query_bit = QUERY_NO_RESPONSE_EQUAL_1,   .query_string = STRING_NO_RESPONSE_EQUAL_1  },
    [13] = { .query_bit = QUERY_BOND_EQUAL_0,          .query_string = STRING_BOND_EQUAL_0         },
    [14] = { .query_bit = QUERY_BOND_EQUAL_1,          .query_string = STRING_BOND_EQUAL_1         },
    [15] = { .query_bit = QUERY_PAIRING_ID_EQUAL,      .query_string = STRING_PAIRING_ID_EQUAL     },
    [16] = { .query_bit = QUERY_PAIRING_ABORT_EQUAL_1, .query_string = STRING_PARING_ABORT_EQUAL_1 },
    [17] = { .query_bit = QUERY_PASSIVE_EQUAL_1,       .query_string = STRING_PASSIVE_EQUAL_1      },
    [18] = { .query_bit = QUERY_ACTIVE_EQUAL_1,        .query_string = STRING_ACTIVE_EQUAL_1       },
    [19] = { .query_bit = QUERY_ENABLE_EQUAL_1,        .query_string = STRING_ENABLE_EQUAL_1       },
    [20] = { .query_bit = QUERY_ENABLE_EQUAL_0,        .query_string = STRING_ENABLE_EQUAL_0       },
    [21] = { .query_bit = QUERY_CONNECT_EQUAL_1,       .query_string = STRING_CONNECT_EQUAL_1      },
    [22] = { .query_bit = QUERY_NAME_EQUAL_1,          .query_string = STRING_NAME_EQUAL_1         },
    [23] = { .query_bit = 0,                           .query_string = STRING_IO_CAPABILITY_EQUAL  },
    [24] = { .query_bit = 0,                           .query_string = STRING_LEGACY_OOB           },
    [25] = { .query_bit = 0,                           .query_string = STRING_OOB                  },
    [26] = { .query_bit = 0,                           .query_string = STRING_PASSKEY_EQUAL        },
    [27] = { .query_bit = 0,                           .query_string = STRING_TK_EQUAL             },
    [28] = { .query_bit = 0,                           .query_string = STRING_RB_EQUAL             },
    [29] = { .query_bit = 0,                           .query_string = STRING_CB_EQUAL             },
    [30] = { .query_bit = 0,                           .query_string = STRING_CONFIRMED_EQUAL      },

};

static const uint32_t query_table_size = sizeof( query_table ) / sizeof( bt_rest_api_query_t );

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t restful_smart_server_start( void )
{
    wiced_result_t result;

    /* Initialise BT stack */
    result = restful_smart_init_smartbridge();
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Start HTTP server */
    wiced_http_server_start( &http_server, 80, 5, web_pages, WICED_STA_INTERFACE, 5000 );

    return result;
}

wiced_result_t restful_smart_server_stop( void )
{
    wiced_result_t result;

    /* Deinitialise stack */
    result = restful_smart_deinit_smartbridge();
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Deinitialise SmartBridge */
    result = smartbridge_deinit( );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    return result;
}

static int32_t process_restful_api_request( const char* url_parameters, wiced_http_response_stream_t* stream, void* arg, wiced_http_message_body_t* http_message_body )
{
    rest_api_type_t               api_type         = REST_API_UNKNOWN;
    path_depth_t                  path_depth       = PATH_ROOT_DEPTH;
    char*                         path             = NULL;
    char*                         query            = NULL;
    char*                         query_uuid       = NULL;
    char*                         query_start      = NULL;
    char*                         query_end        = NULL;
    char*                         query_io_caps    = NULL;
    char*                         query_pairing_id = NULL;
    char*                         query_leg_oob    = NULL;
    char*                         query_sc_oob     = NULL;
    char*                         query_passkey    = NULL;
    char*                         query_tk         = NULL;
    char*                         query_rb         = NULL;
    char*                         query_cb         = NULL;
    char*                         query_confirm    = NULL;
    uint8_t                       io_capabilities  = BTM_IO_CAPABILIES_MAX;
    uint8_t                       oob_type         = 0;
    uint32_t                      passkey          = 0;
    wiced_bool_t                  confirmed        = WICED_FALSE;
    wiced_result_t                result           = WICED_SUCCESS;
    uint32_t                      temp;
    wiced_bt_uuid_t               uuid;
    smart_node_handle_t           node;
    smart_service_handle_t        service;
    smart_characteristic_handle_t characteristic;
    smart_value_handle_t*         value = NULL;
    uint16_t                      descriptor_handle = 0;


    /* Decode HTTP request type */
    if ( http_message_body->request_type == WICED_HTTP_GET_REQUEST )
    {
        api_type |= HTTP_GET;
    }
    else if ( http_message_body->request_type == WICED_HTTP_PUT_REQUEST )
    {
        api_type |= HTTP_PUT;
    }

    /* Separate path from query string */
    path  = strtok( (char*)url_parameters, "?" );
    query = strtok( NULL,                  "?" );

    /* Order of sub-directory matters in path. Need to decode per token */
    path  = strtok( path, "/" );
    while ( path != NULL )
    {
        switch ( path_depth )
        {
            case PATH_ROOT_DEPTH:
            {
                if ( strstr( path, STRING_GATT ) != NULL )
                {
                    api_type |= SLASH_GATT;
                }
                else if ( strstr( path, STRING_MANAGEMENT ) != NULL )
                {
                    api_type |= SLASH_MANAGEMENT;
                }
                else if ( strstr( path, STRING_GAP ) != NULL )
                {
                    api_type |= SLASH_GAP;
                }
                break;
            }
            case PATH_DEPTH_1:
            {
                if ( strstr( path, STRING_NODES ) != NULL )
                {
                    api_type |= SLASH_NODES;
                }
                break;
            }
            case PATH_DEPTH_2:
            {
                /* device address detected. Convert to Bluetooth device address type */
                string_to_device_address( path, &node.address );
                string_to_unsigned( path + 12, 1, &temp, 1 );
                node.type = (wiced_bt_ble_address_type_t)( temp & 0xff );
                api_type |= SLASH_NODE_HANDLE;
                break;
            }
            case PATH_DEPTH_3:
            {
                if ( strstr( path, STRING_SERVICES ) != NULL )
                {
                    api_type |= SLASH_SERVICES;
                }
                else if ( strstr( path, STRING_CHARACTERISTICS ) != NULL )
                {
                    api_type |= SLASH_CHARACTERISTICS;
                }
                else if ( strstr( path, STRING_DESCRIPTORS ) != NULL )
                {
                    api_type |= SLASH_DESCRIPTORS;
                }
                break;
            }
            case PATH_DEPTH_4:
            {
                if ( ( api_type & SLASH_SERVICES ) != 0 )
                {
                    /* Decode service handle. Format is XXXXXXXX ( <hex start handle><hex end handle> ) */
                    string_to_unsigned( path,     4, &temp, 1 );
                    service.end_handle   = (uint16_t)( temp & 0xffff );
                    string_to_unsigned( path + 4, 4, &temp, 1 );
                    service.start_handle = (uint16_t)( temp & 0xffff );
                    api_type |= SLASH_SERVICE_HANDLE;
                }
                else if ( ( api_type & SLASH_CHARACTERISTICS ) != 0 )
                {
                    if ( strstr( path, STRING_VALUE ) != NULL )
                    {
                        api_type |= SLASH_VALUE;
                    }
                    else
                    {
                        /* Decode characteristic handle here */
                        string_to_unsigned( path,     4, &temp, 1 );
                        characteristic.end_handle   = (uint16_t)( temp & 0xffff );
                        string_to_unsigned( path + 4, 4, &temp, 1 );
                        characteristic.value_handle = (uint16_t)( temp & 0xffff );
                        string_to_unsigned( path + 8, 4, &temp, 1 );
                        characteristic.start_handle = (uint16_t)( temp & 0xffff );
                        api_type |= SLASH_CHARACTERISTIC_HANDLE;
                    }
                }
                else if ( ( api_type & SLASH_DESCRIPTORS ) != 0 )
                {
                    /* Decode descriptor handle here */
                    string_to_unsigned( path, 4, &temp, 1 );
                    descriptor_handle = (uint16_t)( temp & 0xffff );
                    api_type |= SLASH_DESCRIPTOR_HANDLE;
                }
                break;
            }
            case PATH_DEPTH_5:
            {
                if ( ( api_type & SLASH_SERVICE_HANDLE ) != 0 )
                {
                    api_type |= SLASH_CHARACTERISTICS;
                }
                else if ( ( api_type & SLASH_CHARACTERISTIC_HANDLE ) != 0 )
                {
                    if ( strstr( path, STRING_VALUE ) != NULL )
                    {
                        api_type |= SLASH_VALUE;
                    }
                    else
                    {
                        api_type |= SLASH_DESCRIPTORS;
                    }
                }
                else if ( ( api_type & SLASH_DESCRIPTOR_HANDLE ) != 0 )
                {
                    api_type |= SLASH_VALUE;
                }
                break;
            }
            case PATH_DEPTH_6:
            {
                if ( ( api_type & SLASH_VALUE ) != 0 )
                {
                    /* Grab pointer to value content here */
                    temp  = strlen( path ) / 2;
                    value = (smart_value_handle_t*)malloc_named( "value", sizeof( smart_value_handle_t ) + temp );
                    if ( value != NULL )
                    {
                        uint32_t a;
                        uint32_t b;

                        memset( value, 0, sizeof( smart_value_handle_t ) + temp );
                        value->length = temp;
                        for ( a = 0, b = ( value->length - 1 ) * 2 ; a < value->length; a++, b-=2 )
                        {
                            string_to_unsigned( &path[b], 2, &temp, 1 );
                            value->value[a] = (uint8_t)( temp & 0xff );
                        }
                    }
                    api_type |= SLASH_VALUE_CONTENT;
                }
                break;
            }
            default:
            {
                break;
            }
        }

        path  = strtok( NULL, "/" );
        path_depth++;
    }

    /* Decode query string */
    query = strtok( query, "&" );
    while ( query != NULL )
    {
        uint32_t a;

        for ( a = 0; a < query_table_size; a++ )
        {
            if ( strstr( query, query_table[a].query_string ) != NULL )
            {
                api_type |= query_table[a].query_bit;

                if ( strstr( query, STRING_UUID_EQUAL ) != NULL )
                {
                    query_uuid = query;
                }
                else if ( strstr( query, STRING_START_EQUAL ) != NULL )
                {
                    query_start = query;
                }
                else if ( strstr( query, STRING_END_EQUAL ) != NULL )
                {
                    query_end = query;
                }
                else if ( strstr( query, STRING_IO_CAPABILITY_EQUAL ) != NULL )
                {
                    query_io_caps = query;
                }
                else if ( strstr( query, STRING_LEGACY_OOB ) != NULL )
                {
                    query_leg_oob = query;
                }
                else if ( strstr( query, STRING_OOB ) != NULL )
                {
                    query_sc_oob = query;
                }
                else if ( strstr( query, STRING_PAIRING_ID_EQUAL ) != NULL )
                {
                    query_pairing_id = query;
                }
                else if ( strstr( query, STRING_PASSKEY_EQUAL ) != NULL )
                {
                    query_passkey = query;
                }
                else if ( strstr( query, STRING_TK_EQUAL ) != NULL )
                {
                    query_tk = query;
                }
                else if ( strstr( query, STRING_RB_EQUAL ) != NULL )
                {
                    query_rb = query;
                }
                else if ( strstr( query, STRING_CB_EQUAL ) != NULL )
                {
                    query_cb = query;
                }
                else if ( strstr( query, STRING_CONFIRMED_EQUAL ) != NULL )
                {
                    query_confirm = query;
                }
            }
        }
        query = strtok( NULL, "&" );
    }

    if ( query_uuid != NULL )
    {
        query_uuid = strtok( (char*)query_uuid, "=" );
        query_uuid = strtok( NULL,              "=" );
        string_to_uuid( query_uuid, &uuid );
    }

    if ( query_start != NULL )
    {
        query_start = strtok( (char*)query_start, "=" );
        query_start = strtok( NULL,               "=" );
        string_to_unsigned( query_start, strlen( query_start ), &temp, 0 );
//        query_handle_range.start_handle = (uint16_t)( temp & 0xffff );
    }

    if ( query_end != NULL )
    {
        query_end = strtok( (char*)query_end, "=" );
        query_end = strtok( NULL,             "=" );
        string_to_unsigned( query_end, strlen( query_end ), &temp, 0 );
//        query_handle_range.end_handle = (uint16_t)( temp & 0xffff );
    }

    if ( query_io_caps != NULL )
    {
        if ( strstr( query_io_caps, STRING_DISPLAY_ONLY ) != NULL )
        {
            io_capabilities = BTM_IO_CAPABILIES_DISPLAY_ONLY;
        }
        else if ( strstr( query_io_caps, STRING_DISPLAY_YES_NO ) != NULL )
        {
            io_capabilities = BTM_IO_CAPABILIES_DISPLAY_AND_KEYBOARD;
        }
        else if ( strstr( query_io_caps, STRING_KEYBOARD_ONLY ) != NULL )
        {
            io_capabilities = BTM_IO_CAPABILIES_KEYBOARD_ONLY;
        }
        else if ( strstr( query_io_caps, STRING_NO_INPUT_NO_OUTPUT ) != NULL )
        {
            io_capabilities = BTM_IO_CAPABILIES_NONE;
        }
        else if ( strstr( query_io_caps, STRING_KEYBOARD_DISPLAY ) != NULL )
        {
            io_capabilities = BTM_IO_CAPABILIES_KEYBOARD_DISPLAY;
        }
    }

    if ( query_leg_oob != NULL )
    {
        oob_type = 1;
    }

    if ( query_sc_oob != NULL )
    {
        oob_type = 2;
    }

    if ( query_pairing_id != NULL )
    {
        query_pairing_id = strtok( (char*)query_pairing_id, "=" );
        query_pairing_id = strtok( NULL,                    "{" );
        query_pairing_id += 2;
    }

    if ( query_passkey != NULL )
    {
        query_passkey = strtok( (char*)query_passkey, "=" );
        query_passkey = strtok( NULL,                 "=" );
        string_to_unsigned( query_passkey, strlen( query_passkey ), &passkey, 0 );
    }

    if ( query_tk != NULL )
    {
        query_tk = strtok( (char*)query_tk, "=" );
        query_tk = strtok( NULL,            "=" );
        query_tk += 2;
    }

    if ( query_rb != NULL )
    {
        query_rb = strtok( (char*)query_rb, "=" );
        query_rb = strtok( NULL,            "=" );
        query_rb += 2;
    }

    if ( query_cb != NULL )
    {
        query_cb = strtok( (char*)query_cb, "=" );
        query_cb = strtok( NULL,            "}" );
        query_cb += 2;
    }

    if ( query_confirm != NULL )
    {
        confirmed = ( strstr( query_confirm, "=yes" ) != NULL ) ? WICED_TRUE : WICED_FALSE;
    }

    wiced_http_response_stream_enable_chunked_transfer( stream );

    /* Send command */
    switch ( api_type )
    {
        case REST_API_DISCOVER_ALL_SERVICES:
            result = restful_smart_discover_all_primary_services( stream, &node );
            break;

        case REST_API_DISCOVER_ALL_PRIMARY_SERVICES:
            result = restful_smart_discover_all_primary_services( stream, &node );
            break;

        case REST_API_DISCOVER_PRIMARY_SERVICES_BY_UUID:
            result = restful_smart_discover_primary_services_by_uuid( stream, &node, &uuid );
            break;

        case REST_API_DISCOVER_ALL_CHARACTERISTICS_OF_A_SERVICE:
            result = restful_smart_discover_characteristics_of_a_service( stream, &node, &service );
            break;

        case REST_API_DISCOVER_CHARACTERISTICS_BY_UUID:
            result = restful_smart_discover_characteristics_by_uuid( stream, &node, &uuid );
            break;

        case REST_API_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS:
            result = restful_smart_discover_characteristic_descriptors( stream, &node, &characteristic );
            break;

        case REST_API_READ_CHARACTERISTIC_VALUE:
            result = restful_smart_read_characteristic_value( stream, &node, &characteristic );
            break;

        case REST_API_READ_CHARACTERISTIC_VALUES_BY_UUID:
            result = restful_smart_read_characteristic_values_by_uuid( stream, &node, &characteristic, &uuid );
            break;

        case REST_API_READ_CACHED_INDICATED_VALUE:
        case REST_API_READ_CACHED_NOTIFIED_VALUE:
            result = restful_smart_read_cached_value( stream, &node, &characteristic );
            break;

        case REST_API_WRITE_CHARACTERISTIC_VALUE:
            result = restful_smart_write_characteristic_value( stream, &node, &characteristic, value );
            break;

        case REST_API_READ_CHARACTERISTIC_DESCRIPTOR_VALUE:
            result = restful_smart_read_descriptor_value( stream, &node, descriptor_handle );
            break;

        case REST_API_WRITE_CHARACTERISTIC_DESCRIPTOR_VALUE:
            result = restful_smart_write_descriptor_value( stream, &node, descriptor_handle, value );
            break;

        case REST_API_READ_LONG_CHARACTERISTIC_VALUE:
            result = restful_smart_read_characteristic_long_value( stream, &node, &characteristic );
            break;

        case REST_API_WRITE_CHARACTERISTIC_VALUE_WITHOUT_RESPONSE:
            result = restful_smart_write_characteristic_value_without_response( stream, &node, &characteristic, value );
            break;

        case REST_API_BONDED_NODES:
            result = restful_smart_return_bonded_nodes( stream, NULL );
            break;

        case REST_API_READ_SPECIFIC_BOND_DATA:
            result = restful_smart_return_bonded_nodes( stream, &node.address );
            break;

        case REST_API_REMOVE_BOND:
            result = restful_smart_remove_bond( stream, &node.address );
            break;

        case REST_API_CONFIGURE_BOND:
            result = restful_smart_configure_bond( stream, &node.address, &io_capabilities, &oob_type );
            break;

        case REST_API_CONFIGURE_PAIRING:
            result = restful_smart_configure_pairing( stream, &node.address, query_pairing_id, query_tk, &passkey, query_rb, query_cb, confirmed );
            break;

        case REST_API_PAIRING_ABORT:
            result = restful_smart_cancel_bond( stream, &node.address );
            break;

        case REST_API_PASSIVE_SCAN:
            result = restful_smart_passive_scan( stream );
            break;

        case REST_API_ACTIVE_SCAN:
            result = restful_smart_active_scan( stream );
            break;

        case REST_API_ENABLED_NODES:
            result = restful_smart_enabled_devices( stream );
            break;

        case REST_API_ENABLED_NODE_DATA:
            result = restful_smart_enabled_node_data( stream, &node.address  );
            break;

        case REST_API_ENABLE_AND_CONNECT:
            restful_smart_connect( stream, &node );
            break;

        case REST_API_REMOVE_NODE:
            result = restful_smart_remove_device( stream, &node.address );
            break;

        case REST_API_DISCOVER_NODE_NAME:
            result = restful_smart_discover_node_name( stream, &node.address );
            break;
        case REST_API_RELIABLE_WRITE_CHARACTERISTIC_VALUES:        /* Fall-through */
        case REST_API_READ_MULTIPLE_CHARACTERISTIC_VALUES:         /* Fall-through */
        case REST_API_WRITE_LONG_CHARACTERISTIC_DESCRIPTOR_VALUE:  /* Fall-through */
        case REST_API_WRITE_LONG_CHARACTERISTIC_VALUE:             /* Fall-through */
        case REST_API_SUBCSRIBE_INDICATION_EVENT:                  /* Fall-through */
        case REST_API_SUBSCRIBE_NOTIFICATION_EVENT:                /* Fall-through */
        case REST_API_GET_AVAILABLE_NODES:                         /* Fall-through */
        case REST_API_READ_SPECIFIC_NODE:                          /* Fall-through */
        case REST_API_READ_SPECIFIC_SERVICE:                       /* Fall-through */
        case REST_API_READ_SPECIFC_CHARACTERISTIC:                 /* Fall-through */
        case REST_API_READ_CHARACTERISTIC_DESCRIPTOR:              /* Fall-through */
            result = WICED_NOT_FOUND;
            break;

        case REST_API_UNKNOWN:                                     /* Fall-through */
        case REST_API_FORCE_32BIT:                                 /* Fall-through */
        default:
            WPRINT_LIB_INFO(( "Unknown REST API Type: 0x%.08x\n", (unsigned int)api_type ));
            break;
    }

    if ( value != NULL )
    {
        free( value );
    }

    if ( result == WICED_NOT_FOUND )
    {
        restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_404 );
    }
    else if ( result != WICED_SUCCESS )
    {
        restful_gateway_write_status_code( stream, BT_REST_GATEWAY_STATUS_400 );
    }

    wiced_http_response_stream_flush( stream );
    wiced_http_response_stream_disable_chunked_transfer( stream );
    wiced_http_server_queue_disconnect_request( &http_server, stream->tcp_stream.socket );
    return 0;
}
