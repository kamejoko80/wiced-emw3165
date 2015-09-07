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
 * Define default LPC18xx initialization functions
 */
#include "platform_init.h"
#include "platform_config.h"
#include "platform_toolchain.h"
#include "platform_peripheral.h"
#include "platform/wwd_platform_interface.h"
#include "chip.h"

/******************************************************
 *                      Macros
 ******************************************************/
/*
 * If desired clocked frequency is larger, special
 * staged clock initialization is required
 * Wait this period for crystal oscillator to settle.
 */
#define WAIT_250US_IRC_MODE 1500
/*
 * Specifies time to wait after setting base clock source.
 * The value given here is for 50us for clock of 204Mhz
 * Can be used in wait loop with any frequency equal to or less then
 * then 204MHz
 */
#define WAIT_50US_AT_MAX_FREQ  10200

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
/* Structure for initial base clock states */
struct CLK_BASE_STATES {
    CHIP_CGU_BASE_CLK_T clk;            /* Base clock */
    CHIP_CGU_CLKIN_T    clkin;          /* Base clock source, see UM for allowable souorces per base clock */
    bool                autoblock_enab; /* Set to true to enable autoblocking on frequency change */
    bool                powerdn;        /* Set to true if the base clock is initially powered down */
};
/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/
/*
 * Specifies the value in Hz for the external oscillator for the board.
 * The variable is used by LPC BSP for clock configuration.
 * For more details see chip.h.
 */
const uint32_t OscRateIn = OSCILLATOR_IN;
/*
 * Specifies the value in Hz for CLKIN for the board.
 * The variable is used by LPC BSP for clock configuration.
 * The current implementation doesn't support CLKIN configurtion. The
 * variable should be set to 0.
 * For more details see chip.h.
 */
const uint32_t ExtRateIn = 0;

/* Initial base clock states are mostly on */
static const struct CLK_BASE_STATES InitClkStates[] =
{
    {CLK_BASE_PHY_TX, CLKIN_ENET_TX, true, false},
    {CLK_BASE_PHY_RX, CLKIN_ENET_RX, true, false},
    {CLK_BASE_SDIO,   CLKIN_MAINPLL, true, false},
};

/******************************************************
 *               Function Definitions
 ******************************************************/

/* common clock initialization function
 * This brings up enough clocks to allow the processor to run quickly while initializing memory.
 * Other platform specific clock init can be done in init_platform() or init_host_mcu()
 */
WEAK void platform_init_system_clocks( void )
{
    uint32_t i;

    CHIP_CGU_CLKIN_T core_clock_input;
    bool autoblocken, powerdn;
    UNUSED_PARAMETER(autoblocken);
    UNUSED_PARAMETER(powerdn);
    Chip_Clock_GetBaseClockOpts( CLK_BASE_MX, &core_clock_input, &autoblocken, &powerdn );

    /* Init clocks only when we enter this code from the NXP bootloader, do not do extra clock initialization when
     * jumping from our bootloader to the application. In case when the application is running with no
     * bootloader upon reset CPU will execute NXP bootloader anyway and will initialize the clocks
     * According to the reference manual: After the NXP boot code is completed the clock configuration is as follows
     * The core clock BASE_M3_CLK is connected to the output of PLL1 and running at
       96 MHz.
       The clock source for the PLL1 is the 12 MHz IRC. */
    if ( core_clock_input == CLKIN_IRC || ( ( ( LPC_CGU->PLL1_CTRL & 0x1F000000 ) >> 24 ) == 0x01 ) )
    {
        /* Setup flash acceleration to target clock rate prior to clock switch. */
        Chip_SetupCoreClock(CLKIN_CRYSTAL, CPU_CLOCK_HZ, true);
        Chip_CREG_SetFlashAcceleration( CPU_CLOCK_HZ );
        /* Setup system base clocks and initial states. This won't enable and
         * disable individual clocks, but sets up the base clock sources for
         * each individual peripheral clock.
         */
        for (i = 0; i < (sizeof(InitClkStates) / sizeof(InitClkStates[0])); i++)
        {
            Chip_Clock_SetBaseClock(InitClkStates[i].clk, InitClkStates[i].clkin,
                                    InitClkStates[i].autoblock_enab, InitClkStates[i].powerdn);
        }
    #if (CPU_CLOCK_HZ >= 180000000)
        Chip_Clock_SetDivider(CLK_IDIV_E, CLKIN_MAINPLL, 5);
    #else
        Chip_Clock_SetDivider(CLK_IDIV_E, CLKIN_MAINPLL, 4);
    #endif
        Chip_Clock_SetBaseClock(CLK_BASE_SPIFI, CLKIN_IDIVE, true, false);
        SystemCoreClockUpdate( );
    }
}

WEAK void platform_init_memory( void )
{

}

void platform_mcu_reset( void )
{
    NVIC_SystemReset();

    /* Loop forever */
    while ( 1 )
    {
    }
}


void platform_init_mcu_infrastructure( void )
{
    uint8_t i;

    /* Initialize interrupt priorities */
    for ( i = 0; i < 46; i++ )
    {
        NVIC_SetPriority( i, 0xFF );
    }
    platform_gpio_irq_init( );
    platform_init_rtos_irq_priorities( );
    platform_init_peripheral_irq_priorities( );
}

void platform_init_connectivity_module( void )
{
    /* Ensure 802.11 device is in reset. */
    host_platform_init( );
}

WEAK void platform_init_external_devices( void )
{

}
