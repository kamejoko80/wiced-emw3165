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
 * Defines macros for defining and mapping interrupt handlers to ARM-Cortex-R4 CPU interrupts
 */
#pragma once
#include "platform_constants.h"
#include "wwd_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/* Section where IRQ handlers are placed */
#define IRQ_SECTION ".text.irq"

/* Macro for defining an interrupt handler (non-RTOS-aware)
 *
 * @warning:
 * Do NOT call any RTOS primitive functions from here. If you need to call them,
 * define your interrupt handler using WWD_RTOS_DEFINE_ISR()
 *
 * @usage:
 * PLATFORM_DEFINE_ISR( my_irq )
 * {
 *     // Do something here
 * }
 *
 */
#if defined ( __GNUC__ )
/* GCC */
#define PLATFORM_DEFINE_ISR( name ) \
        void name( void ); \
        __attribute__(( section( IRQ_SECTION ) )) void name( void )

#elif defined ( __IAR_SYSTEMS_ICC__ )
/* IAR Systems */
#define PLATFORM_DEFINE_ISR( name ) \
        void name( void ); \
        void name( void )

#else

#define PLATFORM_DEFINE_ISR( name )

#endif


/* Macro for mapping a defined function to an interrupt handler declared in
 * <Wiced-SDK>/WICED/platform/MCU/<Family>/platform_isr_interface.h
 *
 * @usage:
 * PLATFORM_MAP_ISR( my_irq, USART1_irq )
 */
#if defined( __GNUC__ )

#define PLATFORM_MAP_ISR( function, irq_handler ) \
        extern void irq_handler( void ); \
        __attribute__(( alias( #function ))) void irq_handler ( void );

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define PLATFORM_MAP_ISR( function, irq_handler ) \
        extern void irq_handler( void ); \
        _Pragma( TO_STRING( weak irq_handler=function ) )

#else

#define PLATFORM_MAP_ISR( function, irq_handler )

#endif


/* Macro for declaring a default handler for an unhandled interrupt
 *
 * @usage:
 * PLATFORM_SET_DEFAULT_ISR( USART1_irq, default_handler )
 */
#if defined( __GNUC__ )

#define PLATFORM_SET_DEFAULT_ISR( irq_handler, default_handler ) \
        __attribute__(( weak, alias( #default_handler ))) void irq_handler ( void );

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define PLATFORM_SET_DEFAULT_ISR( irq_handler, default_handler ) \
        _Pragma( TO_STRING( weak irq_handler=default_handler ) )

#else

#define PLATFORM_SET_DEFAULT_ISR( irq_handler, default_handler )

#endif

#if defined( __GNUC__ )

#define WICED_SAVE_INTERRUPTS(flags) \
    __asm__ volatile("mrs %0, cpsr\ncpsid i" : "=r" (flags) : : "memory", "cc");

#define WICED_RESTORE_INTERRUPTS(flags) \
    __asm__ volatile ("msr cpsr_c, %0" : : "r" (flags) : "memory", "cc")

#endif

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    PLATFORM_CPU_CLOCK_FREQUENCY_160_MHZ,
    PLATFORM_CPU_CLOCK_FREQUENCY_320_MHZ
} platform_cpu_clock_frequency_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

extern uint32_t platform_cpu_clock_hz;
#define CPU_CLOCK_HZ platform_cpu_clock_hz

/******************************************************
 *               Function Declarations
 ******************************************************/

extern void         platform_tick_start                  ( void );
extern void         platform_tick_stop                   ( void );
extern void         platform_tick_irq_init               ( void );
extern wiced_bool_t platform_tick_irq_handler            ( void );
extern void         platform_tick_number_since_last_sleep( uint32_t* since_last_sleep, uint32_t* since_last_deep_sleep );

extern void platform_cpu_clock_init( platform_cpu_clock_frequency_t freq );

#ifdef __cplusplus
} /*extern "C" */
#endif
