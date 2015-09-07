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
 * LPC43xx vector table
 */
#include <stdint.h>
#include "platform_cmsis.h"
#include "platform_assert.h"
#include "platform_constants.h"
#include "platform_isr.h"
#include "platform_isr_interface.h"
#include "wwd_rtos_isr.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#ifndef SVC_irq
#define SVC_irq UnhandledInterrupt
#endif

#ifndef PENDSV_irq
#define PENDSV_irq UnhandledInterrupt
#endif

#ifndef SYSTICK_irq
#define SYSTICK_irq UnhandledInterrupt
#endif


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
extern void reset_handler     ( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* Pointer to stack location */
extern void* link_stack_end;

uint32_t  crp_bank = 0xFFFFFFFF;


uint32_t interrupt_vector_table[] =
{
    (uint32_t)&link_stack_end       , /* 0 Initial stack location                                             */
    (uint32_t)reset_handler         , /* 1 Reset vector                                                       */
    (uint32_t)NMIException          , /* 2 Non Maskable Interrupt                                             */
    (uint32_t)HardFaultException    , /* 3 Hard Fault interrupt                                               */
    (uint32_t)MemManageException    , /* 4 Memory Management Fault interrupt                                  */
    (uint32_t)BusFaultException     , /* 5 Bus Fault interrupt                                                */
    (uint32_t)UsageFaultException   , /* 6 Usage Fault interrupt                                              */
    (uint32_t)0                     , /* 7 Contains 2's compliment checksum of vector entries 0-6 mc_hijack   */
    (uint32_t)0                     , /* 8 Reserved                                                           */
    (uint32_t)0                     , /* 9 Reserved                                                           */
    (uint32_t)0                     , /* 10 Reserved                                                          */
    (uint32_t)SVC_irq               , /* 11 SVC interrupt                                                     */
    (uint32_t)DebugMonitor          , /* 12 Debug Monitor interrupt                                           */
    (uint32_t)0                     , /* 13 Reserved                                                          */
    (uint32_t)PENDSV_irq            , /* 14 PendSV interrupt                                                  */
    (uint32_t)SYSTICK_irq           , /* 15 Sys Tick Interrupt                                                */
    (uint32_t)DAC_irq               , /* 16 DAC                                                               */
    (uint32_t)MAPP_irq              , /* 17 Cortex M0 APP                                                     */
    (uint32_t)DMA_irq               , /* 18 DMA                                                               */
    (uint32_t)0                     , /* 19 Reserved                                                          */
    (uint32_t)FLASHEEPROM_irq       , /* 20 ORed flash bank A, flash bank B,EEPROM interrupts                 */
    (uint32_t)ETH_irq               , /* 21 Ethernet                                                          */
    (uint32_t)SDIO_irq              , /* 22 SD/MMC                                                            */
    (uint32_t)LCD_irq               , /* 23 LCD                                                               */
    (uint32_t)USB0_irq              , /* 24 USB0                                                              */
    (uint32_t)USB1_irq              , /* 25 USB1                                                              */
    (uint32_t)SCT_irq               , /* 26 State Configurable Timer                                          */
    (uint32_t)RIT_irq               , /* 27 Repetitive Interrupt Timer                                        */
    (uint32_t)TIMER0_irq            , /* 28 Timer0                                                            */
    (uint32_t)TIMER1_irq            , /* 29 Timer1                                                            */
    (uint32_t)TIMER2_irq            , /* 30 Timer2                                                            */
    (uint32_t)TIMER3_irq            , /* 31 Timer3                                                            */
    (uint32_t)MCPWM_irq             , /* 32 Motor Control PWM                                                 */
    (uint32_t)ADC0_irq              , /* 33 A/D Converter 0                                                   */
    (uint32_t)I2C0_irq              , /* 34 I2C0                                                              */
    (uint32_t)I2C1_irq              , /* 35 I2C1                                                              */
    (uint32_t)SPI_irq               , /* 36 SPI_irq                                                           */
    (uint32_t)ADC1_irq              , /* 37 A/D Converter 1                                                   */
    (uint32_t)SSP0_irq              , /* 38 SSP0                                                              */
    (uint32_t)SSP1_irq              , /* 39 SSP1                                                              */
    (uint32_t)USART0_irq            , /* 40 UART0                                                             */
    (uint32_t)USART1_irq            , /* 41 UART1                                                             */
    (uint32_t)USART2_irq            , /* 42 UART2                                                             */
    (uint32_t)USART3_irq            , /* 43 UART3                                                             */
    (uint32_t)I2S0_irq              , /* 44 I2S0                                                              */
    (uint32_t)I2S1_irq              , /* 45 I2S1                                                              */
    (uint32_t)0                     , /* 46 Reserved                                                          */
    (uint32_t)0                     , /* 47 Reserved                                                          */
    (uint32_t)GPIO0_irq             , /* 48 GPIO0                                                             */
    (uint32_t)GPIO1_irq             , /* 49 GPIO1                                                             */
    (uint32_t)GPIO2_irq             , /* 50 GPIO2                                                             */
    (uint32_t)GPIO3_irq             , /* 51 GPIO3                                                             */
    (uint32_t)GPIO4_irq             , /* 52 GPIO4                                                             */
    (uint32_t)GPIO5_irq             , /* 53 GPIO5                                                             */
    (uint32_t)GPIO6_irq             , /* 54 GPIO6                                                             */
    (uint32_t)GPIO7_irq             , /* 55 GPIO7                                                             */
    (uint32_t)GINT0_irq             , /* 56 GINT0                                                             */
    (uint32_t)GINT1_irq             , /* 57 GINT1                                                             */
    (uint32_t)EVRT_irq              , /* 58 Event Router                                                      */
    (uint32_t)CAN1_irq              , /* 59 C_CAN1                                                            */
    (uint32_t)0                     , /* 60 Reserved                                                          */
    (uint32_t)0                     , /* 61 Reserved                                                          */
    (uint32_t)ATIMER_irq            , /* 62 ATIMER                                                            */
    (uint32_t)RTC_irq               , /* 63 RTC                                                               */
    (uint32_t)0                     , /* 64 Reserved                                                          */
    (uint32_t)WDT_irq               , /* 65 WDT                                                               */
    (uint32_t)0                     , /* 66 Reserved                                                          */
    (uint32_t)CAN0_irq              , /* 67 C_CAN0                                                            */
    (uint32_t)QEI_irq
};
