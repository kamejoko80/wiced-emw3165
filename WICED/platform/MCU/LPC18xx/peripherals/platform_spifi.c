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
 *This file implements the quad SPI flash interface SPIFI.
 */

#include "platform_peripheral.h"
#include "chip.h"
#include "spifilib_api.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wwd_assert.h"

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
#define SPIFLASH_BASE_ADDRESS (0x14000000)

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *                   Variables
 ******************************************************/
static uint32_t lmem[21];
static SPIFI_HANDLE_T *spifi_handle;

/******************************************************
 *               Function Definitions
 ******************************************************/

platform_result_t platform_spifi_init( const platform_spifi_t* spifi )
{
    uint32_t spifiBaseClockRate;
    uint32_t maxSpifiClock;

    /*
    if( spi_ptr->semaphore_is_inited == WICED_FALSE )
    {
        host_rtos_init_semaphore( &spi_ptr->in_use_semaphore );
        spi_ptr->semaphore_is_inited = WICED_TRUE;
    }
    else
    {
        host_rtos_get_semaphore( &spi_ptr->in_use_semaphore, NEVER_TIMEOUT, WICED_FALSE );
    }
    */

    /* Mux the port and pin to direct it to SPIFI */
    platform_pin_set_alternate_function( &spifi->clock );
    platform_pin_set_alternate_function( &spifi->miso  );
    platform_pin_set_alternate_function( &spifi->mosi  );
    platform_pin_set_alternate_function( &spifi->sio2 );
    platform_pin_set_alternate_function( &spifi->sio3  );
    platform_pin_set_alternate_function( &spifi->cs  );

    /* SPIFI base clock will be based on the main PLL rate and a divider */
    spifiBaseClockRate = Chip_Clock_GetClockInputHz(CLKIN_MAINPLL);

    /* Setup SPIFI clock to run around 1Mhz. Use divider E for this, as it allows
     * higher divider values up to 256 maximum)
     */
    Chip_Clock_SetDivider(CLK_IDIV_E, CLKIN_MAINPLL, ((spifiBaseClockRate / 1000000) + 1));
    Chip_Clock_SetBaseClock(CLK_BASE_SPIFI, CLKIN_IDIVE, true, false);

    /* Initialize LPCSPIFILIB library, reset the interface */
    spifiInit( ( uint32_t )spifi->spifi_base, true);

    /* register support for the family(s) we may want to work with */
    spifiRegisterFamily( SPIFI_REG_FAMILY_MacronixMX25L );

    /* Initialize and detect a device and get device context */
    spifi_handle = spifiInitDevice( &lmem, sizeof(lmem), ( uint32_t ) spifi->spifi_base, SPIFLASH_BASE_ADDRESS );

    /* Get some info needed for the application */
    maxSpifiClock = spifiDevGetInfo(spifi_handle, SPIFI_INFO_MAXCLOCK);

    /* Setup SPIFI clock to at the maximum interface rate the detected device
       can use. This should be done after device init. */
    Chip_Clock_SetDivider(CLK_IDIV_E, CLKIN_MAINPLL, ((spifiBaseClockRate / maxSpifiClock) + 1));

    /* start by unlocking the device */
    if (spifiDevUnlockDevice(spifi_handle) != SPIFI_ERR_NONE) {
        return WICED_ERROR;
    }

    /* Enable quad.  If not supported it will be ignored */
    spifiDevSetOpts(spifi_handle, SPIFI_OPT_USE_QUAD, true);

    /* Enter memMode */
    spifiDevSetMemMode(spifi_handle, true);

    /* Just a test */
    maxSpifiClock = *( (uint32_t *)SPIFLASH_BASE_ADDRESS );

    return spifi_handle == NULL ? WICED_ERROR : WICED_SUCCESS;
}

platform_result_t platform_spifi_deinit( const platform_spifi_t* spifi )
{
    return PLATFORM_UNSUPPORTED;
}
