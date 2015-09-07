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
 *  Broadcom WLAN M2M Protocol interface
 *
 *  Implements the WICED Bus Protocol Interface for M2M
 *  Provides functions for initialising, de-intitialising 802.11 device,
 *  sending/receiving raw packets etc
 */

#include "m2m_hnddma.h"
#include "m2mdma_core.h"

#include "cr4.h"
#include "platform_cache.h"
#include "wiced_osl.h"

#include "network/wwd_network_constants.h"
#include "platform/wwd_bus_interface.h"
#include "wwd_assert.h"
#include "wwd_rtos_isr.h"

#include "platform_appscr4.h"
#include "platform_mcu_peripheral.h"
#include "platform_m2m.h"
#include "platform_config.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define VERIFY_RESULT( x )     { wiced_result_t verify_result; verify_result = (x); if ( verify_result != WICED_SUCCESS ) return verify_result; }

#define M2M_DESCRIPTOR_ALIGNMENT            16

#if !defined( WICED_DCACHE_WTHROUGH ) && defined( PLATFORM_L1_CACHE_SHIFT )
#define M2M_CACHE_WRITE_BACK                1
#define M2M_OPTIMIZED_DESCRIPTOR_ALIGNMENT  MAX(M2M_DESCRIPTOR_ALIGNMENT, PLATFORM_L1_CACHE_BYTES)
#define M2M_MEMCPY_DCACHE_CLEAN( ptr, size) platform_dcache_clean_range( ptr, size )
#else
#define M2M_CACHE_WRITE_BACK                0
#define M2M_OPTIMIZED_DESCRIPTOR_ALIGNMENT  M2M_DESCRIPTOR_ALIGNMENT
#define M2M_MEMCPY_DCACHE_CLEAN( ptr, size) do { UNUSED_PARAMETER( ptr ); UNUSED_PARAMETER( size ); cpu_data_synchronisation_barrier( ); } while(0)
#endif /* !WICED_DCACHE_WTHROUGH && PLATFORM_L1_CACHE_SHIFT */

#if defined( WICED_DCACHE_WTHROUGH ) && defined( PLATFORM_L1_CACHE_SHIFT )
#define M2M_CACHE_WRITE_THROUGH             1
#else
#define M2M_CACHE_WRITE_THROUGH             0
#endif /* WICED_DCACHE_WTHROUGH && PLATFORM_L1_CACHE_SHIFT */

/******************************************************
 *             Constants
 ******************************************************/
#define IRL_FC_SHIFT    24        /* frame count */
#define DEF_IRL                   (1 << IRL_FC_SHIFT)
#define DEF_INTMASK               (I_SMB_SW_MASK | I_RI | I_XI | I_ERRORS | I_WR_OOSYNC | I_RD_OOSYNC | I_RF_TERM | I_WF_TERM)
#define DEF_SDIO_INTMASK          (DEF_INTMASK | I_SRESET | I_IOE2)
#define DEF_SB_INTMASK            (I_SB_SERR | I_SB_RESPERR | I_SB_SPROMERR)
#define BT_INTMASK                (I_RI | I_XI | I_ERRORS | I_WR_OOSYNC | I_RD_OOSYNC)

#define ACPU_DMA_TX_CHANNEL             (1)
#define ACPU_DMA_RX_CHANNEL             (0)
#define WCPU_DMA_TX_CHANNEL             (0)
#define WCPU_DMA_RX_CHANNEL             (1)
#define MEMCPY_M2M_DMA_CHANNEL          (2)


/* DMA tunable parameters */
#ifndef M2M_DMA_RX_BUFFER_COUNT
#define M2M_DMA_RX_BUFFER_COUNT                (20)  /*(3)*/    /* Number of rx buffers */
#endif

#ifndef M2M_DMA_TX_DESCRIPTOR_COUNT
#define M2M_DMA_TX_DESCRIPTOR_COUNT            (32)  /*(3)*/
#endif

#ifndef M2M_DMA_RX_DESCRIPTOR_COUNT
#define M2M_DMA_RX_DESCRIPTOR_COUNT            (32)
#endif

