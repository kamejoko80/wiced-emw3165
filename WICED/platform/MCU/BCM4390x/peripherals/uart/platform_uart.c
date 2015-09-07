/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * BCM43909 supports 3 UART ports as follows:
 * 1) Slow UART (ChipCommon offset 0x300-0x327), 2-wire (TX/RX), 16550-compatible.
 * 2) Fast UART (ChipCommon offset 0x130), 4-wire (TX/RX/RTS/CTS), SECI.
 * 3) GCI UART (GCI core offset 0x1D0), 2-wire (TX/RX), SECI.
 *
 * ChipCommon Fast UART has dedicated pins.
 * ChipCommon Slow UART and GCI UART share pins with RF_SW_CTRL_6-9.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "typedefs.h"
#include "platform_constants.h"
#include "RTOS/wwd_rtos_interface.h"
#include "platform_peripheral.h"
#include "platform_map.h"
#include "wwd_rtos.h"
#include "wwd_assert.h"
#include "wiced_osl.h"
#include "hndsoc.h"
#ifdef CONS_LOG_BUFFER_SUPPORT
#include "platform_cache.h"
#endif  /* CONS_LOG_BUFFER_SUPPORT */

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Some of the bits in slow UART status and control registers */
#define UART_SLOW_LSR_THRE   (0x20)    /* Transmit-hold-register empty */
#define UART_SLOW_LSR_TDHR   (0x40)    /* Data-hold-register empty */
#define UART_SLOW_LSR_RXRDY  (0x01)    /* Receiver ready */
#define UART_SLOW_LCR_DLAB   (0x80)    /* Divisor latch access bit */
#define UART_SLOW_LCR_WLEN8  (0x03)    /* Word length: 8 bits */

/* Some of the bits in slow UART Interrupt Enable Register */
#define UART_SLOW_IER_THRE   (0x02)     /* Transmit-hold-register empty */
#define UART_SLOW_IER_RXRDY  (0x01)     /* Receiver ready */

/* Some of the bits in slow UART Interrupt Identification Register */
#define UART_SLOW_IIR_INT_ID_MASK   (0xF)
#define UART_SLOW_IIR_INT_ID_THRE   (0x2)   /* Transmit-hold-register empty */
#define UART_SLOW_IIR_INT_ID_RXRDY  (0x4)   /* Receiver ready */

/* Some of the bits in slow UART Modem Control Register */
#define UART_SLOW_MCR_OUT2          (0x08)

/* Parity bits in slow UART LCR register */
#define UART_EPS_BIT    (4)
#define UART_PEN_BIT    (3)

#define UART_SLOW_REGBASE               (PLATFORM_CHIPCOMMON_REGBASE(0x300))
#define UART_FAST_REGBASE               (PLATFORM_CHIPCOMMON_REGBASE(0x1C0))

#define CHIPCOMMON_CORE_CAP_REG         *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x004)))
#define CHIPCOMMON_CORE_CTRL_REG        *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x008)))
#define CHIPCOMMON_INT_STATUS_REG       *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x020)))
#define CHIPCOMMON_INT_MASK_REG         *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x024)))
#define CHIPCOMMON_CORE_CLK_DIV         *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x0A4)))
#define CHIPCOMMON_CORE_CAP_EXT_REG     *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x0AC)))
#define CHIPCOMMON_SECI_CONFIG_REG      *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x130)))
#define CHIPCOMMON_SECI_STATUS_REG      *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x134)))
#define CHIPCOMMON_SECI_STATUS_MASK_REG *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x138)))
#define CHIPCOMMON_SECI_FIFO_LEVEL_REG  *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x18C)))
#define CHIPCOMMON_CLK_CTL_STATUS_REG   *((volatile uint32_t*)(PLATFORM_CHIPCOMMON_REGBASE(0x1E0)))

/* ChipCommon Capabilities Extension */
#define CC_CAP_EXT_SECI_PRESENT                  (0x00000001)    /* SECI present */
#define CC_CAP_EXT_SECI_PLAIN_UART_PRESENT       (0x00000008)    /* SECI Plain UART present */

/* ChipCommon ClockCtlStatus bits */
#define CC_CLK_CTL_ST_SECI_CLK_REQ                  (1 << 8)
#define CC_CLK_CTL_ST_SECI_CLK_AVAIL                (1 << 24)
#define CC_CLK_CTL_ST_BACKPLANE_ALP                 (1 << 18)
#define CC_CLK_CTL_ST_BACKPLANE_HT                  (1 << 19)

/* ChipCommon Slow UART IntStatus and IntMask register bit */
#define UART_SLOW_CC_INT_STATUS_MASK                (1 << 6)

/* ChipCommon FAST UART IntStatus and IntMask register bit */
#define UART_FAST_CC_INT_STATUS_MASK                (1 << 4)

/* ChipCommon SECI Config register bits */
#define CC_SECI_CONFIG_HT_CLOCK                     (1 << 13)

/* ChipCommon SECI Status register bits */
#define CC_SECI_STATUS_TX_FIFO_FULL                 (1 << 9)
#define CC_SECI_STATUS_TX_FIFO_ALMOST_EMPTY         (1 << 10)
#define CC_SECI_STATUS_RX_FIFO_EMPTY                (1 << 11)
#define CC_SECI_STATUS_RX_FIFO_ALMOST_FULL          (1 << 12)

#define UART_SLOW_MAX_TRANSMIT_WAIT_TIME         (10)
#define UART_SLOW_MAX_READ_WAIT_TIME             (200)
#define UART_SLOW_CLKDIV_MASK                    (0x000000FF)

#define UART_FAST_MAX_TRANSMIT_WAIT_TIME         (10)
#define UART_FAST_MAX_READ_WAIT_TIME             (200)

/* SECI configuration */
#define SECI_MODE_UART                           (0x0)
#define SECI_MODE_SECI                           (0x1)
#define SECI_MODE_LEGACY_3WIRE_BT                (0x2)
#define SECI_MODE_LEGACY_3WIRE_WLAN              (0x3)
#define SECI_MODE_HALF_SECI                      (0x4)

#define SECI_RESET                               (1 << 0)
#define SECI_RESET_BAR_UART                      (1 << 1)
#define SECI_ENAB_SECI_ECI                       (1 << 2)
#define SECI_ENAB_SECIOUT_DIS                    (1 << 3)
#define SECI_MODE_MASK                           (0x7)
#define SECI_MODE_SHIFT                          (4)
#define SECI_UPD_SECI                            (1 << 7)

