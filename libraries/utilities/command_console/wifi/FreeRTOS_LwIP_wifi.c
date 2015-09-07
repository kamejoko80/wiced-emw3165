/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "wwd_wifi.h"
#include "wiced_management.h"
#include "wiced_network.h"
#include "wwd_network.h"
#include "network/wwd_buffer_interface.h"
#include "RTOS/wwd_rtos_interface.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include <string.h>
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/sockets.h"  /* equivalent of <sys/socket.h> */
#include "lwip/inet.h"
#include "wwd_debug.h"
#include "wwd_assert.h"
#include "command_console.h"
#include "wiced.h"

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/


int set_ip( int argc, char* argv[] )
{
    struct ip_addr ipaddr_lwip, netmask_lwip, gw_lwip;
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

    ipaddr_lwip.addr  = ipaddr.ip.v4;
    netmask_lwip.addr = netmask.ip.v4;
    gw_lwip.addr = gw.ip.v4;

    netif_set_addr( &IP_HANDLE(WICED_STA_INTERFACE), &ipaddr_lwip, &netmask_lwip, &gw_lwip );
    return ERR_CMD_OK;
}

void network_print_status( wiced_interface_t interface )
{
    if ( netif_is_up(&IP_HANDLE(interface)) )
    {
        WPRINT_APP_INFO( ( "   IP Addr     : %u.%u.%u.%u\r\n", (unsigned char) ( ( htonl( IP_HANDLE(interface).ip_addr.addr ) >> 24 ) & 0xff ), (unsigned char) ( ( htonl( IP_HANDLE(interface).ip_addr.addr ) >> 16 ) & 0xff ), (unsigned char) ( ( htonl( IP_HANDLE(interface).ip_addr.addr ) >> 8 ) & 0xff ), (unsigned char) ( ( htonl( IP_HANDLE(interface).ip_addr.addr ) >> 0 ) & 0xff ) ) );
        WPRINT_APP_INFO( ( "   Gateway     : %u.%u.%u.%u\r\n", (unsigned char) ( ( htonl( IP_HANDLE(interface).gw.addr ) >> 24 ) & 0xff ),      (unsigned char) ( ( htonl( IP_HANDLE(interface).gw.addr ) >> 16 ) & 0xff ),      (unsigned char) ( ( htonl( IP_HANDLE(interface).gw.addr ) >> 8 ) & 0xff ),      (unsigned char) ( ( htonl( IP_HANDLE(interface).gw.addr ) >> 0 ) & 0xff ) ) );
        WPRINT_APP_INFO( ( "   Netmask     : %u.%u.%u.%u\r\n", (unsigned char) ( ( htonl( IP_HANDLE(interface).netmask.addr ) >> 24 ) & 0xff ), (unsigned char) ( ( htonl( IP_HANDLE(interface).netmask.addr ) >> 16 ) & 0xff ), (unsigned char) ( ( htonl( IP_HANDLE(interface).netmask.addr ) >> 8 ) & 0xff ), (unsigned char) ( ( htonl( IP_HANDLE(interface).netmask.addr ) >> 0 ) & 0xff ) ) );
    }
}
