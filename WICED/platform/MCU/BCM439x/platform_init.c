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
 * Define default BCM439x initialisation functions
 */
#include "wiced_defaults.h"
#include "platform_init.h"
#include "platform_peripheral.h"
#include "platform_sleep.h"
#include "platform_cmsis.h"
#include "platform_config.h"
#include "platform_toolchain.h"
#include "platform/wwd_platform_interface.h"
#include "network/wwd_network_constants.h"

#ifndef WICED_DISABLE_BOOTLOADER
#include "wiced_framework.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WATCHDOG_TIMEOUT_MULTIPLIER     ( 0x1745D17 )

#ifndef MAX_WATCHDOG_TIMEOUT_SECONDS
#define MAX_WATCHDOG_TIMEOUT_SECONDS    ( 22 )
#endif

#ifdef APPLICATION_WATCHDOG_TIMEOUT_SECONDS
#define WATCHDOG_TIMEOUT                ( APPLICATION_WATCHDOG_TIMEOUT_SECONDS * WATCHDOG_TIMEOUT_MULTIPLIER )
#else
#define WATCHDOG_TIMEOUT                ( MAX_WATCHDOG_TIMEOUT_SECONDS * WATCHDOG_TIMEOUT_MULTIPLIER )
#endif /* APPLICATION_WATCHDOG_TIMEOUT_SECONDS */

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

char bcm439x_platform_inited = 0;

/* This variable is used internally to set the watchdog timeout */
const uint32_t watchdog_timeout = WATCHDOG_TIMEOUT;

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Reset request */
 /*
  * Cortex M3 provides two reset control features by setting respective bit in application interrupt and reset control register
  * SYSRESETREQ (bit 2)--Which will reset whole system.
  * VECTRESET (bit 0)  --Which will reset only ARM core and not the other peripherals
  * If we are using SYSRESETREQ bit set it will signal reset signal generator which is not a
  * part of ARM CM3 core, it specific to chip design.
  *
  * SYSRESETREQ signal not connected to anything on BCM4390
  *
  * To overcome this we are triggering watchdog to reset by making count to 0 Forcibly */

void platform_mcu_reset( void )
{
    /*if BCM439x platform do Forcibly  reset the watchdog*/
    platform_watchdog_force_reset( );

    /* Loop forever */
    while ( 1 )
    {
    }
}

/* BCM439x common clock initialisation function
 * - weak attribute is intentional in case a specific platform (board) needs to override this.
 */
WEAK void platform_init_system_clocks( void )
{
    /* System clock initialisation is already done in ROM bootloader */
}

WEAK void platform_init_memory( void )
{
    /* Init WLAN SRAM for apps core to use */
    platform_pmu_wifi_sram_init();
}

void platform_init_mcu_infrastructure( void )
{
    if ( bcm439x_platform_inited == 1 )
    {
        return;
    }

    platform_apps_core_init();
    platform_mcu_powersave_disable( );

#ifndef WICED_DISABLE_WATCHDOG
    platform_watchdog_init( );
#endif

    platform_init_rtos_irq_priorities( );
    platform_init_peripheral_irq_priorities( );

    /* Initialise GPIO IRQ manager */
    platform_gpio_irq_manager_init();

    /* Initialise external serial flash */
    platform_sflash_init();

    bcm439x_platform_inited = 1;
}

void platform_init_connectivity_module( void )
{
    /* Ensure 802.11 device is in reset. */
    host_platform_init( );
}

WEAK void platform_init_external_devices( void )
{

}
