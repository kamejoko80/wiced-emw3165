/******************************************************************************
 * GRID2HOME SMART ENERGY 2
 *****************************************************************************/
/**
 * Copyright (C) 2009 Grid2Home, Inc. All rights reserved.
 * This source code and any compilation or derivative thereof is the proprietary
 * information of Grid2Home, Inc and is confidential in nature.
 *
 * \file          sep2_client.c
 *
 * \version       $Revision: $
 *
 * \date          $Date: $
 *
 * \author        DMZ
 *
 * \brief         Application example code
 *
 *
 * \section info  Change Information
 *
 * \verbatim
   $History: $
 */

#include "wiced.h"
#include "nx_api.h"
#include "tx_api.h"


/******************************************************
 *                      Macros
 ******************************************************/
#define XMDNS_V6_ADDR_MSB   0xFF050000      // "ff05::fb"
#define XMDNS_V6_ADDR_LSB   0x000000FB

#define XMDNS_V4_ADDR       0xEFFFFFFB //"239.255.255.251" //224.0.0.251, 0xE00000FB


/******************************************************
 *               Function Declarations
 ******************************************************/
extern int  kitu_main(int argc, char*  argv[]);
extern void ISAL_Init(uint8_t * ipaddr);
extern int  inet_pton4(const char *src, uint8_t *dst);
extern void SetupIPV6Address(NXD_ADDRESS * global, NXD_ADDRESS * group);

void Kitu_MulticastJoin(void);
int GetIPv6Address(void);
int GetIPv4Address(void);

void udp_send(void);
void udp6_send(void);
void tcp6_send(void);
void CheckSocket(int sockfd);

/******************************************************
 *               Variables Definitions
 ******************************************************/


/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start(void)
{

    /* Initialise the device and WICED framework */
    wiced_init( );

    /* Bring up the network interface */
    wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );

    printf("<<< Starting... >>>\r\n");

#ifdef IPV6_CLIENT
    // get IP address
    GetIPv6Address();
#else
    GetIPv4Address();

    Kitu_MulticastJoin();
#endif

    wiced_rtos_delay_milliseconds(10000);         // let ip stack be ready

    kitu_main(1, NULL);
}


///////////////////////////////////////////////////////////////////////////////
// GetIPv6Address
//   use wiced function to retrieve ipv4 address.
///////////////////////////////////////////////////////////////////////////////
int GetIPv4Address(void)
{
    wiced_ip_address_t address;
    uint8_t myip[16];
    int i, result = -1;
    int n= 0;

    // we need the IPv4 address.
    while(result == -1)
    {
        // first we try to get global ip address
        if ( (wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &address) == WICED_SUCCESS) )
        {
            result = 1;
        }
        else
        {
            printf("Retrieving IPv4 Address ...\r\n");

            wiced_rtos_delay_milliseconds(500);         // try again later
        }
    }

    // convert to u8_t[16];
    for(i=0; i < 4; i++)
    {
        myip[n++] = address.ip.v4 >> 24;
        myip[n++] = address.ip.v4 >> 16;
        myip[n++] = address.ip.v4 >> 8;
        myip[n++] = address.ip.v4;
    }
    memset(&myip[4], 0x0, 12);

    printf("IPv4 Address: %d.%d.%d.%d\r\n", myip[0], myip[1], myip[2], myip[3]);

    // set up ISAL layer
    ISAL_Init(myip);

    return result;
}


