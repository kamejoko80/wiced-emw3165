/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */


#include "wiced_tcpip.h"
#include "wiced_network.h"
#include "wiced_tls.h"
#include "internal/wiced_internal_api.h"
#include "wwd_assert.h"
#include "dns.h"

wiced_result_t wiced_tcp_send_packet( wiced_tcp_socket_t* socket, wiced_packet_t* packet )
{
#ifndef WICED_DISABLE_TLS
    if ( socket->tls_context != NULL )
    {
        wiced_tls_encrypt_packet( &socket->tls_context->context, packet );
    }
#endif /* ifndef WICED_DISABLE_TLS */

    return network_tcp_send_packet( socket, packet );
}

wiced_result_t wiced_tcp_send_buffer( wiced_tcp_socket_t* socket, const void* buffer, uint16_t buffer_length )
{
    wiced_packet_t* packet = NULL;
    uint8_t* data_ptr = (uint8_t*)buffer;
    uint8_t* packet_data_ptr;
    uint16_t available_space;

    wiced_assert("Bad args", socket != NULL);

    WICED_LINK_CHECK_TCP_SOCKET( socket );

    /* Create a packet, copy the data and send it off */
    while ( buffer_length != 0 )
    {
        uint16_t data_to_write;
        wiced_result_t result = wiced_packet_create_tcp( socket, buffer_length, &packet, &packet_data_ptr, &available_space );
        if ( result != WICED_TCPIP_SUCCESS )
        {
            return result;
        }

        /* Write data */
        data_to_write   = MIN(buffer_length, available_space);
        packet_data_ptr = MEMCAT(packet_data_ptr, data_ptr, data_to_write);

        wiced_packet_set_data_end( packet, packet_data_ptr );

        /* Update variables */
        data_ptr       += data_to_write;
        buffer_length   = (uint16_t) ( buffer_length - data_to_write );
        available_space = (uint16_t) ( available_space - data_to_write );

        result = wiced_tcp_send_packet( socket, packet );
        if ( result != WICED_TCPIP_SUCCESS )
        {
            wiced_packet_delete( packet );
            return result;
        }
    }

    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_tcp_receive( wiced_tcp_socket_t* socket, wiced_packet_t** packet, uint32_t timeout )
{
    wiced_assert("Bad args", (socket != NULL) && (packet != NULL) );

    WICED_LINK_CHECK_TCP_SOCKET( socket );

#ifndef WICED_DISABLE_TLS
    if ( socket->tls_context != NULL )
    {
        return wiced_tls_receive_packet( socket, packet, timeout );
    }
    else
#endif /* ifndef WICED_DISABLE_TLS */
    {
        return network_tcp_receive( socket, packet, timeout );
    }
}

wiced_result_t wiced_tcp_stream_init( wiced_tcp_stream_t* tcp_stream, wiced_tcp_socket_t* socket )
{
    tcp_stream->tx_packet                 = NULL;
    tcp_stream->rx_packet                 = NULL;
    tcp_stream->socket                    = socket;
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_tcp_stream_deinit( wiced_tcp_stream_t* tcp_stream )
{
    /* Flush the TX */
    wiced_tcp_stream_flush( tcp_stream );

    /* Flush the RX */
    if ( tcp_stream->rx_packet != NULL )
    {
        wiced_packet_delete( tcp_stream->rx_packet );
        tcp_stream->rx_packet = NULL;
    }
    tcp_stream->tx_packet = NULL;
    tcp_stream->rx_packet = NULL;
    tcp_stream->socket    = NULL;

    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_tcp_stream_write( wiced_tcp_stream_t* tcp_stream, const void* data, uint32_t data_length )
{
    wiced_assert("Bad args", tcp_stream != NULL);

    WICED_LINK_CHECK_TCP_SOCKET( tcp_stream->socket );

    while ( data_length != 0 )
    {
        uint16_t amount_to_write;

        /* Check if we don't have a packet */
        if ( tcp_stream->tx_packet == NULL )
        {
            wiced_result_t result;
            result = wiced_packet_create_tcp( tcp_stream->socket, (uint16_t) MIN( data_length, 0xffff ), &tcp_stream->tx_packet, &tcp_stream->tx_packet_data , &tcp_stream->tx_packet_space_available );
            if ( result != WICED_TCPIP_SUCCESS )
            {
                return result;
            }
        }

        /* Write data */
        amount_to_write = (uint16_t) MIN( data_length, tcp_stream->tx_packet_space_available );
        tcp_stream->tx_packet_data     = MEMCAT( tcp_stream->tx_packet_data, data, amount_to_write );

        /* Update variables */
        data_length                           = (uint16_t)(data_length - amount_to_write);
        tcp_stream->tx_packet_space_available = (uint16_t) ( tcp_stream->tx_packet_space_available - amount_to_write );
        data                                  = (void*)((uint32_t)data + amount_to_write);

        /* Check if the packet is full */
        if ( tcp_stream->tx_packet_space_available == 0 )
        {
            wiced_result_t result;

            /* Send the packet */
            wiced_packet_set_data_end( tcp_stream->tx_packet, (uint8_t*)tcp_stream->tx_packet_data );
            result = wiced_tcp_send_packet( tcp_stream->socket, tcp_stream->tx_packet );

            tcp_stream->tx_packet_data            = NULL;
            tcp_stream->tx_packet_space_available = 0;

            if ( result != WICED_TCPIP_SUCCESS )
            {
                wiced_packet_delete( tcp_stream->tx_packet );
                tcp_stream->tx_packet = NULL;
                return result;
            }

            tcp_stream->tx_packet = NULL;
        }
    }

    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_tcp_stream_read( wiced_tcp_stream_t* tcp_stream, void* buffer, uint16_t buffer_length, uint32_t timeout )
{
    return wiced_tcp_stream_read_with_count( tcp_stream, buffer, buffer_length, timeout, NULL );
}

wiced_result_t wiced_tcp_stream_read_with_count( wiced_tcp_stream_t* tcp_stream, void* buffer, uint16_t buffer_length, uint32_t timeout, uint32_t* read_count )
{
    WICED_LINK_CHECK_TCP_SOCKET( tcp_stream->socket );

    if ( read_count != NULL )
    {
        *read_count = 0;
    }

    while ( buffer_length != 0 )
    {
        uint16_t amount_to_read;
        uint16_t total_available;
        uint8_t* packet_data     = NULL;
        uint16_t space_available = 0;

        /* Check if we don't have a packet */
        if (tcp_stream->rx_packet == NULL)
        {
            wiced_result_t result;
            result = wiced_tcp_receive(tcp_stream->socket, &tcp_stream->rx_packet, timeout );
            if ( result != WICED_TCPIP_SUCCESS)
            {
                if ( ( read_count != NULL ) && ( *read_count != 0 ) )
                {
                    result = WICED_TCPIP_SUCCESS;
                }
                return result;
            }
        }

        wiced_packet_get_data(tcp_stream->rx_packet, 0, &packet_data, &space_available, &total_available);

        /* Read data */
        amount_to_read = MIN(buffer_length, space_available);
        buffer         = MEMCAT((uint8_t*)buffer, packet_data, amount_to_read);

        if ( read_count != NULL )
        {
            *read_count += amount_to_read;
        }

        /* Update buffer length */
        buffer_length = (uint16_t)(buffer_length - amount_to_read);

        /* Check if we need a new packet */
        if ( amount_to_read == space_available )
        {
            wiced_packet_delete( tcp_stream->rx_packet );
            tcp_stream->rx_packet = NULL;
        }
        else
        {
            /* Otherwise update the start of the data for the next read request */
            wiced_packet_set_data_start(tcp_stream->rx_packet, packet_data + amount_to_read);
        }
    }
    return WICED_TCPIP_SUCCESS;
}

wiced_result_t wiced_tcp_stream_flush( wiced_tcp_stream_t* tcp_stream )
{
    wiced_result_t result = WICED_TCPIP_SUCCESS;

    wiced_assert("Bad args", tcp_stream != NULL);

    WICED_LINK_CHECK_TCP_SOCKET( tcp_stream->socket );

    /* Check if there is a packet to send */
    if ( tcp_stream->tx_packet != NULL )
    {
        wiced_packet_set_data_end(tcp_stream->tx_packet, tcp_stream->tx_packet_data);
        result = wiced_tcp_send_packet( tcp_stream->socket, tcp_stream->tx_packet );

        tcp_stream->tx_packet_data            = NULL;
        tcp_stream->tx_packet_space_available = 0;
        if ( result != WICED_TCPIP_SUCCESS )
        {
            wiced_packet_delete( tcp_stream->tx_packet );
        }

        tcp_stream->tx_packet = NULL;
    }
    return result;
}

wiced_result_t wiced_hostname_lookup( const char* hostname, wiced_ip_address_t* address, uint32_t timeout_ms )
{
    wiced_assert("Bad args", (hostname != NULL) && (address != NULL));

    WICED_LINK_CHECK( WICED_STA_INTERFACE );

    /* Check if address is a string representation of a IPv4 or IPv6 address i.e. xxx.xxx.xxx.xxx */
    if ( str_to_ip( hostname, address ) == 0 )
    {
        /* yes this is a string representation of a IPv4/6 address */
        return WICED_TCPIP_SUCCESS;
    }

    return dns_client_hostname_lookup( hostname, address, timeout_ms );
}
