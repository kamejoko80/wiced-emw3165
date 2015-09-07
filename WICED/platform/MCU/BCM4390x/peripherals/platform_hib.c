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

#include "platform_peripheral.h"
#include "platform_appscr4.h"
#include "platform_config.h"

#include "wwd_assert.h"

#include "typedefs.h"
#include "wiced_osl.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define PLATFORM_HIBERNATE_CLOCK_INTERNAL_FREQ_128KHZ  128000
#define PLATFORM_HIBERNATE_CLOCK_INTERNAL_FREQ_32KHZ   32000
#define PLATFORM_HIBERNATE_CLOCK_INTERNAL_FREQ_16KHZ   16000
#define PLATFORM_HIBERNATE_CLOCK_EXTERNAL_DEFAULT_FREQ 32768

#define PLATFORM_HIBERNATE_CLOCK_AS_EXT_LPO            ( PLATFORM_LPO_CLOCK_EXT && PLATFORM_HIB_CLOCK_AS_EXT_LPO )
#define PLATFORM_HIBERNATE_CLOCK_INIT                  ( PLATFORM_HIB_ENABLE || PLATFORM_HIBERNATE_CLOCK_AS_EXT_LPO )
#define PLATFORM_HIBERNATE_CLOCK_POWER_UP              ( PLATFORM_HIBERNATE_CLOCK_INIT || PLATFORM_HIB_CLOCK_POWER_UP )

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

/******************************************************
 *               Function Definitions
 ******************************************************/

static platform_result_t platform_hibernate_clock_power_up( const platform_hibernation_t* hib )
{
#if PLATFORM_HIBERNATE_CLOCK_POWER_UP

    const wiced_bool_t external_clock = ( hib->clock == PLATFORM_HIBERNATION_CLOCK_EXTERNAL );
    uint32_t wake_ctl_mask            = 0x0;
    uint32_t wake_ctl_val             = 0x0;

    if ( !external_clock )
    {
        switch ( hib->clock )
        {
            case PLATFORM_HIBERNATION_CLOCK_INTERNAL_128KHZ:
                wake_ctl_val |= GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_FREQ_128KHZ;
                break;

            case PLATFORM_HIBERNATION_CLOCK_INTERNAL_32KHZ:
                wake_ctl_val |= GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_FREQ_32KHZ;
                break;

            case PLATFORM_HIBERNATION_CLOCK_INTERNAL_16KHZ:
                wake_ctl_val |= GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_FREQ_16KHZ;
                break;

            default:
                wiced_assert( "unknown requested clock", 0 );
                return PLATFORM_BADARG;
        }

        wake_ctl_mask |= GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_FREQ_MASK;
    }

    if ( hib->rc_code >= 0 )
    {
        wake_ctl_val  |= GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_RC_VAL( (uint32_t)hib->rc_code );
        wake_ctl_mask |= GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_RC_MASK;
    }

    if ( wake_ctl_mask || wake_ctl_val )
    {
        platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_WAKE_CTL_REG, wake_ctl_mask, wake_ctl_val );
    }

    if ( external_clock )
    {
        platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_WAKE_CTL_REG, GCI_CHIPCONTROL_HIB_WAKE_CTL_XTAL_DOWN_MASK, 0x0 );
    }
    else
    {
        platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_WAKE_CTL_REG, GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_DOWN_MASK, 0x0 );
    }

#else

    UNUSED_PARAMETER( hib );

#endif /* PLATFORM_HIBERNATE_CLOCK_POWER_UP */

    return PLATFORM_SUCCESS;
}

#if PLATFORM_HIBERNATE_CLOCK_INIT

static uint32_t platform_hibernation_get_raw_status( uint32_t selector )
{
    platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_READ_SEL_REG,
                              GCI_CHIPCONTROL_HIB_READ_SEL_MASK,
                              selector);

    return ( platform_gci_chipstatus( GCI_CHIPSTATUS_HIB_READ_REG ) & GCI_CHIPSTATUS_HIB_READ_MASK ) >> GCI_CHIPSTATUS_HIB_READ_SHIFT;
}

static hib_status_t platform_hibernation_get_status( void )
{
    hib_status_t status = { .raw = platform_hibernation_get_raw_status( GCI_CHIPCONTROL_HIB_READ_SEL_STATUS ) };
    return status;
}

static platform_result_t platform_hibernation_clock_init( const platform_hibernation_t* hib, uint32_t* hib_ext_clock_freq )
{
    const wiced_bool_t      external_clock  = ( hib->clock == PLATFORM_HIBERNATION_CLOCK_EXTERNAL );
    const platform_result_t power_up_result = platform_hibernate_clock_power_up( hib );

    if ( power_up_result != PLATFORM_SUCCESS )
    {
        return power_up_result;
    }

    if ( external_clock )
    {
        while ( platform_hibernation_get_status( ).bits.pmu_ext_lpo_avail == 0 );

        platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_FORCE_EXT_LPO_REG, GCI_CHIPCONTROL_HIB_FORCE_EXT_LPO_MASK, GCI_CHIPCONTROL_HIB_FORCE_EXT_LPO_EXEC );
        platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_FORCE_INT_LPO_REG, GCI_CHIPCONTROL_HIB_FORCE_INT_LPO_MASK, 0x0 );

        platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_WAKE_CTL_REG, GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_DOWN_MASK, GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_DOWN_EXEC );
    }
    else
    {
        platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_FORCE_INT_LPO_REG, GCI_CHIPCONTROL_HIB_FORCE_INT_LPO_MASK, GCI_CHIPCONTROL_HIB_FORCE_INT_LPO_EXEC );
        platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_FORCE_EXT_LPO_REG, GCI_CHIPCONTROL_HIB_FORCE_EXT_LPO_MASK, 0x0 );

        platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_WAKE_CTL_REG, GCI_CHIPCONTROL_HIB_WAKE_CTL_XTAL_DOWN_MASK, GCI_CHIPCONTROL_HIB_WAKE_CTL_XTAL_DOWN_EXEC );
    }

    if ( hib_ext_clock_freq != NULL )
    {
        *hib_ext_clock_freq = ( hib->hib_ext_clock_freq == 0 ) ? PLATFORM_HIBERNATE_CLOCK_EXTERNAL_DEFAULT_FREQ : hib->hib_ext_clock_freq;
    }

    return PLATFORM_SUCCESS;
}

