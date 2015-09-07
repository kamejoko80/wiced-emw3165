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
 *  Platform Deep Sleep Functions
 */
#include "wiced_deep_sleep.h"

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

extern wiced_deep_sleep_event_registration_t link_deep_sleep_event_registrations_location;
extern wiced_deep_sleep_event_registration_t link_deep_sleep_event_registrations_end;

/******************************************************
 *               Function Definitions
 ******************************************************/

void wiced_deep_sleep_call_event_handlers( wiced_deep_sleep_event_type_t event )
{
    wiced_deep_sleep_event_registration_t* p;

    for ( p = &link_deep_sleep_event_registrations_location; p < &link_deep_sleep_event_registrations_end; ++p )
    {
        (*p->handler)( event );
    }
}
