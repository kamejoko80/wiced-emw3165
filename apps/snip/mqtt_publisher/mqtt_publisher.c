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
 * MQTT Publisher Sample Application
 *
 * Features demonstrated
 *  - WICED MQTT PUBLISHER API
 *
 * This application snippet publish the topic
 *
 * Application Instructions
 *   Connect a PC terminal to the serial port of the WICED Eval board,
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
 *   Start Wiced Application, Make sure Wiced board and PC (in which Mosquitto server installed) both are in same domain
 *   Now Subscriber update the publisher topic
 *
 *   Public brokers reference - https://github.com/mqtt/mqtt.github.io/wiki/public_brokers
 *   List of Public brokers
 *      server name -                       broker.mqttdashboard.com
 *      port                1883, 8000 (WebSockets)
 *      server name -                       iot.eclipse.org
 *      port                1883, 80(web socket)
 *      server name -                       test.mosquitto.org
 *      port                1883, 8883 (SSL), 8884 (SSL), 80 (WebSockets)
 *      server name -                       dev.rabbitmq.com
 *      port                1883
 *      server name -                       q.m2m.io
 *      port                1883
 *      server name -                       www.cloudmqtt.com
 *      port                18443, 28443 (SSL)
 *
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
static const wiced_ip_setting_t ap_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};
/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
#define MQTT_SERVER_THREAD_PRIORITY          (WICED_DEFAULT_LIBRARY_PRIORITY)
#define MQTT_SERVER_STACK_SIZE               (6200)
#define MQTT_TARGET_IP                       MAKE_IPV4_ADDRESS(192,168,0,2)
#define MQTT_CLIENT_ID                       "wiced"
#define MQTT_TOPIC_NAME                      "wiced"
#define MQTT_USERNAME                        ""
#define MQTT_PASSWD                          ""
/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

char            publish_buffer[128];
MQTTMessage     publish_message;
wiced_bool_t    Switch = WICED_FALSE;

void application_start( )
{
    Network              n;
    Client               c;
    unsigned char        buf[100];
    unsigned char        readbuf[100];

    wiced_ip_address_t INITIALISER_IPV4_ADDRESS( ip_address, MQTT_TARGET_IP );

    wiced_init( );

    /* Initialize MQTT */
    wiced_mqtt_init( &n );

    wiced_mqtt_buffer_init( &c, &n, buf, 100, readbuf, 100);

    n.hostname      = "broker.mqttdashboard.com";
    n.ip_address    = &ip_address;
    n.port          = MQTT_BROKER_PORT;

    /* Bring up the network interface AP or STA Mode*/
    //wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &ap_ip_settings );

    wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );

    if (WICED_SUCCESS != wiced_connectnetwork( &n, WICED_STA_INTERFACE) )
    {
        WPRINT_APP_INFO ( ("connection failed\n") );
        return ;
    }

    wiced_mqtt_connect( &c, MQTT_CLIENT_ID, NULL, NULL );

    publish_message.dup         = 0;
    publish_message.id          = 0;
    publish_message.qos         = 0;
    publish_message.retained    = 1;

    sprintf (publish_buffer, "%s", "ONOFF");


    while (1)
    {
        publish_message.payload = (void *)publish_buffer;
        publish_message.payloadlen = strlen (publish_buffer);

        // Publisher
        wiced_mqtt_publish (&c, MQTT_TOPIC_NAME, &publish_message);

        wiced_rtos_delay_milliseconds( 5000 );

        if (Switch)
        {
            sprintf (publish_buffer, "%s", "Turn ON");
        }
        else
        {
            sprintf (publish_buffer, "%s", "TURN OFF");
        }

        WPRINT_APP_INFO ( ("Switch %d \r\n", Switch));

        Switch = ~Switch;
    }
}
