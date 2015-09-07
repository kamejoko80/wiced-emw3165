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
 * WICED Ethernet driver.
 */

#include "typedefs.h"
#include "osl.h"
#include "bcmdevs.h"
#include "bcmgmacrxh.h"
#include "bcmendian.h"
#include "bcmenetphy.h"
#include "etc.h"
#include "etcgmac.h"

#include "platform_appscr4.h"
#include "platform_ethernet.h"
#include "platform_map.h"

#include "wiced.h"
#include "internal/wiced_internal_api.h"

#include "wwd_rtos_isr.h"
#include "platform/wwd_platform_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef WICED_ETHERNET_LOOPBACK
#define WICED_ETHERNET_LOOPBACK                 LOOPBACK_MODE_NONE
#endif

#ifndef WICED_ETHERNET_WORKER_THREAD
#define WICED_ETHERNET_WORKER_THREAD            WICED_HARDWARE_IO_WORKER_THREAD
#endif

#ifndef WICED_ETHERNET_WATCHDOG_THREAD
#define WICED_ETHERNET_WATCHDOG_THREAD          WICED_NETWORKING_WORKER_THREAD
#endif

#define WICED_ETHERNET_CHECKRET(expr, err_ret)  do {if (!(expr)) {wiced_assert("failed\n", 0); return err_ret;}} while(0)
#define WICED_ETHERNET_CHECKADV(speed_adv, bit) ethernet_check_and_clear_adv(speed_adv, PLATFORM_ETHERNET_SPEED_ADV(bit))

#define WICED_ETHERNET_INC_ATOMIC(var, cond) \
    do \
    {  \
        uint32_t flags; \
        WICED_SAVE_INTERRUPTS(flags);    \
        if (cond) var++;                 \
        WICED_RESTORE_INTERRUPTS(flags); \
    } \
    while(0)

#define WICED_ETHERNET_DEC_ATOMIC(var, cond) \
    do \
    {  \
        uint32_t flags; \
        WICED_SAVE_INTERRUPTS(flags);    \
        if (cond) var--;                 \
        WICED_RESTORE_INTERRUPTS(flags); \
    } \
    while(0)

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct et_info
{
    etc_info_t*                 etc;
    wiced_timed_event_t         wd_event;
    wiced_mutex_t               op_mutex;
    wiced_semaphore_t           barrier_sem;
    platform_ethernet_config_t* cfg;
    uint8_t                     isr_event_cnt;
} et_info_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

static et_info_t et_instance;

extern platform_ethernet_config_t platform_ethernet_config;

/******************************************************
 *             Function definitions
 ******************************************************/

/* Functions required by underlying GMAC driver */

static void ethernet_config_adv_speed( et_info_t* et );
static void ethernet_reclaim_packets( et_info_t* et );

void et_intrson( et_info_t* et )
{
    (*et->etc->chops->intrson)( et->etc->ch );
}

/* Note: Changes etc->linkstate = TRUE before calling this function */
void et_link_up( void )
{
    WPRINT_PLATFORM_DEBUG(("ethernet: link up\n"));

    wiced_network_notify_link_up( WICED_ETHERNET_INTERFACE );

    wiced_ethernet_link_up_handler();
}

/* Note: Changes etc->linkstate = FALSE before calling this function */
void et_link_down( et_info_t* et )
{
    WPRINT_PLATFORM_DEBUG(("ethernet: link down\n"));

    wiced_ethernet_link_down_handler();

    wiced_network_notify_link_down( WICED_ETHERNET_INTERFACE );
}

void et_reset( et_info_t* et )
{
    etc_info_t* etc = et->etc;
    etc_reset( etc );
}

void et_init( et_info_t* et, uint options )
{
    etc_info_t* etc = et->etc;

    etc->loopbk = WICED_ETHERNET_LOOPBACK;

    ethernet_config_adv_speed( et );

    et_reset( et );
    etc_init( etc, options );
}

int et_up( et_info_t* et )
{
    etc_info_t* etc = et->etc;

    WPRINT_PLATFORM_DEBUG(("ethernet: up\n"));

    etc_up( etc );

    etc_watchdog( etc );

    return 0;
}

int et_down( et_info_t* et, int reset )
{
    etc_info_t* etc = et->etc;

    WPRINT_PLATFORM_DEBUG(("ethernet: down\n"));

    etc_down( etc, reset );

    ethernet_reclaim_packets( et );

    return 0;
}