///////////////////////////////////////////////////////////////////////////////
// GetIPv6Address
//   use wiced function to retrieve ipv6 address.
///////////////////////////////////////////////////////////////////////////////
int GetIPv6Address(void)
{
    wiced_ip_address_t g_address;
    uint8_t myip[16];
    int i, result = -1;
    int n= 0;

    // we need the IPv6 address.
    while(result == -1)
    {
        // first we try to get global ip address
        if ( (wiced_ip_get_ipv6_address( WICED_STA_INTERFACE,
                     &g_address, IPv6_GLOBAL_ADDRESS ) == WICED_SUCCESS) &&
             ((g_address.ip.v6[0] & 0xFF000000)!= 0x0) )
        {
            result = IPv6_GLOBAL_ADDRESS;
        }
        else
        {
            printf("Retriving IPv6 Address ...\r\n");

            wiced_rtos_delay_milliseconds(500);         // try again later
        }
    }

    // convert to u8_t[16];
    for(i=0; i < 4; i++)
    {
        myip[n++] = g_address.ip.v6[i] >> 24;
        myip[n++] = g_address.ip.v6[i] >> 16;
        myip[n++] = g_address.ip.v6[i] >> 8;
        myip[n++] = g_address.ip.v6[i];
    }

    printf("IPv6 Address: ");
    for(i=0; i < 16; i++)
        printf("%02X", myip[i]);

    printf("\r\n");

    // set up ISAL layer
    ISAL_Init(myip);

    // set ipv6 address for NXD
    NXD_ADDRESS gaddr;

    gaddr.nxd_ip_version = NX_IP_VERSION_V6;
    memcpy(&gaddr.nxd_ip_address, &g_address.ip, 16);

    // multicast group
    NXD_ADDRESS group;
    group.nxd_ip_version = NX_IP_VERSION_V6;

    group.nxd_ip_address.v6[0] = 0xFF020000;    // 0xFF050000
    group.nxd_ip_address.v6[1] = 0;
    group.nxd_ip_address.v6[2] = 0;
    group.nxd_ip_address.v6[3] = 0xFB;

    // setup ip address and multicast group
    SetupIPV6Address(&gaddr, &group);

    /* Wait for IPv6 stack to finish DAD process. */
    tx_thread_sleep(400);

    return result;
}


///////////////////////////////////////////////////////////////////////////////
// Kitu_MulticastJoin
///////////////////////////////////////////////////////////////////////////////
void Kitu_MulticastJoin(void)
{
    // convert ip address
    wiced_ip_address_t ipaddr = {0};
    int ret;

    printf("Join xMDNS group\r\n");

    //inet_pton4(XMDNS_V4_ADDR, buf);
    ipaddr.version = WICED_IPV4;
    ipaddr.ip.v4  = IP_ADDRESS(239, 255, 255, 251); //IP_ADDRESS(251, 255, 255, 239); //XMDNS_V4_ADDR;

    // join the multicast group
    ret = wiced_multicast_join(WICED_STA_INTERFACE, &ipaddr);
    if( ret != WICED_SUCCESS)
        printf("Join Multicast group failed:%d\r\n", ret);
    else
        printf("Join Multicast group: %lX\r\n", ipaddr.ip.v4);
}


///////////////////////////////////////////////////////////////////////////////
// need to modify wiced_network.c for BSD socket.
// \wiced2.4.1\Wiced\Network\NetX_Duo\wiced\wiced_network.c
///////////////////////////////////////////////////////////////////////////////


///////////test

/******************************************************
 *                      Macros
 ******************************************************/
#include "netx_bsd_layer/nxd_bsd.h"

uint16_t ISAL_Htons(uint16_t hostshort);
uint32_t ISAL_Htonl(uint32_t hostlong);

void udp_send(void)
{
    char buffer[256];
    static int sockfd = -1;
    int code = -1;

    if(sockfd < 0)
    {
        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(sockfd < 0)
        {
          printf("DnsClientOpenSendSockets: socket failed\r\n");
          return;
        }

        if((code = fcntl(sockfd, F_SETFL, O_NONBLOCK)) < 0)
            printf("Cannot set to Non blocking: %d\r\n", code);

        // Bind server socket
        struct sockaddr_in    sClientLocalAddr = {0};

        sClientLocalAddr.sin_family = AF_INET;
        sClientLocalAddr.sin_port   = ISAL_Htons(53535);

        printf("Port: %d\r\n", sClientLocalAddr.sin_port);

        //struct in_addr  MyLocalAddrStruct = (struct in_addr *) ISAL_GetAddr(0);

        //sClientLocalAddr.sin_addr.s_addr = IP_ADDRESS(192, 168, 2, 102);//MyLocalAddrStruct->s_addr;

        int code = bind(sockfd, (struct sockaddr *)&sClientLocalAddr, sizeof(sClientLocalAddr));
        if(code == -1)
        {
          printf("DnsClientOpenSendSockets: bind failed\r\n");
          return;
        }
    }

    // send data
    struct sockaddr_in    sRemoteAddr = {0};
    sRemoteAddr.sin_family = AF_INET;
    sRemoteAddr.sin_port = ISAL_Htons(5353);
    sRemoteAddr.sin_addr.s_addr = IP_ADDRESS(192, 168, 2, 102);

    code = sendto(sockfd, "ABCD", (strlen("ABCD") + 1), 0,
                   (struct sockaddr *)&sRemoteAddr, sizeof(sRemoteAddr));

    if(code < 0)
    {
        printf("Sendto error\r\n");
        return;
    }

    printf("Send message\r\n");

    // recv data
    struct sockaddr from;
    INT fromLen;
    code = recvfrom(sockfd, buffer, 255, 0, &from, &fromLen);
    if(code < 0)
        printf("Recvfrom failed\r\n");
}


