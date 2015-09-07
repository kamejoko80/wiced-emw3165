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
#include "platform_config.h"
#include "platform_init.h"
#include "platform_m2m.h"
#include "platform_toolchain.h"
#include "platform/wwd_platform_interface.h"

#include "typedefs.h"

#include "wiced_platform.h"
#include "wiced_framework.h"
#include "wiced_deep_sleep.h"
#include "wiced_osl.h"

#include "generated_mac_address.txt"

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef PLATFORM_CPU_CLOCK_FREQUENCY
#define PLATFORM_CPU_CLOCK_FREQUENCY PLATFORM_CPU_CLOCK_FREQUENCY_160_MHZ
#endif

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
 *               Variables Declarations
 ******************************************************/

extern platform_hibernation_t hibernation_config;

/******************************************************
 *               Variables Definitions
 ******************************************************/

static char bcm4390x_platform_inited = 0;

uint32_t platform_capabilities_word = 0;

/*
 * BCM4390x Platform Capabilities Table:
 *
 * Part      PkgOpt[3]   PkgOpt[2:0]   sdio   gmac   usb   hsic   i2s   i2c   uart   ddr   spi   jtag
 * 43909     0           0,1           x      x      x     x      x     x     x      x     x     x
 * 43907     0           2             x      x      x            x     x     x            x     x
 * 43906     0           3             x                          x     x     x            x     x
 * 43905     0           4             x                                x     x            x     x
 * 43904     0           5                           x     x            x     x            x     x
 * 43903,01  0           6                                              x     x            x     x
 *
 * 43909     1           0,1                  x                   x     x     x      x     x
 * 43907     1           2                    x                   x     x     x            x
 * 43906     1           3                                        x     x     x            x
 * 43905     1           4                                              x     x            x
 * 43904     1           5                                              x     x            x
 * 43903,01  1           6                                              x     x            x
 */
static uint32_t platform_capabilities_table[] =
{
    /* 43909 */
    [0]  = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_SDIO | PLATFORM_CAPS_GMAC | PLATFORM_CAPS_USB |
            PLATFORM_CAPS_HSIC   | PLATFORM_CAPS_I2S  | PLATFORM_CAPS_DDR  | PLATFORM_CAPS_JTAG),
    /* 43909 */
    [1]  = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_SDIO | PLATFORM_CAPS_GMAC | PLATFORM_CAPS_USB |
            PLATFORM_CAPS_HSIC   | PLATFORM_CAPS_I2S  | PLATFORM_CAPS_DDR  | PLATFORM_CAPS_JTAG),
    /* 43907 */
    [2]  = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_SDIO | PLATFORM_CAPS_GMAC | PLATFORM_CAPS_USB |
            PLATFORM_CAPS_I2S    | PLATFORM_CAPS_JTAG),
    /* 43906 */
    [3]  = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_SDIO | PLATFORM_CAPS_I2S  | PLATFORM_CAPS_JTAG),
    /* 43905 */
    [4]  = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_SDIO | PLATFORM_CAPS_JTAG),
    /* 43904 */
    [5]  = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_USB  | PLATFORM_CAPS_HSIC | PLATFORM_CAPS_JTAG),
    /* 43903,01 */
    [6]  = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_JTAG),
    [7]  = (0),
    /* 43909 Secure */
    [8]  = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_GMAC | PLATFORM_CAPS_I2S  | PLATFORM_CAPS_DDR),
    /* 43909 Secure */
    [9]  = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_GMAC | PLATFORM_CAPS_I2S  | PLATFORM_CAPS_DDR),
    /* 43907 Secure */
    [10] = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_GMAC | PLATFORM_CAPS_I2S),
    /* 43906 Secure */
    [11] = (PLATFORM_CAPS_COMMON | PLATFORM_CAPS_I2S),
    /* 43905 Secure */
    [12] = (PLATFORM_CAPS_COMMON),
    /* 43904 Secure */
    [13] = (PLATFORM_CAPS_COMMON),
    /* 43903,01 Secure */
    [14] = (PLATFORM_CAPS_COMMON),
    [15] = (0)
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static void platform_capabilities_init( void )
{
    uint32_t package_options = 0;

    /* Extract package options from ChipCommon ChipID register */
    package_options = PLATFORM_CHIPCOMMON->core_ctrl_status.chip_id.bits.package_option;

    /* Initialize the platform capabilities */
    platform_capabilities_word = platform_capabilities_table[package_options];
}

