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
 * BCM43909 vector table
 */

#include "cr4.h"
#include "platform_isr.h"
#include "platform_isr_interface.h"
#include "platform_appscr4.h"
#include "platform_mcu_peripheral.h"
#include "wwd_rtos_isr.h"

#include "typedefs.h"
#include "sbchipc.h"
#include "aidmp.h"

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

typedef struct
{
    IRQn_Type irqn;
    void (*isr)( void );
} interrupt_vector_t;

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

interrupt_vector_t interrupt_vector_table[] =
{
    {
        .irqn  = SW0_ExtIRQn,
        .isr   = Sw0_ISR,
    },
    {
        .irqn  = SW1_ExtIRQn,
        .isr   = Sw1_ISR,
    },
    {
        .irqn  = Timer_ExtIRQn,
        .isr   = Timer_ISR,
    },
    {
        .irqn  = GMAC_ExtIRQn,
        .isr   = GMAC_ISR,
    },
    {
        .irqn  = Serror_ExtIRQn,
        .isr   = Serror_ISR,
    },
#ifndef M2M_RX_POLL_MODE
    {
        .irqn  = M2M_ExtIRQn,
        .isr   = M2M_ISR,
    },
#endif
    {
        .irqn  = I2S0_ExtIRQn,
        .isr   = I2S0_ISR,
    },
    {
        .irqn  = I2S1_ExtIRQn,
        .isr   = I2S1_ISR,
    },
    {
        .irqn  = ChipCommon_ExtIRQn,
        .isr   = ChipCommon_ISR,
    },
};

/******************************************************
 *               Function Definitions
 ******************************************************/

WWD_RTOS_DEFINE_ISR_DEMUXER( platform_irq_demuxer )
{
    uint32_t mask = PLATFORM_APPSCR4->irq_mask;
    uint32_t status = mask & PLATFORM_APPSCR4->fiqirq_status;

    if ( status )
    {
        unsigned i;

        PLATFORM_APPSCR4->fiqirq_status = status;

        for ( i = 0; status && (i < ARRAYSIZE(interrupt_vector_table)); ++i )
        {
            uint32_t irqn_mask = IRQN2MASK(interrupt_vector_table[i].irqn);

            if ( status & irqn_mask )
            {
                interrupt_vector_table[i].isr();
                status &= ~irqn_mask;
            }
        }
    }

    if ( status )
    {
        Unhandled_ISR();
    }

    cpu_data_synchronisation_barrier( );
}

WWD_RTOS_MAP_ISR_DEMUXER( platform_irq_demuxer )

void platform_irq_init( void )
{
    PLATFORM_APPSCR4->irq_mask = 0;
    PLATFORM_APPSCR4->int_mask = 0;

    platform_tick_irq_init( );
}

platform_result_t platform_irq_remap_sink( uint8_t bus_line_num, uint8_t sink_num )
{
    volatile uint32_t* oobselina = (volatile uint32_t*)( PLATFORM_APPSCR4_MASTER_WRAPPER_REGBASE(0x0) + ( ( sink_num < 4) ? AI_OOBSELINA30 : AI_OOBSELINA74 ) );
    int shift = ( sink_num % 4 ) * 8;

    if ( ( bus_line_num >= AI_OOBSEL_BUSLINE_COUNT ) || ( sink_num >= AI_OOBSEL_SINK_COUNT ) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    *oobselina = ( *oobselina & ~( (uint32_t)AI_OOBSEL_MASK << shift ) ) | ( (uint32_t)bus_line_num << shift );

    return PLATFORM_SUCCESS;
}

platform_result_t platform_irq_remap_source( uint32_t wrapper_addr, uint8_t source_num, uint8_t bus_line_num )
{
    volatile uint32_t* oobselouta = (volatile uint32_t*)( wrapper_addr + ( ( source_num < 4) ? AI_OOBSELOUTA30 : AI_OOBSELOUTA74 ) );
    int shift = ( source_num % 4 ) * 8;

    if ( ( bus_line_num >= AI_OOBSEL_BUSLINE_COUNT ) || ( source_num >= AI_OOBSEL_SOURCE_COUNT ) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    *oobselouta = ( *oobselouta & ~( (uint32_t)AI_OOBSEL_MASK << shift ) ) | ( (uint32_t)bus_line_num << shift );

    return PLATFORM_SUCCESS;
}
