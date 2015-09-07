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
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include "MQTTWiced.h"
#include "MQTTClient.h"

void NewMessageData( MessageData* md, MQTTString* aTopicName, MQTTMessage* aMessgage )
{
    md->topicName = aTopicName;
    md->message = aMessgage;
}

int getNextPacketId( Client *c )
{
    return c->next_packetid = ( c->next_packetid == MAX_PACKET_ID ) ? 1 : c->next_packetid + 1;
}

int sendPacket( Client* c, int length, Timer* timer )
{
    int rc = MQTT_FAILURE, sent = 0;

    while ( sent < length && !wiced_expired( timer ) )
    {
        rc = c->ipstack->mqttwrite( c->ipstack, &c->buf[ sent ], length, wiced_left_ms( timer ) );
        if ( rc < 0 )
        {
            break;
        }

        sent += rc;
    }

    if ( sent == length )
    {
        wiced_countdown( &c->ping_timer, c->keepAliveInterval ); // record the fact that we have successfully sent the packet
        rc = MQTT_SUCCESS;
    }
    else
    {
        rc = MQTT_FAILURE;
    }

    return rc;
}

void MQTTClient( Client* c, Network* network, unsigned int command_timeout_ms, unsigned char* buf, size_t buf_size, unsigned char* readbuf, size_t readbuf_size )
{
    int i;
    c->ipstack = network;

    for ( i = 0; i < MAX_MESSAGE_HANDLERS; ++i )
    {
        c->messageHandlers[ i ].topicFilter = 0;
    }

    c->command_timeout_ms = command_timeout_ms;
    c->buf = buf;
    c->buf_size = buf_size;
    c->readbuf = readbuf;
    c->readbuf_size = readbuf_size;
    c->isconnected = 0;
    c->ping_outstanding = 0;
    c->defaultMessageHandler = NULL;
    wiced_init_timer( &c->ping_timer );
}

int decodePacket( Client* c, int* value, int timeout )
{
    unsigned char i;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if ( ++len > MAX_NO_OF_REMAINING_LENGTH_BYTES )
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = c->ipstack->mqttread( c->ipstack, &i, 1, timeout );
        if ( rc != 1 )
        {
            goto exit;
        }

        *value += ( i & 127 ) * multiplier;

        multiplier *= 128;

    } while ( ( i & 128 ) != 0 );

    exit: return len;
}

int readPacket( Client* c, Timer* timer )
{
    int rc = MQTT_FAILURE;
    MQTTHeader header =
    { 0 };
    int len = 0;

    len = c->ipstack->mqttread( c->ipstack, c->readbuf, c->readbuf_size, wiced_left_ms( timer ) );

    if ( len < 0 )
    {
        goto exit;
    }

    header.byte = c->readbuf[ 0 ];

    rc = header.bits.type;

    exit: return rc;
}

// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
char isTopicMatched( char* topicFilter, MQTTString* topicName )
{
    char* curf = topicFilter;
    char* curn = topicName->lenstring.data;
    char* curn_end = curn + topicName->lenstring.len;

    while ( *curf && curn < curn_end )
    {
        if ( *curn == '/' && *curf != '/' )
        {
            break;
        }

        if ( *curf != '+' && *curf != '#' && *curf != *curn )
        {
            break;
        }

        if ( *curf == '+' )
        { // skip until we meet the next separator, or end of string
            char* nextpos = curn + 1;
            while ( nextpos < curn_end && *nextpos != '/' )
            {
                nextpos = ++curn + 1;
            }
        }
        else if ( *curf == '#' )
        {
            curn = curn_end - 1; // skip until end of string
        }

        curf++ ;
        curn++ ;
    };

    return ( curn == curn_end ) && ( *curf == '\0' );
}

