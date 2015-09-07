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

#include <stddef.h>
#include "wwd_buffer_interface.h"
#include "wwd_debug.h"

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
void* osl_pktget( void *osh, unsigned int len, unsigned char send );
void osl_pktfree(void *osh, void* p, unsigned char send);
void* bcm439x_packet_data(void* p);
int bcm439x_packet_length(void* p);
int bcm439x_packet_set_length(void* p, uint32_t length);
int bcm439x_packet_add_remove_at_front(void** p, int32_t bytes);

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

void* osl_pktget( void *osh, unsigned int len, unsigned char send )
{
    wiced_buffer_t buffer = NULL;
    wwd_result_t result;
    wwd_buffer_dir_t dir = send ? WWD_NETWORK_TX : WWD_NETWORK_RX;

    UNUSED_PARAMETER(osh);

    result = host_buffer_get( &buffer, dir, (unsigned short) ( (uint16_t) len ), WICED_FALSE );

    if ( result == WWD_SUCCESS )
    {
        host_buffer_add_remove_at_front( &buffer, sizeof(wwd_buffer_header_t) - sizeof(wwd_bus_header_t) );
        return (void *) buffer;
    }
    else
    {
        WPRINT_PLATFORM_DEBUG( ( "PKTGET failed!!\n" ) );
        return NULL;
    }
}


void osl_pktfree(void *osh, void* p, unsigned char send)
{
    wwd_buffer_dir_t dir = send ? WWD_NETWORK_TX : WWD_NETWORK_RX;

    UNUSED_PARAMETER(osh);

    /* This is to compensate for host_buffer_add_remove_at_front() in wwd_bus_send_buffer.
     * For NetX/NetX_Duo TCP, need to ensure that the prepend_ptr is set back to the beginning of
     * the IP packet because the packet isn't freed immediately, but is reused by NetX/NetX_Duo in
     * case TCP re-transmission is required.
     */
    host_buffer_add_remove_at_front( (wiced_buffer_t*) &p, -(int32_t) sizeof(wwd_buffer_header_t) );

    host_buffer_release((wiced_buffer_t)p, dir);
}

void* bcm439x_packet_data(void* p)
{
    return host_buffer_get_current_piece_data_pointer((wiced_buffer_t)p);
}

int bcm439x_packet_length(void* p)
{
    return host_buffer_get_current_piece_size((wiced_buffer_t)p);
}

int bcm439x_packet_set_length(void* p, uint32_t length)
{
    return host_buffer_set_size((wiced_buffer_t)p, (unsigned short)length);
}

int bcm439x_packet_add_remove_at_front(void** p, int32_t bytes)
{
    return host_buffer_add_remove_at_front((wiced_buffer_t*)p, bytes);
}
