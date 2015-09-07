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
 *  Cryptographic functions
 *
 *  Provides cryptographic functions for use in applications
 */

#include <string.h> /* For memcpy */
#include "wwd_wlioctl.h"
#include "wwd_crypto.h"
#include "network/wwd_buffer_interface.h"
#include "RTOS/wwd_rtos_interface.h"
#include "internal/wwd_sdpcm.h"
#include "internal/wwd_internal.h"


/**
 * Gets a 16 bit random number from the 802.11 device
 *
 * Allows user applications to retrieve 16 bit random numbers
 * which are seeded by the Broadcom 802.11 chip using a
 * pseudo-random number generator.
 *
 * @param val : pointer to a variable which will receive the
 *              generated random number
 *
 * @return WWD_SUCCESS or error code
 */

wwd_result_t wwd_wifi_get_random( void* data_buffer, uint16_t buffer_length )
{
    wiced_buffer_t buffer;
    uint8_t* tmp_buffer_ptr;
    wiced_buffer_t response;
    wwd_result_t ret = WWD_PENDING;
    static uint16_t pseudo_random = 0;
    static uint16_t random_seed   = 0;

    /* Seed the pseudo random number generator. */
    if ( ( pseudo_random == 0 ) || ( random_seed == 0 ) )
    {
        if ( random_seed == 0 )
        {
            if ( wwd_sdpcm_get_ioctl_buffer( &buffer, (uint16_t) 2 ) != NULL ) /* Do not need to put anything in buffer hence void cast */
            {
                ret = wwd_sdpcm_send_ioctl( SDPCM_GET, WLC_GET_RANDOM_BYTES, buffer, &response, WWD_STA_INTERFACE );
            }
            if ( ret == WWD_SUCCESS )
            {
                uint8_t* data = (uint8_t*) host_buffer_get_current_piece_data_pointer( response );
                memcpy( &random_seed, data, (size_t) 2 );
                host_buffer_release( response, WWD_NETWORK_RX );
            }
        }
        if ( pseudo_random == 0 )
        {
            pseudo_random = (uint16_t)host_rtos_get_time();
        }
        if ( random_seed != 0 )
        {
            pseudo_random = (uint16_t)(pseudo_random * random_seed);
        }
    }

    tmp_buffer_ptr = (uint8_t*) data_buffer;
    while( buffer_length != 0 )
    {
        pseudo_random = (uint16_t)((pseudo_random * 32719 + 3) % 32749);

        *tmp_buffer_ptr = ((uint8_t*)&pseudo_random)[0];
        buffer_length--;
        tmp_buffer_ptr++;

        if ( buffer_length > 0 )
        {
            *tmp_buffer_ptr = ((uint8_t*)&pseudo_random)[1];
            buffer_length--;
            tmp_buffer_ptr++;
        }
    }

    /*@+branchstate@*/
    return WWD_SUCCESS;
}