#define SDPCMD_RXOFFSET                        (8)

#define M2M_DMA_RX_BUFFER_SIZE                 WICED_LINK_MTU
#define M2M_DMA_RX_BUFFER_HEADROOM             (0)

/* M2M register mapping */
#define M2M_REG_ADR              PLATFORM_M2M_REGBASE(0)
#define M2M_DMA_ADR              PLATFORM_M2M_REGBASE(0x200)

#define ARMCR4_SW_INT0           (0x1)
#define PLATFORM_WLANCR4         ( (volatile wlancr4_regs_t*    ) PLATFORM_WLANCR4_REGBASE(0x0))

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    M2M_DMA_INTERRUPTS_OFF,
    M2M_DMA_INTERRUPTS_ON,
} m2m_dma_interrupt_state_t;

/******************************************************
 *             Structures
 ******************************************************/
typedef appscr4_regs_t wlancr4_regs_t;

/******************************************************
 *             Variables
 ******************************************************/

static m2m_hnddma_t*         dma_handle = NULL;
static osl_t*                osh        = NULL;
static volatile sbm2mregs_t* m2mreg     = (sbm2mregs_t *)M2M_REG_ADR;
static int                   deiniting  = 0;
static int                   initing    = 0;

/******************************************************
 *             Function declarations
 ******************************************************/

static void m2m_dma_set_interrupt_state( m2m_dma_interrupt_state_t state );

/******************************************************
 *             Global Function definitions
 ******************************************************/
/* Device data transfer functions */

/*
 * From internal documentation: hwnbu-twiki/SdioMessageEncapsulation
 * When data is available on the device, the device will issue an interrupt:
 * - the device should signal the interrupt as a hint that one or more data frames may be available on the device for reading
 * - the host may issue reads of the 4 byte length tag at any time -- that is, whether an interupt has been issued or not
 * - if a frame is available, the tag read should return a nonzero length (>= 4) and the host can then read the remainder of the frame by issuing one or more CMD53 reads
 * - if a frame is not available, the 4byte tag read should return zero
 */

static void m2m_core_enable( void )
{
    osl_wrapper_enable( (void*)PLATFORM_M2M_MASTER_WRAPPER_REGBASE(0x0) );
}

#if PLATFORM_WLAN_POWERSAVE

static wiced_bool_t m2m_dma_tx_channel_idle( void )
{
    if ( ( m2mreg->dmaregs[ACPU_DMA_TX_CHANNEL].tx.status0 & D64_XS0_XS_MASK) == D64_XS0_XS_ACTIVE )
    {
        return WICED_FALSE;
    }

    if ( (m2mreg->dmaregs[ACPU_DMA_TX_CHANNEL].rx.status0 & D64_XS0_XS_MASK) == D64_XS0_XS_ACTIVE )
    {
        return WICED_FALSE;
    }

    return WICED_TRUE;
}

void m2m_powersave_comm_begin( void )
{
    if ( platform_wlan_powersave_res_up() )
    {
        M2M_INIT_DMA_ASYNC();
    }
}

void m2m_powersave_comm_end( wiced_bool_t force )
{
    platform_wlan_powersave_res_down( force ? NULL : &m2m_dma_tx_channel_idle, force );
}

static wiced_bool_t m2m_wlan_is_ready( void )
{
   return ( ( m2mreg->dmaregs[ACPU_DMA_RX_CHANNEL].tx.control & D64_XC_XE ) == 0 ) ? WICED_FALSE : WICED_TRUE;
}

#endif /* PLATFORM_WLAN_POWERSAVE */

static void m2m_signal_txdone( void )
{
    M2M_POWERSAVE_COMM_BEGIN();
    PLATFORM_WLANCR4->sw_int = ARMCR4_SW_INT0; /* Signal WLAN core there is a TX done by setting wlancr4 SW interrupt 0 */
    M2M_POWERSAVE_COMM_END();
}