int deliverMessage( Client* c, MQTTString* topicName, MQTTMessage* message )
{
    int i;
    int rc = MQTT_FAILURE;

    // we have to find the right message handler - indexed by topic
    for ( i = 0; i < MAX_MESSAGE_HANDLERS; ++i )
    {
        if ( c->messageHandlers[ i ].topicFilter != 0 && ( MQTTPacket_equals( topicName, (char*) c->messageHandlers[ i ].topicFilter ) || isTopicMatched( (char*) c->messageHandlers[ i ].topicFilter, topicName ) ) )
        {
            if ( c->messageHandlers[ i ].fp != NULL )
            {
                MessageData md;
                NewMessageData( &md, topicName, message );
                c->messageHandlers[ i ].fp( &md );
                rc = MQTT_SUCCESS;
            }
        }
    }

    if ( rc == MQTT_FAILURE && c->defaultMessageHandler != NULL )
    {
        MessageData md;
        NewMessageData( &md, topicName, message );
        c->defaultMessageHandler( &md );
        rc = MQTT_SUCCESS;
    }

    return rc;
}

int keepalive( Client* c )
{
    int rc = MQTT_FAILURE;

    if ( c->keepAliveInterval == 0 )
    {
        rc = MQTT_SUCCESS;
        goto exit;
    }

    if ( wiced_expired( &c->ping_timer ) )
    {
        if ( !c->ping_outstanding )
        {
            Timer timer;
            wiced_init_timer( &timer );
            wiced_countdown_ms( &timer, 1000 );
            int len = MQTTSerialize_pingreq( c->buf, c->buf_size );
            if ( len > 0 && ( rc = sendPacket( c, len, &timer ) ) == MQTT_SUCCESS ) // send the ping packet
            {
                c->ping_outstanding = 1;
            }
        }
    }

    exit: return rc;
}

int cycle( Client* c, Timer* timer )
{
    // read the socket, see what work is due
    unsigned short packet_type = readPacket( c, timer );
    int len = 0;
    int rc = MQTT_SUCCESS;

    WPRINT_APP_INFO( ("Packet Type %d\n", packet_type) );

    switch ( packet_type )
    {
        case CONNACK:
        case PUBACK:
        case SUBACK:
            break;
        case PUBLISH:
        {
            MQTTString topicName;
            MQTTMessage msg;

            if ( MQTTDeserialize_publish( (unsigned char*) &msg.dup, (int*) &msg.qos, (unsigned char*) &msg.retained, (unsigned short*) &msg.id, &topicName, (unsigned char**) &msg.payload, (int*) &msg.payloadlen, c->readbuf, c->readbuf_size ) != 1 )
            {
                goto exit;
            }

            deliverMessage( c, &topicName, &msg );
            if ( msg.qos != QOS0 )
            {
                if ( msg.qos == QOS1 )
                {
                    len = MQTTSerialize_ack( c->buf, c->buf_size, PUBACK, 0, msg.id );
                }
                else if ( msg.qos == QOS2 )
                {
                    len = MQTTSerialize_ack( c->buf, c->buf_size, PUBREC, 0, msg.id );
                }
                if ( len <= 0 )
                {
                    rc = MQTT_FAILURE;
                }
                else
                {
                    rc = sendPacket( c, len, timer );
                }

                if ( rc == MQTT_FAILURE )
                {
                    goto exit;
                    // there was a problem
                }
            }
            break;
        }
        case PUBREC:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if ( MQTTDeserialize_ack( &type, &dup, &mypacketid, c->readbuf, c->readbuf_size ) != 1 )
            {
                rc = MQTT_FAILURE;
            }
            else if ( ( len = MQTTSerialize_ack( c->buf, c->buf_size, PUBREL, 0, mypacketid ) ) <= 0 )
            {
                rc = MQTT_FAILURE;
            }
            else if ( ( rc = sendPacket( c, len, timer ) ) != MQTT_SUCCESS ) // send the PUBREL packet
            {
                rc = MQTT_FAILURE; // there was a problem
            }

            if ( rc == MQTT_FAILURE )
            {
                goto exit;
                // there was a problem
            }

            break;
        }
        case PUBCOMP:
            break;
        case PINGRESP:
            c->ping_outstanding = 0;
            break;
    }

    keepalive( c );

    exit: if ( rc == MQTT_SUCCESS )
    {
        rc = packet_type;
    }

    return rc;
}

