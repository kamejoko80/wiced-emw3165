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
 */

#include <string.h>
#include <stdlib.h>

#include "wwd_wifi.h"
#include "wwd_debug.h"
#include "wwd_constants.h"
#include "internal/wwd_bcmendian.h"
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"

#include "platform_map.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WLAN_SHARED_ADDR (PLATFORM_ATCM_RAM_BASE(0) + PLATFORM_WLAN_RAM_SIZE -4)
#define CBUF_LEN 128
#define WLAN_SHARED_VERSION_MASK    0x00ff
#define WLAN_SHARED_VERSION         0x0001

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef unsigned int uint;

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint buf;
    uint buf_size;
    uint idx;
    uint out_idx; /* output index */
} hnd_log_t;

typedef struct
{
    /* Virtual UART
     *   When there is no UART (e.g. Quickturn), the host should write a complete
     *   input line directly into cbuf and then write the length into vcons_in.
     *   This may also be used when there is a real UART (at risk of conflicting with
     *   the real UART).  vcons_out is currently unused.
     */
    volatile uint vcons_in;
    volatile uint vcons_out;

    /* Output (logging) buffer
     *   Console output is written to a ring buffer log_buf at index log_idx.
     *   The host may read the output when it sees log_idx advance.
     *   Output will be lost if the output wraps around faster than the host polls.
     */
    hnd_log_t log;

    /* Console input line buffer
     *   Characters are read one at a time into cbuf until <CR> is received, then
     *   the buffer is processed as a command line.  Also used for virtual UART.
     */
    uint cbuf_idx;
    char cbuf[ CBUF_LEN ];
} hnd_cons_t;

typedef struct wifi_console
{
    uint count; /* Poll interval msec counter */
    uint log_addr; /* Log struct address (fixed) */
    hnd_log_t log; /* Log struct (host copy) */
    uint bufsize; /* Size of log buffer */
    char *buf; /* Log buffer (host copy) */
    uint last; /* Last buffer read index */
} wifi_console_t;

typedef struct
{
    uint flags;
    uint trap_addr;
    uint assert_exp_addr;
    uint assert_file_addr;
    uint assert_line;
    uint console_addr;
    uint msgtrace_addr;
    uint fwid;
} wlan_shared_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

static wifi_console_t *c;
wifi_console_t console;
static wlan_shared_t sh;
static uint console_addr = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

wwd_result_t wwd_wifi_read_wlan_log( char* buffer, uint32_t buffer_size )
{
    char ch;
    uint32_t n;
    uint32_t index;
    uint32_t address;
    c = &console;

    if ( console_addr == 0 )
    {
        uint shared_addr;

        address = WLAN_SHARED_ADDR;
        wwd_bus_set_backplane_window( address );
        wwd_bus_transfer_bytes( BUS_READ, BACKPLANE_FUNCTION, ( address & BACKPLANE_ADDRESS_MASK ), 4, (wwd_transfer_bytes_packet_t*) ( (char *) &shared_addr - WWD_BUS_HEADER_SIZE ) );
        wwd_bus_set_backplane_window( shared_addr );
        wwd_bus_transfer_bytes( BUS_READ, BACKPLANE_FUNCTION, ( shared_addr & BACKPLANE_ADDRESS_MASK ), sizeof(wlan_shared_t), (wwd_transfer_bytes_packet_t*) ( (char *) &sh - WWD_BUS_HEADER_SIZE ) );

        sh.flags            = ltoh32( sh.flags );
        sh.trap_addr        = ltoh32( sh.trap_addr );
        sh.assert_exp_addr  = ltoh32( sh.assert_exp_addr );
        sh.assert_file_addr = ltoh32( sh.assert_file_addr );
        sh.assert_line      = ltoh32( sh.assert_line );
        sh.console_addr     = ltoh32( sh.console_addr );
        sh.msgtrace_addr    = ltoh32( sh.msgtrace_addr );

        if ( ( sh.flags & WLAN_SHARED_VERSION_MASK ) != WLAN_SHARED_VERSION )
        {
            WPRINT_APP_INFO( ( "Readconsole: WLAN shared version is not valid\n\r") );
            return WWD_WLAN_ERROR;
        }
        console_addr = sh.console_addr;
    }

    /* Read console log struct */
    address = console_addr + offsetof( hnd_cons_t, log );
    memcpy( (char *) &c->log, (char *) address, sizeof( c->log ) );
    wwd_bus_set_backplane_window( address );
    wwd_bus_transfer_bytes( BUS_READ, BACKPLANE_FUNCTION, ( address & BACKPLANE_ADDRESS_MASK ), sizeof( c->log ), (wwd_transfer_bytes_packet_t*) ( (char *) &c->log - WWD_BUS_HEADER_SIZE ) );

    /* Allocate console buffer (one time only) */
    if ( c->buf == NULL )
    {
        c->bufsize = ltoh32( c->log.buf_size );
        c->buf = malloc( c->bufsize );
        if ( c->buf == NULL )
        {
            return WWD_WLAN_ERROR;
        }
    }

    index = ltoh32( c->log.idx );

    /* Protect against corrupt value */
    if ( index > c->bufsize )
    {
        return WWD_WLAN_ERROR;
    }

    /* Skip reading the console buffer if the index pointer has not moved */
    if ( index == c->last )
    {
        return WWD_SUCCESS;
    }

    /* Read the console buffer */
    /* xxx this could optimize and read only the portion of the buffer needed, but
     * it would also have to handle wrap-around.
     */
    address = ltoh32( c->log.buf );
    memcpy( c->buf, (char *) address, c->bufsize );

    while ( c->last != index )
    {
        for ( n = 0; n < buffer_size - 2; n++ )
        {
            if ( c->last == index )
            {
                /* This would output a partial line.  Instead, back up
                 * the buffer pointer and output this line next time around.
                 */
                if ( c->last >= n )
                {
                    c->last -= n;
                }
                else
                {
                    c->last = c->bufsize - n;
                }
                return WWD_SUCCESS;
            }
            ch = c->buf[ c->last ];
            c->last = ( c->last + 1 ) % c->bufsize;
            if ( ch == '\n' )
            {
                break;
            }
            buffer[ n ] = ch;
        }

        if ( n > 0 )
        {
            if ( buffer[ n - 1 ] == '\r' )
                n--;
            buffer[ n ] = 0;
            WPRINT_APP_INFO( ("CONSOLE: %s\n", buffer) );
        }
    }

    return WWD_SUCCESS;
}