uint32_t m2m_read_intstatus(void)
{
    uint32_t intstatus = 0, wlan_txstatus = 0;
    uint32_t txint;
    uint32_t rxint;

    txint = R_REG( osh, &m2mreg->intregs[ ACPU_DMA_TX_CHANNEL ].intstatus );
    rxint = R_REG( osh, &m2mreg->intregs[ ACPU_DMA_RX_CHANNEL ].intstatus );
    if (txint & (I_XI | I_ERRORS)){
        W_REG( osh, &m2mreg->intregs[ ACPU_DMA_TX_CHANNEL ].intstatus, (txint & I_XI));
    }

    if (rxint & (I_RI | I_ERRORS)){
        W_REG( osh, &m2mreg->intregs[ ACPU_DMA_RX_CHANNEL ].intstatus, (rxint & I_RI) );
    }
    intstatus = ( txint & I_XI ) | ( rxint & I_RI );
    wlan_txstatus = (rxint & I_XI);
    if (wlan_txstatus != 0) {
        m2m_signal_txdone();
    }
    return intstatus;
}

void m2m_init_dma( void )
{
    if ( ( dma_handle != NULL ) || ( initing != 0 ) )
    {
        return;
    }

    initing = 1;

    WPRINT_PLATFORM_DEBUG(("init_m2m_dma.. txreg: %p, rxreg: %p, txchan: %d, rxchannl: %d\n",
        &(m2mreg->dmaregs[ACPU_DMA_TX_CHANNEL].tx), &(m2mreg->dmaregs[ACPU_DMA_RX_CHANNEL].rx), ACPU_DMA_TX_CHANNEL, ACPU_DMA_RX_CHANNEL));

    M2M_POWERSAVE_COMM_BEGIN();

    m2m_core_enable();

#if PLATFORM_WLAN_POWERSAVE
    if ( !m2m_wlan_is_ready() )
    {
        m2m_signal_txdone();
    }
#endif

    osh = MALLOC(NULL, sizeof(osl_t));
    wiced_assert ("m2m_init_dma failed", osh != 0);

    /* Need to reserve 4 bytes header room so that the manipulation in wwd_bus_read_frame
     * which moves the data offset back 12 bytes would not break the data pointer */
    osh->pktget_add_remove  = sizeof(wwd_buffer_header_t) - sizeof( wwd_bus_header_t );
    osh->pktfree_add_remove = sizeof(wwd_buffer_header_t) - sizeof( wwd_bus_header_t );

    dma_handle = m2m_dma_attach( osh, "M2MDEV", NULL,
                                 (volatile void *)&(m2mreg->dmaregs[ACPU_DMA_TX_CHANNEL].tx),
                                 (volatile void *)&(m2mreg->dmaregs[ACPU_DMA_RX_CHANNEL].rx),
                                 M2M_DMA_TX_DESCRIPTOR_COUNT,
                                 M2M_DMA_RX_DESCRIPTOR_COUNT,
                                 M2M_DMA_RX_BUFFER_SIZE,
                                 M2M_DMA_RX_BUFFER_HEADROOM,
                                 M2M_DMA_RX_BUFFER_COUNT,
                                 SDPCMD_RXOFFSET,
                                 NULL );

    wiced_assert ("m2m_dma_attach failed", dma_handle != 0);

    m2m_dma_rxinit( dma_handle );
    m2m_dma_rxfill( dma_handle );
    W_REG( osh, &m2mreg->intrxlazy[ACPU_DMA_RX_CHANNEL], DEF_IRL );

#ifndef M2M_RX_POLL_MODE
    platform_irq_remap_sink(OOB_AOUT_M2M_INTR1, M2M_ExtIRQn);
    m2m_dma_set_interrupt_state( M2M_DMA_INTERRUPTS_ON );
#else
    m2m_dma_set_interrupt_state( M2M_DMA_INTERRUPTS_OFF );
#endif /* M2M_RX_POLL_MODE */

    m2m_dma_txinit( dma_handle ); /* Last step of initialization. Register settings inside tells WLAN that RX is ready. */

#if PLATFORM_WLAN_POWERSAVE
    while ( !m2m_wlan_is_ready() );
#endif

    M2M_POWERSAVE_COMM_END();

    initing = 0;
}