int MQTTYield( Client* c, int timeout_ms )
{
    int rc = MQTT_SUCCESS;
    Timer timer;

    wiced_init_timer( &timer );
    wiced_countdown_ms( &timer, timeout_ms );

    if ( cycle( c, &timer ) == MQTT_FAILURE )
    {
        rc = MQTT_FAILURE;
    }

    return rc;
}

// only used in single-threaded mode where one command at a time is in process
int waitfor( Client* c, int packet_type, Timer* timer )
{
    int rc = MQTT_FAILURE;

    do
    {
        if ( wiced_expired( timer ) )
        {
            break; // we timed out
        }
    } while ( ( rc = cycle( c, timer ) ) != packet_type );

    return rc;
}

int MQTTConnect( Client* c, MQTTPacket_connectData* options )
{
    Timer connect_timer;
    int rc = MQTT_FAILURE;

    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
    int len = 0;

    wiced_init_timer( &connect_timer );
    wiced_countdown_ms( &connect_timer, c->command_timeout_ms );

    if ( c->isconnected ) // don't send connect packet again if we are already connected
    {
        goto exit;
    }

    if ( options == 0 )
    {
        options = &default_options; // set default options if none were supplied
    }

    c->keepAliveInterval = options->keepAliveInterval;
    wiced_countdown( &c->ping_timer, c->keepAliveInterval );

    if ( ( len = MQTTSerialize_connect( c->buf, c->buf_size, options ) ) <= 0 )
    {
        goto exit;
    }

    if ( ( rc = sendPacket( c, len, &connect_timer ) ) != MQTT_SUCCESS ) // send the connect packet
    {
        goto exit;
        // there was a problem
    }

    // this will be a blocking call, wait for the connack
    if ( waitfor( c, CONNACK, &connect_timer ) == CONNACK )
    {
        unsigned char connack_rc = 255;
        char sessionPresent = 0;

        if ( MQTTDeserialize_connack( (unsigned char*) &sessionPresent, &connack_rc, c->readbuf, c->readbuf_size ) == 1 )
        {
            rc = connack_rc;
        }
        else
        {
            rc = MQTT_FAILURE;
        }
    }
    else
    {
        rc = MQTT_FAILURE;
    }

    exit:

    if ( rc != MQTT_FAILURE )
    {
        c->isconnected = 1;
    }

    return rc;
}

int MQTTSubscribe( Client* c, const char* topicFilter, enum QoS qos, messageHandler messageHandler )
{
    int rc = MQTT_FAILURE;
    int len = 0;
    Timer timer;

    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *) topicFilter;

    wiced_init_timer( &timer );
    wiced_countdown_ms( &timer, c->command_timeout_ms );

    if ( !c->isconnected )
    {
        goto exit;
    }

    len = MQTTSerialize_subscribe( c->buf, c->buf_size, 0, getNextPacketId( c ), 1, &topic, (int*) &qos );
    if ( len <= 0 )
    {
        goto exit;
    }

    if ( ( rc = sendPacket( c, len, &timer ) ) != MQTT_SUCCESS ) // send the subscribe packet
    {
        goto exit;
        // there was a problem
    }

    if ( waitfor( c, SUBACK, &timer ) == SUBACK ) // wait for suback
    {
        int count = 0, grantedQoS = -1;
        unsigned short mypacketid;

        if ( MQTTDeserialize_suback( &mypacketid, 1, &count, &grantedQoS, c->readbuf, c->readbuf_size ) == 1 )
        {
            rc = grantedQoS; // 0, 1, 2 or 0x80
        }

        if ( rc != 0x80 )
        {
            int i;
            for ( i = 0; i < MAX_MESSAGE_HANDLERS; ++i )
            {
                if ( c->messageHandlers[ i ].topicFilter == 0 )
                {
                    c->messageHandlers[ i ].topicFilter = topicFilter;
                    c->messageHandlers[ i ].fp = messageHandler;
                    rc = 0;
                    break;
                }
            }
        }
    }
    else
    {
        rc = MQTT_FAILURE;
    }

    exit: return rc;
}

