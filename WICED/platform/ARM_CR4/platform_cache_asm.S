@
@ Copyright 2015, Broadcom Corporation
@ All Rights Reserved.
@
@ This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
@ the contents of this file may not be disclosed to third parties, copied
@ or duplicated in any form, in whole or in part, without the prior
@ written permission of Broadcom Corporation.
@

#ifndef PLATFORM_L1_CACHE_WAY_NUM
#define PLATFORM_L1_CACHE_WAY_NUM 4
#endif

#ifndef PLATFORM_L1_CACHE_SET_NUM
#define PLATFORM_L1_CACHE_SET_NUM 256
#endif

#define CACHE_FUNCTION( name )     \
        .section .text.name, "ax"; \
        .global name;              \
        .type name, %function;     \
    name:


     .text
    .align 2
    .arm

/*
 *  void platform_icache_inv_all( void )
 */
CACHE_FUNCTION( platform_icache_inv_all )
    MOV        r0, #0
    MCR        p15, 0, r0, c7, c5, 0
    DSB
    BX         lr

#ifdef PLATFORM_L1_CACHE_SHIFT

/*
 *  void platform_dcache_inv_all( void )
 */
CACHE_FUNCTION( platform_dcache_inv_all )
    MOV        r0, #0
    MCR        p15, 0, r0, c15, c5, 0
    DSB
    BX         lr

#ifndef WICED_DCACHE_WTHROUGH
/*
 *  void _platform_dcache_clean_all( int is_invalidate )
 */
_platform_dcache_clean_all:
    LDR        r1, =(PLATFORM_L1_CACHE_WAY_NUM - 1)
1:
    LDR        r2, =(PLATFORM_L1_CACHE_SET_NUM - 1)
2:
    MOV        r3, #0
    ADD        r3, r3, r1
    LSL        r3, #25
    ADD        r3, r3, r2
    LSL        r3, #5
    CMP        r0, #0                 /* do clean or clean and invalidation? */
    BNE        3f
    MCR        p15, 0, r3, c7, c10, 2 /* clean by set/way */
    B          4f
3:
    MCR        p15, 0, r3, c7, c14, 2 /* clean and invalidate by set/way */
4:
    CMP        r2, #0                 /* loop through sets */
    SUBNE      r2, r2, #1
    BNE        2b
    CMP        r1, #0                 /* loop through ways */
    SUBNE      r1, r1, #1
    BNE        1b
    DSB
    BX         lr
#endif /* !WICED_DCACHE_WTHROUGH */

/*
 *  void platform_dcache_clean_all( void )
 */
CACHE_FUNCTION( platform_dcache_clean_all )
#ifdef WICED_DCACHE_WTHROUGH
    DSB
    BX         lr
#else
    MOV        r0, #0
    B          _platform_dcache_clean_all
#endif /* WICED_DCACHE_WTHROUGH */

/*
 *  void platform_dcache_clean_and_inv_all( void )
 */
CACHE_FUNCTION( platform_dcache_clean_and_inv_all )
#ifdef WICED_DCACHE_WTHROUGH
    B          platform_dcache_inv_all
#else
    MOV        r0, #1
    B          _platform_dcache_clean_all
#endif /* WICED_DCACHE_WTHROUGH */


/*
 *  void platform_dcache_disable( void )
 */
CACHE_FUNCTION( platform_dcache_disable )
    PUSH       {r4, lr}
    MRS        r4, cpsr                          /* save register to restore interrupts state before leaving function */
    CPSID      if                                /* disable interrupts */
    BL         platform_dcache_clean_and_inv_all
    MRC        p15, 0, r0, c1, c0, 0             /* read register */
    BIC        r0, r0, #0x1 << 2                 /* disable dcache */
    MCR        p15, 0, r0, c1, c0, 0             /* write back */
    ISB
    MSR        cpsr, r4                          /* restore interrupts state */
    POP        {r4, lr}
    BX         lr

/*
 *  void platform_dcache_enable( void )
 */
CACHE_FUNCTION( platform_dcache_enable )
    PUSH       {r4, lr}
    MRS        r4, cpsr                          /* save register to restore interrupts state before leaving function */
    CPSID      if                                /* disable interrupts */
    BL         platform_dcache_inv_all
    MRC        p15, 0, r0, c1, c0, 0             /* read register */
    ORR        r0, r0, #0x1 << 2                 /* enable dcache */
    MCR        p15, 0, r0, c1, c0, 0             /* write back */
    ISB
    MSR        cpsr, r4                          /* restore interrupts state */
    POP        {r4, lr}
    BX         lr

#endif /* PLATFORM_L1_CACHE_SHIFT */