int m2m_is_dma_inited( void )
{
    return ( dma_handle != NULL );
}

void m2m_dma_tx_reclaim()
{
    void* txd;

    while ( ( txd = m2m_dma_getnexttxp( dma_handle, FALSE ) ) != NULL )
    {
        PKTFREE( NULL, txd, TRUE );
        M2M_POWERSAVE_COMM_END();
    }
}

void* m2m_read_dma_packet( uint16_t** hwtag )
{
    void* packet = NULL;

retry:
    packet = m2m_dma_rx( dma_handle );
    if ( packet == NULL )
    {
        return NULL;
    }

    PKTPULL(osh, packet, (uint16)SDPCMD_RXOFFSET);

    *hwtag = (uint16_t*) PKTDATA(osh, packet);

    if ( *hwtag == NULL )
    {
        PKTFREE(osh, packet, FALSE);
        goto retry;
    }

    if ( ( (*hwtag)[0] == 0 ) &&
         ( (*hwtag)[1] == 0 ) &&
         ( PKTLEN(osh, packet) == 504 ) )
    {
        // Dummy frame to keep M2M happy
        PKTFREE( osh, packet, FALSE );
        goto retry;
    }

    if ( ( ( (*hwtag)[0] | (*hwtag)[1] ) == 0                 ) ||
         ( ( (*hwtag)[0] ^ (*hwtag)[1] ) != (uint16_t) 0xFFFF ) )
    {
        WPRINT_PLATFORM_DEBUG(("Hardware Tag mismatch..\n"));

         PKTFREE(osh, packet, FALSE);

         goto retry;
    }
    return packet;
}

void m2m_refill_dma( void )
{
    if ( deiniting == 0 )
    {
        m2m_dma_rxfill(dma_handle);
    }
}

int m2m_rxactive_dma( void )
{
    return m2m_dma_rxactive( dma_handle );
}

void m2m_dma_tx_data( void* buffer, uint32_t unused_data_size )
{
    M2M_POWERSAVE_COMM_BEGIN();
    m2m_dma_txfast( dma_handle, buffer, 0, TRUE );
}

void m2m_deinit_dma( void )
{
    deiniting = 1;
    m2m_dma_set_interrupt_state( M2M_DMA_INTERRUPTS_OFF );
    m2m_dma_txreset  ( dma_handle );
    m2m_dma_rxreset  ( dma_handle );
    m2m_dma_rxreclaim( dma_handle );
    m2m_dma_txreclaim( dma_handle, HNDDMA_RANGE_ALL );
    m2m_dma_detach   ( dma_handle );
    MFREE(NULL, osh, sizeof(osl_t));
    dma_handle = NULL;
    deiniting = 0;
    M2M_POWERSAVE_COMM_DONE();
}

