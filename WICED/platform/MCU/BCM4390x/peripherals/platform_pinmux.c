/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <stdint.h>
#include <string.h>
#include "platform_constants.h"
#include "platform_peripheral.h"
#include "platform_mcu_peripheral.h"
#include "platform_map.h"
#include "platform_appscr4.h"
#include "wwd_rtos.h"
#include "RTOS/wwd_rtos_interface.h"
#include "wwd_assert.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define GCI_REG_MASK(mask, pos)              (((uint32_t)(mask)) << (pos))

/******************************************************
 *                    Constants
 ******************************************************/

#define GPIO_TOTAL_PIN_NUMBERS          (32)
#define PIN_FUNCTION_MAX_COUNT          (12)
#define PIN_FUNCTION_UNSUPPORTED        (-1)

#define GCI_CHIP_CONTROL_REG_0          (0)
#define GCI_CHIP_CONTROL_REG_1          (1)
#define GCI_CHIP_CONTROL_REG_2          (2)
#define GCI_CHIP_CONTROL_REG_3          (3)
#define GCI_CHIP_CONTROL_REG_4          (4)
#define GCI_CHIP_CONTROL_REG_5          (5)
#define GCI_CHIP_CONTROL_REG_6          (6)
#define GCI_CHIP_CONTROL_REG_7          (7)
#define GCI_CHIP_CONTROL_REG_8          (8)
#define GCI_CHIP_CONTROL_REG_9          (9)
#define GCI_CHIP_CONTROL_REG_10         (10)
#define GCI_CHIP_CONTROL_REG_11         (11)

#define GCI_CHIP_CONTROL_REG_INVALID    (0xFF)
#define GCI_CHIP_CONTROL_MASK_INVALID   (0x0)
#define GCI_CHIP_CONTROL_POS_INVALID    (0xFF)

/* ChipCommon GPIO IntStatus and IntMask register bit */
#define GPIO_CC_INT_STATUS_MASK         (1 << 0)

/******************************************************
 *                   Enumerations
 ******************************************************/

/* 43909 pin function index values */
typedef enum
{
    PIN_FUNCTION_INDEX_0    = 0,
    PIN_FUNCTION_INDEX_1    = 1,
    PIN_FUNCTION_INDEX_2    = 2,
    PIN_FUNCTION_INDEX_3    = 3,
    PIN_FUNCTION_INDEX_4    = 4,
    PIN_FUNCTION_INDEX_5    = 5,
    PIN_FUNCTION_INDEX_6    = 6,
    PIN_FUNCTION_INDEX_7    = 7,
    PIN_FUNCTION_INDEX_8    = 8,
    PIN_FUNCTION_INDEX_9    = 9,
    PIN_FUNCTION_INDEX_10   = 10,
    PIN_FUNCTION_INDEX_11   = 11
} pin_function_index_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/* 43909 pin function selection values (based on PinMux table) */
typedef enum
{
    PIN_FUNCTION_TYPE_GPIO,
    PIN_FUNCTION_TYPE_GCI_GPIO,
    PIN_FUNCTION_TYPE_FAST_UART,
    PIN_FUNCTION_TYPE_UART_DBG,
    PIN_FUNCTION_TYPE_SECI,
    PIN_FUNCTION_TYPE_PWM,
    PIN_FUNCTION_TYPE_RF_SW_CTRL,
    PIN_FUNCTION_TYPE_SPI0,
    PIN_FUNCTION_TYPE_SPI1,
    PIN_FUNCTION_TYPE_I2C0,
    PIN_FUNCTION_TYPE_I2C1,
    PIN_FUNCTION_TYPE_I2S,
    PIN_FUNCTION_TYPE_SDIO,
    PIN_FUNCTION_TYPE_USB,
    PIN_FUNCTION_TYPE_JTAG,
    PIN_FUNCTION_TYPE_MISC,
    PIN_FUNCTION_TYPE_UNKNOWN,
    PIN_FUNCTION_TYPE_MAX /* Denotes max value. Not a valid pin function type */
} platform_pin_function_type_t;

typedef struct
{
    platform_pin_function_type_t pin_function_type;
    platform_pin_function_t      pin_function;
} platform_pin_function_info_t;

typedef struct
{
    platform_pin_t                  pin_pad_name;
    uint8_t                         gci_chip_ctrl_reg;
    uint8_t                         gci_chip_ctrl_mask;
    uint8_t                         gci_chip_ctrl_pos;
    platform_pin_function_info_t    pin_function_selection[PIN_FUNCTION_MAX_COUNT];
} platform_pin_internal_config_t;

typedef struct
{
    platform_pin_function_t gpio_pin_function;
    uint32_t                gpio_function_bit;
} chipcommon_gpio_function_t;

typedef struct
{
    uint8_t output_disable              ;
    uint8_t pullup_enable               ;
    uint8_t pulldown_enable             ;
    uint8_t schmitt_trigger_input_enable;
    uint8_t drive_strength              ;
    uint8_t input_disable               ;
} platform_pin_gpio_config_t;

/* Structure of runtime GPIO IRQ data */
typedef struct
{
    platform_gpio_irq_callback_t handler;  /* User callback function for this GPIO IRQ */
    void*                        arg;      /* User argument passed to callback function */
} platform_gpio_irq_data_t;

/******************************************************
 *               Variables Definitions
 ******************************************************/

static const pin_function_index_t function_index_hw_default  = PIN_FUNCTION_INDEX_0;
static const pin_function_index_t function_index_same_as_pin = PIN_FUNCTION_INDEX_1;

/*
 * Run-time mapping of 43909 GPIO pin and associated GPIO bit, indexed using the GPIO pin.
 * This array enables fast lookup of GPIO pin in order to achieve rapid read/write of GPIO.
 */
