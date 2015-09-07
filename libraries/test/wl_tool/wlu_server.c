/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * Wl server for generic RTOS
 */

#include <stdio.h>
#include <string.h>
#include "wl_tool.h"
#include "wiced_management.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "internal/wwd_sdpcm.h"
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"

#include "internal/wwd_internal.h"
#include "wwd_assert.h"
#include "wwd_buffer_interface.h"

/******************************************************
 *                    Constants
 ******************************************************/

#if ( WICED_PAYLOAD_MTU < 8192 )
#pragma message "\n\n" \
                "!!!! WICED_PAYLOAD_MTU NOTE:\n" \
                "!!!!     WICED_PAYLOAD_MTU is too small to work with all WL commands\n" \
                "!!!!     Some, like scanresults will fail\n"
#endif

#define WLU_SERVER_THREAD_PRIORITY  (WICED_DEFAULT_LIBRARY_PRIORITY)
#define WLU_SERVER_STACK_SIZE       (WICED_DEFAULT_APPLICATION_STACK_SIZE)

#define SUCCESS                     1
#define FAIL                        -1
#define NO_PACKET                   -2
#define SERIAL_PORT_ERR             -3
#ifdef __cplusplus

#define TYPEDEF_BOOL
#ifndef FALSE
#define FALSE   (false)
#endif
#ifndef TRUE
#define TRUE    (true)
#endif

#else   /* ! __cplusplus */

#define TYPEDEF_BOOL
#ifndef bool
typedef unsigned char   bool;           /* consistent w/BOOL */
#endif
#ifndef FALSE
#define FALSE   (0)
#endif
#ifndef TRUE
#define TRUE    (1)
#endif

#endif  /* ! __cplusplus */

#define BCME_STRLEN        64  /* Max string length for BCM errors */

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_thread_t         thread;
    volatile wiced_bool_t  started;
    wiced_uart_t           uart_id;
#ifdef  RWL_SOCKET
    volatile wiced_bool_t   eth_started;
    int                     eth_port;
#endif
} wiced_wlu_server_t;

static wiced_wlu_server_t wlu_server =
{
    .started            = WICED_FALSE,
#ifdef  RWL_SOCKET
    .eth_started        = WICED_FALSE,
#endif
};

#ifdef  RWL_SOCKET
wiced_tcp_socket_t      tcp_client_socket;
#endif

/******************************************************
 *               Function Declarations
 ******************************************************/

static void wlu_server_thread( uint32_t thread_input );
wiced_result_t wiced_start_wlu_server( void );
wiced_result_t wiced_stop_wlu_server( void );

extern int remote_server_exec(int argc, char **argv, void *ifr);
extern const char *bcmerrorstr(int bcmerror);

/******************************************************
 *               Variable Definitions
 ******************************************************/

static int ifnum = 0;
static int                   currently_downloading    = 0;
static host_semaphore_type_t downloading_semaphore;
static host_semaphore_type_t download_ready_semaphore;
static wiced_thread_t        downloading_thread;
unsigned short defined_debug = 0; //DEBUG_ERR | DEBUG_INFO;

/******************************************************
 *               Function Definitions
 ******************************************************/

int
rwl_create_dir(void)
{
    /* not supported */
    return 0;
}

int
remote_shell_execute(char *buf_ptr)
{
    /* not supported */
    UNUSED_PARAMETER(buf_ptr);
    return 0;
}

int
remote_shell_get_resp(char* shell_fname, char* buf_ptr, int msg_len)
{
    /* not supported */
    UNUSED_PARAMETER(shell_fname);
    UNUSED_PARAMETER(buf_ptr);
    UNUSED_PARAMETER(msg_len);
    return 0;
}
int
rwl_write_serial_port(void* hndle, char* write_buf, unsigned long size, unsigned long *numwritten)
{
    wiced_result_t result;
    wiced_uart_t uart = *((wiced_uart_t*) hndle);

    wiced_assert( "invalid argument", uart < WICED_UART_MAX );

    if ( size == 0 )
    {
        return SUCCESS;
    }

    result = wiced_uart_transmit_bytes( uart, write_buf, size );
    *numwritten = size;

    return ( result==WICED_SUCCESS )? SUCCESS : SERIAL_PORT_ERR;
}