#define SECI_SLIP_ESC_CHAR                       (0xDB)
#define SECI_SIGNOFF_0                           SECI_SLIP_ESC_CHAR
#define SECI_SIGNOFF_1                           (0)
#define SECI_REFRESH_REQ                         (0xDA)

#define UART_SECI_MSR_CTS_STATE                  (1 << 0)
#define UART_SECI_MSR_RTS_STATE                  (1 << 1)
#define UART_SECI_SECI_IN_STATE                  (1 << 2)
#define UART_SECI_SECI_IN2_STATE                 (1 << 3)

/* SECI FIFO Level Register */
#define SECI_RX_FIFO_LVL_MASK                    (0x3F)
#define SECI_RX_FIFO_LVL_SHIFT                   (0)
#define SECI_TX_FIFO_LVL_MASK                    (0x3F)
#define SECI_TX_FIFO_LVL_SHIFT                   (8)
#define SECI_RX_FIFO_LVL_FLOW_CTL_MASK           (0x3F)
#define SECI_RX_FIFO_LVL_FLOW_CTL_SHIFT          (16)

/* Default FIFO levels for RX, TX and host flow control */
#define SECI_RX_FIFO_LVL_DEFAULT                 (0x1)
#define SECI_TX_FIFO_LVL_DEFAULT                 (0x1)
#define SECI_RX_FIFO_LVL_FLOW_CTL_DEFAULT        (0x1)

/* SECI UART FCR bit definitions */
#define UART_SECI_FCR_RFR                        (1 << 0)
#define UART_SECI_FCR_TFR                        (1 << 1)
#define UART_SECI_FCR_SR                         (1 << 2)
#define UART_SECI_FCR_THP                        (1 << 3)
#define UART_SECI_FCR_AB                         (1 << 4)
#define UART_SECI_FCR_ATOE                       (1 << 5)
#define UART_SECI_FCR_ARTSOE                     (1 << 6)
#define UART_SECI_FCR_ABV                        (1 << 7)
#define UART_SECI_FCR_ALM                        (1 << 8)

/* SECI UART LCR bit definitions */
#define UART_SECI_LCR_STOP_BITS                  (1 << 0) /* 0 - 1bit, 1 - 2bits */
#define UART_SECI_LCR_PARITY_EN                  (1 << 1)
#define UART_SECI_LCR_PARITY                     (1 << 2) /* 0 - odd, 1 - even */
#define UART_SECI_LCR_RX_EN                      (1 << 3)
#define UART_SECI_LCR_LBRK_CTRL                  (1 << 4) /* 1 => SECI_OUT held low */
#define UART_SECI_LCR_TXO_EN                     (1 << 5)
#define UART_SECI_LCR_RTSO_EN                    (1 << 6)
#define UART_SECI_LCR_SLIPMODE_EN                (1 << 7)
#define UART_SECI_LCR_RXCRC_CHK                  (1 << 8)
#define UART_SECI_LCR_TXCRC_INV                  (1 << 9)
#define UART_SECI_LCR_TXCRC_LSBF                 (1 << 10)
#define UART_SECI_LCR_TXCRC_EN                   (1 << 11)
#define UART_SECI_LCR_RXSYNC_EN                  (1 << 12)

/* SECI UART MCR bit definitions */
#define UART_SECI_MCR_TX_EN                      (1 << 0)
#define UART_SECI_MCR_PRTS                       (1 << 1)
#define UART_SECI_MCR_SWFLCTRL_EN                (1 << 2)
#define UART_SECI_MCR_HIGHRATE_EN                (1 << 3)
#define UART_SECI_MCR_LOOPBK_EN                  (1 << 4)
#define UART_SECI_MCR_AUTO_RTS                   (1 << 5)
#define UART_SECI_MCR_AUTO_TX_DIS                (1 << 6)
#define UART_SECI_MCR_BAUD_ADJ_EN                (1 << 7)
#define UART_SECI_MCR_XONOFF_RPT                 (1 << 9)

/* SECI UART LSR bit definitions */
#define UART_SECI_LSR_RXOVR_MASK                 (1 << 0)
#define UART_SECI_LSR_RFF_MASK                   (1 << 1)
#define UART_SECI_LSR_TFNE_MASK                  (1 << 2)
#define UART_SECI_LSR_TI_MASK                    (1 << 3)
#define UART_SECI_LSR_TPR_MASK                   (1 << 4)
#define UART_SECI_LSR_TXHALT_MASK                (1 << 5)

/* SECI UART MSR bit definitions */
#define UART_SECI_MSR_CTSS_MASK                  (1 << 0)
#define UART_SECI_MSR_RTSS_MASK                  (1 << 1)
#define UART_SECI_MSR_SIS_MASK                   (1 << 2)
#define UART_SECI_MSR_SIS2_MASK                  (1 << 3)

/* SECI UART Data bit definitions */
#define UART_SECI_DATA_RF_NOT_EMPTY_BIT          (1 << 12)
#define UART_SECI_DATA_RF_FULL_BIT               (1 << 13)
#define UART_SECI_DATA_RF_OVRFLOW_BIT            (1 << 14)
#define UART_SECI_DATA_FIFO_PTR_MASK             (0xFF)
#define UART_SECI_DATA_RF_RD_PTR_SHIFT           (16)
#define UART_SECI_DATA_RF_WR_PTR_SHIFT           (24)

/* Range for High rate SeciUARTBaudRateDivisor is 0xF1 - 0xF8 */
#define UART_SECI_HIGH_RATE_THRESHOLD_LOW        (0xF1)
#define UART_SECI_HIGH_RATE_THRESHOLD_HIGH       (0xF8)

/*
* Maximum delay for the PMU state transition in us.
* This is an upper bound intended for spinwaits etc.
*/
#define PMU_MAX_TRANSITION_DLY  15000

/* Slow UART RX ring buffer size */
#define UART_SLOW_RX_BUFFER_SIZE  64

/* Fast UART RX ring buffer size */
#define UART_FAST_RX_BUFFER_SIZE  64

#ifdef CONS_LOG_BUFFER_SUPPORT
#define CONS_LOG_BUFFER_SIZE      (16*1024)
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