int m2m_unprotected_blocking_dma_memcpy( void* destination, const void* source, uint32_t byte_count )
{
#if M2M_CACHE_WRITE_BACK

    /* It is unsafe to perform DMA to partial cache lines, since it is impossible to predict when a
     * cache line will be evicted back to main memory, and such an event would overwrite the DMA data.
     * Hence it is required that any cache lines which contain other data must be manually copied.
     * These will be the first & last cache lines.
     */
    uint32_t start_offset = PLATFORM_L1_CACHE_LINE_OFFSET( destination );
    if ( start_offset != 0 )
    {
        /* Start of destination not at start of cache line
         * memcpy the first part of source data to destination
         */
        uint32_t start_copy_byte_count = MIN( byte_count, PLATFORM_L1_CACHE_BYTES - start_offset );
        memcpy( destination, source, start_copy_byte_count );
        source      += start_copy_byte_count;
        destination += start_copy_byte_count;
        byte_count  -= start_copy_byte_count;
    }

    uint32_t end_offset   = ((uint32_t) destination + byte_count) & PLATFORM_L1_CACHE_LINE_MASK;
    if ( ( byte_count > 0 ) && ( end_offset != 0 ))
    {
        /* End of destination not at end of cache line
         * memcpy the last part of source data to destination
         */
        uint32_t end_copy_byte_count = MIN( byte_count, end_offset );
        uint32_t offset_from_start = byte_count - end_copy_byte_count;

        memcpy( (char*)destination + offset_from_start, (char*)source + offset_from_start, end_copy_byte_count );
        byte_count  -= end_copy_byte_count;
    }

    /* Remaining data should be fully aligned to cache lines */
    wiced_assert( "check alignment achieved by copy", ( byte_count == 0 ) || ( ( (uint32_t)destination & PLATFORM_L1_CACHE_LINE_MASK ) == 0 ) );
    wiced_assert( "check alignment achieved by copy", ( byte_count == 0 ) || ( ( ( (uint32_t)destination + byte_count ) & PLATFORM_L1_CACHE_LINE_MASK ) == 0 ) );

#endif /* M2M_CACHE_WRITE_BACK */

    /* Check if there are any remaining cache lines (that are entirely part of destination buffer) */
    if ( byte_count <= 0 )
    {
        return 0;
    }

    /* Allocate descriptors */
    static volatile dma64dd_t descriptors[2] ALIGNED(M2M_OPTIMIZED_DESCRIPTOR_ALIGNMENT);

    /* Prepare M2M engine */
    m2m_core_enable( );

    /* Setup descriptors */
    descriptors[0].ctrl1 = D64_CTRL1_EOF | D64_CTRL1_SOF;
    descriptors[1].ctrl1 = D64_CTRL1_EOF | D64_CTRL1_SOF;

    /* Setup DMA channel transmitter */
    m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].tx.addrhigh = 0;
    m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].tx.addrlow  = ((uint32_t)&descriptors[0]);  // Transmit descriptor table address
    m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].tx.ptr      = ((uint32_t)&descriptors[0] + sizeof(dma64dd_t)) & 0xffff; // needs to be lower 16 bits of descriptor address
    m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].tx.control  = D64_XC_PD | ((2 << D64_XC_BL_SHIFT) & D64_XC_BL_MASK) | ((1 << D64_XC_PC_SHIFT) & D64_XC_PC_MASK);

    /* Setup DMA channel receiver */
    m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].rx.addrhigh = 0;
    m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].rx.addrlow  = ((uint32_t)&descriptors[1]);  // Transmit descriptor table address
    m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].rx.ptr      = ((uint32_t)&descriptors[1] + sizeof(dma64dd_t)) & 0xffff; // needs to be lower 16 bits of descriptor address
    m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].rx.control  = D64_XC_PD | ((2 << D64_XC_BL_SHIFT) & D64_XC_BL_MASK) | ((1 << D64_XC_PC_SHIFT) & D64_XC_PC_MASK);

    /* Flush pending writes in source buffer range to main memory, so they are accessable by the DMA */
    M2M_MEMCPY_DCACHE_CLEAN( source, byte_count );

#if M2M_CACHE_WRITE_THROUGH
    void*    original_destination = destination;
    uint32_t original_byte_count  = byte_count;
#elif M2M_CACHE_WRITE_BACK
    /* Invalidate destination buffer - required so that any pending write data is not
     * written back (evicted) over the top of the DMA data.
     * This also prepares the cache for reading the finished DMA data.  However if other functions are silly
     * enough to read the DMA data before it is finished, the cache may get out of sync.
     */
    platform_dcache_inv_range( destination, byte_count );