static int8_t gpio_bit_mapping[PIN_MAX] =
{
    [PIN_GPIO_0]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_1]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_2]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_3]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_4]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_5]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_6]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_7]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_8]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_9]       = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_10]      = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_11]      = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_12]      = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_13]      = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_14]      = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_15]      = PIN_FUNCTION_UNSUPPORTED,
    [PIN_GPIO_16]      = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SDIO_CLK]     = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SDIO_CMD]     = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SDIO_DATA_0]  = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SDIO_DATA_1]  = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SDIO_DATA_2]  = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SDIO_DATA_3]  = PIN_FUNCTION_UNSUPPORTED,
    [PIN_UART0_CTS]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_UART0_RTS]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_UART0_RXD]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_UART0_TXD]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_PWM_0]        = PIN_FUNCTION_UNSUPPORTED,
    [PIN_PWM_1]        = PIN_FUNCTION_UNSUPPORTED,
    [PIN_PWM_2]        = PIN_FUNCTION_UNSUPPORTED,
    [PIN_PWM_3]        = PIN_FUNCTION_UNSUPPORTED,
    [PIN_PWM_4]        = PIN_FUNCTION_UNSUPPORTED,
    [PIN_PWM_5]        = PIN_FUNCTION_UNSUPPORTED,
    [PIN_RF_SW_CTRL_5] = PIN_FUNCTION_UNSUPPORTED,
    [PIN_RF_SW_CTRL_6] = PIN_FUNCTION_UNSUPPORTED,
    [PIN_RF_SW_CTRL_7] = PIN_FUNCTION_UNSUPPORTED,
    [PIN_RF_SW_CTRL_8] = PIN_FUNCTION_UNSUPPORTED,
    [PIN_RF_SW_CTRL_9] = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SPI_0_MISO]   = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SPI_0_CLK]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SPI_0_MOSI]   = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SPI_0_CS]     = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2C0_SDATA]   = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2C0_CLK]     = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_MCLK0]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_SCLK0]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_LRCLK0]   = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_SDATAI0]  = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_SDATAO0]  = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_SDATAO1]  = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_SDATAI1]  = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_MCLK1]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_SCLK1]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2S_LRCLK1]   = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SPI_1_CLK]    = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SPI_1_MISO]   = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SPI_1_MOSI]   = PIN_FUNCTION_UNSUPPORTED,
    [PIN_SPI_1_CS]     = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2C1_CLK]     = PIN_FUNCTION_UNSUPPORTED,
    [PIN_I2C1_SDATA]   = PIN_FUNCTION_UNSUPPORTED
};

static chipcommon_gpio_function_t chipcommon_gpio_mapping[GPIO_TOTAL_PIN_NUMBERS] =
{
    {.gpio_pin_function = PIN_FUNCTION_GPIO_0,  .gpio_function_bit = 0},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_1,  .gpio_function_bit = 1},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_2,  .gpio_function_bit = 2},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_3,  .gpio_function_bit = 3},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_4,  .gpio_function_bit = 4},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_5,  .gpio_function_bit = 5},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_6,  .gpio_function_bit = 6},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_7,  .gpio_function_bit = 7},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_8,  .gpio_function_bit = 8},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_9,  .gpio_function_bit = 9},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_10, .gpio_function_bit = 10},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_11, .gpio_function_bit = 11},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_12, .gpio_function_bit = 12},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_13, .gpio_function_bit = 13},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_14, .gpio_function_bit = 14},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_15, .gpio_function_bit = 15},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_16, .gpio_function_bit = 16},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_17, .gpio_function_bit = 17},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_18, .gpio_function_bit = 18},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_19, .gpio_function_bit = 19},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_20, .gpio_function_bit = 20},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_21, .gpio_function_bit = 21},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_22, .gpio_function_bit = 22},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_23, .gpio_function_bit = 23},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_24, .gpio_function_bit = 24},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_25, .gpio_function_bit = 25},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_26, .gpio_function_bit = 26},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_27, .gpio_function_bit = 27},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_28, .gpio_function_bit = 28},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_29, .gpio_function_bit = 29},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_30, .gpio_function_bit = 30},
    {.gpio_pin_function = PIN_FUNCTION_GPIO_31, .gpio_function_bit = 31}
};

/*
 * Run-time IRQ mapping of GPIO bit and associated pin owning the GPIO IRQ line
 */
static platform_pin_t gpio_irq_mapping[GPIO_TOTAL_PIN_NUMBERS] =
{
    [0]  = PIN_MAX,
    [1]  = PIN_MAX,
    [2]  = PIN_MAX,
    [3]  = PIN_MAX,
    [4]  = PIN_MAX,
    [5]  = PIN_MAX,
    [6]  = PIN_MAX,
    [7]  = PIN_MAX,
    [8]  = PIN_MAX,
    [9]  = PIN_MAX,
    [10] = PIN_MAX,
    [11] = PIN_MAX,
    [12] = PIN_MAX,
    [13] = PIN_MAX,
    [14] = PIN_MAX,
    [15] = PIN_MAX,
    [16] = PIN_MAX,
    [17] = PIN_MAX,
    [18] = PIN_MAX,
    [19] = PIN_MAX,
    [20] = PIN_MAX,
    [21] = PIN_MAX,
    [22] = PIN_MAX,
    [23] = PIN_MAX,
    [24] = PIN_MAX,
    [25] = PIN_MAX,
    [26] = PIN_MAX,
    [27] = PIN_MAX,
    [28] = PIN_MAX,
    [29] = PIN_MAX,
    [30] = PIN_MAX,
    [31] = PIN_MAX
};

static platform_gpio_irq_data_t gpio_irq_data[GPIO_TOTAL_PIN_NUMBERS] = {{0}};

/*
 * Specification of the 43909 pin function multiplexing table.
 * Documentation on pin function selection mapping can be found
 * inside the 43909 GCI Chip Control Register programming page.
 *
 * Note:
 * The pin function selections need to be explicitly stated.
 * Use PIN_FUNCTION_UNKNOWN if a pin's function selection is
 * not explicitly known or defined from software perspective.
 *
 * Do NOT use PIN_FUNCTION_HW_DEFAULT or PIN_FUNCTION_SAME_AS_PIN.
 * These are NOT explicit pin function selection values.
 */