void tcp6_send(void)
{
    fd_set rd, wd, ed;
    int count = 0;
    struct timeval tv = {0};
    static int sockfd = -1;
    int code = -1;

    if(sockfd < 0)
    {
        sockfd = socket(AF_INET6, SOCK_STREAM, 0);//IPPROTO_TCP);
        if(sockfd < 0)
        {
          printf("tcp6_send: socket failed\r\n");
          return;
        }

        if((code = fcntl(sockfd, F_SETFL, O_NONBLOCK)) < 0)
            printf("Cannot set to Non blocking: %d\r\n", code);

        // Bind server socket
        struct sockaddr_in6    sClientLocalAddr = {0};

        sClientLocalAddr.sin6_family = AF_INET6;
        sClientLocalAddr.sin6_port   = ISAL_Htons(53535);

        printf("Port: %d\r\n", sClientLocalAddr.sin6_port);

        //struct in_addr  MyLocalAddrStruct = (struct in_addr *) ISAL_GetAddr(0);

        //sClientLocalAddr.sin_addr.s_addr = IP_ADDRESS(192, 168, 2, 102);//MyLocalAddrStruct->s_addr;

        int code = bind(sockfd, (struct sockaddr *)&sClientLocalAddr, sizeof(sClientLocalAddr));
        if(code == -1)
        {
          printf("tcp6_send: bind failed\r\n");
          goto SOC_ERROR;
        }
    }

    // send data
    struct sockaddr_in6    sRemoteAddr = {0};
    sRemoteAddr.sin6_family = AF_INET6;
    sRemoteAddr.sin6_port = ISAL_Htons(80); //2015:db6::29fb:4b58:d55a:f167
    sRemoteAddr.sin6_addr._S6_un._S6_u32[0] = ISAL_Htonl(0x20150db6);
    sRemoteAddr.sin6_addr._S6_un._S6_u32[1] = 0x0;
    sRemoteAddr.sin6_addr._S6_un._S6_u32[2] = ISAL_Htonl(0x29fb4b58);
    sRemoteAddr.sin6_addr._S6_un._S6_u32[3] = ISAL_Htonl(0xd55af167);

    code = connect(sockfd, (struct sockaddr *)&sRemoteAddr, sizeof(sRemoteAddr));
    if(code < 0)
    {
        printf("Connect error:%d\r\n", code);
        goto SOC_ERROR;
    }

    // select
    FD_ZERO(&rd);FD_ZERO(&wd);FD_ZERO(&ed);
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    while(!FD_ISSET(sockfd, &wd))
    {
		FD_SET(sockfd, &wd);

		code = select(sockfd +1, NULL, &wd, NULL, &tv);
		if(code > 0 && FD_ISSET(sockfd, &wd))
			break;

		if(count++ > 10)
			break;
		printf(">>> Select return:%d\r\n", code);
        FD_ZERO(&rd);FD_ZERO(&wd);FD_ZERO(&ed);
		tx_thread_sleep(1000);
    }

    code = send(sockfd, "ABCD", (strlen("ABCD") + 1), 0);

    if(code < 0)
    {
        printf("Sendto error\r\n");
        goto SOC_ERROR;
    }

    printf("Send TCP message\r\n");

SOC_ERROR:
	CheckSocket(sockfd);
    soc_close(sockfd);
}

