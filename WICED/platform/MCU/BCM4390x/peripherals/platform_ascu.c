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

#include "platform_ascu.h"

#include "internal/wwd_sdpcm.h"

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

static wlc_avb_timestamp_t* g_avb_timestamp;

/* ASCU resides in ChipCommon core and accessed from Apps core via AXI backplane */
static ascu_register_t* const ascu_base = ( ascu_register_t* )( ASCU_REGBASE );

/******************************************************
 *               Function Definitions
 ******************************************************/

static wlc_avb_timestamp_t* get_avb_timestamp_addr(void)
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    uint32_t*      data;

    data = (uint32_t*)wwd_sdpcm_get_iovar_buffer(&buffer, (uint16_t)4, "avb_timestamp_addr");

    if (data == NULL)
    {
        return NULL;
    }

    if (wwd_sdpcm_send_iovar(SDPCM_GET, buffer, &response, WWD_STA_INTERFACE) != WWD_SUCCESS)
    {
        return NULL;
    }

    data = (uint32_t*)host_buffer_get_current_piece_data_pointer(response);

    host_buffer_release(response, WWD_NETWORK_RX);

    return (wlc_avb_timestamp_t *)*data;
}


static inline int platform_ascu_convert_ntimer(uint32_t ntimer_hi, uint32_t ntimer_lo, uint32_t *secs, uint32_t *nanosecs)
{
    uint32_t remainder;
    uint64_t rx_hi  = ntimer_hi;
    uint64_t ntimer = (rx_hi << 32) | ntimer_lo;

    if (ntimer != 0)
    {
        *secs     = ntimer / NET_TIMER_TICKS_PER_SEC;
        remainder = ntimer - (*secs * NET_TIMER_TICKS_PER_SEC);
        *nanosecs = remainder * NET_TIMER_NANOSECS_PER_TICK;
    }
    else
    {
        *secs = 0;
        *nanosecs = 0;
    }

    return PLATFORM_SUCCESS;
}

static inline void platform_ascu_force_ntimer(void)
{
     ascu_base->ascu_control |= 0x80;
}

static void ascu_chipcommon_interrupt_mask(uint32_t clear_mask, uint32_t set_mask)
{
    uint32_t cc_int_mask;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS(flags);
    cc_int_mask = PLATFORM_CHIPCOMMON->interrupt.mask.raw;

    cc_int_mask = (cc_int_mask & ~clear_mask) | set_mask;

    PLATFORM_CHIPCOMMON->interrupt.mask.raw = cc_int_mask;
    WICED_RESTORE_INTERRUPTS(flags);
}


void platform_ascu_disable_interrupts(uint32_t int_mask)
{
    /* Disable ASCU interrupt in ChipCommon interrupt mask */
    ascu_chipcommon_interrupt_mask((ASCU_RX_CC_INT_STATUS_MASK | ASCU_RX_CC_INT_STATUS_MASK), 0);

    ascu_base->interrupt_mask &= ~int_mask;
}


void platform_ascu_enable_interrupts()
{
    /*
     * If we haven't gotten the address of the shared AVB timestamp
     * structure from the driver, do it now.
     */

    if (g_avb_timestamp == NULL)
    {
        g_avb_timestamp = get_avb_timestamp_addr();
    }

    /*
     * Enable those interrupts.
     */

    ascu_base->interrupt_mask |= (ASCU_TX_START_AVB_INT_MASK | ASCU_RX_START_AVB_INT_MASK);

    /* Enable ASCU interrupt in ChipCommon interrupt mask */
    ascu_chipcommon_interrupt_mask(0x0, (ASCU_RX_CC_INT_STATUS_MASK | ASCU_RX_CC_INT_STATUS_MASK));
}


int platform_ascu_read_ntimer(uint32_t *secs, uint32_t *nanosecs)
{

    return platform_ascu_convert_ntimer(ascu_base->network_timer_rx_hi,
                                        ascu_base->network_timer_rx_lo,
                                        secs, nanosecs);
}

int platform_ascu_read_fw_ntimer(uint32_t *secs, uint32_t *nanosecs)
{

    platform_ascu_force_ntimer();
    return platform_ascu_convert_ntimer(ascu_base->network_timer_fw_hi,
                                        ascu_base->network_timer_fw_lo,
                                        secs, nanosecs);
}

wlc_avb_timestamp_t* platform_ascu_get_avb_ts(void)
{
    return g_avb_timestamp;
}

platform_result_t platform_ascu_init(void)
{
    osl_core_enable( CC_CORE_ID ); /* ASCU is in chipcommon. Enable core before trying to access. */

    /*
     * TODO: any one time initialization. Right now there's nothing to do.
     */

    return PLATFORM_SUCCESS;
}

/******************************************************
 *            IRQ Handlers Definition
 ******************************************************/

void platform_ascu_irq(uint32_t intr_status)
{
    uint16_t int_mask   = ascu_base->interrupt_mask;;
    uint16_t int_status = (uint16_t)(intr_status >> ASCU_INTR_BIT_SHIFT_OFFSET);

    /* Clear the interrupt(s) */
    ascu_base->interrupt_status = ASCU_ALL_INTS;
    ascu_base->interrupt_status = 0;

    /* Turn off ASCU interrupts */
    platform_ascu_disable_interrupts(int_mask);

    if ( (int_status & ASCU_RX_START_AVB_INT_MASK) && (int_mask & ASCU_RX_START_AVB_INT_MASK) )
    {
        /* Rx Start AVB interrupt fired. */

        if (g_avb_timestamp)
        {
            g_avb_timestamp->net_timer_rxlo     = ascu_base->network_timer_rx_lo;
            g_avb_timestamp->net_timer_rxhi     = ascu_base->network_timer_rx_hi;
            g_avb_timestamp->as_net_timer_rx_lo = ascu_base->network_timer_rx_lo;
            g_avb_timestamp->as_net_timer_rx_hi = ascu_base->network_timer_rx_hi;
        }
    }

    if ( (int_status & ASCU_TX_START_AVB_INT_MASK) && (int_mask & ASCU_TX_START_AVB_INT_MASK) )
    {
        /* Tx Start AVB interrupt fired. */
    }

    /* Turn ASCU interrupts back on */
    platform_ascu_enable_interrupts();
}