void*
rwl_open_transport(int remote_type, char *port, int ReadTotalTimeout, int debug)
{
    /* not invoked for dongle transports */
    UNUSED_PARAMETER(remote_type);
    UNUSED_PARAMETER(ReadTotalTimeout);
    UNUSED_PARAMETER(debug);

#ifdef  RWL_SOCKET
    if (wiced_tcp_accept( &tcp_client_socket ) != WICED_SUCCESS)
    {
        return NULL;
    }
#endif

    return port;
}

int
rwl_close_transport(int remote_type, void* Des)
{
    /* not invoked for dongle transports */
    UNUSED_PARAMETER(remote_type);
    UNUSED_PARAMETER(Des);
    return FAIL;
}

int
rwl_read_serial_port(void* hndle, char* read_buf, unsigned int data_size, unsigned int *numread)
{
    wiced_result_t result;
    wiced_uart_t uart = *((wiced_uart_t*) hndle);

    wiced_assert( "invalid argument", uart < WICED_UART_MAX );

    if ( data_size == 0 )
    {
        return SUCCESS;
    }

    result = wiced_uart_receive_bytes( uart, read_buf, data_size, WICED_NEVER_TIMEOUT );
    *numread = data_size;
    return ( result==WICED_SUCCESS )? SUCCESS : SERIAL_PORT_ERR;
}

void
rwl_sleep(int delay)
{
    host_rtos_delay_milliseconds( delay );
}

void
rwl_sync_delay(unsigned int noframes)
{
    if (noframes > 1) {
        rwl_sleep(200);
    }
}

int
wl_ioctl(void *wl, int cmd, void *buf, int len, bool set)
{
    wwd_result_t ret;
    wiced_buffer_t internalPacket;
    wiced_buffer_t response_buffer;

    UNUSED_PARAMETER(wl);

    // Set Wireless Security Type
    void* ioctl_data = wwd_sdpcm_get_ioctl_buffer( &internalPacket, len );
    if (ioctl_data == NULL)
    {
        return WLAN_ENUM_OFFSET - WWD_WLAN_NOMEM;
    }
    memcpy( ioctl_data, buf, len );
    ret = wwd_sdpcm_send_ioctl( (set==TRUE)?SDPCM_SET:SDPCM_GET, cmd, internalPacket, &response_buffer, (wwd_interface_t) ifnum );
    if (ret == WWD_SUCCESS)
    {
        if (set!=TRUE)
        {
            memcpy( buf, host_buffer_get_current_piece_data_pointer( response_buffer ), MIN( host_buffer_get_current_piece_size( response_buffer ), len )  );
        }
        host_buffer_release(response_buffer, WWD_NETWORK_RX);
    }
    else
    {
        ret = WLAN_ENUM_OFFSET - ret;
    }
    return ret; /*( ret == WICED_SUCCESS)?0:IOCTL_ERROR;*/
}


/*
 * Function to set interface.
 */
int
set_interface(void *wl, char *intf_name)
{
    if ( sscanf( intf_name, "eth%d", &ifnum ) != 1 )
    {
        ifnum = 0;
        return -1;
    }
    return 0;
}

static void downloading_init_func( uint32_t arg )
{
    wiced_init( );
    WICED_END_OF_CURRENT_THREAD_NO_LEAK_CHECK( );
}

