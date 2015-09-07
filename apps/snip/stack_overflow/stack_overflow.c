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
 * Stack Overflow Application
 *
 * Features demonstrated
 *  - How to set the application stack size
 *
 * The Application Stack
 *   The main application runs in its own thread, and just like
 *   every thread in the system, the app thread has a stack
 *   to manipulate local variables.
 *
 *   By default, the application thread stack size is set by the
 *   the #define WICED_DEFAULT_APPLICATION_STACK_SIZE in wiced_defaults.h.
 *
 *   To change the application stack size, the default stack size
 *   can be changed, or alternately, a global define can be added
 *   to the application makefile (located in the same directory as
 *   this file) as follows:
 *      #GLOBAL_DEFINES := APPLICATION_STACK_SIZE=XXXXX
 *
 * Overflowing the Stack
 *   If the application declares a local variable that requires more
 *   memory than is available on stack, the application stack will
 *   overflow.
 *
 *   This will have an indeterminate effect on the system, since
 *   the area of memory that is inadvertently overwritten may be
 *   critical to system operation, or it may be otherwise unused
 *   memory. Either way, it's a very bad idea to let this happen!
 *
 *   The demo included in this file calls a function that declares
 *   an array with 10240 bytes. Since the default application stack
 *   size is 6192 bytes, when the function is called, the stack
 *   is blown with unknown affects. In actual fact, since this app
 *   is so simple, there may not actually be any noticeable side
 *   effects -- but you can never be sure, and therein lies the
 *   trouble.
 *
 */

#include "wiced.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef APPLICATION_STACK_SIZE
#define APPLICATION_STACK_SIZE   WICED_DEFAULT_APPLICATION_STACK_SIZE
#endif

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

static void blow_the_stack(void);

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{

    WPRINT_APP_INFO( ( "Application stack = %u\r\n", APPLICATION_STACK_SIZE ) );

    blow_the_stack();

}

static void blow_the_stack(void)
{
    uint8_t excessively_large_array[10240];

    memset(excessively_large_array, 0, sizeof(excessively_large_array));
}
