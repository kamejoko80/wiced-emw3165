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
 *  Platform ChipCommon Functions
 */

#include <stdint.h>
#include "platform_appscr4.h"
#include "wiced_platform.h"
#include "wwd_assert.h"
#include "wwd_rtos_isr.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* ChipCommon IntStatus and IntMask register bits */
#define CC_INT_STATUS_MASK_GPIOINT             (1 << 0)
#define CC_INT_STATUS_MASK_EXTINT              (1 << 1)
#define CC_INT_STATUS_MASK_ECIGCIINT           (1 << 4)
#define CC_INT_STATUS_MASK_PMUINT              (1 << 5)
#define CC_INT_STATUS_MASK_UARTINT             (1 << 6)
#define CC_INT_STATUS_MASK_SECIGCIWAKEUPINT    (1 << 7)
#define CC_INT_STATUS_MASK_SPMINT              (1 << 8)
#define CC_INT_STATUS_MASK_ASCURXINT           (1 << 9)
#define CC_INT_STATUS_MASK_ASCUTXINT           (1 << 10)
#define CC_INT_STATUS_MASK_ASCUASTPINT         (1 << 11)

/* ChipCommon IntStatus register bit only */
#define CC_INT_STATUS_WDRESET                  (1 << 31)

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

/* Extern'ed from platforms/<43909_Platform>/platform.c */
extern platform_uart_driver_t platform_uart_drivers[];

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Disable external interrupts from ChipCommon to APPS Core */
void platform_chipcommon_disable_irq( void )
{
    platform_irq_disable_irq( ChipCommon_ExtIRQn );
}

/* Enable external interrupts from ChipCommon to APPS Core */
void platform_chipcommon_enable_irq( void )
{
    platform_irq_enable_irq(ChipCommon_ExtIRQn);
}

WWD_RTOS_DEFINE_ISR( platform_chipcommon_isr )
{
    uint32_t int_mask;
    uint32_t int_status;

    int_mask   = PLATFORM_CHIPCOMMON->interrupt.mask.raw;
    int_status = PLATFORM_CHIPCOMMON->interrupt.status.raw;

    /* DeMultiplex the ChipCommon interrupt */
    if ( ( ( int_status & CC_INT_STATUS_MASK_GPIOINT ) != 0 ) && ( ( int_mask & CC_INT_STATUS_MASK_GPIOINT ) != 0 ) )
    {
        /* GPIO interrupt */
        platform_gpio_irq();
    }

    if ( ( ( int_status & CC_INT_STATUS_MASK_UARTINT ) != 0 ) && ( ( int_mask & CC_INT_STATUS_MASK_UARTINT ) != 0 ) )
    {
#ifndef BCM4390X_UART_SLOW_POLL_MODE
        /* Slow UART interrupt */
        platform_uart_irq(&platform_uart_drivers[WICED_UART_1]);
#else
        /* Unsolicited slow UART interrupt */
        wiced_assert("Invalid ChipCommon interrupt source: Slow UART", 0);
#endif /* !BCM4390X_UART_SLOW_POLL_MODE */
    }

    if ( ( ( int_status & CC_INT_STATUS_MASK_ECIGCIINT ) != 0 ) && ( ( int_mask & CC_INT_STATUS_MASK_ECIGCIINT ) != 0 ) )
    {
#ifndef BCM4390X_UART_FAST_POLL_MODE
        /* Fast UART interrupt */
        platform_uart_irq(&platform_uart_drivers[WICED_UART_2]);
#else
        /* Unsolicited fast UART interrupt */
        wiced_assert("Invalid ChipCommon interrupt source: Fast UART", 0);
#endif /* !BCM4390X_UART_FAST_POLL_MODE */
    }

    if ( ( ( int_status & CC_INT_STATUS_MASK_ASCURXINT ) != 0 ) && ( ( int_mask & CC_INT_STATUS_MASK_ASCURXINT ) != 0 ) )
    {
        platform_ascu_irq(int_status);
    }

    if ( ( ( int_status & CC_INT_STATUS_MASK_ASCUTXINT ) != 0 ) && ( ( int_mask & CC_INT_STATUS_MASK_ASCUTXINT ) != 0 ) )
    {
        platform_ascu_irq(int_status);
    }

    if ( ( ( int_status & CC_INT_STATUS_MASK_ASCUASTPINT ) != 0 ) && ( ( int_mask & CC_INT_STATUS_MASK_ASCUASTPINT ) != 0 ) )
    {
        platform_ascu_irq(int_status);
    }
}

WWD_RTOS_MAP_ISR( platform_chipcommon_isr, ChipCommon_ISR )
