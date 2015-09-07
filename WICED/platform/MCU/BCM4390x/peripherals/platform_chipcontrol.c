/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "platform_peripheral.h"
#include "platform_map.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define GCI_INDIRECT_ADDR_REG                   (volatile uint32_t*)(PLATFORM_GCI_REGBASE(0x040))
#define GCI_CHIP_CONTROL_REG                    (volatile uint32_t*)(PLATFORM_GCI_REGBASE(0x200))
#define GCI_CHIP_STATUS_REG                     (volatile uint32_t*)(PLATFORM_GCI_REGBASE(0x204))

#define PMU_INDIRECT_ADDR_REG                   (volatile uint32_t*)(PLATFORM_PMU_REGBASE(0x650))
#define PMU_CHIP_CONTROL_REG                    (volatile uint32_t*)(PLATFORM_PMU_REGBASE(0x654))

#define PMU_REGULATOR_CONTROL_INDIRECT_ADDR_REG (volatile uint32_t*)(PLATFORM_PMU_REGBASE(0x658))
#define PMU_REGULATOR_CONTROL_REG               (volatile uint32_t*)(PLATFORM_PMU_REGBASE(0x65C))

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

static uint32_t
platform_chipcontrol(volatile uint32_t* addr_reg, volatile uint32_t* ctrl_reg,
                     uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    uint32_t ret;
    uint32_t val;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS(flags);

    *addr_reg = reg_offset;
    val = *ctrl_reg;
    ret = (val & ~clear_mask) | set_mask;
    if (val != ret)
    {
        *ctrl_reg = ret;
    }

    WICED_RESTORE_INTERRUPTS(flags);

    return ret;
}

static uint32_t
platform_chipstatus(volatile uint32_t* addr_reg, volatile uint32_t* status_reg,
                    uint8_t reg_offset)
{
    uint32_t ret;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS(flags);

    *addr_reg = reg_offset;
    ret = *status_reg;

    WICED_RESTORE_INTERRUPTS(flags);

    return ret;
}

uint32_t platform_gci_chipcontrol(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(GCI_INDIRECT_ADDR_REG, GCI_CHIP_CONTROL_REG,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_gci_chipstatus(uint8_t reg_offset)
{
    return platform_chipstatus(GCI_INDIRECT_ADDR_REG, GCI_CHIP_STATUS_REG,
                                reg_offset);
}

uint32_t platform_pmu_chipcontrol(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(PMU_INDIRECT_ADDR_REG, PMU_CHIP_CONTROL_REG,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_pmu_regulatorcontrol(uint8_t reg_offset, uint32_t clear_mask, uint32_t set_mask)
{
    return platform_chipcontrol(PMU_REGULATOR_CONTROL_INDIRECT_ADDR_REG, PMU_REGULATOR_CONTROL_REG,
                                reg_offset, clear_mask, set_mask);
}

uint32_t platform_common_chipcontrol(volatile uint32_t* reg, uint32_t clear_mask, uint32_t set_mask)
{
    uint32_t ret;
    uint32_t val;
    uint32_t flags;

    WICED_SAVE_INTERRUPTS(flags);

    val = *reg;
    ret = (val & ~clear_mask) | set_mask;
    if (val != ret)
    {
        *reg = ret;
    }

    WICED_RESTORE_INTERRUPTS(flags);

    return ret;
}
