/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/* Mapping of platform interrupts over the top of generic versions */

reset_handler = _low_start;

/* Force system to reset via ROM bootloader & serial flash.  This works around A0 issue that FLOPS is not reset by SRSTn */
irq_instruction_vector_low_reset = 0xB03EF000;

ENTRY( _low_start );

MEMORY
{
    ROM              (rx)   : ORIGIN = 0x00400000, LENGTH = 640K
    FLOPS            (rwx)  : ORIGIN = 0x00000000, LENGTH = 32
    SRAM             (rwx)  : ORIGIN = 0x004A0000, LENGTH = 2048K
    AONRAM           (rwx)  : ORIGIN = 0x006A0000, LENGTH = 8K
}

SECTIONS
{
    /* Data transfer area for serial flash writing app  - needs to be at start of memory */
    /* Only exists if building serial flash writer app */
    .sflash_trnsf :
    {
        KEEP(*(*.data_config))
        KEEP(*(*.data_transfer))
    }>SRAM AT> SRAM

    .always_on_ram :
    {
        . = ALIGN(4);

        KEEP(*(.rodata.tinybl*))
        *(.deep_sleep_saved_vars.*)
    } > AONRAM

    .text :
    {
        . = ALIGN(32);

        link_code_location = .;
        KEEP(*(.text.startup)) /* has to be first */
        *(.text .text.* .gnu.linkonce.t.*)
        *(.deep_sleep_event_handlers.*)
        KEEP(*(.text.vPortSVCHandler    .text.xPortPendSVHandler .text.xPortSysTickHandler ))
        KEEP(*(.text.__tx_SVCallHandler .text.__tx_PendSVHandler .text.__tx_SysTickHandler ))
        KEEP(*(.text.platform_tick_isr ))
        KEEP(*(.text.irq ))
        KEEP(*(.text.sdio_rtos_irq .text.sdio_irq ))
        KEEP(*(.text.uart_rtos_irq .text.uart_irq ))
        KEEP(*(.text.dma_rtos_irq  .text.dma_irq  ))
        KEEP(*(.text.gpio_rtos_irq .text.gpio_irq ))
        KEEP(*(.text.dbg_watchdog_rtos_irq  .text.dbg_watchdog_irq  ))
        KEEP(*(.text.usart1_rtos_irq .text.usart1_irq ))
        KEEP(*(.text.usart2_rtos_irq .text.usart2_irq ))
        KEEP(*(.text.usart1_tx_dma_rtos_irq .text.usart1_tx_dma_irq ))
        KEEP(*(.text.usart2_tx_dma_rtos_irq .text.usart2_tx_dma_irq ))
        KEEP(*(.text.usart1_rx_dma_rtos_irq .text.usart1_rx_dma_irq ))
        KEEP(*(.text.usart2_rx_dma_rtos_irq .text.usart2_rx_dma_irq ))
        link_code_end = .;
    } > SRAM

    .rodata :
    {
        . = ALIGN(32);

        link_const_variable_data_location = .;
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        link_const_variable_data_end = .;

        . = ALIGN(4);

        link_deep_sleep_event_registrations_location = .;
        KEEP(*(.deep_sleep_event_registrations.*))
        link_deep_sleep_event_registrations_end = .;

        . = ALIGN(4);

        link_constructors_location = .;
        KEEP(*(.preinit_array))
        KEEP(*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        link_constructors_end = .;

        . = ALIGN(4);

        link_destructors_location = .;
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        link_destructors_end = .;
    } > SRAM

    .vectors_copyfrom :
    {
        link_interrupt_vectors_copyfrom = LOADADDR(.vectors_copyfrom);
        KEEP(*(.vectors_copyfrom))
    } > FLOPS AT > SRAM

    /*
     * The .ARM.exidx and .ARM.extab sections are used for C++ exception handling.
     * It is located here for completeness. Bare-metal ARM projects
     * typically cannot afford the overhead associated with C++
     * exceptions handling.
     */
    .ARM.exidx :
    {
       . = ALIGN(4);

        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > SRAM

    .ARM.extab :
    {
       . = ALIGN(4);

        __extab_start = .;
        *(.ARM.extab*)
        __extab_end = .;
    } > SRAM

    .data : /* Contains the non-zero initialised global variables */
    {
        . = ALIGN(32);

        *(.data*)
    } > SRAM

    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        . = ALIGN(4);

        link_bss_location = .;
        *(.bss*)
        *(COMMON)
        link_bss_end = ALIGN(4);
    } > SRAM

    .stack_start :
    {
        . = ALIGN(8);

        link_stack_location = .;
        *(.stack)
        . = MAX(link_stack_location + START_STACK_SIZE , .);
        link_stack_end = ALIGN(8);
    } > SRAM

    .stack_isr :
    {
        . = ALIGN(8);

        link_stack_isr_location = .;
        . = . + link_stack_isr_location + SYS_STACK_SIZE + IRQ_STACK_SIZE + FIQ_STACK_SIZE + 3 * 8;
        link_stack_isr_end = ALIGN(8);
    } > SRAM

    /DISCARD/ :
    {
        *(.ARM.attributes*)
        *(.comment)
        *(.init)
        *(.preinit)
        *(.fini)
        *(.fini_array)
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
        *(.eh_frame_hdr)
        *(.eh_frame)
        *(.gnu.linkonce.armextab.*)
        *(.v4_bx)
        *(.vfp11_veneer)
        *(.gcc_except_table)
        *(.eh_frame_hdr)
        *(.eh_frame)
        *(.glue*)
    }
}

/* Declare libc Heap to start at end of allocated RAM and end at top of RAM, aligned 8 byte */
PROVIDE( _heap = link_stack_isr_end );
PROVIDE( _eheap = ALIGN( ORIGIN( SRAM ) + LENGTH( SRAM ) - 8, 8 ) );

/* Declare ThreadX free memory (passed to app as tx_application_define() func's argument) */
PROVIDE( __RAM_segment_used_end__ = link_stack_isr_end );

