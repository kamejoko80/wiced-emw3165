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
#include "cr4.h"
#include "platform_isr.h"
#include "platform_assert.h"
#include "platform_peripheral.h"

#define CR4_FAULT_STATUS_MASK  (0x140f)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    CR4_FAULT_STATUS_ALIGNMENT                             = 0x001,   /* Highest Priority */
    CR4_FAULT_STATUS_MPU_BACKGROUND                        = 0x000,
    CR4_FAULT_STATUS_MPU_PERMISSION                        = 0x00D,
    CR4_FAULT_STATUS_SYNC_EXTERNAL_ABORT_AXI_DECODE_ERROR  = 0x008,
    CR4_FAULT_STATUS_ASYNC_EXTERNAL_ABORT_AXI_DECODE_ERROR = 0x406,
    CR4_FAULT_STATUS_SYNC_EXTERNAL_ABORT_AXI_SLAVE_ERROR   = 0x1008,
    CR4_FAULT_STATUS_ASYNC_EXTERNAL_ABORT_AXI_SLAVE_ERROR  = 0x1406,
    CR4_FAULT_STATUS_SYNC_PARITY_ECC                       = 0x409,
    CR4_FAULT_STATUS_ASYNC_PARITY_ECC                      = 0x408,
    CR4_FAULT_STATUS_DEBUG_EVENT                           = 0x002,   /* Lowest Priority */
} cr4_fault_status_t;



#ifndef DEBUG

static void UnhandledException( void );

PLATFORM_SET_DEFAULT_ISR( irq_vector_undefined_instruction, UnhandledException )
PLATFORM_SET_DEFAULT_ISR( irq_vector_prefetch_abort,        UnhandledException )
PLATFORM_SET_DEFAULT_ISR( irq_vector_data_abort,            UnhandledException )
PLATFORM_SET_DEFAULT_ISR( irq_vector_fast_interrupt,        UnhandledException )
PLATFORM_SET_DEFAULT_ISR( irq_vector_software_interrupt,    UnhandledException )
PLATFORM_SET_DEFAULT_ISR( irq_vector_reserved,              UnhandledException )

static void UnhandledException( void )
{
    platform_mcu_reset( );
}


#else /* DEBUG */


static void default_fast_interrupt_handler( void );
static void default_software_interrupt_handler( void );
static void default_reserved_interrupt_handler( void );
static void prefetch_abort_handler( void );
static void data_abort_handler( void );
static void undefined_instruction_handler( void );

PLATFORM_SET_DEFAULT_ISR( irq_vector_undefined_instruction, undefined_instruction_handler )
PLATFORM_SET_DEFAULT_ISR( irq_vector_prefetch_abort,        prefetch_abort_handler )
PLATFORM_SET_DEFAULT_ISR( irq_vector_data_abort,            data_abort_handler )
PLATFORM_SET_DEFAULT_ISR( irq_vector_fast_interrupt,        default_fast_interrupt_handler )
PLATFORM_SET_DEFAULT_ISR( irq_vector_software_interrupt,    default_software_interrupt_handler )
PLATFORM_SET_DEFAULT_ISR( irq_vector_reserved,              default_reserved_interrupt_handler )


static void default_fast_interrupt_handler( void )
{
    /* Fast interrupt triggered without any configured handler */
    WICED_TRIGGER_BREAKPOINT();
    while (1)
    {
        /* Loop forever */
    }
}

static void default_software_interrupt_handler( void )
{
    /* Software interrupt triggered without any configured handler */
    WICED_TRIGGER_BREAKPOINT();
    while (1)
    {
        /* Loop forever */
    }
}

static void default_reserved_interrupt_handler( void )
{
    /* Reserved interrupt triggered - This should never happen! */
    WICED_TRIGGER_BREAKPOINT();
    while (1)
    {
        /* Loop forever */
    }
}

static void prefetch_abort_handler( void )
{
    uint32_t ifar = get_IFAR(); /* <----- CHECK THIS FOR ADDRESS OF INSTRUCTION */

    cr4_fault_status_t status = (cr4_fault_status_t) (get_IFSR( ) & CR4_FAULT_STATUS_MASK);  /* <----- CHECK THIS FOR STATUS */

    /* Prefetch abort occurred */

    /* This means that the processor attempted to execute an instruction
     * which was marked invalid due to a memory access failure (i.e. an abort)
     *
     *  - permission fault indicated by the MPU
     *  - an error response to a transaction on the AXI memory bus
     *  - an error detected in the data by the ECC checking logic.
     *
     */
    WICED_TRIGGER_BREAKPOINT();

    platform_backplane_debug(); /* Step here to check backplane wrappers if timed out because of bus hang */

    __asm__( "SUBS PC, LR,#4 " );  /* Step here to return to the instruction which caused the prefetch abort */

    (void) ifar;   /* These are unreferenced and exist only for debugging */
    (void) status;
}

static void data_abort_handler( void )
{
    uint32_t dfar = get_DFAR(); /* <----- CHECK THIS FOR ADDRESS OF DATA ACCESS */

    cr4_fault_status_t status = (cr4_fault_status_t) (get_DFSR( ) & CR4_FAULT_STATUS_MASK);  /* <----- CHECK THIS FOR STATUS */

    /* Data abort occurred */

    /* This means that the processor attempted a data access which
     * caused a memory access failure (i.e. an abort)
     *
     *  - permission fault indicated by the MPU
     *  - an error response to a transaction on the AXI memory bus
     *  - an error detected in the data by the ECC checking logic.
     *
     */
    WICED_TRIGGER_BREAKPOINT();

    platform_backplane_debug(); /* Step here to check backplane wrappers if timed out because of bus hang */

    __asm( "SUBS PC, LR, #8" );  /* If synchronous abort, Step here to return to the instruction which caused the data abort */

    (void) dfar;   /* These are unreferenced and exist only for debugging */
    (void) status;
}


static void undefined_instruction_handler( void )
{
    /* Undefined instruction exception occurred */

    WICED_TRIGGER_BREAKPOINT();

    __asm( "MOVS PC, LR" );  /* Step here to return to the instruction which caused the undefined instruction exception */
}

#endif /* DEBUG */
