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
#include "platform_config.h"
#include "wiced_defaults.h"

#include "typedefs.h"
#include "hndsoc.h"
#include "wiced_osl.h"

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

typedef struct
{
    uint16_t offset;
    uint16_t num;
    uint32_t regs[];
} ddr_seq_config_t;

typedef struct
{
   uint16_t offset;
   uint32_t reg;
} ddr_reg_config_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/
const static ddr_seq_config_t ctl_0_nanya_nt5cb64m16dp_cf =
{
    .offset = 0x400,
    .num    = 134,
    .regs   =
    {
        0x00000600,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000004,
        0x00000002,
        0x00000040,
        0x000000a0,
        0x00000000,
        0x02000000,
        0x00050a00,
        0x0400060b,
        0x0b100404,
        0x040f0404,
        0x02040503,
        0x0c040406,
        0x0300579c,
        0x0c040403,
        0x03002247,
        0x05040003,
        0x01000202,
        0x03070901,
        0x00000204,
        0x00240100,
        0x000e09b4,
        0x000003c7,
        0x00030003,
        0x000a000a,
        0x00000000,
        0x00000000,
        0x00270200,
        0x000f0200,
        0x00010000,
        0x05050500,
        0x00000005,
        0x00000000,
        0x00000000,
        0x10020100,
        0x00100400,
        0x01000400,
        0x00000120,
        0x00000000,
        0x00000001,
        0x00000000,
        0x00000000,
        0x00021000,
        0x00000046,
        0x00030220,
        0x00000008,
        0x00000000,
        0x00010100,
        0x00000000,
        0x00000000,
        0x00020000,
        0x00400100,
        0x01000200,
        0x02000040,
        0x00000040,
        0x01030000,
        0x01ffff0a,
        0x01010101,
        0x03010101,
        0x0c000001,
        0x00000000,
        0x00010000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x04000000,
        0x00010605,
        0x00000000,
        0x00000002,
        0x00000000,
        0x00000000,
        0x0d000000,
        0x00000028,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00404000,
        0x40400000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00004040,
        0x00004040,
        0x01030101,
        0x00000301,
        0x02020200,
        0x00640002,
        0x01010101,
        0x00006401,
        0x00000000,
        0x0c0d0000,
        0x00000000,
        0x02001368,
        0x02000200,
        0x13680200,
        0x00006108,
        0x078e0505,
        0x02000200,
        0x02000200,
        0x0000078e,
        0x000025c6,
        0x02020605,
        0x000a0301,
        0x00000019,
        0x00000000,
        0x00000000,
        0x04038000,
        0x07030a07,
        0xffff1d1c,
        0x00190010,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000204,
        0x00000000,
        0x00000001
    }
};

const static ddr_seq_config_t phy_0_nanya_nt5cb64m16dp_cf =
{
    .offset = 0x880,
    .num    = 6,
    .regs   =
    {
        0x00000000,
        0x00005006,
        0x00105005,
        0x00000000,
        0x00000000,
        0x00000000
    }
};

const static ddr_seq_config_t phy_1_nanya_nt5cb64m16dp_cf =
{
    .offset = 0x800,
    .num    = 32,
    .regs   =
    {
        0x04130413,
        0x04150415,
        0x00010080,
        0x00000044,
        0x0a12002d,
        0x40404040,
        0x04130413,
        0x04150415,
        0x00010060,
        0x00000033,
        0x0912007f,
        0x40404040,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x04130413,
        0x04150415,
        0x00010080,
        0x00000044,
        0x0a12002d,
        0x40404040,
        0x04130413,
        0x04150415,
        0x00010060,
        0x00000033,
        0x0912007f,
        0x40404040,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000
    }
};


const static ddr_seq_config_t ctl_0_hynix_bp162_cl5_bl8_320 =
{
    .offset = 0x400,
    .num    = 134,
    .regs   =
    {
        0x00000600,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000004,
        0x00000002,
        0x00000040,
        0x000000a0,
        0x00000000,
        0x02000000,
        0x00050a00,
        0x0400060b,
        0x0b100404,
        0x04090504,
        0x02040503,
        0x0c040404,
        0x0300579c,
        0x0c040403,
        0x03002247,
        0x05050003,
        0x01000202,
        0x03070a01,
        0x00000205,
        0x00340100,
        0x001409b4,
        0x000003c7,
        0x00030003,
        0x000a000a,
        0x00000000,
        0x00000000,
        0x00370200,
        0x00160200,
        0x00010000,
        0x05050500,
        0x00000005,
        0x00000000,
        0x00000000,
        0x10020100,
        0x00100400,
        0x01000400,
        0x00000120,
        0x00000000,
        0x00000001,
        0x00000000,
        0x00000000,
        0x00021000,
        0x00000046,
        0x00030220,
        0x00000008,
        0x00000000,
        0x00010100,
        0x00000000,
        0x00000000,
        0x00020000,
        0x00400100,
        0x01000200,
        0x02000040,
        0x00000040,
        0x01000000,
        0x01ffff0a,
        0x01010101,
        0x03010101,
        0x0c000001,
        0x00000000,
        0x00010000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x04000000,
        0x00010605,
        0x00000000,
        0x00000002,
        0x00000000,
        0x00000000,
        0x0d000000,
        0x00000028,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00404000,
        0x40400000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00004040,
        0x00004040,
        0x01030101,
        0x00000301,
        0x02020200,
        0x00640002,
        0x01010101,
        0x00006401,
        0x00000000,
        0x0c0d0000,
        0x00000000,
        0x02001368,
        0x02000200,
        0x13680200,
        0x00006108,
        0x078e0505,
        0x02000200,
        0x02000200,
        0x0000078e,
        0x000025c6,
        0x02020605,
        0x000a0301,
        0x00000019,
        0x00000000,
        0x00000000,
        0x04038000,
        0x07030a07,
        0xffff1d1c,
        0x00190010,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000204,
        0x00000000,
        0x00000001
    }
};

