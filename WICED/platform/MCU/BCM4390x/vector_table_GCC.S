@
@ Copyright 2015, Broadcom Corporation
@ All Rights Reserved.
@
@ This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
@ the contents of this file may not be disclosed to third parties, copied
@ or duplicated in any form, in whole or in part, without the prior
@ written permission of Broadcom Corporation.
@

#define VENEER_NAME_THUMB2(name) veneer_thumb2_##name
#define VENEER_NAME_ARM(name)    veneer_arm_##name

#ifdef __thumb__
#define VECTOR_NAME(name)        VENEER_NAME_THUMB2(name)
#else
#define VECTOR_NAME(name)        name
#endif

#ifndef RESET_ENTRY_POINT
#define RESET_ENTRY_POINT        _low_start
#endif

    /* Vectors */

    .text
    .section .vectors_copyfrom, "ax"
    .align 2
    .global irq_instruction_vector_low_reset
    .global irq_vector_undefined_instruction
    .global irq_vector_software_interrupt
    .global irq_vector_prefetch_abort
    .global irq_vector_reserved
    .global irq_vector_external_interrupt
    .global irq_vector_fast_interrupt

    /* Total 8 entries */

#if defined(RESET_ENTRY_POINT_THUMB2)
    /* CPU starts in Thumb2 mode. This conditional compilation can be used when no ROM bootloader involved and we need to run application directly. */
    .thumb
    .syntax unified
    B.W        VENEER_NAME_ARM(RESET_ENTRY_POINT)
#elif defined(RESET_ENTRY_POINT_ARM)
    .arm
    B          RESET_ENTRY_POINT
#else
     /* Original reset vector which jumps into ROM bootloader. */
    .word      irq_instruction_vector_low_reset
#endif

    .arm

    B          VECTOR_NAME(irq_vector_undefined_instruction)    /* 0x04 - undefined instruction */
    B          VECTOR_NAME(irq_vector_software_interrupt)       /* 0x08 - software interrupt    */
    B          VECTOR_NAME(irq_vector_prefetch_abort)           /* 0x0c - prefetch abort        */
    B          VECTOR_NAME(irq_vector_data_abort)               /* 0x10 - data abort            */
    B          VECTOR_NAME(irq_vector_reserved)                 /* 0x14 - reserved              */
    B          VECTOR_NAME(irq_vector_external_interrupt)       /* 0x18 - external interrupt    */
    B          VECTOR_NAME(irq_vector_fast_interrupt)           /* 0x1c - fast interrupt        */

    /* Vector veneers when need to change instruction set */

#ifdef __thumb__
    .text
    .section .text, "ax"
    .align 2
    .arm
VECTOR_NAME(irq_vector_undefined_instruction):
    LDR        pc, =irq_vector_undefined_instruction
VECTOR_NAME(irq_vector_software_interrupt):
    LDR        pc, =irq_vector_software_interrupt
VECTOR_NAME(irq_vector_prefetch_abort):
    LDR        pc, =irq_vector_prefetch_abort
VECTOR_NAME(irq_vector_data_abort):
    LDR        pc, =irq_vector_data_abort
VECTOR_NAME(irq_vector_reserved):
    LDR        pc, =irq_vector_reserved
VECTOR_NAME(irq_vector_external_interrupt):
    LDR        pc, =irq_vector_external_interrupt
VECTOR_NAME(irq_vector_fast_interrupt):
    LDR        pc, =irq_vector_fast_interrupt
#endif /* __thumb__ */

#ifdef RESET_ENTRY_POINT_THUMB2
    .thumb
    .syntax unified
VENEER_NAME_ARM(RESET_ENTRY_POINT):
    LDR pc, =RESET_ENTRY_POINT
#endif
