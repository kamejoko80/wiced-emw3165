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
 *  NFC tag writing snippet application
 *  This application demonstrates how to write to an NFC tag
 *
 * Application Instructions
 *   1. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *   2. Place any type of NFC tag on top of the NFC antenna
 *   3. The text referenced by DATA_TO_BE_WRITTEN_TO_TAG will be written to the NFC card
 */

#include "wiced.h"
#include "wiced_nfc_api.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define NFC_WRITE_TIMEOUT            ( 10000 )

#define DATA_TO_BE_WRITTEN_TO_TAG    "WICED NFC was here"

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

wiced_nfc_workspace_t nfc_workspace;

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( void )
{
    wiced_result_t      result;
    wiced_nfc_tag_msg_t tag_write_message;

    /* Note: NFC does not require Wi-Fi so we can use wiced_core_init() instead of wiced_init() */
    wiced_core_init( );

    WPRINT_APP_INFO( ("\nNFC tag writing application\n" ) );

    /* Initialize NFC.
     * Note: This can take up to 8 seconds to complete
     */
    WPRINT_APP_INFO(("\nInitializing NFC...\n"));
    if ( wiced_nfc_init( &nfc_workspace ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("NFC error\n") );
        return;
    }

    /* Prepare tag data */
    tag_write_message.buffer        = ( uint8_t* )DATA_TO_BE_WRITTEN_TO_TAG;
    tag_write_message.buffer_length = sizeof( DATA_TO_BE_WRITTEN_TO_TAG );
    tag_write_message.type          = WICED_NFC_TAG_TYPE_TEXT;

    while( 1 )
    {
        WPRINT_APP_INFO( ("\nWaiting for tag...\n") );
        result = wiced_nfc_write_tag( &tag_write_message, NFC_WRITE_TIMEOUT );

        if ( result == WICED_SUCCESS )
        {
            WPRINT_APP_INFO( ("Tag write successful\n") );

            /* Delay for a short time to allow tag to be removed */
            wiced_rtos_delay_milliseconds( 1000 );
        }
        else if ( result == WICED_TIMEOUT )
        {
            WPRINT_APP_INFO( ( "Tag not detected\n" ) );
        }
    }
}
