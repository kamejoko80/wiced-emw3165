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
    AONRAM           (rwx)  : ORIGIN = 0x006A0000, LENGTH = 4K /* Use half of 8K AON memory as hard limit for tiny bootloader */
    RAM              (rwx)  : ORIGIN = 0x0069F000, LENGTH = 4K /* Using the ROM bootloader's Stack , High enough that APP would not overwrite*/
}

SECTIONS
{
    .config_data :
    {
        KEEP(*(.config*))
    } > AONRAM

    .text :
    {
        . = ALIGN(4);

        link_code_location = .;
        KEEP(*(.text.startup)) /* has to be first */
        *(.text .text.* .gnu.linkonce.t.*)

        link_code_end = .;

        . = ALIGN(4);

        link_const_variable_data_location = .;
        *(.rodata .rodata.* .gnu.linkonce.r.*)

        link_const_variable_data_end = .;

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

    } > AONRAM

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
    } > AONRAM

    .ARM.extab :
    {
       . = ALIGN(4);

        __extab_start = .;
        *(.ARM.extab*)
        __extab_end = .;
    } > AONRAM

    .data : /* Contains the non-zero initialised global variables */
    {
        . = ALIGN(4);

        *(.data*)
    } > RAM AT > AONRAM

    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        . = ALIGN(4);

        link_bss_location = .;
        *(.bss*)
        *(COMMON)
        link_bss_end = ALIGN(4);
    } > RAM

    .stack_start :
    {
        . = ALIGN(8);

        link_stack_location = .;
        *(.stack)
        . = MAX(link_stack_location + START_STACK_SIZE , .);
        link_stack_end = ALIGN(8);
    } > RAM

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

link_aon_data_location = LOADADDR(.data);
link_aon_data_end = link_aon_data_location + SIZEOF(.data);
link_ram_data_location = ADDR(.data);