const static ddr_seq_config_t phy_0_hynix_bp162_cl5_bl8_320 =
{
    .offset = 0x880,
    .num    = 6,
    .regs   =
    {
        0x00000000,
        0x00005006,
        0x00105005,
        0x00000000,
        0x00000000,
        0x00000000
    }
};

const static ddr_seq_config_t phy_1_hynix_bp162_cl5_bl8_320 =
{
    .offset = 0x800,
    .num    = 32,
    .regs   =
    {
        0x04130413,
        0x04150415,
        0x00010080,
        0x00000044,
        0x0a12002d,
        0x40404040,
        0x04130413,
        0x04150415,
        0x00010060,
        0x00000033,
        0x0912007f,
        0x40404040,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000,
        0x04130413,
        0x04150415,
        0x00010080,
        0x00000044,
        0x0a12002d,
        0x40404040,
        0x04130413,
        0x04150415,
        0x00010060,
        0x00000033,
        0x0912007f,
        0x40404040,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000
    }
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static inline void
ddr_write_reg(uint16_t offset, uint32_t val)
{
    *(volatile uint32_t*)PLATFORM_DDR_CONTROLLER_REGBASE(offset) = val;
}

static inline uint32_t
ddr_read_reg(uint16_t offset)
{
    return *(volatile uint32_t*)PLATFORM_DDR_CONTROLLER_REGBASE(offset);
}

static inline void
ddr_mask_reg(uint16_t offset, uint32_t and_mask, uint32_t or_mask)
{
    ddr_write_reg(offset, (ddr_read_reg(offset) & and_mask) | or_mask);
}

static void
ddr_init_seq(const ddr_seq_config_t *cfg)
{
    uint16_t i;

    for (i = 0; i < cfg->num; ++i)
    {
        ddr_write_reg(cfg->offset + sizeof(cfg->regs[0]) * i, cfg->regs[i]);
    }
}

PLATFORM_DDR_FUNCDECL(nanya_nt5cb64m16dp_cf)
{
    /* Enable DDR controller */
    osl_core_enable(DMEMC_CORE_ID);

    /* Turn the pads RX on and enable termination */
    ddr_write_reg(0x48, 0x0);
    ddr_write_reg(0x4c, 0x01010101);

    /* Program DDR core */
    ddr_init_seq(&ctl_0_nanya_nt5cb64m16dp_cf);
    ddr_init_seq(&phy_0_nanya_nt5cb64m16dp_cf);
    ddr_init_seq(&phy_1_nanya_nt5cb64m16dp_cf);
    ddr_write_reg(0x45c, 0x03000000);
    ddr_mask_reg(0x504, 0xffffffff, 0x10000);
    ddr_mask_reg(0x4fc, 0xfffeffff, 0x0);
    ddr_mask_reg(0x400, 0xffffffff, 0x1);

    return PLATFORM_SUCCESS;
}

PLATFORM_DDR_FUNCDECL(hynix_bp162_cl5_bl8_320)
{
    /* Enable DDR controller */
    osl_core_enable(DMEMC_CORE_ID);

    /* Turn the pads RX on and enable termination */
    ddr_write_reg(0x48, 0x0);
    ddr_write_reg(0x4c, 0x01010101);

    /* Program DDR core */
    ddr_init_seq(&ctl_0_hynix_bp162_cl5_bl8_320);
    ddr_init_seq(&phy_0_hynix_bp162_cl5_bl8_320);
    ddr_init_seq(&phy_1_hynix_bp162_cl5_bl8_320);
    ddr_write_reg(0x45c, 0x03000000);
    ddr_mask_reg(0x504, 0xffffffff, 0x10000);
    ddr_mask_reg(0x4fc, 0xfffeffff, 0x0);
    ddr_mask_reg(0x400, 0xffffffff, 0x1);

    return PLATFORM_SUCCESS;
}