int et_phy_reset( etc_info_t* etc )
{
    wiced_gpio_init( WICED_GMAC_PHY_RESET, OUTPUT_PUSH_PULL );

    /* Keep RESET low for 2 us */
    wiced_gpio_output_low( WICED_GMAC_PHY_RESET );
    OSL_DELAY( 2 );

    /* Keep RESET high for at least 2 us */
    wiced_gpio_output_high( WICED_GMAC_PHY_RESET );
    OSL_DELAY( 2 );

    return 1;
}

int et_set_addrs( etc_info_t* etc )
{
    platform_ethernet_config_t* cfg;
    wiced_mac_t* mac;

    if ( platform_ethernet_get_config( &cfg ) != PLATFORM_SUCCESS )
    {
        return 0;
    }

    mac = &cfg->mac_addr;

    etc->phyaddr = cfg->phy_addr;

    memcpy(&etc->cur_etheraddr.octet[0], &mac->octet[0], ETHER_ADDR_LEN);

    memcpy(&etc->perm_etheraddr.octet[0], &mac->octet[0], ETHER_ADDR_LEN);

#ifdef WPRINT_ENABLE_NETWORK_INFO
    WPRINT_NETWORK_INFO(("Ethernet MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
        mac->octet[0], mac->octet[1], mac->octet[2], mac->octet[3], mac->octet[4], mac->octet[5]));
#endif

    return 1;
}

/* Driver functions */

static void ethernet_config_phy_interface( et_info_t* et )
{
    uint32_t interface_val;

    switch ( et->cfg->phy_interface )
    {
        case PLATFORM_ETHERNET_PHY_MII:
            interface_val = GCI_CHIPCONTROL_GMAC_INTERFACE_MII;
            break;

        case PLATFORM_ETHERNET_PHY_RMII:
            interface_val = GCI_CHIPCONTROL_GMAC_INTERFACE_RMII;
            break;

        default:
            wiced_assert( "unknown interface" , 0 );
            interface_val = GCI_CHIPCONTROL_GMAC_INTERFACE_MII;
            break;
    }

    platform_gci_chipcontrol(GCI_CHIPCONTROL_GMAC_INTERFACE_REG,
                             GCI_CHIPCONTROL_GMAC_INTERFACE_MASK,
                             interface_val);
}

static wiced_bool_t ethernet_check_and_clear_adv( uint32_t *speed_adv, uint32_t mode )
{
    wiced_bool_t ret = ( *speed_adv & mode ) != 0;

    *speed_adv &= ~mode;

    return ret;
}

static void ethernet_config_adv_speed( et_info_t* et )
{
    uint32_t    adv = et->cfg->speed_adv;
    etc_info_t* etc = et->etc;

    if (adv & PLATFORM_ETHERNET_SPEED_ADV(AUTO) )
    {
        return;
    }

    etc->advertise = 0;
    etc->advertise2 = 0;

    if ( WICED_ETHERNET_CHECKADV( &adv, 10HALF ) )
    {
        etc->advertise |= ADV_10HALF;
    }
    if ( WICED_ETHERNET_CHECKADV( &adv, 10FULL ) )
    {
        etc->advertise |= ADV_10FULL;
    }
    if ( WICED_ETHERNET_CHECKADV( &adv, 100HALF ) )
    {
        etc->advertise |= ADV_100HALF;
    }
    if ( WICED_ETHERNET_CHECKADV( &adv, 100FULL ) )
    {
        etc->advertise |= ADV_100FULL;
    }
    if ( WICED_ETHERNET_CHECKADV( &adv, 1000HALF ) )
    {
        etc->advertise2 |= ADV_1000HALF;
    }
    if ( WICED_ETHERNET_CHECKADV( &adv, 1000FULL ) )
    {
        etc->advertise2 |= ADV_1000FULL;
    }

    wiced_assert( "not all adv mode handled", adv == 0 );
}

static void ethernet_config_force_speed( et_info_t* et )
{
    platform_ethernet_speed_mode_t speed_force = et->cfg->speed_force;
    int force_mode                             = ET_AUTO;

    if ( speed_force == PLATFORM_ETHERNET_SPEED_AUTO )
    {
        return;
    }

    switch ( speed_force )
    {
        case PLATFORM_ETHERNET_SPEED_10HALF:
            force_mode = ET_10HALF;
            break;

        case PLATFORM_ETHERNET_SPEED_10FULL:
            force_mode = ET_10FULL;
            break;

        case PLATFORM_ETHERNET_SPEED_100HALF:
            force_mode = ET_100HALF;
            break;

        case PLATFORM_ETHERNET_SPEED_100FULL:
            force_mode = ET_100FULL;
            break;

        case PLATFORM_ETHERNET_SPEED_1000HALF:
            force_mode = ET_1000HALF;
            break;

        case PLATFORM_ETHERNET_SPEED_1000FULL:
            force_mode = ET_1000FULL;
            break;

        default:
            wiced_assert( "unknown force mode", 0 );
            break;
    }

    etc_ioctl( et->etc, ETCSPEED, &force_mode );
}