typedef struct
{
    volatile uint8_t rx_tx_dll;
    volatile uint8_t ier_dlm;
    volatile uint8_t iir_fcr;
    volatile uint8_t lcr;
    volatile uint8_t mcr;
    volatile uint8_t lsr;
    volatile uint8_t msr;
    volatile uint8_t scr;
} uart_slow_t;

typedef struct
{
    volatile uint32_t data;
    volatile uint32_t bauddiv;
    volatile uint32_t fcr;
    volatile uint32_t lcr;
    volatile uint32_t mcr;
    volatile uint32_t lsr;
    volatile uint32_t msr;
    volatile uint32_t baudadj;
} uart_fast_t;

#ifdef CONS_LOG_BUFFER_SUPPORT
typedef struct
{
    uint8_t *buf;
    uint buf_size;
    uint idx;
    uint out_idx;
} hndrte_log_t;

typedef struct
{
    volatile uint vcons_in;
    volatile uint vcons_out;
    hndrte_log_t log;
    uint reserved[2];
} hndrte_cons_t;
#endif  /* CONS_LOG_BUFFER_SUPPORT */

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* Slow and Fast UART reside in ChipCommon core and accessed from Apps core via AXI backplane */
uart_slow_t* const uart_slow_base = ( uart_slow_t* )( UART_SLOW_REGBASE );
uart_fast_t* const uart_fast_base = ( uart_fast_t* )( UART_FAST_REGBASE );

/* Slow UART RX ring buffer */
wiced_ring_buffer_t uart_slow_rx_buffer;
uint8_t             uart_slow_rx_data[UART_SLOW_RX_BUFFER_SIZE];

/* Fast UART RX ring buffer */
wiced_ring_buffer_t uart_fast_rx_buffer;
uint8_t             uart_fast_rx_data[UART_FAST_RX_BUFFER_SIZE];

#ifdef CONS_LOG_BUFFER_SUPPORT
#define CONS_BUFFER_SIZE        (sizeof(hndrte_cons_t) + CONS_LOG_BUFFER_SIZE)
static uint8_t             cons_buffer[sizeof(hndrte_cons_t) + CONS_LOG_BUFFER_SIZE + 2*PLATFORM_L1_CACHE_BYTES];
static hndrte_cons_t       *cons0;
#endif  /*  CONS_LOG_BUFFER_SUPPORT  */

/******************************************************
 *               Function Definitions
 ******************************************************/

#ifdef CONS_LOG_BUFFER_SUPPORT
static platform_result_t cons_init(void)
{
    uint8* buf_startp;

    buf_startp = (uint8 *) (((uint)&cons_buffer[0] + PLATFORM_L1_CACHE_BYTES-1) & ~(PLATFORM_L1_CACHE_BYTES-1));

    /* prevent any write backs */
    platform_dcache_inv_range(buf_startp, CONS_BUFFER_SIZE);
    cons0 = (hndrte_cons_t *) platform_addr_cached_to_uncached(buf_startp);

    cons0->vcons_in = 0;
    cons0->vcons_out = 0;
    cons0->log.buf = (uint8 *) (cons0 + 1);
    cons0->log.buf_size = CONS_LOG_BUFFER_SIZE;
    cons0->log.idx = 0;
    cons0->log.out_idx = 0;

    return PLATFORM_SUCCESS;
}

platform_result_t cons_transmit_bytes( platform_uart_driver_t* driver, const uint8_t* data_out, uint32_t size )
{
    uint remain_len;

    if (size > cons0->log.buf_size)
    {
        /*  Message is larger than the log buffer. Only hold the last part  */
        data_out += size - cons0->log.buf_size;
        size = cons0->log.buf_size;
    }

    remain_len = cons0->log.buf_size - cons0->log.idx;
    if (size > remain_len)
    {
        memcpy(&cons0->log.buf[cons0->log.idx], data_out, remain_len);
        cons0->log.idx = 0;
        data_out += remain_len;
        size -= remain_len;
    }

    memcpy(&cons0->log.buf[cons0->log.idx], data_out, size);
    cons0->log.idx += size;
    if (cons0->log.idx >= cons0->log.buf_size)
        cons0->log.idx = 0;
    cons0->log.out_idx = cons0->log.idx;

    return PLATFORM_SUCCESS;
}

#endif /* CONS_LOG_BUFFER_SUPPORT */

static void uart_chipcommon_interrupt_mask( uint32_t clear_mask, uint32_t set_mask )
{
    uint32_t cc_int_mask;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS(flags);
    cc_int_mask = PLATFORM_CHIPCOMMON->interrupt.mask.raw;

    cc_int_mask = (cc_int_mask & ~clear_mask) | set_mask;

    PLATFORM_CHIPCOMMON->interrupt.mask.raw = cc_int_mask;
    WICED_RESTORE_INTERRUPTS(flags);
}

static void uart_slow_disable_interrupts( void )
{
    /* Disable slow UART interrupts for RX data */
    uart_slow_base->mcr &= ~UART_SLOW_MCR_OUT2;

    /* Disable slow UART interrupts */
    uart_slow_base->ier_dlm = 0x0;

    /* Disable slow UART interrupt in ChipCommon interrupt mask */
    uart_chipcommon_interrupt_mask(UART_SLOW_CC_INT_STATUS_MASK, 0x0);
}

static void uart_fast_disable_interrupts( void )
{
    /* Disable fast UART interrupts */
    CHIPCOMMON_SECI_STATUS_MASK_REG = 0x0;

    /* Disable fast UART interrupt in ChipCommon interrupt mask */
    uart_chipcommon_interrupt_mask(UART_FAST_CC_INT_STATUS_MASK, 0x0);
}

static void platform_uart_disable_interrupts( platform_uart_port_t uart_port )
{
    if (uart_port == UART_SLOW)
    {
        uart_slow_disable_interrupts();
    }
    else if(uart_port == UART_FAST)
    {
        uart_fast_disable_interrupts();
    }
}

