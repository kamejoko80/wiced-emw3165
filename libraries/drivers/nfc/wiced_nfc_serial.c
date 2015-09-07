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
#include "wiced_result.h"
#include <string.h>
#include "nfc_host.h"
#include "wiced_platform.h"
#include "wiced_utilities.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define EVENT_MASK(evt)       ((uint16_t)(0x0001 << (evt)))

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef GKI_BUF2_SIZE
#define GKI_BUF2_SIZE               660
#endif

#define NFC_HAL_TASK            2
#define APPL_EVT_0              8
#define NFC_HAL_TASK_EVT_DATA_RDY               EVENT_MASK (APPL_EVT_0)

/**** baud rates ****/
#define USERIAL_BAUD_300          0
#define USERIAL_BAUD_600          1
#define USERIAL_BAUD_1200         2
#define USERIAL_BAUD_2400         3
#define USERIAL_BAUD_9600         4
#define USERIAL_BAUD_19200        5
#define USERIAL_BAUD_57600        6
#define USERIAL_BAUD_115200       7
#define USERIAL_BAUD_230400       8
#define USERIAL_BAUD_460800       9
#define USERIAL_BAUD_921600       10
#define USERIAL_BAUD_1M           11
#define USERIAL_BAUD_1_5M         12
#define USERIAL_BAUD_2M           13
#define USERIAL_BAUD_3M           14
#define USERIAL_BAUD_4M           15
#define USERIAL_BAUD_AUTO         16

/**** Serial Operations ****/
#define USERIAL_OP_FLUSH          0
#define USERIAL_OP_FLUSH_RX       1
#define USERIAL_OP_FLUSH_TX       2
#define USERIAL_OP_BREAK_OFF      3
#define USERIAL_OP_BREAK_ON       4
#define USERIAL_OP_BAUD_RD        5
#define USERIAL_OP_BAUD_WR        6
#define USERIAL_OP_FMT_RD         7
#define USERIAL_OP_FMT_WR         8
#define USERIAL_OP_SIG_RD         9
#define USERIAL_OP_SIG_WR         10
#define USERIAL_OP_FC_RD          11
#define USERIAL_OP_FC_WR          12
#define USERIAL_OP_CTS_AS_WAKEUP  13    /* H4IBSS */
#define USERIAL_OP_CTS_AS_FC      14    /* H4IBSS */

/******************************************************
 *                   Enumerations
 ******************************************************/

/* HCI Transport Layer Packet Type */
typedef enum
{
    HCI_UNINITIALIZED   = 0x00,
    HCI_COMMAND_PACKET  = 0x01,
    HCI_ACL_DATA_PACKET = 0x02,
    HCI_SCO_DATA_PACKET = 0x03,
    HCI_EVENT_PACKET    = 0x04,
    NCI_EVENT_PACKET    = 0x10,
    HCI_LOOPBACK_MODE   = 0xFF,
} hci_packet_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef uint8_t tUSERIAL_OP;
typedef uint8_t tUSERIAL_FEATURE;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    tUSERIAL_CBACK* p_cback;
} tUSERIAL_CB;

/* Union used to pass ioctl arguments */
typedef union
{
    uint16_t fmt;
    uint8_t  baud;
    uint8_t  fc;
    uint8_t  sigs;
#if (defined LINUX_OS) && (LINUX_OS == TRUE)
    uint16_t sco_handle;
#endif
} tUSERIAL_IOCTL_DATA;

#pragma pack(1)
typedef struct
{
    hci_packet_type_t packet_type; /* This is transport layer packet type. Not transmitted if transport bus is USB */
    uint8_t event_code;
    uint8_t content_length;
} hci_event_header_t;

typedef struct
{
    hci_packet_type_t packet_type; /* This is transport layer packet type. Not transmitted if transport bus is USB */
    uint16_t hci_handle;
    uint16_t content_length;
} hci_acl_packet_header_t;

typedef struct
{
    hci_packet_type_t packet_type; /* This is transport layer packet type. Not transmitted if transport bus is USB */
    uint16_t nci_handle;
    uint8_t content_length;
} nci_event_packet_header_t;

#pragma pack()

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

BUFFER_Q Userial_in_q;
static BT_HDR *pbuf_USERIAL_Read = NULL;
tUSERIAL_CB userial_cb;
uint8_t g_readThreadAlive = 1;

