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
 *  NFC tag reader snippet application
 *  This application demonstrates how to read an NFC tag
 *
 * Application Instructions
 *   1. Connect a PC terminal to the serial port of the WICED Eval board,
 *      then build and download the application as described in the WICED
 *      Quick Start Guide
 *   2. Place any type of NFC tag on top of the NFC antenna
 *   3. The contents of the NFC tag will be read and printed out the
 *      serial port
 */

#include "wiced.h"
#include "wiced_nfc_api.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define NFC_DATA_LENGTH       ( 250 )
#define NFC_READ_TIMEOUT      ( 10000 )

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
    int            i;
    wiced_result_t result;
    uint8_t        buffer[ NFC_DATA_LENGTH ];
    uint32_t       buffer_length;

    /* Note: NFC does not require Wi-Fi so we can use wiced_core_init() instead of wiced_init() */
    wiced_core_init( );

    WPRINT_APP_INFO( ("\nNFC tag reading application\n") );

    /* Initialize NFC.
     * Note: This can take up to 8 seconds to complete
     */
    WPRINT_APP_INFO(("\nInitializing NFC...\n"));
    if ( wiced_nfc_init( &nfc_workspace ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("NFC error\n") );
        return;
    }

    while ( 1 )
    {
        WPRINT_APP_INFO( ("\nWaiting for tag...\n") );

        /* Reset the buffer_length variable. Note that this is used for both input and output */
        buffer_length = NFC_DATA_LENGTH;

        /* Wait for, and read, and NFC tag */
        result = wiced_nfc_read_tag( buffer, &buffer_length, NFC_READ_TIMEOUT );
        if ( result != WICED_SUCCESS )
        {
            if ( result == WICED_TIMEOUT )
            {
                WPRINT_APP_INFO( ( "Tag not detected\n" ) );
            }
            continue;
        }

        WPRINT_APP_INFO( ( "Received %lu bytes:\n", buffer_length ) );

        /* Print the contents of the NFC tag */
        for ( i = 0; i < buffer_length; i++ )
        {
            /* Check if the character is printable, otherwise use hex notation*/
            if ( buffer[ i ] >= 0x20 && buffer[ i ] <= 0x7E )
            {
                WPRINT_APP_INFO( ( "%c", buffer[i] ) );
            }
            else
            {
                WPRINT_APP_INFO( ( "\\%02x", buffer[i] ) );
            }
        }
        WPRINT_APP_INFO( ( "\n" ) );
    }
}