static void ethernet_sendup( et_info_t* et, wiced_buffer_t buffer )
{
    bcmgmacrxh_t* rxh = (bcmgmacrxh_t *)PKTDATA( NULL, buffer ); /* packet buffer starts with rxhdr */
    etc_info_t*   etc   = et->etc;

    /* check for reported frame errors */
    if ( rxh->flags & htol16( GRXF_CRC | GRXF_OVF | GRXF_OVERSIZE ) )
    {
        etc->rxerror++;
        PKTFREE( NULL, buffer, FALSE );
        return;
    }

    /* strip off rxhdr */
    PKTPULL( NULL, buffer, HWRXOFF );

    /* update statistic */
    etc->rxframe++;
    etc->rxbyte += PKTLEN( NULL, buffer );

    /* indicate to upper layer */
    host_network_process_ethernet_data( buffer, WWD_ETHERNET_INTERFACE );
}

static void ethernet_events( et_info_t* et, uint events )
{
    etc_info_t*   etc   = et->etc;
    struct chops* chops = etc->chops;
    void*         ch    = etc->ch;

    if ( events & INTR_TX )
    {
        chops->txreclaim( ch, FALSE );
    }

    if ( events & INTR_RX )
    {
        while ( TRUE )
        {
            wiced_buffer_t buffer = chops->rx( ch );
            if ( buffer == NULL )
            {
                break;
            }

            ethernet_sendup( et, buffer );
        }

        chops->rxfill( ch );
    }

    if ( events & INTR_ERROR )
    {
        if ( chops->errors(ch) )
        {
            et_down( et, 1 );
            et_up( et );
        }
    }
}

static wiced_result_t ethernet_barrier_event( void* arg )
{
    et_info_t* et = arg;

    wiced_rtos_set_semaphore( &et->barrier_sem );

    return WICED_SUCCESS;
}

static void ethernet_issue_barrier( et_info_t* et, wiced_worker_thread_t* worker_thread)
{
    /*
     * On entry to this function event rescheduling has to be disabled.
     * Function push event to tail of work queue and wait till event (and so all preceding) is handled.
     */

    while ( wiced_rtos_send_asynchronous_event( worker_thread, ethernet_barrier_event, et ) != WICED_SUCCESS )
    {
        wiced_rtos_delay_milliseconds( 10 );
    }
    wiced_rtos_get_semaphore( &et->barrier_sem, WICED_WAIT_FOREVER );
}

static wiced_result_t ethernet_isr_event( void* arg )
{
    et_info_t*     et    = arg;
    etc_info_t*    etc   = et->etc;
    struct chops*  chops = etc->chops;
    void*          ch    = etc->ch;
    wiced_mutex_t* mutex = &et->op_mutex;
    uint events;

    WICED_ETHERNET_DEC_ATOMIC( et->isr_event_cnt, WICED_TRUE );

    wiced_rtos_lock_mutex( mutex );

    events = chops->getintrevents( ch, FALSE );

    chops->intrsoff( ch ); /* disable and ack interrupts retrieved via getintrevents */

    if ( events & INTR_NEW )
    {
        ethernet_events( et, events );
    }

    chops->intrson( ch );

    wiced_rtos_unlock_mutex( mutex );

    platform_irq_enable_irq( GMAC_ExtIRQn );

    return WICED_SUCCESS;
}

static void ethernet_schedule_isr_event( et_info_t* et )
{
    /* If scheduling failed, it will be tried again in watchdog. */
    WICED_ETHERNET_INC_ATOMIC( et->isr_event_cnt,
        (et->isr_event_cnt == 0) && (wiced_rtos_send_asynchronous_event( WICED_ETHERNET_WORKER_THREAD, ethernet_isr_event, et ) == WICED_SUCCESS ) );
}

static wiced_result_t ethernet_watchdog_event( void* arg )
{
    et_info_t* et = arg;

    wiced_rtos_lock_mutex( &et->op_mutex );

    etc_watchdog( et->etc );

    ethernet_schedule_isr_event( et );

    wiced_rtos_unlock_mutex( &et->op_mutex );

    return WICED_SUCCESS;
}

