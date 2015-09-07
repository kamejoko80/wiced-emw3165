/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "platform_cmsis.h"
#include "platform/wwd_platform_interface.h"

uint32_t host_platform_get_cycle_count( void )
{
    /* From the ARM Cortex-M3 Techinical Reference Manual
     * 0xE0001004  DWT_CYCCNT  RW  0x00000000  Cycle Count Register */
    return DWT->CYCCNT;
}

wiced_bool_t host_platform_is_in_interrupt_context( void )
{
    /* From the ARM Cortex-M3 Techinical Reference Manual
     * 0xE000ED04   ICSR    RW [a]  Privileged  0x00000000  Interrupt Control and State Register */
    return ( ( SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk ) != 0 ) ? WICED_TRUE : WICED_FALSE;
}