static uint32_t userial_baud_tbl[] =
{
    300, /* USERIAL_BAUD_300       */
    600, /* USERIAL_BAUD_600       */
    1200, /* USERIAL_BAUD_1200     */
    2400, /* USERIAL_BAUD_2400     */
    9600, /* USERIAL_BAUD_9600     */
    19200, /* USERIAL_BAUD_19200   */
    57600, /* USERIAL_BAUD_57600   */
    115200, /* USERIAL_BAUD_115200 */
    230400, /* USERIAL_BAUD_230400 */
    460800, /* USERIAL_BAUD_460800 */
    921600, /* USERIAL_BAUD_921600 */
    1000000, /* USERIAL_BAUD_1M    */
    1500000, /* USERIAL_BAUD_1_5M  */
    2000000, /* USERIAL_BAUD_2M    */
    3000000, /* USERIAL_BAUD_3M    */
    4000000 /* USERIAL_BAUD_4M     */
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void USERIAL_Init( void *p_cfg )
{
    GKI_init_q( &Userial_in_q );
    return;
}

void USERIAL_Open( tUSERIAL_PORT port, tUSERIAL_OPEN_CFG *p_cfg, tUSERIAL_CBACK *p_cback )
{
    wiced_result_t result;

    result = nfc_bus_init( );
    if ( result != WICED_SUCCESS )
    {
        return;
    }
}

wiced_result_t nfc_hci_transport_driver_bus_read_handler( BT_HDR* packet )
{
    hci_packet_type_t packet_type = HCI_UNINITIALIZED;
    uint8_t *current_packet;

    if ( packet == NULL )
    {
        return WICED_ERROR;
    }

    packet->offset = 0;
    packet->layer_specific = 0;
    current_packet = (uint8_t *) ( packet + 1 );

    // Read 1 byte:
    //    packet_type
    // Use a timeout here so we can shutdown the thread
    if ( nfc_bus_receive( (uint8_t*) &packet_type, 1, WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    switch ( packet_type )
    {
        case HCI_LOOPBACK_MODE:
            *current_packet++ = packet_type;

            // Read 1 byte:
            //    content_length
            if ( nfc_bus_receive( current_packet, 1, WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            packet->len = *current_packet++;

            // Read payload
            if ( nfc_bus_receive( current_packet, packet->len, WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            packet->len = packet->len + 2; // +2 for packet type + read length

            break;

        case NCI_EVENT_PACKET:
        {
            nci_event_packet_header_t header;
            if ( nfc_bus_receive( (uint8_t*) &header.nci_handle, sizeof( header ) - sizeof( packet_type ), WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            header.packet_type = packet_type;
            memcpy(current_packet , (char*)&header , sizeof( header ) );
            packet->len = header.content_length + sizeof( header ); // +3 to include sizeof: packet_type=1 event_code=1 content_length=1
            current_packet += sizeof( header );
            // Read payload
            if ( nfc_bus_receive( (uint8_t*) current_packet, (uint32_t) header.content_length, WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }
        }
        break;
        case HCI_EVENT_PACKET:
        {
            hci_event_header_t header;

            // Read 2 bytes:
            //    event_code
            //    content_length
            if ( nfc_bus_receive( (uint8_t*) &header.event_code, sizeof( header ) - sizeof( packet_type ), WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            header.packet_type = packet_type;
            *current_packet++ = packet_type;
            *current_packet++ = header.event_code;
            *current_packet++ = header.content_length;
            packet->len = header.content_length + 3; // +3 to include sizeof: packet_type=1 event_code=1 content_length=1

            // Read payload
            if ( nfc_bus_receive( (uint8_t*) current_packet, (uint32_t) header.content_length, WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            break;
        }

        case HCI_ACL_DATA_PACKET:
        {
            hci_acl_packet_header_t header;

            // Read 4 bytes:
            //    hci_handle (2 bytes)
            //    content_length (2 bytes)
            if ( nfc_bus_receive( (uint8_t*) &header.hci_handle, sizeof( header ) - sizeof( packet_type ), WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            header.packet_type = packet_type;
            *current_packet++ = packet_type;

            *current_packet = header.hci_handle;
            *current_packet += 2;

            *current_packet = header.content_length;
            *current_packet += 2;

            packet->len = header.content_length + 5; // +5 to include sizeof: packet_type=1 hci_handle=2 content_length=2

            // Read payload
            if ( nfc_bus_receive( (uint8_t*) current_packet, (uint32_t) header.content_length, WICED_NEVER_TIMEOUT ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            break;
        }

        case HCI_COMMAND_PACKET: /* Fall-through */
        default:
            return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

void USERIAL_ReadBuf(void)
{
    BT_HDR *p_buf;
    wiced_result_t ret;

    if ( ( p_buf = (BT_HDR *) GKI_getbuf( GKI_BUF2_SIZE ) ) != NULL )
    {
        ret = nfc_hci_transport_driver_bus_read_handler( p_buf );
        if ( ret == WICED_ERROR )
        {
            GKI_freebuf( p_buf );
            return;
        }
        GKI_enqueue( &Userial_in_q, p_buf );
        GKI_send_event( NFC_HAL_TASK, NFC_HAL_TASK_EVT_DATA_RDY );
    }
}


uint16_t USERIAL_Write( tUSERIAL_PORT port, uint8_t *p_data, uint16_t len )
{
    wiced_result_t result;

    result = nfc_bus_transmit( p_data, len );
    if ( result != WICED_SUCCESS )
    {
        return 0;
    }
    return len;
}


void USERIAL_Close( tUSERIAL_PORT port )
{
    wiced_result_t result;

    // Flush the queue
    do
    {
        pbuf_USERIAL_Read = (BT_HDR *) GKI_dequeue( &Userial_in_q );
        if ( pbuf_USERIAL_Read )
        {
            GKI_freebuf( pbuf_USERIAL_Read );
        }

    } while ( pbuf_USERIAL_Read );

    result = nfc_bus_deinit( );
    if ( result != WICED_SUCCESS )
    {
    }
}

uint16_t USERIAL_Read( tUSERIAL_PORT port, uint8_t* p_data, uint16_t len )
{
    uint16_t total_len = 0;
    uint16_t copy_len = 0;
    uint8_t* current_packet = NULL;

    do
    {
        if ( pbuf_USERIAL_Read != NULL )
        {
            current_packet = ( (uint8_t *) ( pbuf_USERIAL_Read + 1 ) ) + ( pbuf_USERIAL_Read->offset );

            if ( ( pbuf_USERIAL_Read->len ) <= ( len - total_len ) )
                copy_len = pbuf_USERIAL_Read->len;
            else
                copy_len = ( len - total_len );

            memcpy( ( p_data + total_len ), current_packet, copy_len );

            total_len += copy_len;

            pbuf_USERIAL_Read->offset += copy_len;
            pbuf_USERIAL_Read->len -= copy_len;

            if ( pbuf_USERIAL_Read->len == 0 )
            {
                GKI_freebuf( pbuf_USERIAL_Read );
                pbuf_USERIAL_Read = NULL;
            }
        }

        if ( pbuf_USERIAL_Read == NULL )
        {
            pbuf_USERIAL_Read = (BT_HDR *) GKI_dequeue( &Userial_in_q );
        }
    } while ( ( pbuf_USERIAL_Read != NULL ) && ( total_len < len ) );

    return total_len;
}

void USERIAL_Ioctl( tUSERIAL_PORT port, tUSERIAL_OP op, tUSERIAL_IOCTL_DATA *p_data )
{
    // Options for baud rate change
    uint8_t baudRateCmd[] = { 0x01, 0x18, 0xFC, 0x06, 0x00, 0x00, 0x00, 0x09, 0x3D, 0x00 }; // 4Mbit
    //uint8_t baudRateCmd[] = { 0x01, 0x18, 0xFC, 0x06, 0x00, 0x00, 0xC0, 0xC6, 0x2D, 0x00 }; // 3Mbit
    //uint8_t baudRateCmd[] = { 0x01, 0x18, 0xFC, 0x06, 0x00, 0x00, 0x80, 0x84, 0x1E, 0x00 }; // 2Mbit
    //uint8_t baudRateCmd[] = { 0x01, 0x18, 0xFC, 0x06, 0x00, 0x00, 0x60, 0xE3, 0x16, 0x00 }; // 1.5 Mbit
    //uint8_t baudRateCmd[] = { 0x01, 0x18, 0xFC, 0x06, 0x00, 0x00, 0x00, 0xC2, 0x01, 0x00 }; // 115200

    switch ( op )
    {
        case USERIAL_OP_FLUSH:
            break;
        case USERIAL_OP_FLUSH_RX:
            break;
        case USERIAL_OP_FLUSH_TX:
            break;
        case USERIAL_OP_BAUD_RD:
            break;
        case USERIAL_OP_BAUD_WR:
            USERIAL_Write( 0, baudRateCmd, sizeof( baudRateCmd ) );
            break;
        default:
            break;
    }

    return;
}


uint8_t USERIAL_GetBaud( uint32_t line_speed )
{
    uint8_t i;
    for ( i = USERIAL_BAUD_300; i <= USERIAL_BAUD_4M; i++ )
    {
        if ( userial_baud_tbl[i - USERIAL_BAUD_300] == line_speed )
            return i;
    }

    return USERIAL_BAUD_AUTO;
}

uint32_t USERIAL_GetLineSpeed( uint8_t baud )
{
    if ( baud <= USERIAL_BAUD_4M )
        return ( userial_baud_tbl[baud - USERIAL_BAUD_300] );
    else
        return 0;
}