void start_download( void )
{
    wiced_deinit( );
    currently_downloading = 1;

    host_rtos_init_semaphore( &downloading_semaphore );
    host_rtos_init_semaphore( &download_ready_semaphore );

    wiced_rtos_create_thread( &downloading_thread, WICED_NETWORK_WORKER_PRIORITY, "downloading_init_func", downloading_init_func, 5000, NULL);
    host_rtos_get_semaphore( &download_ready_semaphore, NEVER_TIMEOUT, WICED_FALSE );
    host_rtos_deinit_semaphore( &download_ready_semaphore );
}

void finish_download( void )
{
    host_rtos_set_semaphore( &downloading_semaphore, WICED_FALSE );
    host_rtos_deinit_semaphore( &downloading_semaphore );
    currently_downloading = 0;
}

void membytes_write( uint32_t address, uint8_t* buf, uint32_t length )
{
    wwd_bus_set_backplane_window( address );
    wwd_bus_transfer_bytes( BUS_WRITE, BACKPLANE_FUNCTION, ( address & BACKPLANE_ADDRESS_MASK ), length, (wwd_transfer_bytes_packet_t*) ( buf -  WWD_BUS_HEADER_SIZE ) );
}

wwd_result_t external_write_wifi_firmware_and_nvram_image( void )
{
    if ( currently_downloading == 0 )
    {
        wwd_result_t result;
        result = wwd_bus_write_wifi_firmware_image( );
        if ( result != WWD_SUCCESS )
        {
            return result;
        }
        return wwd_bus_write_wifi_nvram_image( );
    }

    host_rtos_set_semaphore( &download_ready_semaphore, WICED_FALSE );
    host_rtos_get_semaphore( &downloading_semaphore, NEVER_TIMEOUT, WICED_FALSE );
    return WWD_SUCCESS;
}

wiced_result_t wiced_wlu_server_serial_start( wiced_uart_t uart_id )
{
    wiced_assert("wlu_server already started",
            (wlu_server.started == WICED_FALSE));

    wlu_server.started = WICED_TRUE;
    wlu_server.uart_id = uart_id;

    WWD_WLAN_KEEP_AWAKE( );

    if ( uart_id != STDIO_UART )
    {
        static const platform_uart_config_t uart_config =
        {
            .baud_rate    = 115200,
            .data_width   = DATA_WIDTH_8BIT,
            .parity       = NO_PARITY,
            .stop_bits    = STOP_BITS_1,
            .flow_control = FLOW_CONTROL_DISABLED,
        };

        WICED_VERIFY( wiced_uart_init( uart_id, &uart_config, NULL ) );
    }
    /* Start wlu server thread */
    wiced_rtos_create_thread(&wlu_server.thread, WLU_SERVER_THREAD_PRIORITY, "wlu_server", wlu_server_thread, WLU_SERVER_STACK_SIZE, &wlu_server);
    return WICED_SUCCESS;
}

wiced_result_t wiced_wlu_server_serial_stop( void )
{
    if ( wiced_rtos_is_current_thread( &wlu_server.thread ) != WICED_SUCCESS )
    {
        wiced_rtos_thread_force_awake( &wlu_server.thread );
        wiced_rtos_thread_join( &wlu_server.thread );
        wiced_rtos_delete_thread( &wlu_server.thread );
    }
    WWD_WLAN_LET_SLEEP( );
    wlu_server.started = WICED_FALSE;
    return WICED_SUCCESS;
}

static void wlu_server_thread( uint32_t thread_input )
{
    wiced_wlu_server_t* wlu = (wiced_wlu_server_t*) thread_input;
    int argc = 2;
    char *argv[] = { (char*)wlu->uart_id, "" };
#ifdef  RWL_SOCKET
    char *arge[] = { (char *)wlu->eth_port, "" };
#endif


    wiced_assert( "invalid argument", wlu->uart_id < WICED_UART_MAX );

    UNUSED_PARAMETER(thread_input);

#ifdef  RWL_SOCKET
    if (wlu->eth_started ==  WICED_TRUE)
#endif
    {
#ifdef  RWL_SOCKET
        remote_server_exec( argc, (wlu->eth_started ==  WICED_TRUE)?arge:argv, NULL );
#else
        remote_server_exec(argc, argv, NULL);
#endif


    }
}

