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
 * MQTT Subscriber Sample Application
 *
 * Features demonstrated
 *  - MQTT protocol library
 *
 * This application snippet demonstrates how to use the MQTT library to subscribe to a topic and regularly check for the latest data
 *
 * Application Instructions
 *   Connect a PC terminal to the USB port of the WICED Eval board,
 *   then build and download the application as described in the WICED Quick Start Guide
 *
 *   Quick MQTT Setup Guide
 *   Install Mosquito MQTT Server on PC
 *   Make sure mosquitto server started (go to services->start mosquitto service
 *   Run mosquitto command -> mosquitto -v, this will enable mosquitto broker
 *
 *   Use TT3 Windows application (https://github.com/francoisvdm/TT3) or Mosquitto publisher (mosquitto_pub.exe) to publish the topic which you select
 *   TOPIC name defined under "TOPIC_NAME"
 *
 *   Start WICED Application making sure the WICED device and the Mosquitto server are in same domain
 *   Now Subscriber update the publisher topic
 *
 *   Public brokers reference - https://github.com/mqtt/mqtt.github.io/wiki/public_brokers
 *   List of Public brokers
 *      broker.mqttdashboard.com:1883
 *      iot.eclipse.org:1883
 *      test.mosquitto.org:1883
 *      dev.rabbitmq.com:1883
 *      q.m2m.io:1883
 *      www.cloudmqtt.com:18443
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "MQTTWiced.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MQTT_SERVER_THREAD_PRIORITY          (WICED_DEFAULT_LIBRARY_PRIORITY)
#define MQTT_SERVER_STACK_SIZE               (6200)
#define MQTT_TARGET_IP                       MAKE_IPV4_ADDRESS(192,168,0,2)
#define MQTT_CLIENT_ID                       "wiced"
#define MQTT_TOPIC_NAME                      "wiced"
#define MQTT_USERNAME                        ""
#define MQTT_PASSWD                          ""

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

static void messageArrived( MessageData* md );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static const wiced_ip_setting_t ap_ip_settings =
{ INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168, 0, 1 ) ), INITIALISER_IPV4_ADDRESS( .netmask, MAKE_IPV4_ADDRESS( 255,255,255, 0 ) ), INITIALISER_IPV4_ADDRESS( .gateway, MAKE_IPV4_ADDRESS( 192,168, 0, 1 ) ), };

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    Network mqtt_network;
    Client mqtt_client;
    unsigned char buf[ 100 ];
    unsigned char readbuf[ 100 ];

    wiced_ip_address_t INITIALISER_IPV4_ADDRESS( ip_address, MQTT_TARGET_IP );

    wiced_init( );

    /* Initialize MQTT */
    wiced_mqtt_init( &mqtt_network );

    wiced_mqtt_buffer_init( &mqtt_client, &mqtt_network, buf, 100, readbuf, 100 );

    mqtt_network.hostname = "broker.mqttdashboard.com";
    mqtt_network.ip_address = &ip_address;
    mqtt_network.port = MQTT_BROKER_PORT;

    /* Bring up the network interface */
    wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );

    if ( wiced_connectnetwork( &mqtt_network, WICED_STA_INTERFACE ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("connection failed\n") );
        return;
    }

    wiced_mqtt_connect( &mqtt_client, MQTT_CLIENT_ID, NULL, NULL );

    wiced_mqtt_subscribe( &mqtt_client, MQTT_TOPIC_NAME, messageArrived );

    while ( 1 )
    {
        wiced_subscribe( &mqtt_client );
        wiced_rtos_delay_milliseconds( 500 );
    }
}

void messageArrived( MessageData* md )
{
    MQTTMessage* message = md->message;
    WPRINT_APP_INFO ( ("Message Subscribed %d \r\n", message->payloadlen));WPRINT_APP_INFO ( ("Message dup %x \r\n", message->dup));WPRINT_APP_INFO ( ("Message id %x \r\n", message->id));WPRINT_APP_INFO ( ("Message qos %x \r\n", message->qos));WPRINT_APP_INFO ( ("Message retained %x \r\n", message->retained));

    WPRINT_APP_INFO( ( "Topic Name ==> %.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data ) );WPRINT_APP_INFO( ( "Message ==> %.*s\n", (int)message->payloadlen, (char*)message->payload ) );

    if ( strstr( (char*) message->payload, "ON" ) )
    {
        printf( "Turn ON LED\n" );
        wiced_gpio_output_high( WICED_LED1 );
    }
    else
    {
        if ( strstr( (char*) message->payload, "OFF" ) )
        {
            printf( "Turn OFF LED\n" );
            wiced_gpio_output_low( WICED_LED1 );
        }
    }
}