static const platform_pin_internal_config_t pin_internal_config[] =
{
    {
        .pin_pad_name           = PIN_GPIO_0,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_0,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 0,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_0},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_RX},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_SDATA},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM0},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_MISO},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM2},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_12},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_8},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM4},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_USB, PIN_FUNCTION_USB20H_CTL1}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_1,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_0,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 4,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_1},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_TX},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_CLK},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM1},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_CLK},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM3},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_13},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_9},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM5},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_2,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_0,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 8,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_2},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_GCI_GPIO, PIN_FUNCTION_GCI_GPIO_0},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_JTAG, PIN_FUNCTION_TCK},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_3,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_0,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 12,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_3},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_GCI_GPIO, PIN_FUNCTION_GCI_GPIO_1},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_JTAG, PIN_FUNCTION_TMS},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_4,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_0,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 16,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_4},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_GCI_GPIO, PIN_FUNCTION_GCI_GPIO_2},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_JTAG, PIN_FUNCTION_TDI},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_5,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_0,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 20,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_5},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_GCI_GPIO, PIN_FUNCTION_GCI_GPIO_3},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_JTAG, PIN_FUNCTION_TDO},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_6,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_0,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 24,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_6},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_GCI_GPIO, PIN_FUNCTION_GCI_GPIO_4},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_JTAG, PIN_FUNCTION_TRST_L},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_7,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_0,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 28,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_7},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_RTS_OUT},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM1},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM3},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_CS},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_CLK},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_15},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_11},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_JTAG, PIN_FUNCTION_PMU_TEST_O},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM5}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_8,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_1,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 0,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_8},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_MISO},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM2},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM4},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_RX},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_16},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_12},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_JTAG, PIN_FUNCTION_TAP_SEL_P},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_SDATA},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM0}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_9,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_1,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 4,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_9},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_CLK},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM3},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM5},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_TX},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_0},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_13},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_CLK},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM1}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_10,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_1,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 8,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_10},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_MOSI},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM4},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_SDATA},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_CTS_IN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM0},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_1},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_14},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM2},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_SEP_INT},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_SEP_INT_0D}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_11,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_1,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 12,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_11},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_CS},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM5},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_CLK},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_RTS_OUT},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM1},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_7},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_15},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM3},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_12,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_1,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 16,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_12},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_SDATA},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_RX},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_MISO},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM2},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM4},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_8},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_16},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM0},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_SEP_INT_0D},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_SEP_INT}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_13,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_1,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 20,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_13},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_CLK},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_TX},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_CLK},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM3},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM5},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_9},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_0},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM1},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_14,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_1,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 24,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_14},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM0},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_CTS_IN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_MOSI},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_SDATA},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_10},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM4},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM2}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_15,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_1,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 28,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_15},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM1},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_RTS_OUT},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_CS},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_CLK},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_11},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_7},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM5},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM3}
        }
    },
    {
        .pin_pad_name           = PIN_GPIO_16,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_2,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 24,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_16},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_FAST_UART, PIN_FUNCTION_FAST_UART_CTS_IN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM0},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM2},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_SPI1, PIN_FUNCTION_SPI1_MOSI},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_I2C1, PIN_FUNCTION_I2C1_SDATA},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_14},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_10},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_MISC, PIN_FUNCTION_RF_DISABLE_L},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_USB, PIN_FUNCTION_USB20H_CTL2},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM4}
        }
    },
    {
        .pin_pad_name           = PIN_SDIO_CLK,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_2,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 0,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_TEST_SDIO_CLK},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_CLK},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_AOS_CLK},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_SDIO_CMD,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_2,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 4,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_TEST_SDIO_CMD},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_CMD},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_AOS_CMD},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_SDIO_DATA_0,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_2,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 8,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_TEST_SDIO_DATA_0},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_D0},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_AOS_D0},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_SDIO_DATA_1,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_2,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 12,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_TEST_SDIO_DATA_1},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_D1},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_AOS_D1},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_SDIO_DATA_2,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_2,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 16,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_TEST_SDIO_DATA_2},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_D2},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_AOS_D2},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_SDIO_DATA_3,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_2,
        .gci_chip_ctrl_mask     = 0xF,
        .gci_chip_ctrl_pos      = 20,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_TEST_SDIO_DATA_3},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_D3},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_SDIO, PIN_FUNCTION_SDIO_AOS_D3},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_RF_SW_CTRL_5,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_11,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 27,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_RF_SW_CTRL, PIN_FUNCTION_RF_SW_CTRL_5},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GCI_GPIO, PIN_FUNCTION_GCI_GPIO_5},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_RF_SW_CTRL_6,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_3,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 10,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_RF_SW_CTRL, PIN_FUNCTION_RF_SW_CTRL_6},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UART_DBG, PIN_FUNCTION_UART_DBG_RX},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_SECI, PIN_FUNCTION_SECI_IN},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_RF_SW_CTRL_7,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_3,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 12,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_RF_SW_CTRL, PIN_FUNCTION_RF_SW_CTRL_7},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_UART_DBG, PIN_FUNCTION_UART_DBG_TX},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_SECI, PIN_FUNCTION_SECI_OUT},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_RF_SW_CTRL_8,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_3,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 14,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_RF_SW_CTRL, PIN_FUNCTION_RF_SW_CTRL_8},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_SECI, PIN_FUNCTION_SECI_IN},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UART_DBG, PIN_FUNCTION_UART_DBG_RX},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_RF_SW_CTRL_9,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_3,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 16,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_RF_SW_CTRL, PIN_FUNCTION_RF_SW_CTRL_9},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_SECI, PIN_FUNCTION_SECI_OUT},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_UART_DBG, PIN_FUNCTION_UART_DBG_TX},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_PWM_0,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_3,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 0,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM0},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_2},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_18},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_PWM_1,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_3,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 18,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM1},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_3},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_19},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_PWM_2,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_3,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 20,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM2},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_4},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_20},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_PWM_3,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_3,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 22,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM3},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_5},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_21},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_PWM_4,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_3,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 24,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM4},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_6},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_22},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_PWM_5,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_5,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 7,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_PWM, PIN_FUNCTION_PWM5},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_8},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_23},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_SPI_0_MISO,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_5,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 9,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SPI0, PIN_FUNCTION_SPI0_MISO},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_17},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_24},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_SPI_0_CLK,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_5,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 11,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SPI0, PIN_FUNCTION_SPI0_CLK},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_18},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_25},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_SPI_0_MOSI,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_5,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 13,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SPI0, PIN_FUNCTION_SPI0_MOSI},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_19},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_26},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_SPI_0_CS,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 0,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_SPI0, PIN_FUNCTION_SPI0_CS},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_20},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_27},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2C0_SDATA,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 2,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2C0, PIN_FUNCTION_I2C0_SDATA},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_21},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_28},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2C0_CLK,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 4,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2C0, PIN_FUNCTION_I2C0_CLK},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_22},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_29},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_MCLK0,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 6,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_MCLK0},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_23},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_0},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_SCLK0,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 8,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_SCLK0},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_24},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_2},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_LRCLK0,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 10,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_LRCLK0},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_25},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_3},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_SDATAI0,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 12,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_SDATAI0},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_26},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_4},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_SDATAO0,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 14,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_SDATAO0},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_27},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_5},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_SDATAO1,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 16,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_SDATAO1},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_28},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_6},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_SDATAI1,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 18,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_SDATAI1},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_29},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_8},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_MCLK1,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 20,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_MCLK1},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_30},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_17},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_SCLK1,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 22,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_SCLK1},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_31},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_30},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_I2S_LRCLK1,
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_9,
        .gci_chip_ctrl_mask     = 0x3,
        .gci_chip_ctrl_pos      = 24,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_I2S, PIN_FUNCTION_I2S_LRCLK1},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_0},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_GPIO, PIN_FUNCTION_GPIO_31},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_UNKNOWN, PIN_FUNCTION_UNKNOWN}
        }
    },
    {
        .pin_pad_name           = PIN_MAX,  /* Invalid PIN_MAX used as table terminator */
        .gci_chip_ctrl_reg      = GCI_CHIP_CONTROL_REG_INVALID,
        .gci_chip_ctrl_mask     = GCI_CHIP_CONTROL_MASK_INVALID,
        .gci_chip_ctrl_pos      = GCI_CHIP_CONTROL_POS_INVALID,
        .pin_function_selection =
        {
            [PIN_FUNCTION_INDEX_0]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_1]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_2]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_3]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_4]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_5]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_6]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_7]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_8]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_9]  = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_10] = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX},
            [PIN_FUNCTION_INDEX_11] = {PIN_FUNCTION_TYPE_MAX, PIN_FUNCTION_MAX}
        }
    }
};

