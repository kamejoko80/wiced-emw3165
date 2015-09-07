/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <stdint.h>
#include "platform_cmsis.h"
#include "platform_assert.h"
#include "platform_constants.h"
#include "platform_isr.h"
#include "platform_isr_interface.h"
#include "wwd_rtos.h"
#include "platform_peripheral.h"

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

extern void UnhandledInterrupt( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/
/******************************************************
 *               Function Definitions
 ******************************************************/

PLATFORM_DEFINE_ISR( UnhandledInterrupt )
{
    uint32_t active_interrupt_vector = (uint32_t) ( SCB->ICSR & 0x3fU );

    /* This variable tells you which interrupt vector is currently active */
    (void)active_interrupt_vector;
    WICED_TRIGGER_BREAKPOINT( );

    /* reset the processor immeditly if not debug */
    platform_mcu_reset( );

    while( 1 )
    {
    }
}

/******************************************************
 *          Default IRQ Handler Declarations
 ******************************************************/
PLATFORM_SET_DEFAULT_ISR(NMIException        ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(HardFaultException  ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(MemManageException  ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(BusFaultException   ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(UsageFaultException ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(DebugMonitor        ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(DAC_irq             ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(MAPP_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(DMA_irq             ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(FLASHEEPROM_irq     ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(ETH_irq             ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(SDIO_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(LCD_irq             ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(USB0_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(USB1_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(SCT_irq             ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(RIT_irq             ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(TIMER0_irq          ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(TIMER1_irq          ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(TIMER2_irq          ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(TIMER3_irq          ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(MCPWM_irq           ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(ADC0_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(SPI_irq             ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(I2C0_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(I2C1_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(ADC1_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(SSP0_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(SSP1_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(USART1_irq          ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(USART2_irq          ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(I2S0_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(I2S1_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(GINT0_irq           ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(GINT1_irq           ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(EVRT_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(CAN1_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(ATIMER_irq          ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(RTC_irq             ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(WDT_irq             ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(CAN0_irq            ,  UnhandledInterrupt )
PLATFORM_SET_DEFAULT_ISR(QEI_irq             ,  UnhandledInterrupt )