int MQTTUnsubscribe( Client* c, const char* topicFilter )
{
    int rc = MQTT_FAILURE;
    int len = 0;
    MQTTString topic = MQTTString_initializer;
    Timer timer;

    topic.cstring = (char *) topicFilter;

    wiced_init_timer( &timer );
    wiced_countdown_ms( &timer, c->command_timeout_ms );

    if ( !c->isconnected )
    {
        goto exit;
    }

    if ( ( len = MQTTSerialize_unsubscribe( c->buf, c->buf_size, 0, getNextPacketId( c ), 1, &topic ) ) <= 0 )
    {
        goto exit;
    }

    if ( ( rc = sendPacket( c, len, &timer ) ) != MQTT_SUCCESS ) // send the subscribe packet
    {
        goto exit;
        // there was a problem
    }

    if ( waitfor( c, UNSUBACK, &timer ) == UNSUBACK )
    {
        unsigned short mypacketid; // should be the same as the packetid above
        if ( MQTTDeserialize_unsuback( &mypacketid, c->readbuf, c->readbuf_size ) == 1 )
        {
            rc = 0;
        }
    }
    else
    {
        rc = MQTT_FAILURE;
    }

    exit: return rc;
}

int MQTTPublish( Client* c, const char* topicName, MQTTMessage* message )
{
    int rc = MQTT_FAILURE;
    Timer timer;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *) topicName;
    int len = 0;

    wiced_init_timer( &timer );
    wiced_countdown_ms( &timer, c->command_timeout_ms );

    if ( !c->isconnected )
    {
        goto exit;
    }

    if ( message->qos == QOS1 || message->qos == QOS2 )
    {
        message->id = getNextPacketId( c );
    }

    len = MQTTSerialize_publish( c->buf, c->buf_size, 0, message->qos, message->retained, message->id, topic, (unsigned char*) message->payload, message->payloadlen );

    if ( len <= 0 )
    {
        goto exit;
    }

    if ( ( rc = sendPacket( c, len, &timer ) ) != MQTT_SUCCESS ) // send the subscribe packet
    {
        goto exit;
        // there was a problem
    }

    if ( message->qos == QOS1 )
    {
        if ( waitfor( c, PUBACK, &timer ) == PUBACK )
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if ( MQTTDeserialize_ack( &type, &dup, &mypacketid, c->readbuf, c->readbuf_size ) != 1 )
            {
                rc = MQTT_FAILURE;
            }
        }
        else
        {
            rc = MQTT_FAILURE;
        }
    }
    else if ( message->qos == QOS2 )
    {
        if ( waitfor( c, PUBCOMP, &timer ) == PUBCOMP )
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if ( MQTTDeserialize_ack( &type, &dup, &mypacketid, c->readbuf, c->readbuf_size ) != 1 )
            {
                rc = MQTT_FAILURE;
            }
        }
        else
        {
            rc = MQTT_FAILURE;
        }
    }

    exit: return rc;
}

int MQTTDisconnect( Client* c )
{
    int rc = MQTT_FAILURE;
    Timer timer; // we might wait for incomplete incoming publishes to complete
    int len = MQTTSerialize_disconnect( c->buf, c->buf_size );

    wiced_init_timer( &timer );
    wiced_countdown_ms( &timer, c->command_timeout_ms );

    if ( len > 0 )
    {
        rc = sendPacket( c, len, &timer ); // send the disconnect packet
    }
    c->isconnected = 0;

    return rc;
}