#if !defined(BCM4390X_UART_SLOW_POLL_MODE) || !defined(BCM4390X_UART_FAST_POLL_MODE)
static void uart_slow_enable_interrupts( void )
{
#ifndef BCM4390X_UART_SLOW_POLL_MODE
    /* Enable slow UART interrupt in ChipCommon interrupt mask */
    uart_chipcommon_interrupt_mask(0x0, UART_SLOW_CC_INT_STATUS_MASK);

    /* Enable RX Data Available interrupt in slow UART interrupt mask */
    uart_slow_base->ier_dlm = UART_SLOW_IER_RXRDY;

    /* Enable slow UART interrupts for RX data */
    uart_slow_base->mcr |= UART_SLOW_MCR_OUT2;
#endif /* !BCM4390X_UART_SLOW_POLL_MODE */
}

static void uart_fast_enable_interrupts( void )
{
#ifndef BCM4390X_UART_FAST_POLL_MODE
    /* Enable fast UART interrupt in ChipCommon interrupt mask */
    uart_chipcommon_interrupt_mask(0x0, UART_FAST_CC_INT_STATUS_MASK);

    /* Enable SRFAF (SECI RX FIFO Almost Full) RX interrupt in fast UART interrupt mask */
    CHIPCOMMON_SECI_STATUS_MASK_REG = CC_SECI_STATUS_RX_FIFO_ALMOST_FULL;
#endif /* !BCM4390X_UART_FAST_POLL_MODE */
}

static void platform_uart_enable_interrupts( platform_uart_port_t uart_port )
{
    if (uart_port == UART_SLOW)
    {
        uart_slow_enable_interrupts();
    }
    else if(uart_port == UART_FAST)
    {
        uart_fast_enable_interrupts();
    }
}
#endif /* !BCM4390X_UART_SLOW_POLL_MODE || !BCM4390X_UART_FAST_POLL_MODE */

static platform_result_t uart_slow_init_internal( platform_uart_driver_t* driver, const platform_uart_config_t* config )
{
    uint32_t            alp_clock_value = 0;
    uint16_t            baud_divider;

    /* Disable interrupts from slow UART during initialization */
    platform_uart_disable_interrupts(driver->interface->port);

    /* Make sure ChipCommon core is enabled */
    osl_core_enable(CC_CORE_ID);

    /* Enable Slow UART clocking */
    /* Turn on ALP clock for UART, when we are not using a divider
     * Set the override bit so we don't divide it
     */
    CHIPCOMMON_CORE_CTRL_REG &= ~( 1 << 3 );
    CHIPCOMMON_CORE_CTRL_REG |= 0x01;
    CHIPCOMMON_CORE_CTRL_REG |= ( 1 << 3 );

    /* Get current ALP clock value */
    alp_clock_value = osl_alp_clock();

    wiced_assert("ALP clock value can not be identified properly", alp_clock_value != 0);

    /* Calculate the necessary divider value for a given baud rate */
    /* divisor = (serial clock frequency/16) / (baud rate)
    *  The baud_rate / 2 is added to reduce error to + / - half of baud rate.
    */
    baud_divider = ( (alp_clock_value / 16 ) + (config->baud_rate / 2) ) / config->baud_rate;

    /* Setup the baudrate */
    uart_slow_base->lcr      |= UART_SLOW_LCR_DLAB;
    uart_slow_base->rx_tx_dll = baud_divider & 0xff;
    uart_slow_base->ier_dlm   = baud_divider >> 8;
    uart_slow_base->lcr      |= UART_SLOW_LCR_WLEN8;

    /* Clear DLAB bit. This bit must be cleared after initial baud rate setup in order to access other registers. */
    uart_slow_base->lcr &= ~UART_SLOW_LCR_DLAB;
    wiced_assert(" Currently we do not support data width different from 8bits ", config->data_width == DATA_WIDTH_8BIT );

    /* Configure stop bits and a parity */
    if ( config->parity == ODD_PARITY )
    {
        uart_slow_base->lcr |= ( 1 << UART_PEN_BIT );
        uart_slow_base->lcr &= ~( 1 << UART_EPS_BIT );
    }
    else if ( config->parity == EVEN_PARITY )
    {
        uart_slow_base->lcr |= ( 1 << UART_PEN_BIT );
        uart_slow_base->lcr |= ( 1 << UART_EPS_BIT );
    }

    /* Slow UART does not have HW flow control */
    driver->hw_flow_control_is_on = WICED_FALSE;

    /* Strap the appropriate platform pins for Slow UART RX/TX functions */
    platform_pin_function_init(driver->interface->rx_pin->pin, PIN_FUNCTION_UART_DBG_RX, PIN_FUNCTION_CONFIG_UNKNOWN);
    platform_pin_function_init(driver->interface->tx_pin->pin, PIN_FUNCTION_UART_DBG_TX, PIN_FUNCTION_CONFIG_UNKNOWN);

    /* Setup ring buffer and related parameters */
    ring_buffer_init(&uart_slow_rx_buffer, uart_slow_rx_data, UART_SLOW_RX_BUFFER_SIZE);
    driver->rx_buffer = &uart_slow_rx_buffer;
    host_rtos_init_semaphore(&driver->rx_complete);
    host_rtos_init_semaphore(&driver->tx_complete);

#ifndef BCM4390X_UART_SLOW_POLL_MODE
    /* Initialization complete, enable interrupts from slow UART */
    platform_uart_enable_interrupts(driver->interface->port);

    /* Make sure ChipCommon Core external IRQ to APPS core is enabled */
    platform_chipcommon_enable_irq();
#endif /* !BCM4390X_UART_SLOW_POLL_MODE */

    return PLATFORM_SUCCESS;
}

