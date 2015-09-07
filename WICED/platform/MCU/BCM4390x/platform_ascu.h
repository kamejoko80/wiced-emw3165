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
 *
 */

#include <stdint.h>
#include "typedefs.h"

#include "osl.h"
#include "hndsoc.h"

#include "wiced_platform.h"
#include "platform_peripheral.h"
#include "platform_appscr4.h"
#include "platform_toolchain.h"

#include "wwd_assert.h"
#include "wwd_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define ASCU_REGBASE            (PLATFORM_CHIPCOMMON_REGBASE(0x200))

/* The bits in the Interrupt Status and Mask Registers */
#define ASCU_ASTP_INT_MASK              (0x04)
#define ASCU_TX_START_AVB_INT_MASK      (0x02)
#define ASCU_RX_START_AVB_INT_MASK      (0x01)
#define ASCU_INTR_BIT_SHIFT_OFFSET      (0x09)
#define ASCU_ALL_INTS                   (ASCU_RX_START_AVB_INT_MASK |\
                                         ASCU_TX_START_AVB_INT_MASK |\
                                         ASCU_ASTP_INT_MASK)

/* ChipCommon ASCU Rx IntStatus and IntMask register bit */
#define ASCU_RX_CC_INT_STATUS_MASK      (1 << 9)

/* ChipCommon ASCU Tx IntStatus and IntMask register bit */
#define ASCU_TX_CC_INT_STATUS_MASK      (1 << 10)

/* ChipCommon ASCU Astp IntStatus and IntMask register bit */
#define ASCU_ASTP_CC_INT_STATUS_MASK    (1 << 11)

/* Network tick timer constants */
#define ONE_BILLION                     ((uint32_t)1000000000)
#define NET_TIMER_TICKS_PER_SEC         ((uint32_t) 160000000)
#define NET_TIMER_NANOSECS_PER_TICK     ((float)ONE_BILLION/(float)NET_TIMER_TICKS_PER_SEC)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/*
 * AVB Timestamp structure.
 * Note that this structure needs to match the definition
 * used in the driver firmware in src/wl/sys/wlc.h
 */

typedef struct wlc_avb_timestamp_s {
    volatile uint32_t lock;
    volatile uint32_t avb_timestamp;
    volatile uint32_t tsf_l;
    volatile uint32_t net_timer_rxlo;
    volatile uint32_t net_timer_rxhi;
    volatile uint32_t clock_source;
    volatile uint32_t as_seconds;
    volatile uint32_t as_nanosecs;
    volatile uint32_t as_avb_timestamp;
    volatile uint32_t as_net_timer_rx_lo;
    volatile uint32_t as_net_timer_rx_hi;
    volatile uint32_t end;
} wlc_avb_timestamp_t;

typedef struct ascu_register_s {
    volatile uint32_t ascu_control;
    volatile uint32_t ascu_gpio_control;
    volatile uint32_t ascu_bitsel_control;
    volatile uint32_t master_clk_offset_lo;
    volatile uint32_t master_clk_offset_hi;
    volatile uint32_t network_clk_offset;
    volatile uint32_t start_i2s0_ts;
    volatile uint32_t start_i2s1_ts;
    volatile uint16_t interrupt_status;
    volatile uint16_t pad0;
    volatile uint16_t interrupt_mask;
    volatile uint16_t pad1;
    volatile uint32_t audio_timer_tx_lo;
    volatile uint32_t audio_timer_tx_hi;
    volatile uint32_t audio_timer_rx_lo;
    volatile uint32_t audio_timer_rx_hi;
    volatile uint32_t audio_timer_frame_sync_lo;
    volatile uint32_t audio_timer_frame_sync_hi;
    volatile uint32_t audio_timer_fw_lo;
    volatile uint32_t audio_timer_fw_hi;
    volatile uint32_t audio_talker_timer_fw_lo;
    volatile uint32_t audio_talker_timer_fw_hi;
    volatile uint32_t network_timer_tx_lo;
    volatile uint32_t network_timer_tx_hi;
    volatile uint32_t network_timer_rx_lo;
    volatile uint32_t network_timer_rx_hi;
    volatile uint32_t network_timer_frame_sync_lo;
    volatile uint32_t network_timer_frame_sync_hi;
    volatile uint32_t network_timer_fw_lo;
    volatile uint32_t network_timer_fw_hi;
    volatile uint32_t sample_cnt0;
    volatile uint32_t sample_cnt1;
} ascu_register_t;


/******************************************************
 *               Function Declarations
 ******************************************************/

void platform_ascu_enable_interrupts(void);
void platform_ascu_disable_interrupts(uint32_t int_mask);

int platform_ascu_read_ntimer(uint32_t *secs, uint32_t *nanosecs);
int platform_ascu_read_fw_ntimer(uint32_t *secs, uint32_t *nanosecs);
wlc_avb_timestamp_t* platform_ascu_get_avb_ts(void);