/******************************************************
 *               Function Definitions
 ******************************************************/

static platform_result_t
platform_pin_function_get( const platform_pin_internal_config_t *pin_conf, uint32_t *pin_function_index_ptr)
{
    uint8_t  reg;
    uint8_t  pos;
    uint32_t mask;
    uint32_t reg_val;
    uint32_t val;

    if ((pin_conf == NULL) || (pin_function_index_ptr == NULL))
    {
        return PLATFORM_ERROR;
    }

    /* Initialize invalid pin function selection value */
    *pin_function_index_ptr = PIN_FUNCTION_MAX_COUNT;

    reg  = pin_conf->gci_chip_ctrl_reg;
    pos  = pin_conf->gci_chip_ctrl_pos;
    mask = GCI_REG_MASK(pin_conf->gci_chip_ctrl_mask, pos);

    /* Read from the appropriate GCI chip control register */
    reg_val = platform_gci_chipcontrol(reg, 0, 0);

    /* Get the actual function selection value for this pin */
    val = (reg_val & mask) >> pos;
    *pin_function_index_ptr = val;

    return PLATFORM_SUCCESS;
}

static platform_result_t
platform_pin_function_set(const platform_pin_internal_config_t *pin_conf, uint32_t pin_function_index)
{
    uint8_t  reg;
    uint8_t  pos;
    uint32_t mask;
    uint32_t val;

    if ((pin_conf == NULL) || (pin_function_index >= PIN_FUNCTION_MAX_COUNT))
    {
        return PLATFORM_ERROR;
    }

    reg  = pin_conf->gci_chip_ctrl_reg;
    pos  = pin_conf->gci_chip_ctrl_pos;
    mask = GCI_REG_MASK(pin_conf->gci_chip_ctrl_mask, pos);
    val  = (pin_function_index << pos) & mask;

    /* Write the pin function selection value to the appropriate GCI chip control register */
    platform_gci_chipcontrol(reg, mask, val);

    return PLATFORM_SUCCESS;
}

static const platform_pin_internal_config_t *
platform_pin_get_internal_config(platform_pin_t pin)
{
    const platform_pin_internal_config_t *pin_conf = NULL;

    if (pin >= PIN_MAX)
    {
        return NULL;
    }

    /* Lookup the desired pin internal function configuration */
    for (pin_conf = pin_internal_config ; ((pin_conf != NULL) && (pin_conf->pin_pad_name != PIN_MAX)) ; pin_conf++)
    {
        if (pin_conf->pin_pad_name == pin)
        {
            /* Found the desired pin configuration */
            return pin_conf;
        }
    }

    return NULL;
}

