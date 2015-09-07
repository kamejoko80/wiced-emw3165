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
#include "restful_smart_constants.h"
#include "restful_smart_server.h"
#include "restful_smart_response.h"
#include "restful_smart_ble.h"
#include "http_server.h"

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

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

//const char* const restful_smart_http_status_table[] =
//{
//    [BT_REST_GATEWAY_STATUS_200] = "HTTP/1.1 - 200 - OK - application/json",
//    [BT_REST_GATEWAY_STATUS_400] = "HTTP/1.1 - 400 - Bad Request",
//    [BT_REST_GATEWAY_STATUS_403] = "HTTP/1.1 - 403 - Forbidden",
//    [BT_REST_GATEWAY_STATUS_404] = "HTTP/1.1 - 404 - Not Found",
//    [BT_REST_GATEWAY_STATUS_405] = "HTTP/1.1 - 405 - Method not allowed",
//    [BT_REST_GATEWAY_STATUS_406] = "HTTP/1.1 - 406 - Not acceptable",
//    [BT_REST_GATEWAY_STATUS_412] = "HTTP/1.1 - 412 - Precondition failed",
//    [BT_REST_GATEWAY_STATUS_415] = "HTTP/1.1 - 415 - Unsupported media type",
//    [BT_REST_GATEWAY_STATUS_504] = "HTTP/1.1 - 504 - Not able to connect",
//};

const http_status_codes_t restful_smart_http_status_table[] =
{
    [BT_REST_GATEWAY_STATUS_200] = HTTP_200_TYPE,
    [BT_REST_GATEWAY_STATUS_400] = HTTP_400_TYPE,
    [BT_REST_GATEWAY_STATUS_403] = HTTP_403_TYPE,
    [BT_REST_GATEWAY_STATUS_404] = HTTP_404_TYPE,
    [BT_REST_GATEWAY_STATUS_405] = HTTP_405_TYPE,
    [BT_REST_GATEWAY_STATUS_406] = HTTP_406_TYPE,
    [BT_REST_GATEWAY_STATUS_412] = HTTP_412_TYPE,
    [BT_REST_GATEWAY_STATUS_415] = HTTP_415_TYPE,
    [BT_REST_GATEWAY_STATUS_504] = HTTP_504_TYPE,
};

const pairing_status_t pairing_status_table[] =
{
    [PAIRING_FAILED]              = { "0", "Pairing Failed"                             },
    [PAIRING_SUCCESSFUL]          = { "1", "Pairing Successful"                         },
    [PAIRING_ABORTED]             = { "2", "Pairing Aborted"                            },
    [LE_LEGACY_OOB_EXPECTED]      = { "3", "LE Legacy Pairing OOB Expected"             },
    [LE_SECURE_OOB_EXPECTED]      = { "4", "LE Secure Connections Pairing OOB Expected" },
    [PASSKEY_INPUT_EXPECTED]      = { "5", "Passkey Input Expected"                     },
    [PASSKEY_DISPLAY_EXPECTED]    = { "6", "Passkey Display Expected"                   },
    [NUMERIC_COMPARISON_EXPECTED] = { "7", "Numeric Comparison Expected"                },
};
