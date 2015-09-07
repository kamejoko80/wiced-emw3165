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
 * 802.1AS clock functionality for the BCM43909.
 */

#include "wiced_result.h"
#include "platform_peripheral.h"
#include "platform_ascu.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/**
 * Enable the 802.1AS time functionality.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t platform_time_enable_8021as(void)
{
    platform_ascu_enable_interrupts();

    /*
     * Enable the ascu_avb_clk.
     */

    platform_pmu_chipcontrol(PMU_CHIPCONTROL_PWM_CLK_ASCU_REG, 0, PMU_CHIPCONTROL_PWM_CLK_ASCU_MASK);

    return WICED_SUCCESS;
}


/**
 * Disable the 802.1AS time functionality.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t platform_time_disable_8021as(void)
{
    platform_ascu_disable_interrupts(ASCU_ALL_INTS);

    return WICED_SUCCESS;
}


/**
 * Read the 802.1AS time.
 *
 * Retrieve the origin timestamp in the last sync message, correct for the
 * intervening interval and return the corrected time in seconds + nanoseconds.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t platform_time_read_8021as(uint32_t *master_secs, uint32_t *master_nanosecs,
                                         uint32_t *local_secs, uint32_t *local_nanosecs)
{
    uint32_t as_secs;
    uint32_t as_nanosecs;
    uint32_t correction_secs;
    uint32_t correction_nanosecs;
    uint32_t avb_secs;
    uint32_t avb_nanosecs;
    wlc_avb_timestamp_t *p_avb;

    if (master_secs == NULL || master_nanosecs == NULL || local_secs == NULL || local_nanosecs == NULL)
        return WICED_ERROR;

    p_avb = platform_ascu_get_avb_ts();
    if (p_avb == NULL)
    {
        *master_secs     = 0;
        *master_nanosecs = 0;
        *local_secs      = 0;
        *local_nanosecs  = 0;

        return WICED_ERROR;
    }

    WICED_DISABLE_INTERRUPTS();
    (void)platform_ascu_read_ntimer(&as_secs, &as_nanosecs);
    (void)platform_ascu_read_fw_ntimer(local_secs, local_nanosecs);

    avb_secs     = p_avb->as_seconds;
    avb_nanosecs = p_avb->as_nanosecs;
    WICED_ENABLE_INTERRUPTS();

    if (as_nanosecs <= *local_nanosecs)
    {
          correction_secs     = *local_secs - as_secs;
          correction_nanosecs = *local_nanosecs - as_nanosecs;
    }
    else
    {
          correction_secs     = (*local_secs - 1) - as_secs;
          correction_nanosecs = (*local_nanosecs + ONE_BILLION) - as_nanosecs;
    }

    *master_secs     = avb_secs + correction_secs;
    *master_nanosecs = avb_nanosecs + correction_nanosecs;

    return WICED_SUCCESS;
}
