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
 * Defines BCM439x non-WWD bus functions
 */
#include <stdint.h>
#include "wwd_constants.h"
#include "wwd_assert.h"
#include "platform_peripheral.h"
#include "platform/wwd_platform_interface.h"
#include "platform_cmsis.h"
#include "platform_config.h"

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wwd_result_t host_platform_init( void )
{
#ifdef USES_RESOURCE_FILESYSTEM
    platform_filesystem_init();
#endif
    host_platform_power_wifi( WICED_FALSE );
    return WWD_SUCCESS;
}

wwd_result_t host_platform_deinit( void )
{
    host_platform_power_wifi( WICED_FALSE );
    return WWD_SUCCESS;
}

void host_platform_reset_wifi( wiced_bool_t reset_asserted )
{
    if ( reset_asserted == WICED_TRUE )
    {
        host_platform_power_wifi( WICED_FALSE );
    }
    else
    {
        host_platform_power_wifi( WICED_TRUE );
    }
}

void host_platform_power_wifi( wiced_bool_t power_enabled )
{
    if ( power_enabled == WICED_FALSE )
    {
        /* Enable WLAN SRAM */
        platform_pmu_wifi_sram_on( );

        platform_pmu_wifi_allowed_to_sleep( );
        platform_pmu_wifi_off( );

        /* delay 100 ms */
        host_rtos_delay_milliseconds( 100 );
    }
    else
    {
        /* Enable WLAN SRAM */
        platform_pmu_wifi_sram_on( );

        /* Power and wake up WLAN core */
        platform_pmu_wifi_on( );
        platform_pmu_wifi_wake_up( );

        /* Disable WLAN SRAM. This is okay because apps core holds WLAN up */
        platform_pmu_wifi_sram_off( );
    }
}


wwd_result_t host_platform_init_wlan_powersave_clock( void )
{
    /* Nothing to do here */
    return WWD_SUCCESS;
}

wwd_result_t host_platform_deinit_wlan_powersave_clock( void )
{
    /* Nothing to do here */
    return WWD_SUCCESS;
}