static platform_result_t
platform_pin_get_function_config(platform_pin_t pin, const platform_pin_internal_config_t **pin_conf_pp, uint32_t *pin_function_index_p)
{
    const platform_pin_internal_config_t *pin_conf_p = NULL;
    uint32_t pin_function_index = PIN_FUNCTION_MAX_COUNT;

    if ((pin_conf_pp == NULL) || (pin_function_index_p == NULL))
    {
        return PLATFORM_ERROR;
    }

    if ( pin >= PIN_MAX )
    {
        return PLATFORM_UNSUPPORTED;
    }

    *pin_conf_pp          = NULL;
    *pin_function_index_p = PIN_FUNCTION_MAX_COUNT;

    /* Lookup the desired pin internal function configuration */
    pin_conf_p = platform_pin_get_internal_config(pin);

    if ( (pin_conf_p == NULL) || (pin_conf_p->pin_pad_name != pin) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Read the current function index value for this pin */
    platform_pin_function_get(pin_conf_p, &pin_function_index);

    if (pin_function_index >= PIN_FUNCTION_MAX_COUNT)
    {
        return PLATFORM_UNSUPPORTED;
    }

    *pin_conf_pp          = pin_conf_p;
    *pin_function_index_p = pin_function_index;

    return PLATFORM_SUCCESS;
}

static int
platform_pin_chipcommon_gpio_function_bit( const platform_pin_internal_config_t *pin_conf, uint32_t pin_function_index)
{
    int      cc_gpio_index = 0;
    int      cc_gpio_bit   = PIN_FUNCTION_UNSUPPORTED;

    if ((pin_conf == NULL) || (pin_function_index >= PIN_FUNCTION_MAX_COUNT))
    {
        return PIN_FUNCTION_UNSUPPORTED;
    }

    /* Lookup the ChipCommon GPIO register bit for this pin */
    for (cc_gpio_index = 0 ; cc_gpio_index < GPIO_TOTAL_PIN_NUMBERS ; cc_gpio_index++)
    {
        if (chipcommon_gpio_mapping[cc_gpio_index].gpio_pin_function == pin_conf->pin_function_selection[pin_function_index].pin_function)
        {
            cc_gpio_bit = chipcommon_gpio_mapping[cc_gpio_index].gpio_function_bit;
            return cc_gpio_bit;
        }
    }

    return PIN_FUNCTION_UNSUPPORTED;
}

static void
platform_pin_set_gpio_bit_mapping( const platform_pin_internal_config_t *pin_conf, uint32_t pin_function_index)
{
    int gpio_index = 0;
    int gpio_bit   = PIN_FUNCTION_UNSUPPORTED;

    /* Lookup the GPIO bit for this GPIO pin */
    gpio_bit = platform_pin_chipcommon_gpio_function_bit(pin_conf, pin_function_index);

    /* Clear any GPIO pin mappings for this GPIO bit */
    for (gpio_index = 0 ; gpio_index < PIN_MAX ; gpio_index++)
    {
        if (gpio_bit_mapping[gpio_index] == gpio_bit)
        {
            gpio_bit_mapping[gpio_index] = PIN_FUNCTION_UNSUPPORTED;
        }
    }

    /* Set the GPIO bit mapping for this GPIO pin */
    gpio_bit_mapping[pin_conf->pin_pad_name] = gpio_bit;
}

static platform_result_t
platform_pin_config_gpio_init( const platform_pin_internal_config_t *pin_conf, uint32_t pin_function_index, platform_pin_function_config_t config)
{
    uint32_t flags;
    int cc_gpio_bit = PIN_FUNCTION_UNSUPPORTED;
    platform_pin_gpio_config_t pin_gpio_conf =
    {
        .output_disable               = 0,
        .pullup_enable                = 0,
        .pulldown_enable              = 0,
        .schmitt_trigger_input_enable = 0,
        .drive_strength               = 0,
        .input_disable                = 0
    };

    if ((pin_conf == NULL) || (pin_function_index >= PIN_FUNCTION_MAX_COUNT))
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Check if the pin function selection is GPIO */
    if (pin_conf->pin_function_selection[pin_function_index].pin_function_type != PIN_FUNCTION_TYPE_GPIO)
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Setup GPIO configuration for this pin */
    switch (config)
    {
        case PIN_FUNCTION_CONFIG_GPIO_INPUT_PULL_UP:
            pin_gpio_conf.output_disable = 1;
            pin_gpio_conf.pullup_enable  = 1;
            break;

        case PIN_FUNCTION_CONFIG_GPIO_INPUT_PULL_DOWN:
            pin_gpio_conf.output_disable  = 1;
            pin_gpio_conf.pulldown_enable = 1;
            break;

        case PIN_FUNCTION_CONFIG_GPIO_INPUT_HIGH_IMPEDANCE:
            pin_gpio_conf.output_disable = 1;
            break;

        case PIN_FUNCTION_CONFIG_GPIO_OUTPUT_PUSH_PULL:
            pin_gpio_conf.output_disable = 0;
            break;

        case PIN_FUNCTION_CONFIG_UNKNOWN:
        case PIN_FUNCTION_CONFIG_MAX:
        default:
            wiced_assert( "Not supported", 0 );
            return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the ChipCommon GPIO register bit for this GPIO pin function */
    cc_gpio_bit = platform_pin_chipcommon_gpio_function_bit(pin_conf, pin_function_index);

    if (cc_gpio_bit == PIN_FUNCTION_UNSUPPORTED)
    {
        return PLATFORM_UNSUPPORTED;
    }

    WICED_SAVE_INTERRUPTS(flags);

    /* Initialize the appropriate ChipCommon GPIO registers */
    PLATFORM_CHIPCOMMON->gpio.pull_down     = ( PLATFORM_CHIPCOMMON->gpio.pull_down     & (~( 1 << cc_gpio_bit ))) | ( (pin_gpio_conf.pulldown_enable)? (1 << cc_gpio_bit) : 0 );
    PLATFORM_CHIPCOMMON->gpio.pull_up       = ( PLATFORM_CHIPCOMMON->gpio.pull_up       & (~( 1 << cc_gpio_bit ))) | ( (pin_gpio_conf.pullup_enable)  ? (1 << cc_gpio_bit) : 0 );
    PLATFORM_CHIPCOMMON->gpio.output_enable = ( PLATFORM_CHIPCOMMON->gpio.output_enable & (~( 1 << cc_gpio_bit ))) | ( (pin_gpio_conf.output_disable) ? 0 : (1 << cc_gpio_bit) );
    PLATFORM_CHIPCOMMON->gpio.control       = ( PLATFORM_CHIPCOMMON->gpio.control       & (~( 1 << cc_gpio_bit )));

    WICED_RESTORE_INTERRUPTS(flags);

    return PLATFORM_SUCCESS;
}

static platform_result_t
platform_pin_config_gpio_deinit( const platform_pin_internal_config_t *pin_conf, uint32_t pin_function_index)
{
    uint32_t flags;
    int      cc_gpio_bit  = PIN_FUNCTION_UNSUPPORTED;
    uint32_t pin_func_idx = PIN_FUNCTION_MAX_COUNT;

    if ((pin_conf == NULL) || (pin_function_index >= PIN_FUNCTION_MAX_COUNT))
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Check if the pin function selection is GPIO */
    if (pin_conf->pin_function_selection[pin_function_index].pin_function_type != PIN_FUNCTION_TYPE_GPIO)
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Read the current function index value for this pin */
    platform_pin_function_get(pin_conf, &pin_func_idx);

    if (pin_func_idx != pin_function_index)
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the ChipCommon GPIO register bit for this GPIO pin function */
    cc_gpio_bit = platform_pin_chipcommon_gpio_function_bit(pin_conf, pin_function_index);

    if (cc_gpio_bit == PIN_FUNCTION_UNSUPPORTED)
    {
        return PLATFORM_UNSUPPORTED;
    }

    WICED_SAVE_INTERRUPTS(flags);

    /* Reset the appropriate ChipCommon GPIO registers */
    PLATFORM_CHIPCOMMON->gpio.pull_down     &= (~( 1 << cc_gpio_bit ));
    PLATFORM_CHIPCOMMON->gpio.pull_up       &= (~( 1 << cc_gpio_bit ));
    PLATFORM_CHIPCOMMON->gpio.output_enable &= (~( 1 << cc_gpio_bit ));
    PLATFORM_CHIPCOMMON->gpio.control       &= (~( 1 << cc_gpio_bit ));

    WICED_RESTORE_INTERRUPTS(flags);

    return PLATFORM_SUCCESS;
}

static platform_result_t
platform_pin_function_selection_init( const platform_pin_internal_config_t *pin_conf, uint32_t pin_function_index, platform_pin_function_config_t config)
{
    platform_result_t result;
    uint32_t pin_func_idx = PIN_FUNCTION_MAX_COUNT;
    const platform_pin_internal_config_t *pin_int_conf = NULL;

    if ((pin_conf == NULL) || (pin_function_index >= PIN_FUNCTION_MAX_COUNT))
    {
        return PLATFORM_UNSUPPORTED;
    }

    if ((pin_conf->pin_function_selection[pin_function_index].pin_function == PIN_FUNCTION_HW_DEFAULT) ||
        (pin_conf->pin_function_selection[pin_function_index].pin_function == PIN_FUNCTION_SAME_AS_PIN))
    {
        /* These pin function selection values should not be used by internal structures */
        return PLATFORM_UNSUPPORTED;
    }

    if (pin_conf->pin_function_selection[pin_function_index].pin_function != PIN_FUNCTION_UNKNOWN)
    {
        /* Check if another pin/pad has currently enabled this function */
        for (pin_int_conf = pin_internal_config ; ((pin_int_conf != NULL) && (pin_int_conf->pin_pad_name != PIN_MAX)) ; pin_int_conf++)
        {
            if (pin_int_conf->pin_pad_name != pin_conf->pin_pad_name)
            {
                /* Read the current function index value for this pin */
                pin_func_idx = PIN_FUNCTION_MAX_COUNT;
                platform_pin_function_get(pin_int_conf, &pin_func_idx);

                if (pin_func_idx >= PIN_FUNCTION_MAX_COUNT)
                {
                    continue;
                }

                if (pin_int_conf->pin_function_selection[pin_func_idx].pin_function == pin_conf->pin_function_selection[pin_function_index].pin_function)
                {
                    /* A function can be enabled on only one pin/pad at a given time */
                    return PLATFORM_UNSUPPORTED;
                }
            }
        }
    }

    if (pin_conf->pin_function_selection[pin_function_index].pin_function_type == PIN_FUNCTION_TYPE_GPIO)
    {
        /* Setup GPIO configuration for the pin */
        result = platform_pin_config_gpio_init(pin_conf, pin_function_index, config);
    }
    else
    {
        /* Other function specific configuration to be added as required */
        result = PLATFORM_SUCCESS;
    }

    if (result == PLATFORM_SUCCESS)
    {
        /* Set the new pin function selection */
        result = platform_pin_function_set(pin_conf, pin_function_index);
    }

    return result;
}

static platform_result_t
platform_pin_function_selection_deinit( const platform_pin_internal_config_t *pin_conf, uint32_t pin_function_index)
{
    platform_result_t result;
    uint32_t pin_func_idx = PIN_FUNCTION_MAX_COUNT;

    if ((pin_conf == NULL) || (pin_function_index >= PIN_FUNCTION_MAX_COUNT))
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Read the current function index value for this pin */
    platform_pin_function_get(pin_conf, &pin_func_idx);

    if (pin_func_idx != pin_function_index)
    {
        return PLATFORM_UNSUPPORTED;
    }

    if (pin_conf->pin_function_selection[pin_function_index].pin_function_type == PIN_FUNCTION_TYPE_GPIO)
    {
        /* Reset GPIO configuration for the pin */
        result = platform_pin_config_gpio_deinit(pin_conf, pin_function_index);
    }
    else
    {
        /* Reset other function specific configuration as required */
        result = PLATFORM_SUCCESS;
    }

    if (result == PLATFORM_SUCCESS)
    {
        /* Reset the pin to HW/Power-On default function selection */
        pin_func_idx = function_index_hw_default;
        result = platform_pin_function_set(pin_conf, pin_func_idx);
    }

    return result;
}

platform_result_t platform_pin_function_init(platform_pin_t pin, platform_pin_function_t function, platform_pin_function_config_t config)
{
    platform_result_t result;
    uint32_t pin_function_index = PIN_FUNCTION_MAX_COUNT;
    const platform_pin_internal_config_t *pin_conf = NULL;

    if ( (pin >= PIN_MAX) || (function >= PIN_FUNCTION_MAX) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    if (function == PIN_FUNCTION_UNKNOWN)
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the desired pin internal function configuration */
    pin_conf = platform_pin_get_internal_config(pin);

    if ( (pin_conf == NULL) || (pin_conf->pin_pad_name != pin) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Determine the pin function selection value */
    if (function == PIN_FUNCTION_HW_DEFAULT)
    {
        /* HW decided - Power ON default */
        pin_function_index = function_index_hw_default;
    }
    else if (function == PIN_FUNCTION_SAME_AS_PIN)
    {
        /* Same as pin name */
        pin_function_index = function_index_same_as_pin;
    }
    else
    {
        /* Check if this pin supports the desired function selection */
        for (pin_function_index = 0 ; (pin_function_index < PIN_FUNCTION_MAX_COUNT) ; pin_function_index++)
        {
            if (pin_conf->pin_function_selection[pin_function_index].pin_function == function)
            {
                /* Found the desired function selection */
                break;
            }
        }
    }

    if (pin_function_index >= PIN_FUNCTION_MAX_COUNT)
    {
        /* The pin function index value is not valid */
        return PLATFORM_UNSUPPORTED;
    }

    result = platform_pin_function_selection_init(pin_conf, pin_function_index, config);

    return result;
}

platform_result_t platform_pin_function_deinit(platform_pin_t pin, platform_pin_function_t function)
{
    platform_result_t result;
    uint32_t pin_function_index = PIN_FUNCTION_MAX_COUNT;
    const platform_pin_internal_config_t *pin_conf = NULL;

    if ( (pin >= PIN_MAX) || (function >= PIN_FUNCTION_MAX) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    if (function == PIN_FUNCTION_UNKNOWN)
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the pin internal configuration and current function index value */
    result = platform_pin_get_function_config(pin, &pin_conf, &pin_function_index);

    if (result != PLATFORM_SUCCESS)
    {
        return result;
    }

    if ((pin_conf == NULL) || (pin_conf->pin_pad_name != pin) || (pin_function_index >= PIN_FUNCTION_MAX_COUNT))
    {
        return PLATFORM_UNSUPPORTED;
    }

    if (function == PIN_FUNCTION_HW_DEFAULT)
    {
        /* HW decided - Power ON default */
        if (pin_function_index != function_index_hw_default)
        {
            return PLATFORM_UNSUPPORTED;
        }
    }
    else if (function == PIN_FUNCTION_SAME_AS_PIN)
    {
        /* Same as pin name */
        if (pin_function_index != function_index_same_as_pin)
        {
            return PLATFORM_UNSUPPORTED;
        }
    }
    else
    {
        if (pin_conf->pin_function_selection[pin_function_index].pin_function != function)
        {
            return PLATFORM_UNSUPPORTED;
        }
    }

    result = platform_pin_function_selection_deinit(pin_conf, pin_function_index);

    return result;
}

platform_result_t platform_gpio_init( const platform_gpio_t* gpio, platform_pin_config_t config )
{
    platform_result_t result = PLATFORM_UNSUPPORTED;
    uint32_t pin_function_index = PIN_FUNCTION_MAX_COUNT;
    const platform_pin_internal_config_t *pin_conf = NULL;
    platform_pin_function_config_t pin_func_conf = PIN_FUNCTION_CONFIG_UNKNOWN;

    if ((gpio == NULL) || (gpio->pin >= PIN_MAX))
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the desired pin internal function configuration */
    pin_conf = platform_pin_get_internal_config(gpio->pin);

    if ( (pin_conf == NULL) || (pin_conf->pin_pad_name != gpio->pin) )
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Initialize the GPIO pin function configuration */
    switch (config)
    {
        case INPUT_PULL_UP:
            pin_func_conf = PIN_FUNCTION_CONFIG_GPIO_INPUT_PULL_UP;
            break;

        case INPUT_PULL_DOWN:
            pin_func_conf = PIN_FUNCTION_CONFIG_GPIO_INPUT_PULL_DOWN;
            break;

        case INPUT_HIGH_IMPEDANCE:
            pin_func_conf = PIN_FUNCTION_CONFIG_GPIO_INPUT_HIGH_IMPEDANCE;
            break;

        case OUTPUT_PUSH_PULL:
            pin_func_conf = PIN_FUNCTION_CONFIG_GPIO_OUTPUT_PUSH_PULL;
            break;

        case OUTPUT_OPEN_DRAIN_NO_PULL:
        case OUTPUT_OPEN_DRAIN_PULL_UP:
        default:
            wiced_assert( "Not supported", 0 );
            return PLATFORM_UNSUPPORTED;
    }

    /*
     * Iterate through the function selections supported by this pin
     * and acquire a GPIO function selection that is currently available.
     */

    for (pin_function_index = 0 ; (pin_function_index < PIN_FUNCTION_MAX_COUNT) ; pin_function_index++)
    {
        if (pin_conf->pin_function_selection[pin_function_index].pin_function_type == PIN_FUNCTION_TYPE_GPIO)
        {
            /* Try to acquire this GPIO function if currently not enabled */
            result = platform_pin_function_selection_init(pin_conf, pin_function_index, pin_func_conf);

            if (result == PLATFORM_SUCCESS)
            {
                /* The pin was successfully initialized with this GPIO function */
                platform_pin_set_gpio_bit_mapping(pin_conf, pin_function_index);
                platform_gpio_irq_disable(gpio);
                return result;
            }
        }
    }

    return PLATFORM_UNSUPPORTED;
}

platform_result_t platform_gpio_deinit( const platform_gpio_t* gpio )
{
    platform_result_t result = PLATFORM_UNSUPPORTED;
    uint32_t pin_function_index = PIN_FUNCTION_MAX_COUNT;
    const platform_pin_internal_config_t *pin_conf = NULL;

    if ((gpio == NULL) || (gpio->pin >= PIN_MAX))
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the pin internal configuration and current function index value */
    result = platform_pin_get_function_config(gpio->pin, &pin_conf, &pin_function_index);

    if (result != PLATFORM_SUCCESS)
    {
        return result;
    }

    if ((pin_conf == NULL) || (pin_conf->pin_pad_name != gpio->pin) || (pin_function_index >= PIN_FUNCTION_MAX_COUNT))
    {
        return PLATFORM_UNSUPPORTED;
    }

    if (pin_conf->pin_function_selection[pin_function_index].pin_function_type != PIN_FUNCTION_TYPE_GPIO)
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Reset the pin GPIO function selection */
    result = platform_pin_function_selection_deinit(pin_conf, pin_function_index);

    if (result == PLATFORM_SUCCESS)
    {
        /* The pin GPIO function selection was successfully reset */
        platform_gpio_irq_disable(gpio);
        gpio_bit_mapping[pin_conf->pin_pad_name] = PIN_FUNCTION_UNSUPPORTED;
    }

    return result;
}

platform_result_t platform_gpio_output_low( const platform_gpio_t* gpio )
{
    uint32_t flags;
    int cc_gpio_bit = PIN_FUNCTION_UNSUPPORTED;

    if ((gpio == NULL) || (gpio->pin >= PIN_MAX))
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the ChipCommon GPIO number for this pin */
    cc_gpio_bit = gpio_bit_mapping[gpio->pin];

    if ((cc_gpio_bit >= GPIO_TOTAL_PIN_NUMBERS) || (cc_gpio_bit < 0))
    {
        return PLATFORM_UNSUPPORTED;
    }

    WICED_SAVE_INTERRUPTS(flags);

    /* Drive the GPIO pin output low */
    PLATFORM_CHIPCOMMON->gpio.output &= (~( 1 << cc_gpio_bit ));

    WICED_RESTORE_INTERRUPTS(flags);

    return PLATFORM_SUCCESS;
}

platform_result_t platform_gpio_output_high( const platform_gpio_t* gpio )
{
    uint32_t flags;
    int cc_gpio_bit = PIN_FUNCTION_UNSUPPORTED;

    if ((gpio == NULL) || (gpio->pin >= PIN_MAX))
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the ChipCommon GPIO number for this pin */
    cc_gpio_bit = gpio_bit_mapping[gpio->pin];

    if ((cc_gpio_bit >= GPIO_TOTAL_PIN_NUMBERS) || (cc_gpio_bit < 0))
    {
        return PLATFORM_UNSUPPORTED;
    }

    WICED_SAVE_INTERRUPTS(flags);

    /* Drive the GPIO pin output high */
    PLATFORM_CHIPCOMMON->gpio.output |= ( 1 << cc_gpio_bit );

    WICED_RESTORE_INTERRUPTS(flags);

    return PLATFORM_SUCCESS;
}

wiced_bool_t platform_gpio_input_get( const platform_gpio_t* gpio )
{
    uint32_t flags;
    wiced_bool_t gpio_input;
    int cc_gpio_bit = PIN_FUNCTION_UNSUPPORTED;

    if ((gpio == NULL) || (gpio->pin >= PIN_MAX))
    {
        return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the ChipCommon GPIO number for this pin */
    cc_gpio_bit = gpio_bit_mapping[gpio->pin];

    if ((cc_gpio_bit >= GPIO_TOTAL_PIN_NUMBERS) || (cc_gpio_bit < 0))
    {
        return PLATFORM_UNSUPPORTED;
    }

    WICED_SAVE_INTERRUPTS(flags);

    /* Get the GPIO pin input */
    gpio_input = ( ( PLATFORM_CHIPCOMMON->gpio.input & ( 1 << cc_gpio_bit ) ) == 0 ) ? WICED_FALSE : WICED_TRUE;

    WICED_RESTORE_INTERRUPTS(flags);

    return gpio_input;
}

platform_result_t platform_gpio_irq_enable( const platform_gpio_t* gpio, platform_gpio_irq_trigger_t trigger, platform_gpio_irq_callback_t handler, void* arg )
{
    wiced_bool_t level_trigger_enable;
    uint32_t cc_gpio_bit_mask;
    int cc_gpio_bit;
    uint32_t flags;

    if ((handler == NULL) || (gpio == NULL) || (gpio->pin >= PIN_MAX))
    {
        return PLATFORM_ERROR;
    }

    /* Identify the GPIO interrupt trigger type */
    switch(trigger)
    {
        case IRQ_TRIGGER_RISING_EDGE:
        case IRQ_TRIGGER_FALLING_EDGE:
            level_trigger_enable = WICED_FALSE;
            break;

        case IRQ_TRIGGER_LEVEL_HIGH:
        case IRQ_TRIGGER_LEVEL_LOW:
            level_trigger_enable = WICED_TRUE;
            break;

        case IRQ_TRIGGER_BOTH_EDGES:
        default:
            /* Unsupported trigger type */
            return PLATFORM_UNSUPPORTED;
    }

    /* Lookup the ChipCommon GPIO number for this pin */
    cc_gpio_bit = gpio_bit_mapping[gpio->pin];

    if ((cc_gpio_bit >= GPIO_TOTAL_PIN_NUMBERS) || (cc_gpio_bit < 0))
    {
        /* GPIO pin not initialized for GPIO function */
        return PLATFORM_ERROR;
    }

    cc_gpio_bit_mask = (1 << cc_gpio_bit);

    if ((PLATFORM_CHIPCOMMON->gpio.output_enable & cc_gpio_bit_mask) != 0)
    {
        /* GPIO pin not configured for input direction */
        return PLATFORM_ERROR;
    }

    WICED_SAVE_INTERRUPTS(flags);

    /* Clear the GPIO pin interrupt bits */
    PLATFORM_CHIPCOMMON->gpio.int_mask &= ~cc_gpio_bit_mask;
    PLATFORM_CHIPCOMMON->gpio.event_int_mask &= ~cc_gpio_bit_mask;

    /* Setup the GPIO interrupt parameters */
    if (level_trigger_enable == WICED_TRUE)
    {
        if (trigger == IRQ_TRIGGER_LEVEL_HIGH)
        {
            PLATFORM_CHIPCOMMON->gpio.int_polarity &= ~cc_gpio_bit_mask;
        }
        else if (trigger == IRQ_TRIGGER_LEVEL_LOW)
        {
            PLATFORM_CHIPCOMMON->gpio.int_polarity |= cc_gpio_bit_mask;
        }

        /* Enable the GPIO level interrupt */
        PLATFORM_CHIPCOMMON->gpio.int_mask |= cc_gpio_bit_mask;
    }
    else
    {
        if (trigger == IRQ_TRIGGER_RISING_EDGE)
        {
            PLATFORM_CHIPCOMMON->gpio.event_int_polarity &= ~cc_gpio_bit_mask;
        }
        else if (trigger == IRQ_TRIGGER_FALLING_EDGE)
        {
            PLATFORM_CHIPCOMMON->gpio.event_int_polarity |= cc_gpio_bit_mask;
        }

        /* Clear and enable the GPIO edge interrupt */
        PLATFORM_CHIPCOMMON->gpio.event |= cc_gpio_bit_mask;
        PLATFORM_CHIPCOMMON->gpio.event_int_mask |= cc_gpio_bit_mask;
    }

    gpio_irq_mapping[cc_gpio_bit] = gpio->pin;
    gpio_irq_data[cc_gpio_bit].handler = handler;
    gpio_irq_data[cc_gpio_bit].arg = arg;

    WICED_RESTORE_INTERRUPTS(flags);

    /* Make sure GPIO interrupts are enabled in ChipCommon interrupt mask */
    platform_common_chipcontrol(&(PLATFORM_CHIPCOMMON->interrupt.mask.raw), 0x0, GPIO_CC_INT_STATUS_MASK);

    /* Make sure ChipCommon Core external interrupt to APPS core is enabled */
    platform_chipcommon_enable_irq();

    return PLATFORM_SUCCESS;
}

platform_result_t platform_gpio_irq_disable( const platform_gpio_t* gpio )
{
    int cc_gpio_bit;
    uint32_t flags;

    if ((gpio == NULL) || (gpio->pin >= PIN_MAX))
    {
        return PLATFORM_ERROR;
    }

    for (cc_gpio_bit = 0 ; cc_gpio_bit < GPIO_TOTAL_PIN_NUMBERS ; cc_gpio_bit++)
    {
        if (gpio_irq_mapping[cc_gpio_bit] == gpio->pin)
        {
            WICED_SAVE_INTERRUPTS(flags);

            /* Disable the GPIO interrupt for this GPIO pin */
            PLATFORM_CHIPCOMMON->gpio.int_mask &= ~(1 << cc_gpio_bit);
            PLATFORM_CHIPCOMMON->gpio.event_int_mask &= ~(1 << cc_gpio_bit);

            gpio_irq_mapping[cc_gpio_bit] = PIN_MAX;
            gpio_irq_data[cc_gpio_bit].handler = NULL;
            gpio_irq_data[cc_gpio_bit].arg = NULL;

            WICED_RESTORE_INTERRUPTS(flags);
        }
    }

    return PLATFORM_SUCCESS;
}

void platform_gpio_irq( void )
{
    uint32_t input;
    uint32_t int_polarity;
    uint32_t int_mask;
    uint32_t event;
    uint32_t event_int_mask;
    uint32_t edge_triggered;
    uint32_t level_triggered;
    uint32_t cc_gpio_bit_mask;
    int cc_gpio_bit;

    /* Read the GPIO registers */
    input = PLATFORM_CHIPCOMMON->gpio.input;
    event = PLATFORM_CHIPCOMMON->gpio.event;
    int_mask = PLATFORM_CHIPCOMMON->gpio.int_mask;
    int_polarity = PLATFORM_CHIPCOMMON->gpio.int_polarity;
    event_int_mask = PLATFORM_CHIPCOMMON->gpio.event_int_mask;

    /* Traverse the GPIO pins and process any level or edge triggered interrupts */
    for (cc_gpio_bit = 0 ; cc_gpio_bit < GPIO_TOTAL_PIN_NUMBERS ; cc_gpio_bit++)
    {
        cc_gpio_bit_mask = (1 << cc_gpio_bit);
        edge_triggered = (event & event_int_mask) & cc_gpio_bit_mask;
        level_triggered = ((input ^ int_polarity) & int_mask) & cc_gpio_bit_mask;

        if ((edge_triggered != 0) || (level_triggered != 0))
        {
            if (edge_triggered != 0)
            {
                /* Clear the GPIO edge trigger event */
                PLATFORM_CHIPCOMMON->gpio.event |= cc_gpio_bit_mask;
            }

            if (gpio_irq_data[cc_gpio_bit].handler != NULL)
            {
                /* Invoke the GPIO interrupt handler */
                gpio_irq_data[cc_gpio_bit].handler(gpio_irq_data[cc_gpio_bit].arg);
            }
        }
    }
}
