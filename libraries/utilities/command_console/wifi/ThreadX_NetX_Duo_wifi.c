/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include <stdint.h>
#include "nx_api.h"
#include "command_console.h"
#include "wwd_debug.h"
#include "wiced_network.h"
#include "wiced_management.h"

int set_ip( int argc, char* argv[] )
{
    wiced_ip_address_t ipaddr, netmask, gw;
    if ( argc < 4 )
    {
        return ERR_UNKNOWN;
    }
    if ( ( str_to_ip(argv[1], &ipaddr)  != 0 ) ||
         ( str_to_ip(argv[2], &netmask) != 0 ) ||
         ( str_to_ip(argv[3], &gw)      != 0 ) )
    {
        /* Failed to parse IP address */
        return ERR_UNKNOWN;
    }
    if ( ( ipaddr.version  != WICED_IPV4 ) ||
         ( netmask.version != WICED_IPV4 ) ||
         ( gw.version      != WICED_IPV4 ) )
    {
        /* Wrong IP version */
        return ERR_UNKNOWN;
    }
    nx_ip_interface_address_set( &IP_HANDLE(WICED_STA_INTERFACE), 0, htonl(ipaddr.ip.v4), htonl(netmask.ip.v4) );
    nx_ip_gateway_address_set( &IP_HANDLE(WICED_STA_INTERFACE), htonl(gw.ip.v4) );
    return ERR_CMD_OK;
}

void network_print_status( wiced_interface_t interface )
{
    if ( IP_HANDLE(interface).nx_ip_driver_link_up )
    {
        WPRINT_APP_INFO( ( "   IP Addr     : %u.%u.%u.%u\r\n", (unsigned char) ( ( IP_HANDLE(interface).nx_ip_address >> 24 ) & 0xff ), (unsigned char) ( ( IP_HANDLE(interface).nx_ip_address >> 16 ) & 0xff ), (unsigned char) ( ( IP_HANDLE(interface).nx_ip_address >> 8 ) & 0xff ), (unsigned char) ( ( IP_HANDLE(interface).nx_ip_address >> 0 ) & 0xff ) ) );
        WPRINT_APP_INFO( ( "   Gateway     : %u.%u.%u.%u\r\n", (unsigned char) ( ( IP_HANDLE(interface).nx_ip_gateway_address >> 24 ) & 0xff ), (unsigned char) ( ( IP_HANDLE(interface).nx_ip_gateway_address >> 16 ) & 0xff ), (unsigned char) ( ( IP_HANDLE(interface).nx_ip_gateway_address >> 8 ) & 0xff ), (unsigned char) ( ( IP_HANDLE(interface).nx_ip_gateway_address >> 0 ) & 0xff ) ) );
        WPRINT_APP_INFO( ( "   Netmask     : %u.%u.%u.%u\r\n", (unsigned char) ( ( IP_HANDLE(interface).nx_ip_network_mask >> 24 ) & 0xff ), (unsigned char) ( ( IP_HANDLE(interface).nx_ip_network_mask >> 16 ) & 0xff ), (unsigned char) ( ( IP_HANDLE(interface).nx_ip_network_mask >> 8 ) & 0xff ), (unsigned char) ( ( IP_HANDLE(interface).nx_ip_network_mask >> 0 ) & 0xff ) ) );
    }
}

