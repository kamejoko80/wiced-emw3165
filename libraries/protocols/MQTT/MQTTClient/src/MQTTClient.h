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

#ifndef __MQTT_CLIENT_C_
#define __MQTT_CLIENT_C_

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                   WICED Timer Library APIs
 ******************************************************/

char wiced_expired( Timer* );
void wiced_countdown_ms( Timer*, unsigned int );
void wiced_countdown( Timer*, unsigned int );
int wiced_left_ms( Timer* );
int wiced_roundup( int numToRound, int multiple );
void wiced_init_timer( Timer* );

/******************************************************
 *                   WICED Client Library APIs
 ******************************************************/
int MQTTConnect( Client*, MQTTPacket_connectData* );
int MQTTPublish( Client*, const char*, MQTTMessage* );
int MQTTSubscribe( Client*, const char*, enum QoS, messageHandler );
int MQTTUnsubscribe( Client*, const char* );
int MQTTDisconnect( Client* );
int MQTTYield( Client*, int );

void setDefaultMessageHandler( Client*, messageHandler );
void MQTTClient( Client*, Network*, unsigned int, unsigned char*, size_t, unsigned char*, size_t );

#ifdef __cplusplus
} /* extern "C" */

#endif

#endif