#endif /* M2M_CACHE_WRITE_THROUGH */

    while ( byte_count > 0 )
    {
        uint32_t write_size = MIN( byte_count, D64_CTRL2_BC_USABLE_MASK );

        descriptors[0].addrlow = (uint32_t)source;
        descriptors[1].addrlow = (uint32_t)destination;
        descriptors[0].ctrl2   = D64_CTRL2_BC_USABLE_MASK & write_size;
        descriptors[1].ctrl2   = D64_CTRL2_BC_USABLE_MASK & write_size;

        /* Flush the DMA descriptors to main memory to ensure DMA picks them up */
        M2M_MEMCPY_DCACHE_CLEAN( (void*)&descriptors[0], sizeof(descriptors) );

        /* Fire off the DMA */
        m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].tx.control |= D64_XC_XE;
        m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].rx.control |= D64_XC_XE;

        /* Update variables */
        byte_count  -= write_size;
        destination += write_size;
        source      += write_size;

        /* Wait till DMA has finished */
        while ( ( (m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].tx.status0 & D64_XS0_XS_MASK) == D64_XS0_XS_ACTIVE ) ||
                ( (m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].rx.status0 & D64_XS0_XS_MASK) == D64_XS0_XS_ACTIVE ) )
        {
        }

        /* Switch off the DMA */
        m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].tx.control &= (~D64_XC_XE);
        m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].rx.control &= (~D64_XC_XE);
    }

#if M2M_CACHE_WRITE_THROUGH
    /* Prepares the cache for reading the finished DMA data. */
    platform_dcache_inv_range( original_destination, original_byte_count );
#endif /* M2M_CACHE_WRITE_THROUGH */

    /* Indicate empty table. Otherwise core is not freeing PMU resources. */
    m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].tx.ptr = m2mreg->dmaregs[MEMCPY_M2M_DMA_CHANNEL].tx.addrlow & 0xFFFF;

    return 0;
}

void m2m_wlan_dma_deinit( void )
{
    /* Stop WLAN side M2M dma */
    m2m_dma_txreset_inner(osh, (void *)&m2mreg->dmaregs[ACPU_DMA_RX_CHANNEL].tx);
    m2m_dma_rxreset_inner(osh, (void *)&m2mreg->dmaregs[ACPU_DMA_TX_CHANNEL].rx);

    /* Reset wlan m2m tx/rx pointers */
    W_REG(osh, &m2mreg->dmaregs[ACPU_DMA_RX_CHANNEL].tx.ptr, 0);
    W_REG(osh, &m2mreg->dmaregs[ACPU_DMA_TX_CHANNEL].rx.ptr, 0);

    /* Mask interrupt & clear all interrupt status */
    W_REG(osh, &m2mreg->intregs[ACPU_DMA_TX_CHANNEL].intmask, 0);
    W_REG(osh, &m2mreg->intregs[ACPU_DMA_RX_CHANNEL].intstatus, 0xffffffff);
    W_REG(osh, &m2mreg->intregs[ACPU_DMA_TX_CHANNEL].intstatus, 0xffffffff);
}

/******************************************************
 *             Static  Function definitions
 ******************************************************/

static void m2m_dma_set_interrupt_state( m2m_dma_interrupt_state_t state )
{
    uint32_t rx_mask;

    rx_mask = R_REG( osh, &m2mreg->intregs[ ACPU_DMA_RX_CHANNEL ].intmask );
    if ( state == M2M_DMA_INTERRUPTS_ON )
    {
        rx_mask |= I_RI | I_ERRORS;
    }
    else
    {
        rx_mask &= ~( I_RI | I_ERRORS );
    }
    W_REG( osh, &m2mreg->intregs[ ACPU_DMA_RX_CHANNEL ].intmask, rx_mask );
}

#ifndef M2M_RX_POLL_MODE
/* M2M DMA:
 * APPS: TX/RX : M2M channel 1/channel 0
 * WLAN: TX/RX : M2M channel 0/channel 1
 * APPS and WLAN core only enable RX interrupt. Regarding TX DONE interrupt,
 * APPS core uses SW0 interrupt to signal WLAN core once there is TX DONE interrupt happening.
 * WLAN core also uses SW0 interrupt to signal APPS core the TX DONE interrupt.
 * */
WWD_RTOS_DEFINE_ISR( platform_m2mdma_isr )
{
    host_platform_bus_disable_interrupt();
    wwd_thread_notify_irq();
}

WWD_RTOS_MAP_ISR( platform_m2mdma_isr, M2M_ISR)
WWD_RTOS_MAP_ISR( platform_m2mdma_isr, Sw0_ISR)
#endif

