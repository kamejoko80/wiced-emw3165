/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/
#include "MQTTWiced.h"
#include "MQTTClient.h"

char wiced_expired( Timer* timer )
{
    wiced_time_t current_time = host_rtos_get_time( );
    long left = timer->end_time - current_time;
    return ( left < 0 );
}

void wiced_countdown_ms( Timer* timer, unsigned int timeout )
{
    wiced_time_t current_time = host_rtos_get_time( );
    timer->end_time = current_time + timeout;
    timer->end_time = wiced_roundup( timer->end_time, 100 );
}

void wiced_countdown( Timer* timer, unsigned int timeout )
{
    wiced_time_t current_time = host_rtos_get_time( );
    timer->end_time = current_time + ( timeout * 1000 );
    timer->end_time = wiced_roundup( timer->end_time, 100 );
}

int wiced_left_ms( Timer* timer )
{
    wiced_time_t current_time = host_rtos_get_time( );
    long left = timer->end_time - current_time;
    return ( left < 0 ) ? 0 : wiced_roundup( left, 100 );
}

void wiced_init_timer( Timer* timer )
{
    timer->end_time = 0;
    timer->systick_period = 0;
}

int wiced_roundup( int numToRound, int multiple )
{
    if ( multiple == 0 )
    {
        return numToRound;
    }

    int roundDown = ( (int) ( numToRound ) / multiple ) * multiple;
    int roundUp = roundDown + multiple;
    int roundCalc = roundUp;
    return ( roundCalc );
}

wiced_result_t wiced_subscribe( Client* c )
{
    if ( MQTTYield( c, 1000 ) == MQTT_FAILURE )
    {
        return WICED_ERROR;
    }
    return WICED_SUCCESS;
}

int wiced_read( Network* n, unsigned char* buffer, int len, int timeout_ms )
{
    char* request;
    uint16_t request_length;
    wiced_result_t result;
    wiced_packet_t* rx_packet = NULL;
    uint16_t available_data_length;
    wiced_tcp_socket_t* tcp_socket = (wiced_tcp_socket_t*) &n->my_socket;
    uint32_t data_to_copy = 0;

    result = wiced_tcp_receive( tcp_socket, &rx_packet, 1000 );

    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    wiced_packet_get_data( rx_packet, 0, (uint8_t**) &request, &request_length, &available_data_length );

    data_to_copy = MIN(request_length, len);

    memcpy( buffer, request, data_to_copy );

    wiced_packet_delete( rx_packet );

    return (int) data_to_copy;
}

int wiced_write( Network* n, unsigned char* buffer, int len, int timeout_ms )
{
    wiced_result_t result;
    wiced_packet_t* tx_packet;
    char* tx_data;
    uint16_t available_data_length;
    wiced_tcp_socket_t* tcp_socket = (wiced_tcp_socket_t*) &n->my_socket;

    /* Create the TCP packet. Memory for the tx_data is automatically allocated */
    if ( wiced_packet_create_tcp( tcp_socket, MQTT_PACKET_MAX_DATA_LENGTH, &tx_packet, (uint8_t**) &tx_data, &available_data_length ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("TCP packet creation failed\n") );
        return -1;
    }

    if ( available_data_length < len )
    {
        len = available_data_length;
    }

    /* Write the message into tx_data"  */
    memcpy( tx_data, buffer, len );

    /* Set the end of the data portion */
    wiced_packet_set_data_end( tx_packet, (uint8_t*) tx_data + len );

    /* Send the TCP packet */
    result = wiced_tcp_send_packet( tcp_socket, tx_packet );

    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("TCP packet send failed %d\r\n", result) );

        /* Delete packet, since the send failed */
        wiced_packet_delete( tx_packet );
        return -1;
    }

    wiced_packet_delete( tx_packet );

    return len;
}

void wiced_disconnect( Network* n )
{
    wiced_tcp_disconnect( &n->my_socket );
    wiced_tcp_delete_socket( &n->my_socket );
}

void wiced_mqtt_init( Network* n )
{
    n->mqttread = wiced_read;
    n->mqttwrite = wiced_write;
    n->disconnect = wiced_disconnect;
}

void wiced_mqtt_buffer_init( Client* c, Network* n, unsigned char* buf, size_t buf_size, unsigned char* readbuf, size_t readbuf_size )
{
    MQTTClient( c, n, MQTT_DEFAULT_TIMEOUT, buf, buf_size, readbuf, readbuf_size );
}

wiced_result_t wiced_connectnetwork( Network* n, wiced_interface_t interface )
{
    wiced_result_t result;
    wiced_ip_address_t ip_address;
    int host_connection_retries = 0;

    if ( n->hostname != NULL )
    {
        wiced_hostname_lookup( n->hostname, &ip_address, MQTT_CLIENT_RECEIVE_TIMEOUT );
    }
    else
    {
        SET_IPV4_ADDRESS( ip_address, GET_IPV4_ADDRESS((*n->ip_address)) );
    }

    WPRINT_APP_INFO( ( "Server is at %u.%u.%u.%u\n", (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 24),
                    (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 16),
                    (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 8),
                    (uint8_t)(GET_IPV4_ADDRESS(ip_address) >> 0) ) );

    if ( wiced_tcp_create_socket( &n->my_socket, interface ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("TCP socket creation failed\n"));
        return WICED_ERROR;
    }

    wiced_tcp_bind( &n->my_socket, n->port );

    do
    {
        result = wiced_tcp_connect( &n->my_socket, &ip_address, n->port, MQTT_CLIENT_CONNECT_TIMEOUT );
        host_connection_retries++ ;
    } while ( ( result != WICED_SUCCESS ) && ( host_connection_retries < MQTT_CONNECTION_NUMBER_OF_RETRIES ) );

    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Unable to connect to the server! Halt %d\r\n", result) );
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t wiced_mqtt_connect( Client *c, char *client_id, char *username, char *passwd )
{
    int rc = 0;

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.willFlag = 0;
    data.MQTTVersion = 3;
    data.clientID.cstring = client_id;
    data.username.cstring = username;
    data.password.cstring = passwd;

    data.keepAliveInterval = 1000;
    data.cleansession = 1;

    rc = MQTTConnect( c, &data );

    return ( rc == MQTT_FAILURE ) ? WICED_SUCCESS : WICED_ERROR;
}

wiced_result_t wiced_mqtt_subscribe( Client *c, char *topic_id, messageHandler msg )
{
    int rc = 0;

    rc = MQTTSubscribe( c, topic_id, QOS0, msg );

    WPRINT_APP_INFO ( ( "Subscribed %d\n", rc ) );

    return ( rc == MQTT_FAILURE ) ? WICED_SUCCESS : WICED_ERROR;
}

wiced_result_t wiced_mqtt_publish( Client *c, char *topic_id, MQTTMessage *msg )
{
    int rc = 0;

    rc = MQTTPublish( c, topic_id, msg );

    WPRINT_APP_INFO ( ( "Publisher %d\n", rc ) );

    return ( rc == MQTT_FAILURE ) ? WICED_SUCCESS : WICED_ERROR;
}