#ifdef  RWL_SOCKET
wiced_result_t wiced_wlu_server_eth_start( int tcp_server_port , char** argv)
{
    wiced_result_t result;
    UNUSED_PARAMETER(argv);
    wiced_assert("wlu_server already started", (wlu_server.started == WICED_FALSE));

    wlu_server.started                  = WICED_TRUE;
    wlu_server.eth_started              = WICED_TRUE;
    wlu_server.eth_port                 = tcp_server_port;



    // WWD_WLAN_KEEP_AWAKE( );

    /* Bring up the network interface */
    result = wiced_network_up( WICED_ETHERNET_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL );

    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Failed to connect to Ethernet.\n"));
    }

    /* Create a TCP socket */
    if ( wiced_tcp_create_socket( &tcp_client_socket, WICED_ETHERNET_INTERFACE ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("TCP socket creation failed\n") );
    }
    if (wiced_tcp_listen( &tcp_client_socket, tcp_server_port ) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("TCP server socket initialization failed\n"));
        wiced_tcp_delete_socket(&tcp_client_socket);
        return WICED_ERROR;
    }

    wiced_rtos_create_thread( &wlu_server.thread, WLU_SERVER_THREAD_PRIORITY, "wlu_eth_server", wlu_server_thread, WLU_SERVER_STACK_SIZE, &wlu_server );

    return WICED_SUCCESS;
}

int
rwl_read_eth_port(void* hndle, char* read_buf, unsigned int data_size, unsigned int *numread)
{
    wiced_result_t  result = WICED_SUCCESS;
    char*           request;
    uint16_t        request_length;
    uint16_t        available_data_length;
    wiced_packet_t* temp_packet = NULL;

    *numread = 0;

    if ( data_size == 0 )
     {
         return SUCCESS;
     }

    if (wiced_tcp_receive( &tcp_client_socket, &temp_packet, WICED_WAIT_FOREVER ) == WICED_SUCCESS)
    {
        result = result;

        if (temp_packet)
        {
            /* Process the client request */
            wiced_packet_get_data( temp_packet, 0, (uint8_t **)&request, &request_length, &available_data_length );
        }

        memcpy(read_buf,request,data_size);

        // read_buf = request;
        *numread = request_length;

        wiced_packet_delete( temp_packet );
    }

    return SUCCESS;
}

int
rwl_write_eth_port(void* hndle, char* write_buf, unsigned long size, unsigned long *numwritten)
{
    uint16_t        available_data_length;
    wiced_packet_t* tx_packet;
    char*           tx_data;
    unsigned long temp_size = 0;
    unsigned long temp=0;

    if ( size == 0 )
     {
         return SUCCESS;
     }

    while (size > 0)
    {
    if (wiced_packet_create_tcp(&tcp_client_socket, 128, &tx_packet, (uint8_t **)&tx_data, &available_data_length) != WICED_SUCCESS)
      {
          WPRINT_APP_INFO(("TCP packet creation failed\n"));
          return WICED_ERROR;
      }
    if (size > available_data_length)
        {
        temp_size = available_data_length;
        }
    else
    {
        temp_size =size;
    }

       /*  Write the message into tx_data"*/
        memcpy(tx_data, (write_buf + temp), temp_size);

        wiced_packet_set_data_end( tx_packet, (uint8_t*)tx_data + temp_size );

        /* Send the TCP packet*/
        if ( wiced_tcp_send_packet( &tcp_client_socket, tx_packet ) != WICED_SUCCESS )
        {
            WPRINT_APP_INFO(("TCP packet send failed \n"));

             /*Delete packet, since the send failed*/
            wiced_packet_delete(tx_packet);
            return WICED_ERROR;
        }

        wiced_packet_delete(tx_packet);
        size = size - temp_size;
        temp = temp+temp_size;

     }

    return available_data_length;
}
#endif