static void platform_init_early_misc( void )
{
    /* Fixup max resource mask register. */
    platform_common_chipcontrol( &PLATFORM_PMU->max_res_mask,
                                 PMU_RES_MAX_CLEAR_MASK,
                                 PMU_RES_MAX_SET_MASK );

    /*
     * Boot strap pins may force ILP be always requested. This prevents deep sleep from working properly.
     * Current register modification clears this request.
     * Boot strap pin issue was observed with FTDI chip which shared strap pin and pulled its up during board reset.
     */
    platform_common_chipcontrol( &PLATFORM_PMU->pmucontrol_ext,
                                 PMU_CONTROL_EXT_ILP_ON_BP_DEBUG_MODE_MASK,
                                 0x0 );

    /*
     * Make sure that board is waking up after reset pin reboot triggered.
     * Software drops min_res_mask to low value for deep sleep, need to restore mask during board resetting.
     * Below statement forces chip to restore min_res_mask to default value when reset pin reboot triggered.
     */
    platform_common_chipcontrol( &PLATFORM_PMU->pmucontrol,
                                 PMU_CONTROL_RESETCONTROL_MASK,
                                 PMU_CONTROL_RESETCONTROL_RESTORE_RES_MASKS );
}

static void platform_slow_clock_init( void )
{
    pmu_slowclkperiod_t period     = { .raw = 0 };
    uint32_t            clock      = osl_alp_clock( );
    unsigned long       alp_period = ( ( 16UL * 1000000UL ) / ( clock / 1024UL ) ); /* 1 usec period of ALP clock measured in 1/16384 usec of ALP clock */

    period.bits.one_mhz_toggle_en = 1;
    period.bits.alp_period        = alp_period & PMU_SLOWCLKPERIOD_ALP_PERIOD_MASK;

    PLATFORM_PMU->slowclkperiod.raw = period.raw;
}

/* BCM4390x common clock initialisation function
 * - weak attribute is intentional in case a specific platform (board) needs to override this.
 */
WEAK void platform_init_system_clocks( void )
{
    platform_mcu_powersave_warmboot_init( );

    platform_init_early_misc( );

    platform_backplane_init( );

    platform_hibernation_init( &hibernation_config );

    platform_lpo_clock_init( );

    platform_cpu_clock_init( PLATFORM_CPU_CLOCK_FREQUENCY );

    platform_slow_clock_init( );
}

/* BCM4390x memory initialisation function
 * - weak attribute is intentional in case a specific platform (board) needs to override this.
 */
WEAK void platform_init_memory( void )
{
}

void platform_init_mcu_infrastructure( void )
{
    if ( bcm4390x_platform_inited == 1 )
    {
        return;
    }

    /* Initialize platform capabilities */
    platform_capabilities_init( );

    /* Start watchdog */
    platform_watchdog_init( );

    /* Initialize MCU powersave */
    platform_mcu_powersave_init( );

    /* Initialize interrupts */
    platform_irq_init( );

    /* Ensure 802.11 device is in reset. */
    host_platform_init( );


    /* Initialise external serial flash */
    platform_sflash_init( );

    bcm4390x_platform_inited = 1;
}

WEAK void platform_init_complete( void )
{
    /* Notify if returned from deep-sleep */
    WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS( WICED_DEEP_SLEEP_IS_WARMBOOT( ), WICED_DEEP_SLEEP_EVENT_LEAVE );
}

/******************************************************
 *                 DCT Functions
 ******************************************************/

wwd_result_t host_platform_get_mac_address( wiced_mac_t* mac )
{
#if 0 //ifndef WICED_DISABLE_BOOTLOADER
    wiced_mac_t* temp_mac;
    wiced_dct_read_lock( (void**)&temp_mac, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, OFFSETOF(platform_dct_wifi_config_t, mac_address), sizeof(wiced_mac_t) );
    memcpy(mac->octet, temp_mac, sizeof(wiced_mac_t));
    wiced_dct_read_unlock( temp_mac, WICED_FALSE );
#else
    UNUSED_PARAMETER( mac );
#endif
    return WWD_SUCCESS;
}

#ifdef WICED_USE_ETHERNET_INTERFACE
wwd_result_t host_platform_get_ethernet_mac_address( wiced_mac_t* mac )
{
#ifdef WICED_DISABLE_BOOTLOADER
    memcpy( mac->octet, DCT_GENERATED_ETHERNET_MAC_ADDRESS, sizeof( wiced_mac_t ) );
#else
    wiced_mac_t* temp_mac;
    wiced_dct_read_lock( (void**) &temp_mac, WICED_FALSE, DCT_ETHERNET_CONFIG_SECTION, OFFSETOF( platform_dct_ethernet_config_t, mac_address ), sizeof(wiced_mac_t) );
    memcpy( mac->octet, temp_mac, sizeof(wiced_mac_t) );
    wiced_dct_read_unlock( temp_mac, WICED_FALSE );
#endif
    return WWD_SUCCESS;
}
#endif

/******************************************************
 *            Interrupt Service Routines
 ******************************************************/