#else

static platform_result_t platform_hibernation_clock_init( const platform_hibernation_t* hib, uint32_t* hib_ext_clock_freq )
{
    platform_result_t result = platform_hibernate_clock_power_up( hib );

    UNUSED_PARAMETER( hib_ext_clock_freq );

    return result;
}

#endif /* PLATFORM_HIBERNATE_CLOCK_INIT */

#if PLATFORM_HIB_ENABLE

static uint32_t platform_hibernation_ext_clock_freq;

platform_result_t platform_hibernation_init( const platform_hibernation_t* hib )
{
    return platform_hibernation_clock_init( hib, &platform_hibernation_ext_clock_freq );
}

platform_result_t platform_hibernation_start( uint32_t ticks_to_wakeup )
{
    platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_WAKE_COUNT_REG,
                              GCI_CHIPCONTROL_HIB_WAKE_COUNT_MASK,
                              GCI_CHIPCONTROL_HIB_WAKE_COUNT_VAL( ticks_to_wakeup ) );

    platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_START_REG,
                              GCI_CHIPCONTROL_HIB_START_MASK,
                              GCI_CHIPCONTROL_HIB_START_EXEC );
    /*
     * Hibernation mode essentially power off all components, except special hibernation block.
     * This block wakes up whole system when its timer expired.
     * Returning from hibernation mode is same as powering up, and whole boot sequence is involved.
     * So if we continue here it means system not entered hibernation mode, and so return error.
     */
    return PLATFORM_ERROR;
}

wiced_bool_t platform_hibernation_is_returned_from( void )
{
    return ( platform_hibernation_get_status( ).bits.boot_from_wake != 0 );
}

uint32_t platform_hibernation_get_ticks_spent( void )
{
    return (platform_hibernation_get_raw_status( GCI_CHIPCONTROL_HIB_READ_SEL_COUNT_0_7   ) << 0  ) |
           (platform_hibernation_get_raw_status( GCI_CHIPCONTROL_HIB_READ_SEL_COUNT_15_8  ) << 8  ) |
           (platform_hibernation_get_raw_status( GCI_CHIPCONTROL_HIB_READ_SEL_COUNT_23_16 ) << 16 ) |
           (platform_hibernation_get_raw_status( GCI_CHIPCONTROL_HIB_READ_SEL_COUNT_31_24 ) << 24 );
}

uint32_t platform_hibernation_get_clock_freq( void )
{
#if PLATFORM_HIBERNATE_CLOCK_AS_EXT_LPO

    return osl_ilp_clock( );

#else

    hib_status_t status = platform_hibernation_get_status( );

    if ( status.bits.clk_sel_out_int_lpo )
    {
        uint32_t freq_code = platform_gci_chipcontrol( GCI_CHIPCONTROL_HIB_WAKE_CTL_REG, 0x0, 0x0 ) & GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_FREQ_MASK;

        switch ( freq_code )
        {
            case GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_FREQ_128KHZ:
                return PLATFORM_HIBERNATE_CLOCK_INTERNAL_FREQ_128KHZ;

            case GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_FREQ_32KHZ:
                return PLATFORM_HIBERNATE_CLOCK_INTERNAL_FREQ_32KHZ;

            case GCI_CHIPCONTROL_HIB_WAKE_CTL_LPO_FREQ_16KHZ:
                return PLATFORM_HIBERNATE_CLOCK_INTERNAL_FREQ_16KHZ;

            default:
                wiced_assert( "unknown code", 0 );
                return 0;
        }
    }
    else if ( status.bits.clk_sel_out_ext_lpo )
    {
        return platform_hibernation_ext_clock_freq;
    }
    else
    {
        wiced_assert( "unknown clock", 0 );
        return 0;
    }

#endif /* PLATFORM_HIBERNATE_CLOCK_AS_EXT_LPO */
}

uint32_t platform_hibernation_get_max_ticks( void )
{
    return ( GCI_CHIPCONTROL_HIB_WAKE_COUNT_MASK >> GCI_CHIPCONTROL_HIB_WAKE_COUNT_SHIFT );
}

#else

platform_result_t platform_hibernation_init( const platform_hibernation_t* hib )
{
    platform_result_t result = platform_hibernation_clock_init( hib, NULL );
    return ( result == PLATFORM_SUCCESS ) ? PLATFORM_FEATURE_DISABLED : result;
}

platform_result_t platform_hibernation_start( uint32_t ticks_to_wakeup )
{
    UNUSED_PARAMETER( ticks_to_wakeup );
    return PLATFORM_FEATURE_DISABLED;
}

wiced_bool_t platform_hibernation_is_returned_from( void )
{
    return WICED_FALSE;
}

uint32_t platform_hibernation_get_ticks_spent( void )
{
    return 0;
}

uint32_t platform_hibernation_get_clock_freq( void )
{
    return 0;
}

uint32_t platform_hibernation_get_max_ticks( void )
{
    return 0;
}

#endif /* PLATFORM_HIB_ENABLE */
