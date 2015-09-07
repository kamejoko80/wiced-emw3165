/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/**
 * @file
 *
 * Traffic generation commands
 *
 */


#include "command_console.h"
#include "wiced.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_PAYLOAD_SIZE  1500

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

static int     tcp_send_ipv4( const wiced_ip_address_t* destination, uint16_t port, const uint16_t payload_length, const int duration, const int rate, const wiced_interface_t interface );
static int     tcp_receive_ipv4( const uint16_t port, const wiced_interface_t interface );

/******************************************************
 *               Variable Definitions
 ******************************************************/

int traffic_stream_ipv4( int argc, char *argv[] )
{
    wiced_ip_address_t destination;

    int                port              = 5000;          /* Default port is 5000 */
    uint32_t           payload_length    = 1000;          /* Default packet payload length is 1000 bytes  */
    wiced_bool_t       udp_traffic       = WICED_FALSE;   /* Default traffic type is TCP */
    wiced_bool_t       client            = WICED_FALSE;   /* Default mode is server */
    int                duration          = 10;            /* Default duration */
    int                rate              = 1;             /* Default packets per second */
    int                type_of_service   = 0;             /* Default type of service */
    int                interface         = 0;             /* Default interface is STA */
    int                i                 = 0;
    int                temp[4];

    if ( argc == 1 )
    {
        return ERR_INSUFFICENT_ARGS;
    }

    i = 1;
    while ( i < argc )
    {
        switch (argv[i][1])
        {
            case 'c':
                client = WICED_TRUE;
                sscanf( argv[i+1], "%d.%d.%d.%d", &temp[0], &temp[1], &temp[2], &temp[3] );
                destination.ip.v4 = temp[0] << 24 | temp[1] << 16 | temp[2] << 8 | temp[3];
                destination.version = WICED_IPV4;
                WPRINT_APP_INFO ( ("Connecting to %u.%u.%u.%u\n", (unsigned char) ( ( destination.ip.v4 >> 24 ) & 0xff ),
                                                                  (unsigned char) ( ( destination.ip.v4 >> 16 ) & 0xff ),
                                                                  (unsigned char) ( ( destination.ip.v4 >>  8 ) & 0xff ),
                                                                  (unsigned char) ( ( destination.ip.v4 >>  0 ) & 0xff ) ) );
                i+=2;
                break;

            case 'p':
                port = atoi(argv[i+1]);
                WPRINT_APP_INFO(("Port: %d\n", port));
                i+=2;
                break;

            case 'u':
                udp_traffic = WICED_TRUE;
                WPRINT_APP_INFO(("UDP traffic\n"));
                i++;
                break;

            case 'l':
                payload_length = (uint32_t)atoi(argv[i+1]);
                if ( ( payload_length > MAX_PAYLOAD_SIZE ) || ( payload_length < 1 ) )
                {
                    WPRINT_APP_INFO(("Max payload length: %d, min: 1\n", MAX_PAYLOAD_SIZE));
                    return ERR_CMD_OK;
                }
                WPRINT_APP_INFO(("Payload length: %u\n", (unsigned int)payload_length));
                i+=2;
                break;

            case 'd':
                duration = atoi(argv[i+1]);
                if ( duration < 1 )
                {
                    WPRINT_APP_INFO(("Minimum duration in seconds: 1\n"));
                    return ERR_CMD_OK;
                }
                WPRINT_APP_INFO(("duration: %d\n", duration));
                i+=2;
                break;

            case 'r':
                rate = atoi(argv[i+1]);
                if ( rate < 1 )
                {
                    WPRINT_APP_INFO(("Minimum packets per second: 1\n"));
                    return ERR_CMD_OK;
                }
                WPRINT_APP_INFO(("rate: %d\n", rate));
                i+=2;
                break;

            case 'S':
                type_of_service = atoi(argv[i+1]);
                WPRINT_APP_INFO(("Type of service: %d\n", type_of_service));
                i+=2;
                break;

            case 'i':
                interface = atoi(argv[i+1]);
                if ( interface > 2 )
                {
                    WPRINT_APP_INFO(("0 for STA, 1 for AP, 2 for P2P interface\n"));
                    return ERR_CMD_OK;
                }
                WPRINT_APP_INFO(("interface: %d\n", interface));
                i+=2;
                break;

            default:
                WPRINT_APP_INFO(("Not supported, ignoring: %s\n", argv[i]));
                i++;
            break;
        }
    }

    if ( wwd_wifi_is_ready_to_transceive( interface ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Interface %d is not up\n", interface));
        return ERR_CMD_OK;
    }

    if ( client == WICED_TRUE )
    {
        if ( udp_traffic == WICED_FALSE )
        {
            return tcp_send_ipv4( &destination, (uint16_t)port, (uint16_t)payload_length, duration, rate, interface );
        }
    }
    else
    {
        if ( udp_traffic == WICED_FALSE )
        {
            return tcp_receive_ipv4( (uint16_t)port, interface );
        }
    }
    return ERR_CMD_OK;
}

static int tcp_send_ipv4( const wiced_ip_address_t* destination, uint16_t port, const uint16_t payload_length, const int duration, const int rate, const wiced_interface_t interface )
{
    wiced_packet_t*     packet;
    uint8_t*            data;
    uint16_t            available_space;
    uint32_t            result;
    int                 packet_interval;
    int                 number_of_packets;
    wiced_tcp_socket_t  send_socket;
    wiced_time_t        finish_time;

    packet_interval   = 1000 / rate;
    number_of_packets = duration * rate;

    /* Connect to server */
    memset( &send_socket, 0, sizeof( wiced_tcp_socket_t ) );
    if ((result = wiced_tcp_create_socket(&send_socket, interface)) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Unable to create TCP socket %x\n", (unsigned int)result));
        goto close_connection;
    }
    if ((result = wiced_tcp_bind( &send_socket, port)) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Unable to bind socket to port %x\n", (unsigned int)result));
        goto close_connection;
    }
    if ((result = wiced_tcp_connect(&send_socket, destination, port, 5000)) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Unable to connect to TCP socket %x\n", (unsigned int)result));
        goto close_connection;
    }

    finish_time = host_rtos_get_time( ) + ( duration * 1000 );

    while ( ( number_of_packets > 0 ) && ( host_rtos_get_time( ) < finish_time ) )
    {
        if ((result = wiced_packet_create_tcp( &send_socket, (uint16_t)payload_length, &packet, &data, &available_space )) != WICED_SUCCESS )
        {
            WPRINT_APP_INFO(("Unable to create TCP packet %x\n", (unsigned int)result));
            goto close_connection;
        }
        wiced_packet_set_data_end( packet, data + payload_length);
        if ((result = wiced_tcp_send_packet( &send_socket, packet )) != WICED_SUCCESS )
        {
            WPRINT_APP_INFO(("Unable to send TCP packet %x\n", (unsigned int)result));
            wiced_packet_delete( packet );
            wiced_tcp_delete_socket(&send_socket);
            wiced_rtos_delay_milliseconds( 2000 );
            /* Connect to server */
            memset( &send_socket, 0, sizeof( wiced_tcp_socket_t ) );
            if ((result = wiced_tcp_create_socket(&send_socket, interface)) != WICED_SUCCESS)
            {
                WPRINT_APP_INFO(("Unable to create TCP socket %x\n", (unsigned int)result));
                return ERR_CMD_OK;
            }
            else
            {
                if ((result = wiced_tcp_bind(&send_socket, port)) != WICED_SUCCESS)
                {
                    WPRINT_APP_INFO(("Unable to bind socket to port %x\n", (unsigned int)result));
                    wiced_tcp_delete_socket(&send_socket);
                }
                else
                {
                    if ((result = wiced_tcp_connect(&send_socket, destination, port, 5000)) != WICED_SUCCESS)
                    {
                        WPRINT_APP_INFO(("Unable to connect to TCP socket %x\n", (unsigned int)result));
                        wiced_tcp_delete_socket(&send_socket);
                    }
                }
            }
        }
        else
        {
            WPRINT_APP_INFO((".\n"));
        }
        number_of_packets--;
        wiced_rtos_delay_milliseconds( packet_interval );
    }

    /* Close the connection */
close_connection:
    wiced_tcp_delete_socket(&send_socket);

    return ERR_CMD_OK;
}

static int tcp_receive_ipv4( const uint16_t port, const wiced_interface_t interface )
{
    wiced_tcp_socket_t receive_socket;

    memset( &receive_socket, 0, sizeof( wiced_tcp_socket_t ) );
    if (wiced_tcp_create_socket( &receive_socket, interface ) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Unable to create TCP socket\n"));
        return WICED_ERROR;
    }

    if ( wiced_tcp_listen( &receive_socket, port ) != WICED_SUCCESS)
    {
        wiced_tcp_delete_socket(&receive_socket);
        WPRINT_APP_INFO(("Unable to receive on TCP socket\n"));
        return WICED_ERROR;
    }

    while ( 1 )
    {
        wiced_packet_t* temp_packet = NULL;

        /* Wait for a connection */
        wiced_result_t result = wiced_tcp_accept( &receive_socket );
        if ( result == WICED_SUCCESS )
        {
            while (wiced_tcp_receive( &receive_socket, &temp_packet, 5000 ) == WICED_SUCCESS)
            {
                WPRINT_APP_INFO((".\n"));
                wiced_packet_delete( temp_packet );
            }
        }
        wiced_tcp_disconnect( &receive_socket );
    }

    wiced_tcp_delete_socket( &receive_socket );

    return WICED_SUCCESS;
}