static platform_result_t uart_fast_init_internal( platform_uart_driver_t* driver, const platform_uart_config_t* config )
{
    uint32_t seci_mode  = SECI_MODE_UART;
    uint32_t cc_cap_ext = 0;
    uint32_t clk_value  = 0;
    uint32_t clk_timeout = 100000000;
    uint32_t cc_seci_fifo_level = 0;
    uint32_t flags;

    uint32_t baud_rate_div_high_rate  = 0;
    uint32_t baud_rate_div_low_rate   = 0;
    uint32_t baud_rate_temp_value     = 0;
    uint32_t baud_rate_adjustment     = 0;
    uint32_t baud_rate_adjust_low     = 0;
    uint32_t baud_rate_adjust_high    = 0;
    uint32_t uart_high_rate_mode      = 0;

    /* Disable interrupts from fast UART during initialization */
    platform_uart_disable_interrupts(driver->interface->port);

    /* SECI mode of legacy 3-wire WLAN/BT currently not supported */
    if ( (seci_mode == SECI_MODE_LEGACY_3WIRE_WLAN) || (seci_mode == SECI_MODE_LEGACY_3WIRE_BT) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Make sure ChipCommon core is enabled */
    osl_core_enable(CC_CORE_ID);

    /* 43909 Fast UART has dedicated pins, so pin-mux selection not required */

    /* Make sure SECI Fast UART is available on the chip */
    cc_cap_ext = CHIPCOMMON_CORE_CAP_EXT_REG;
    if ( !(cc_cap_ext & CC_CAP_EXT_SECI_PRESENT) && !(cc_cap_ext & CC_CAP_EXT_SECI_PLAIN_UART_PRESENT) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Configure SECI Fast UART clock */
    if ( driver->interface->src_clk == CLOCK_HT )
    {
        CHIPCOMMON_SECI_CONFIG_REG |= (CC_SECI_CONFIG_HT_CLOCK);
        clk_value = osl_ht_clock();
    }
    else if ( driver->interface->src_clk == CLOCK_ALP )
    {
        CHIPCOMMON_SECI_CONFIG_REG &= ~(CC_SECI_CONFIG_HT_CLOCK);
        clk_value = osl_alp_clock();
    }
    else
    {
        return PLATFORM_UNSUPPORTED;
    }

    WICED_SAVE_INTERRUPTS(flags);

    /* Enable SECI clock */
    CHIPCOMMON_CLK_CTL_STATUS_REG |= CC_CLK_CTL_ST_SECI_CLK_REQ;

    WICED_RESTORE_INTERRUPTS(flags);

    /* Wait for the transition to complete */
    while ( ( ( CHIPCOMMON_CLK_CTL_STATUS_REG & CC_CLK_CTL_ST_SECI_CLK_AVAIL ) == 0 ) && ( clk_timeout > 0 ) )
    {
        /* spin wait for clock transition */
        clk_timeout--;
    }

    /* Put SECI block into reset */
    CHIPCOMMON_SECI_CONFIG_REG &= ~(SECI_ENAB_SECI_ECI);
    CHIPCOMMON_SECI_CONFIG_REG |= SECI_RESET;

    /* set force-low, and set EN_SECI for all non-legacy modes */
    CHIPCOMMON_SECI_CONFIG_REG |= SECI_ENAB_SECIOUT_DIS;
    CHIPCOMMON_SECI_CONFIG_REG |= SECI_ENAB_SECI_ECI;

    /* Take SECI block out of reset */
    CHIPCOMMON_SECI_CONFIG_REG &= ~(SECI_RESET);

    /* Setup SECI UART baud rate (refer ChipCommon and GCI programming guides for explanation) */
    baud_rate_temp_value = clk_value/config->baud_rate;
    if ( baud_rate_temp_value > 256 )
    {
        uart_high_rate_mode = 0;
    }
    else
    {
        baud_rate_div_high_rate = 256 - baud_rate_temp_value;
        if ( (baud_rate_div_high_rate < UART_SECI_HIGH_RATE_THRESHOLD_LOW) || (baud_rate_div_high_rate > UART_SECI_HIGH_RATE_THRESHOLD_HIGH) )
        {
            uart_high_rate_mode = 0;
        }
        else
        {
            uart_high_rate_mode = 1;
        }
    }

    if ( uart_high_rate_mode == 1 )
    {
        /* Setup in high rate mode, disable baudrate adjustment */
        baud_rate_div_high_rate  = 256 - (clk_value/config->baud_rate);
        baud_rate_adjustment = 0x0;

        uart_fast_base->bauddiv = baud_rate_div_high_rate;
        uart_fast_base->baudadj = baud_rate_adjustment;
        uart_fast_base->mcr |= UART_SECI_MCR_HIGHRATE_EN;
        uart_fast_base->mcr &= ~(UART_SECI_MCR_BAUD_ADJ_EN);
    }
    else
    {
        /* Setup in low rate mode, enable baudrate adjustment */
        baud_rate_div_low_rate  = 256 - (clk_value/(16 * config->baud_rate));
        baud_rate_adjust_low  = ((clk_value/config->baud_rate) % 16) / 2;
        baud_rate_adjust_high = ((clk_value/config->baud_rate) % 16) - baud_rate_adjust_low;
        baud_rate_adjustment  = (baud_rate_adjust_high << 4) | baud_rate_adjust_low;

        uart_fast_base->bauddiv = baud_rate_div_low_rate;
        uart_fast_base->baudadj = baud_rate_adjustment;
        uart_fast_base->mcr &= ~(UART_SECI_MCR_HIGHRATE_EN);
        uart_fast_base->mcr |= UART_SECI_MCR_BAUD_ADJ_EN;
    }

    wiced_assert(" Currently supported data width is 8bits ", config->data_width == DATA_WIDTH_8BIT );

    /* Configure parity */
    if ( config->parity == ODD_PARITY )
    {
        uart_fast_base->lcr |= UART_SECI_LCR_PARITY_EN;
        uart_fast_base->lcr &= ~(UART_SECI_LCR_PARITY);
    }
    else if ( config->parity == EVEN_PARITY )
    {
        uart_fast_base->lcr |= UART_SECI_LCR_PARITY_EN;
        uart_fast_base->lcr |= UART_SECI_LCR_PARITY;
    }
    else
    {
        /* Default NO_PARITY */
        uart_fast_base->lcr &= ~(UART_SECI_LCR_PARITY_EN);
    }

    /* Configure stop bits */
    if ( config->stop_bits == STOP_BITS_1 )
    {
        uart_fast_base->lcr &= ~(UART_SECI_LCR_STOP_BITS);
    }
    else if ( config->stop_bits == STOP_BITS_2 )
    {
        uart_fast_base->lcr |= UART_SECI_LCR_STOP_BITS;
    }
    else
    {
        /* Default STOP_BITS_1 */
        uart_fast_base->lcr &= ~(UART_SECI_LCR_STOP_BITS);
    }

    /* Configure Flow Control */
    if ( config->flow_control == FLOW_CONTROL_DISABLED )
    {
        /* No Flow Control */
        uart_fast_base->mcr &= ~(UART_SECI_MCR_AUTO_RTS);
        uart_fast_base->mcr &= ~(UART_SECI_MCR_AUTO_TX_DIS);

        driver->hw_flow_control_is_on = WICED_FALSE;
    }
    else
    {
        /* RTS+CTS Flow Control */
        uart_fast_base->mcr |= UART_SECI_MCR_AUTO_RTS;
        uart_fast_base->mcr |= UART_SECI_MCR_AUTO_TX_DIS;

        driver->hw_flow_control_is_on = WICED_TRUE;
    }

    /* Setup LCR and MCR registers for TX, RX and HW flow control */
    uart_fast_base->lcr |= UART_SECI_LCR_RX_EN;
    uart_fast_base->lcr |= UART_SECI_LCR_TXO_EN;
    uart_fast_base->mcr |= UART_SECI_MCR_TX_EN;

    /* Setup SECI block operation mode */
    CHIPCOMMON_SECI_CONFIG_REG &= ~(SECI_MODE_MASK << SECI_MODE_SHIFT);
    CHIPCOMMON_SECI_CONFIG_REG |= (seci_mode << SECI_MODE_SHIFT);

    /* Setup default FIFO levels for TX, RX and flow control */
    cc_seci_fifo_level = CHIPCOMMON_SECI_FIFO_LEVEL_REG;
    cc_seci_fifo_level &= ~(SECI_RX_FIFO_LVL_MASK << SECI_RX_FIFO_LVL_SHIFT);
    cc_seci_fifo_level |= (SECI_RX_FIFO_LVL_DEFAULT << SECI_RX_FIFO_LVL_SHIFT);
    cc_seci_fifo_level &= ~(SECI_TX_FIFO_LVL_MASK << SECI_TX_FIFO_LVL_SHIFT);
    cc_seci_fifo_level |= (SECI_TX_FIFO_LVL_DEFAULT << SECI_TX_FIFO_LVL_SHIFT);
    cc_seci_fifo_level &= ~(SECI_RX_FIFO_LVL_FLOW_CTL_MASK << SECI_RX_FIFO_LVL_FLOW_CTL_SHIFT);
    cc_seci_fifo_level |= (SECI_RX_FIFO_LVL_FLOW_CTL_DEFAULT << SECI_RX_FIFO_LVL_FLOW_CTL_SHIFT);
    CHIPCOMMON_SECI_FIFO_LEVEL_REG = cc_seci_fifo_level;

    /* Clear force-low bit */
    CHIPCOMMON_SECI_CONFIG_REG &= ~SECI_ENAB_SECIOUT_DIS;

    /* Wait 10us for enabled SECI block serial output to stabilize */
    OSL_DELAY(10);

    /* Setup ring buffer and related parameters */
    ring_buffer_init(&uart_fast_rx_buffer, uart_fast_rx_data, UART_FAST_RX_BUFFER_SIZE);
    driver->rx_buffer = &uart_fast_rx_buffer;
    host_rtos_init_semaphore(&driver->rx_complete);
    host_rtos_init_semaphore(&driver->tx_complete);

#ifndef BCM4390X_UART_FAST_POLL_MODE
    /* Initialization complete, enable interrupts from fast UART */
    platform_uart_enable_interrupts(driver->interface->port);

    /* Make sure ChipCommon Core external IRQ to APPS core is enabled */
    platform_chipcommon_enable_irq();
#endif /* !BCM4390X_UART_FAST_POLL_MODE */

    return PLATFORM_SUCCESS;
}

platform_result_t platform_uart_init( platform_uart_driver_t* driver, const platform_uart_t* interface, const platform_uart_config_t* config, wiced_ring_buffer_t* optional_ring_buffer )
{
    wiced_assert( "bad argument", ( driver != NULL ) && ( interface != NULL ) && ( config != NULL ) );

    UNUSED_PARAMETER( optional_ring_buffer );

#ifdef CONS_LOG_BUFFER_SUPPORT
#ifdef CONS_LOG_BUFFER_ONLY
    driver->interface = (platform_uart_t*) interface;
    return cons_init();
#else
    cons_init();
#endif  /* CONS_LOG_BUFFER_ONLY */
#endif  /* CONS_LOG_BUFFER_SUPPORT */

    if ( interface->port == UART_SLOW )
    {
        driver->interface = (platform_uart_t*) interface;
        return uart_slow_init_internal( driver, config );
    }
    else if ( interface->port == UART_FAST )
    {
        driver->interface = (platform_uart_t*) interface;
        return uart_fast_init_internal( driver, config );
    }

    return PLATFORM_UNSUPPORTED;
}

platform_result_t platform_uart_deinit( platform_uart_driver_t* driver )
{
    UNUSED_PARAMETER( driver );

    return PLATFORM_UNSUPPORTED;
}

#if !defined(BCM4390X_UART_SLOW_POLL_MODE) || !defined(BCM4390X_UART_FAST_POLL_MODE)
static platform_result_t uart_receive_bytes_irq( platform_uart_driver_t* driver, uint8_t* data_in, uint32_t expected_data_size, uint32_t timeout_ms )
{
    wiced_assert( "bad argument", ( driver != NULL ) && ( data_in != NULL ) && ( expected_data_size != 0 ) );
    wiced_assert( "not inited", ( driver->rx_buffer != NULL ) );

    if ( driver->rx_buffer != NULL )
    {
        uint32_t bytes_read = 0;
        uint32_t read_index = 0;

        while ( expected_data_size > 0 )
        {
            uint32_t flags;

            /* Get the semaphore whenever a byte needs to be consumed from the ring buffer */
            if ( host_rtos_get_semaphore( &driver->rx_complete, timeout_ms, WICED_TRUE ) != WWD_SUCCESS )
            {
                /* Failure to get the semaphore */
                break;
            }

            /* Disable interrupts */
            WICED_SAVE_INTERRUPTS(flags);

            /* Read the byte from the ring buffer */
            ring_buffer_read( driver->rx_buffer, &data_in[read_index], 1, &bytes_read );
            read_index         += bytes_read;
            expected_data_size -= bytes_read;

            /* Make sure the UART interrupts are re-enabled, they could have
             * been disabled by the ISR if ring buffer was about to overflow */
            platform_uart_enable_interrupts(driver->interface->port);

            /* Enable interrupts */
            WICED_RESTORE_INTERRUPTS(flags);
        }
    }

    return ( expected_data_size == 0 ) ? PLATFORM_SUCCESS : PLATFORM_ERROR;
}
#endif /* !BCM4390X_UART_SLOW_POLL_MODE || !BCM4390X_UART_FAST_POLL_MODE */

#ifdef BCM4390X_UART_SLOW_POLL_MODE
platform_result_t uart_slow_receive_bytes_poll( platform_uart_driver_t* driver, uint8_t* data_in, uint32_t expected_data_size, uint32_t timeout_ms )
{
    wwd_time_t   total_start_time   = host_rtos_get_time( );
    wwd_time_t   total_elapsed_time = 0;
    wiced_bool_t first_read         = WICED_TRUE;

    do
    {
        wwd_time_t read_start_time   = host_rtos_get_time( );
        wwd_time_t read_elapsed_time = 0;
        wwd_time_t read_timeout_ms   = ( first_read == WICED_TRUE ) ? timeout_ms : UART_SLOW_MAX_READ_WAIT_TIME;

        /* Wait till there is something in the RX register of the UART */
        while ( ( ( uart_slow_base->lsr & UART_SLOW_LSR_RXRDY ) == 0 ) && ( read_elapsed_time < read_timeout_ms ) )
        {
            read_elapsed_time = host_rtos_get_time() - read_start_time;

            host_rtos_delay_milliseconds(1);
        }

        if ( read_elapsed_time >= read_timeout_ms )
        {
            return PLATFORM_TIMEOUT;
        }

        *data_in++ = uart_slow_base->rx_tx_dll;
        expected_data_size--;
        total_elapsed_time = host_rtos_get_time() - total_start_time;

    } while ( ( expected_data_size != 0 ) && ( total_elapsed_time < timeout_ms ) );

    if ( total_elapsed_time >= timeout_ms )
    {
        return PLATFORM_TIMEOUT;
    }

    return PLATFORM_SUCCESS;
}
#endif /* BCM4390X_UART_SLOW_POLL_MODE */

#ifdef BCM4390X_UART_FAST_POLL_MODE
platform_result_t uart_fast_receive_bytes_poll( platform_uart_driver_t* driver, uint8_t* data_in, uint32_t expected_data_size, uint32_t timeout_ms )
{
    wwd_time_t   total_start_time   = host_rtos_get_time( );
    wwd_time_t   total_elapsed_time = 0;
    wiced_bool_t first_read         = WICED_TRUE;

    do
    {
        wwd_time_t read_start_time   = host_rtos_get_time( );
        wwd_time_t read_elapsed_time = 0;
        wwd_time_t read_timeout_ms   = ( first_read == WICED_TRUE ) ? timeout_ms : UART_FAST_MAX_READ_WAIT_TIME;

        /* Wait till there is something in the RX FIFO of the Fast UART */
        while ( (CHIPCOMMON_SECI_STATUS_REG & CC_SECI_STATUS_RX_FIFO_EMPTY) && (read_elapsed_time < read_timeout_ms) )
        {
            read_elapsed_time = host_rtos_get_time() - read_start_time;

            host_rtos_delay_milliseconds(1);
        }

        if ( read_elapsed_time >= read_timeout_ms )
        {
            return PLATFORM_TIMEOUT;
        }

        *data_in++ = uart_fast_base->data;
        expected_data_size--;
        total_elapsed_time = host_rtos_get_time() - total_start_time;

    } while ( ( expected_data_size != 0 ) && ( total_elapsed_time < timeout_ms ) );

    if ( total_elapsed_time >= timeout_ms )
    {
        return PLATFORM_TIMEOUT;
    }

    return PLATFORM_SUCCESS;
}
#endif /* BCM4390X_UART_FAST_POLL_MODE */

platform_result_t uart_slow_transmit_bytes( platform_uart_driver_t* driver, const uint8_t* data_out, uint32_t size )
{
    do
    {
        wwd_time_t   start_time   = host_rtos_get_time();
        wwd_time_t   elapsed_time = 0;

        /* Wait until Transmit Register of the Slow UART is empty */
        while ( ( ( uart_slow_base->lsr & (UART_SLOW_LSR_TDHR | UART_SLOW_LSR_THRE) ) == 0 ) && ( elapsed_time < UART_SLOW_MAX_TRANSMIT_WAIT_TIME ) )
        {
            elapsed_time = host_rtos_get_time( ) - start_time;
        }

        if ( elapsed_time >= UART_SLOW_MAX_TRANSMIT_WAIT_TIME )
        {
            return PLATFORM_TIMEOUT;
        }

        uart_slow_base->rx_tx_dll = *data_out++;
        size--;

    } while ( size != 0 );

    return PLATFORM_SUCCESS;
}

platform_result_t uart_fast_transmit_bytes( platform_uart_driver_t* driver, const uint8_t* data_out, uint32_t size )
{
    do
    {
        wwd_time_t   start_time   = host_rtos_get_time();
        wwd_time_t   elapsed_time = 0;

        /* Wait till there is space in the TX FIFO of the Fast UART */
        while ( (CHIPCOMMON_SECI_STATUS_REG & CC_SECI_STATUS_TX_FIFO_FULL) && (elapsed_time < UART_FAST_MAX_TRANSMIT_WAIT_TIME) )
        {
            elapsed_time = host_rtos_get_time( ) - start_time;
        }

        if ( elapsed_time >= UART_FAST_MAX_TRANSMIT_WAIT_TIME )
        {
            return PLATFORM_TIMEOUT;
        }

        uart_fast_base->data = *data_out++;
        size--;

    } while ( size != 0 );

    return PLATFORM_SUCCESS;
}

platform_result_t platform_uart_transmit_bytes( platform_uart_driver_t* driver, const uint8_t* data_out, uint32_t size )
{
    wiced_assert( "bad argument", ( driver != NULL ) && ( data_out != NULL ) && ( size != 0 ) );

#ifdef CONS_LOG_BUFFER_SUPPORT
#ifdef CONS_LOG_BUFFER_ONLY
    return cons_transmit_bytes(driver, data_out, size);
#else
    cons_transmit_bytes(driver, data_out, size);
#endif  /* CONS_LOG_BUFFER_ONLY */
#endif  /* CONS_LOG_BUFFER_SUPPORT */

    if ( driver->interface->port == UART_SLOW )
    {
        return uart_slow_transmit_bytes( driver, data_out, size );
    }
    else if ( driver->interface->port == UART_FAST )
    {
        return uart_fast_transmit_bytes( driver, data_out, size );
    }

    return PLATFORM_UNSUPPORTED;
}

platform_result_t platform_uart_receive_bytes( platform_uart_driver_t* driver, uint8_t* data_in, uint32_t expected_data_size, uint32_t timeout_ms )
{
    wiced_assert( "bad argument", ( driver != NULL ) && ( data_in != NULL ) && ( expected_data_size != 0 ) );

    if ( driver->interface->port == UART_SLOW )
    {
#ifndef BCM4390X_UART_SLOW_POLL_MODE
        return uart_receive_bytes_irq( driver, data_in, expected_data_size, timeout_ms );
#else
        return uart_slow_receive_bytes_poll( driver, data_in, expected_data_size, timeout_ms );
#endif /* !BCM4390X_UART_SLOW_POLL_MODE */
    }
    else if ( driver->interface->port == UART_FAST )
    {
#ifndef BCM4390X_UART_FAST_POLL_MODE
        return uart_receive_bytes_irq( driver, data_in, expected_data_size, timeout_ms );
#else
        return uart_fast_receive_bytes_poll( driver, data_in, expected_data_size, timeout_ms );
#endif /* !BCM4390X_UART_FAST_POLL_MODE */
    }

    return PLATFORM_UNSUPPORTED;
}

/******************************************************
 *            IRQ Handlers Definition
 ******************************************************/

#ifndef BCM4390X_UART_SLOW_POLL_MODE
/*
 * Slow UART interrupt service routine
 */
static void uart_slow_irq( platform_uart_driver_t* driver )
{
    uint32_t int_status;
    uint32_t int_mask;

    int_mask = uart_slow_base->ier_dlm;
    int_status = uart_slow_base->iir_fcr & UART_SLOW_IIR_INT_ID_MASK;

    /* Turn off slow UART interrupts */
    platform_uart_disable_interrupts(driver->interface->port);

    if ( (int_status == UART_SLOW_IIR_INT_ID_RXRDY) && (int_mask & UART_SLOW_IER_RXRDY) )
    {
        if (driver->rx_buffer != NULL)
        {
            /*
             * Check whether the ring buffer is already about to overflow.
             * This condition cannot happen during correct operation of the driver, but checked here only as precaution
             * to protect against ring buffer overflows for e.g. if UART interrupts got inadvertently enabled elsewhere.
             */
            if ( ( ( driver->rx_buffer->tail + 1 ) % driver->rx_buffer->size ) == driver->rx_buffer->head )
            {
                /* Slow UART interrupts remain turned off */
                return;
            }

            /* Transfer data from the slow UART RX Buffer Register into the ring buffer */
            driver->rx_buffer->buffer[driver->rx_buffer->tail] = uart_slow_base->rx_tx_dll;
            driver->rx_buffer->tail = ( driver->rx_buffer->tail + 1 ) % driver->rx_buffer->size;

            /* Set the semaphore whenever a byte is produced into the ring buffer */
            host_rtos_set_semaphore( &driver->rx_complete, WICED_TRUE );

            /* Check whether the ring buffer is about to overflow */
            if ( ( ( driver->rx_buffer->tail + 1 ) % driver->rx_buffer->size ) == driver->rx_buffer->head )
            {
                /* Slow UART interrupts remain turned off */
                return;
            }
        }
        else
        {
            /* Slow UART interrupts remain turned off */
            return;
        }
    }

    /* Turn on slow UART interrupts */
    platform_uart_enable_interrupts(driver->interface->port);
}
#endif /* !BCM4390X_UART_SLOW_POLL_MODE */

#ifndef BCM4390X_UART_FAST_POLL_MODE
/*
 * Fast UART interrupt service routine
 */
static void uart_fast_irq( platform_uart_driver_t* driver )
{
    uint32_t int_status;
    uint32_t int_mask;

    int_mask = CHIPCOMMON_SECI_STATUS_MASK_REG;
    int_status = CHIPCOMMON_SECI_STATUS_REG;

    /* Turn off fast UART interrupts */
    platform_uart_disable_interrupts(driver->interface->port);

    if ( (int_status & CC_SECI_STATUS_RX_FIFO_ALMOST_FULL) && (int_mask & CC_SECI_STATUS_RX_FIFO_ALMOST_FULL) )
    {
        if (driver->rx_buffer != NULL)
        {
            /*
             * Check whether the ring buffer is already about to overflow.
             * This condition cannot happen during correct operation of the driver, but checked here only as precaution
             * to protect against ring buffer overflows for e.g. if UART interrupts got inadvertently enabled elsewhere.
             */
            if ( ( ( driver->rx_buffer->tail + 1 ) % driver->rx_buffer->size ) == driver->rx_buffer->head )
            {
                /* Fast UART interrupts remain turned off */
                return;
            }

            /* Transfer data from the SECI UART Data Register into the ring buffer */
            driver->rx_buffer->buffer[driver->rx_buffer->tail] = uart_fast_base->data;
            driver->rx_buffer->tail = ( driver->rx_buffer->tail + 1 ) % driver->rx_buffer->size;

            /* Set the semaphore whenever a byte is produced into the ring buffer */
            host_rtos_set_semaphore( &driver->rx_complete, WICED_TRUE );

            /* Check whether the ring buffer is about to overflow */
            if ( ( ( driver->rx_buffer->tail + 1 ) % driver->rx_buffer->size ) == driver->rx_buffer->head )
            {
                /* Fast UART interrupts remain turned off */
                return;
            }
        }
        else
        {
            /* Fast UART interrupts remain turned off */
            return;
        }
    }

    /* Turn on fast UART interrupts */
    platform_uart_enable_interrupts(driver->interface->port);
}
#endif /* !BCM4390X_UART_FAST_POLL_MODE */

/*
 * Platform UART interrupt service routine
 */
void platform_uart_irq( platform_uart_driver_t* driver )
{
    if (driver->interface->port == UART_SLOW)
    {
#ifndef BCM4390X_UART_SLOW_POLL_MODE
        uart_slow_irq(driver);
#endif /* !BCM4390X_UART_SLOW_POLL_MODE */
    }
    else if(driver->interface->port == UART_FAST)
    {
#ifndef BCM4390X_UART_FAST_POLL_MODE
        uart_fast_irq(driver);
#endif /* !BCM4390X_UART_FAST_POLL_MODE */
    }
}
