/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "wwd_bus_protocol.h"
#include "typedefs.h"
//#include "Platform/wwd_sdio_interface.h"
#include "platform/wwd_bus_interface.h"
#include "wwd_assert.h"
#include "RTOS/wwd_rtos_interface.h"
#include "string.h" /* For memcpy */
//#include "gpio_irq.h"
#include "platform_config.h"
#include "platform_appscr4.h"


/******************************************************
 *             Constants
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/

/******************************************************
 *             Function declarations
 ******************************************************/

/******************************************************
 *             Function definitions
 ******************************************************/



wwd_result_t host_platform_bus_init( void )
{
    return  WWD_SUCCESS;
}


wwd_result_t host_platform_bus_enable_interrupt( void )
{
#ifndef M2M_RX_POLL_MODE
    platform_irq_enable_irq(M2M_ExtIRQn);
    platform_irq_enable_irq(SW0_ExtIRQn);
#endif
    return  WWD_SUCCESS;
}

wwd_result_t host_platform_bus_disable_interrupt( void )
{
#ifndef M2M_RX_POLL_MODE
    platform_irq_disable_irq(M2M_ExtIRQn);
    platform_irq_disable_irq(SW0_ExtIRQn);
#endif
    return  WWD_SUCCESS;
}



wwd_result_t host_platform_bus_deinit( void )
{
    return WWD_SUCCESS;
}