static void ethernet_reclaim_packets( et_info_t* et )
{
    etc_info_t*   etc   = et->etc;
    struct chops* chops = etc->chops;
    void*         ch    = etc->ch;

    (*chops->txreclaim)( ch, TRUE );
    (*chops->rxreclaim)( ch );
}

/* Driver interface */

wiced_bool_t platform_ethernet_is_ready_to_transceive( void )
{
    et_info_t* et = &et_instance;

    if( et->etc == NULL )
    {
        return WICED_FALSE;
    }
    else
    {
        return WICED_TRUE;
    }
}

platform_result_t platform_ethernet_init( void )
{
    et_info_t* et = &et_instance;

    wiced_assert( "Eth already inited", et->etc == NULL );

    memset( et, 0, sizeof(*et) );

    WICED_ETHERNET_CHECKRET( platform_ethernet_get_config( &et->cfg ) == PLATFORM_SUCCESS, PLATFORM_ERROR );

    WICED_ETHERNET_CHECKRET( wiced_rtos_init_mutex( &et->op_mutex ) == WICED_SUCCESS, PLATFORM_ERROR );

    WICED_ETHERNET_CHECKRET( wiced_rtos_init_semaphore( &et->barrier_sem ) == WICED_SUCCESS, PLATFORM_ERROR );

    ethernet_config_phy_interface( et );

    et->etc = etc_attach( et,
                          VENDOR_BROADCOM,
                          BCM47XX_GMAC_ID,
                          0 /* unit */,
                          NULL /* osh */,
                          NULL /* regsva */ );
    WICED_ETHERNET_CHECKRET( et->etc != NULL, PLATFORM_ERROR) ;

    ethernet_config_force_speed( et );

    et_up( et );

    platform_irq_enable_irq( GMAC_ExtIRQn );

    WICED_ETHERNET_CHECKRET( wiced_rtos_register_timed_event( &et->wd_event, WICED_ETHERNET_WATCHDOG_THREAD,
        &ethernet_watchdog_event, et->cfg->wd_period_ms, et ) == WICED_SUCCESS, PLATFORM_ERROR );

    return PLATFORM_SUCCESS;
}

void platform_ethernet_deinit( void )
{
    et_info_t* et = &et_instance;

    wiced_assert( "Eth not inited", et->etc != NULL );

    wiced_rtos_deregister_timed_event( &et->wd_event );
    ethernet_issue_barrier( et, WICED_ETHERNET_WATCHDOG_THREAD);

    platform_irq_disable_irq( GMAC_ExtIRQn );
    ethernet_issue_barrier( et, WICED_ETHERNET_WORKER_THREAD);

    et_down( et, TRUE );

    etc_detach( et->etc );
    et->etc = NULL;

    wiced_rtos_deinit_semaphore( &et->barrier_sem );

    wiced_rtos_deinit_mutex( &et->op_mutex );
}

WWD_RTOS_DEFINE_ISR( platform_ethernet_isr )
{
    et_info_t* et = &et_instance;

    platform_irq_disable_irq( GMAC_ExtIRQn ); /* interrupts will be enabled in event handler */

    ethernet_schedule_isr_event( et );
}
WWD_RTOS_MAP_ISR( platform_ethernet_isr, GMAC_ISR )

platform_result_t platform_ethernet_send_data( wiced_buffer_t buffer )
{
    et_info_t*     et    = &et_instance;
    wiced_mutex_t* mutex = &et->op_mutex;
    etc_info_t*    etc;
    struct chops*  chops;
    void*          ch;

    etc = et->etc;
    wiced_assert( "Eth not inited", etc != NULL );

    chops = etc->chops;
    wiced_assert( "Ops not inited", chops != NULL );

    ch = etc->ch;
    wiced_assert( "Ch not inited", ch != NULL );

    etc->txframe++;
    etc->txbyte += PKTLEN( NULL, buffer );

    wiced_rtos_lock_mutex( mutex );

    chops->tx( ch, buffer );

    wiced_rtos_unlock_mutex( mutex );

    return PLATFORM_SUCCESS;
}

platform_result_t platform_ethernet_get_config( platform_ethernet_config_t** config )
{
    static wiced_bool_t first_time = WICED_TRUE;

    if ( first_time == WICED_TRUE )
    {
        if ( host_platform_get_ethernet_mac_address( &platform_ethernet_config.mac_addr ) != WWD_SUCCESS )
        {
            return PLATFORM_ERROR;
        }
        first_time = WICED_FALSE;
    }

    *config = &platform_ethernet_config;

    return PLATFORM_SUCCESS;
}
