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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "wiced.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define TRACE_TYPE_ERROR            0x00000000
#define TRACE_TYPE_MASK             0x000000ff
#define TRACE_GET_TYPE(x)           (((uint32_t)(x)) & TRACE_TYPE_MASK)

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

/******************************************************
 *               Variables Definitions
 ******************************************************/

extern wiced_mutex_t global_trace_mutex;

uint8_t wiced_log_enabled = 1;

/******************************************************
 *               Function Definitions
 ******************************************************/

void LogMsg( uint32_t trace_set_mask, const char *fmt_str, ... )
{
    char buffer[256];
    va_list ap;

    if ((!wiced_log_enabled) && (TRACE_GET_TYPE(trace_set_mask) != TRACE_TYPE_ERROR))
    {
        /* If  wiced logging disabled, then only log errors */
        return;
    }

#ifdef USE_ISO8601_TIME
    wiced_iso8601_time_t iso8601_time;
    wiced_time_get_iso8601_time(&iso8601_time);

    // wiced_iso8601_time_t is a structure containing all the time fields
    // We "fake" a real time *string* by setting the 26th byte to null
    // This gets us HH:MM:SS.SSSSSS (no UTC timezone) when we print from iso8601_time.hour
    // DO NOT CHNANGE THIS!
    memset(((char*)&iso8601_time) + 26, 0, 1);

    sprintf(timeBuf, "%s", iso8601_time.hour);
#else
    wiced_time_t time_ms;
    wiced_time_get_time( &time_ms );
#endif

    va_start(ap, fmt_str);
    vsprintf(buffer, fmt_str, ap);
    va_end(ap);


    wiced_rtos_lock_mutex(&global_trace_mutex);
#ifdef USE_ISO8601_TIME
    WPRINT_APP_INFO(("%s %s\r\n", timeBuf, buffer)); // Append user message with time
#else
    WPRINT_APP_INFO(("%08lu %s\r\n", time_ms, buffer)); // Append user message with time
#endif
    wiced_rtos_unlock_mutex(&global_trace_mutex);
}

void ScrLog( uint32_t trace_set_mask, const char *fmt_str, ... )
{
    char buffer[256];
    char timeBuf[16];
    va_list ap;

    wiced_iso8601_time_t iso8601_time;
    wiced_time_get_iso8601_time(&iso8601_time);

    if ((!wiced_log_enabled) && (TRACE_GET_TYPE(trace_set_mask) != TRACE_TYPE_ERROR))
    {
        /* If  wiced logging disabled, then only log errors */
        return;
    }

    memset(((char*)&iso8601_time) + 26, 0, 1);

    sprintf(timeBuf, "%s", iso8601_time.hour);

    va_start(ap, fmt_str);
    vsprintf(buffer, fmt_str, ap);
    va_end(ap);

    wiced_rtos_lock_mutex(&global_trace_mutex);
    WPRINT_APP_INFO(("%s %s\r\n", timeBuf, buffer)); // Append user message with time
    wiced_rtos_unlock_mutex(&global_trace_mutex);
}


/********************************************************************************
 **
 **    Function Name:   LogMsg_0
 **
 **    Purpose:  Encodes a trace message that has no parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_0( uint32_t trace_set_mask, const char *fmt_str )
{
    LogMsg( trace_set_mask, fmt_str );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_1
 **
 **    Purpose:  Encodes a trace message that has one parameter argument
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_1( uint32_t trace_set_mask, const char *fmt_str, uint32_t* p1 )
{
    LogMsg( trace_set_mask, fmt_str, p1 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_2
 **
 **    Purpose:  Encodes a trace message that has two parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_2( uint32_t trace_set_mask, const char *fmt_str, uint32_t* p1, uint32_t* p2 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_3
 **
 **    Purpose:  Encodes a trace message that has three parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_3( uint32_t trace_set_mask, const char *fmt_str, uint32_t* p1, uint32_t* p2, uint32_t* p3 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2, p3 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_4
 **
 **    Purpose:  Encodes a trace message that has four parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_4( uint32_t trace_set_mask, const char *fmt_str, uint32_t* p1, uint32_t* p2, uint32_t* p3, uint32_t* p4 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2, p3, p4 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_5
 **
 **    Purpose:  Encodes a trace message that has five parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_5( uint32_t trace_set_mask, const char *fmt_str, uint32_t* p1, uint32_t* p2, uint32_t* p3, uint32_t* p4, uint32_t* p5 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2, p3, p4, p5 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_6
 **
 **    Purpose:  Encodes a trace message that has six parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_6( uint32_t trace_set_mask, const char *fmt_str, uint32_t* p1, uint32_t* p2, uint32_t* p3, uint32_t* p4, uint32_t* p5, uint32_t* p6 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2, p3, p4, p5, p6 );
}

