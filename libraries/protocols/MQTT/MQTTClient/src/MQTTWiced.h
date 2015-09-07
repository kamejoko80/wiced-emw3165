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

#ifndef __MQTT_WICED_C_
#define __MQTT_WICED_C_

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdio.h"
#include "wiced.h"
#include "MQTTPacket.h"

/******************************************************
 *                    Constants
 ******************************************************/
#define MAX_PACKET_ID                       65535
#define MAX_MESSAGE_HANDLERS                5
#define MQTT_PACKET_MAX_DATA_LENGTH         100
#define MQTT_CLIENT_INTERVAL                2
#define MQTT_BROKER_PORT                    1883
#define MQTT_SECURE_BROKER_PORT             8000
#define MQTT_CLIENT_CONNECT_TIMEOUT         25000
#define MQTT_CLIENT_RECEIVE_TIMEOUT         5000
#define MQTT_CONNECTION_NUMBER_OF_RETRIES   5
#define MQTT_DEFAULT_TIMEOUT                1000

enum QoS
{
    QOS0, QOS1, QOS2
};

// all failure return codes must be negative
enum returnCode
{
    MQTT_BUFFER_OVERFLOW = -2, MQTT_FAILURE = -1, MQTT_SUCCESS = 0
};

typedef struct
{
        unsigned long systick_period;
        unsigned long end_time;
} Timer;

struct Network;
typedef struct Network
{
        //wiced_tcp_socket_t      *my_socket;
        char *hostname;
        wiced_ip_address_t *ip_address;
        int port;

        int (*mqttread)( struct Network*, unsigned char*, int, int );
        int (*mqttwrite)( struct Network*, unsigned char*, int, int );
        void (*disconnect)( struct Network* );
        wiced_tcp_socket_t my_socket;
} Network;
void NewTimer( Timer* );

typedef struct MQTTMessage MQTTMessage;
typedef struct MessageData MessageData;
struct MQTTMessage
{
        enum QoS qos;
        char retained;
        char dup;
        unsigned short id;
        void *payload;
        size_t payloadlen;
};

struct MessageData
{
        MQTTMessage* message;
        MQTTString* topicName;
};

typedef void (*messageHandler)( MessageData* );
typedef struct Client Client;

struct Client
{
        unsigned int next_packetid;
        unsigned int command_timeout_ms;
        size_t buf_size;
        size_t readbuf_size;
        unsigned char *buf;
        unsigned char *readbuf;
        unsigned int keepAliveInterval;
        char ping_outstanding;
        int isconnected;

        struct MessageHandlers
        {
                const char* topicFilter;
                void (*fp)( MessageData* );
        } messageHandlers[ MAX_MESSAGE_HANDLERS ]; // Message handlers are indexed by subscription topic

        void (*defaultMessageHandler)( MessageData* );

        Network* ipstack;
        Timer ping_timer;
};

#define DefaultClient {0, 0, 0, 0, NULL, NULL, 0, 0, 0}

/******************************************************
 *                   WICED APIs
 ******************************************************/

wiced_result_t wiced_connectnetwork( Network* n, wiced_interface_t interface );
void wiced_mqtt_init( Network* n );
void wiced_disconnect( Network* n );
wiced_result_t wiced_subscribe( Client* c );
int wiced_read( Network* n, unsigned char* buffer, int len, int timeout_ms );
int wiced_write( Network* n, unsigned char* buffer, int len, int timeout_ms );
void wiced_mqtt_buffer_init( Client*, Network*, unsigned char*, size_t, unsigned char*, size_t );
wiced_result_t wiced_mqtt_connect( Client *c, char *client_id, char *username, char *passwd );
wiced_result_t wiced_mqtt_subscribe( Client *c, char *topic_id, messageHandler msg );
wiced_result_t wiced_mqtt_publish( Client *c, char *topic_id, MQTTMessage *msg );

#ifdef __cplusplus
} /* extern "C" */

#endif

#endif