void udp6_send(void)
{
    char buffer[256];
    fd_set rd, wd, ed;
    struct timeval tv = {0};
    int count = 0;
    static int sockfd = -1;
    int code = -1;

    if(sockfd < 0)
    {
        sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
        if(sockfd < 0)
        {
          printf("DnsClientOpenSendSockets: socket failed\r\n");
          return;
        }

        if((code = fcntl(sockfd, F_SETFL, O_NONBLOCK)) < 0)
            printf("Cannot set to Non blocking: %d\r\n", code);

        // Bind server socket
        struct sockaddr_in6    sClientLocalAddr = {0};

        sClientLocalAddr.sin6_family = AF_INET6;
        sClientLocalAddr.sin6_port   = ISAL_Htons(53530);

        printf("Port: %d\r\n", sClientLocalAddr.sin6_port);

        //struct in_addr  MyLocalAddrStruct = (struct in_addr *) ISAL_GetAddr(0);

        //sClientLocalAddr.sin_addr.s_addr = IP_ADDRESS(192, 168, 2, 102);//MyLocalAddrStruct->s_addr;

        int code = bind(sockfd, (struct sockaddr *)&sClientLocalAddr, sizeof(sClientLocalAddr));
        if(code == -1)
        {
          printf("DnsClientOpenSendSockets: bind failed\r\n");
          goto UDP_ERROR;
        }
    }

    // send data
    struct sockaddr_in6    sRemoteAddr = {0};
    sRemoteAddr.sin6_family = AF_INET6;
    sRemoteAddr.sin6_port = ISAL_Htons(5353); //2015:db6::29fb:4b58:d55a:f167
    sRemoteAddr.sin6_addr._S6_un._S6_u32[0] = ISAL_Htonl(0x20150db6);
    sRemoteAddr.sin6_addr._S6_un._S6_u32[1] = 0x0;
    sRemoteAddr.sin6_addr._S6_un._S6_u32[2] = ISAL_Htonl(0x29fb4b58);
    sRemoteAddr.sin6_addr._S6_un._S6_u32[3] = ISAL_Htonl(0xd55af167);

    // select
    FD_ZERO(&rd);FD_ZERO(&wd);FD_ZERO(&ed);
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    while(!FD_ISSET(sockfd, &wd))
    {
		FD_SET(sockfd, &rd);FD_SET(sockfd, &wd);FD_SET(sockfd, &ed);

		code = select(sockfd +1, NULL, &wd, NULL, &tv);
		if(code > 0 && FD_ISSET(sockfd, &wd))
			break;

		if(count++ > 10)
			break;
		printf(">>> Select return:%d\r\n", code);
        FD_ZERO(&rd);FD_ZERO(&wd);FD_ZERO(&ed);
		tx_thread_sleep(1000);
    }

    code = sendto(sockfd, "ABCD", (strlen("ABCD") + 1), 0,
                   (struct sockaddr *)&sRemoteAddr, sizeof(sRemoteAddr));

    if(code < 0)
    {
        printf("Sendto error\r\n");
        goto UDP_ERROR;
    }

    printf("Send UDP message\r\n");

    // recv data
    struct sockaddr_in6 from;
    INT fromLen;
    code = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr *)&from, &fromLen);
    if(code < 0)
        printf("Recvfrom failed\r\n");

    UDP_ERROR:
	CheckSocket(sockfd);
    soc_close(sockfd);
}


#if 0
/************************************************************************
 *   WICED thread priority table
 *
 * +----------+-----------------+
 * | Priority |      Thread     |
 * |----------|-----------------|
 * |     0    |      Wiced      |   Highest priority
 * |     1    |     Network     |
 * |     2    |                 |
 * |     3    | Network worker  |
 * |     4    |                 |
 * |     5    | Default Library |
 * |          | Default worker  |
 * |     6    |                 |
 * |     7    |   Application   |
 * |     8    |                 |
 * |     9    |      Idle       |   Lowest priority
 * +----------+-----------------+
 */
#define WICED_NETWORK_WORKER_PRIORITY      (3)
#define WICED_DEFAULT_WORKER_PRIORITY      (5)
#define WICED_DEFAULT_LIBRARY_PRIORITY     (5)
#define WICED_APPLICATION_PRIORITY         (7)
#endif
