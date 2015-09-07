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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "command_console.h"
#include "wiced_rtos.h"
#include "wiced_defaults.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define CONSOLE_THREAD_STACKSIZE    (4096)
#define MAX_SPAWNED_THREADS         (5)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    wiced_thread_t     thread;
    wiced_bool_t       thread_running;
    char*              line;
    int                return_value;
} thread_details_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static void thread_wrapper( uint32_t arg );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_bool_t threads_inited = WICED_FALSE;
static thread_details_t* spawned_threads[MAX_SPAWNED_THREADS] = {0};

/******************************************************
 *               Function Definitions
 ******************************************************/

int thread_spawn( int argc, char *argv[] )
{
    thread_details_t* detail = NULL;

    if (threads_inited == WICED_FALSE)
    {
        memset(spawned_threads, 0, sizeof(spawned_threads));
        threads_inited = WICED_TRUE;
    }

    if ( argc < 2 )
    {
        return ERR_INSUFFICENT_ARGS;
    }

    /* Find a spare thread detail */
    int a;
    for (a=0; a < MAX_SPAWNED_THREADS; ++a)
    {
        if (spawned_threads[a] == NULL)
        {
            spawned_threads[a] = calloc( 1, sizeof( thread_details_t ) );
            detail = spawned_threads[a];
            break;
        }
        else if (spawned_threads[a]->thread_running == WICED_FALSE)
        {
            wiced_rtos_delete_thread(&spawned_threads[a]->thread);
            detail = spawned_threads[a];
            break;
        }
    }

    if ( detail == NULL )
    {
        return ERR_UNKNOWN;
    }

    /*
     * Check if a priority was specified.
     *
     * Note that this will fail if a legitimate console command consists purely
     * of numbers, as this function simply checks that the first argument is
     * numeric.
     */
    char *c;
    int is_numeric = WICED_TRUE; /* assume until proven otherwise */
    for ( c = argv[1]; *c != '\0' && is_numeric; c++ )
    {
        if ( !isdigit( (int) *c ) )
        {
            is_numeric = WICED_FALSE;
        }
    }

    int priority_arg = WICED_DEFAULT_LIBRARY_PRIORITY;
    if ( is_numeric )
    {
        /* Check if the specified thread priority is valid */
        priority_arg = atoi( argv[1] );
//        if ( priority_arg < 0 || (unsigned long) priority_arg >= RTOS_HIGHEST_PRIORITY )
//        {
//            printf( "%d is not a valid thread priority. Resetting to %u.\r\n", priority_arg, RTOS_DEFAULT_THREAD_PRIORITY );
//            priority_arg = RTOS_DEFAULT_THREAD_PRIORITY;
//        }

        /* Make sure a command was specified */
        if ( argc < 3 )
        {
            return ERR_INSUFFICENT_ARGS;
        }

        argc -= 2;
        argv = &argv[2];
    }
    else
    { /* No priority specified */
        /* Get the arguments for the command to be spawned */
        argc--;
        argv = &argv[1];
    }

    /* Count the total argument length */
    uint32_t command_size = 0;
    int i;
    for( i = 0; i < argc; i++ )
    {
        command_size += strlen( argv[i] );
    }
    command_size += argc; /* Add space for delimiting spaces and terminating null */

    detail->line = (char*) calloc( 1, command_size );
    memset(detail->line, 0, command_size);
    for( i = 0; i < argc; i++ )
    {
        strcat( detail->line, argv[i] );
        strcat( detail->line, " " );
    }
    detail->line[ command_size - 1] = '\x00';

    /* Spawn the new thread */
    detail->thread_running = WICED_TRUE;
    if (wiced_rtos_create_thread(&detail->thread, priority_arg, argv[0], thread_wrapper, CONSOLE_THREAD_STACKSIZE, detail) != WICED_SUCCESS)
    {
        detail->thread_running = WICED_FALSE;
        return ERR_UNKNOWN;
    }

    return ERR_CMD_OK;
}


static void thread_wrapper( uint32_t arg )
{
    thread_details_t *detail = (thread_details_t*) arg;

    printf( "Started thread %p (\"%s\")\r\n", &detail->thread, detail->line );

    /* Execute the command and fetch the return value */
    detail->return_value = (int) console_parse_cmd( detail->line );
    printf( "Thread %p (""%s"") exited with return value %d\r\n", &detail->thread, detail->line, detail->return_value );
    free( detail->line );

    detail->thread_running = WICED_FALSE;
    WICED_END_OF_CURRENT_THREAD( );
}

int thread_list( int argc, char* argv[] )
{
    int a;
    printf("Running threads:\r\n");
    for (a=0; a < MAX_SPAWNED_THREADS; ++a)
    {
        if (spawned_threads[a] != NULL && spawned_threads[a]->thread_running == WICED_TRUE)
        {
            printf("  %d: %s\r\n", a, spawned_threads[a]->line);
        }
    }
    return ERR_CMD_OK;
}

int thread_kill( int argc, char* argv[] )
{
    int a = atoi(argv[1]);
    if (a >= 0 && a < MAX_SPAWNED_THREADS && spawned_threads[a] != NULL && spawned_threads[a]->thread_running == WICED_TRUE)
    {
//        tx_thread_terminate(&spawned_threads[a]->thread_handle);
//        spawned_threads[a]->thread_running = WICED_FALSE;
//        printf("Thread %d has been terminated\r\n", a);
        printf("Not supported\r\n");
    }
    else
    {
        printf("Thread not found\r\n");
    }
    return ERR_CMD_OK;
}
