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
 */

#pragma once

#include <stdint.h>
#include "wwd_constants.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Macros
 ******************************************************/

#if PLATFORM_WLAN_POWERSAVE
#define M2M_POWERSAVE_COMM_BEGIN()             m2m_powersave_comm_begin( )
#define M2M_POWERSAVE_COMM_END()               m2m_powersave_comm_end( WICED_FALSE )
#define M2M_POWERSAVE_COMM_DONE()              m2m_powersave_comm_end( WICED_TRUE )
#define PLATFORM_M2M_DMA_SYNC_INIT             0
#else
#define M2M_POWERSAVE_COMM_BEGIN()
#define M2M_POWERSAVE_COMM_END()
#define M2M_POWERSAVE_COMM_DONE()
#define PLATFORM_M2M_DMA_SYNC_INIT             1
#endif

#if PLATFORM_M2M_DMA_SYNC_INIT
#define M2M_INIT_DMA_SYNC()                    m2m_init_dma( )
#define M2M_INIT_DMA_ASYNC()
#else
#define M2M_INIT_DMA_SYNC()
#define M2M_INIT_DMA_ASYNC()                   m2m_init_dma( )
#endif

/******************************************************
 *             Constants
 ******************************************************/

/******************************************************
 *             Enumerations
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/

/******************************************************
 *             Function declarations
 ******************************************************/

uint32_t m2m_read_intstatus      ( void );
void     m2m_init_dma            ( void );
void     m2m_deinit_dma          ( void );
int      m2m_is_dma_inited       ( void );
void     m2m_dma_tx_reclaim      ( void );
void*    m2m_read_dma_packet     ( uint16_t** hwtag );
void     m2m_refill_dma          ( void );
int      m2m_rxactive_dma        ( void );
void     m2m_dma_tx_data         ( void* data, uint32_t data_size );
void     m2m_powersave_comm_begin( void );
void     m2m_powersave_comm_end  ( wiced_bool_t force );
void     m2m_wlan_dma_deinit     ( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
