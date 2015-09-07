/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "spi_flash.h"
#include "spi_flash_internal.h"
#include <string.h> /* for NULL */
#include "platform_appscr4.h"
#include "platform_mcu_peripheral.h"
#include "platform_config.h"
#include "m2m_hnddma.h"

#if (PLATFORM_NO_SFLASH_WRITE == 0)
#include "wiced_osl.h"
#include <hndsoc.h>
#endif

#define SFLASH_POLLING_TIMEOUT       (3000)     /* sflash status wait timeout in milisecond */
#define SFLASH_POLLING_ERASE_TIMEOUT (200000)   /* sflash chip erase timeout in milisecond */

#define SFLASH_DRIVE_STRENGTH_MASK   (0x1C000)

#define SFLASH_CTRL_OPCODE_MASK      (0xff)
#define SFLASH_CTRL_ACTIONCODE_MASK  (0xff)
#define SFLASH_CTRL_NUMBURST_MASK    (0x03)

/* QuadAddrMode(bit 24) of SFlashCtrl register only works after Chipcommon Core Revision 55 */
#define CHIP_SUPPORT_QUAD_ADDR_MODE(ccrev)   ( ccrev >= 55  )

struct sflash_capabilities
{
    unsigned long  size;
    unsigned short max_write_size;
    unsigned int   fast_read_divider:5;
    unsigned int   normal_read_divider:5;
    unsigned int   supports_fast_read:1;
    unsigned int   supports_quad_read:1;
    unsigned int   supports_quad_write:1;
    unsigned int   fast_dummy_cycles:4;
    unsigned int   write_enable_required_before_every_write:1;
};

typedef struct sflash_capabilities sflash_capabilities_t;

#define MAX_DIVIDER  ((1<<5)-1)

#define MEGABYTE  (0x100000)

#define DIRECT_WRITE_BURST_LENGTH    (64)
#define MAX_NUM_BURST                (2)
#define INDIRECT_DATA_4BYTE          (4)
#define INDIRECT_DATA_1BYTE          (1)

typedef struct
{
        uint32_t device_id;
        sflash_capabilities_t capabilities;
} sflash_capabilities_table_element_t;

const sflash_capabilities_table_element_t sflash_capabilities_table[] =
{
#ifdef SFLASH_SUPPORT_MACRONIX_PARTS
        { SFLASH_ID_MX25L8006E,  { .size = 1*MEGABYTE,  .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_MX25L1606E,  { .size = 2*MEGABYTE,  .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_MX25L6433F,  { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_MX25L25635F, { .size = 32*MEGABYTE, .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif /* SFLASH_SUPPORT_MACRONIX_PARTS */
#ifdef SFLASH_SUPPORT_SST_PARTS
        { SFLASH_ID_SST25VF080B { .size = 1*MEGABYTE,   .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif /* ifdef SFLASH_SUPPORT_SST_PARTS */
#ifdef SFLASH_SUPPORT_EON_PARTS
        { SFLASH_ID_EN25QH16,    { .size = 2*MEGABYTE,  .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif /* ifdef SFLASH_SUPPORT_EON_PARTS */
#ifdef SFLASH_SUPPORT_ISSI_PARTS
        { SFLASH_ID_ISSI25CQ032, { .size = 4*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_ISSI25LP064, { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif
        { SFLASH_ID_DEFAULT,     { .size = 0,           .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = MAX_DIVIDER, .fast_read_divider = MAX_DIVIDER, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 0, .fast_dummy_cycles = 0 } }
};

static const uint32_t address_masks[SFLASH_ACTIONCODE_MAX_ENUM] =
{
        [SFLASH_ACTIONCODE_ONLY]           = 0x00,
        [SFLASH_ACTIONCODE_1DATA]          = 0x00,
        [SFLASH_ACTIONCODE_3ADDRESS]       = 0x00ffffff,
        [SFLASH_ACTIONCODE_3ADDRESS_1DATA] = 0x00ffffff,
        [SFLASH_ACTIONCODE_3ADDRESS_4DATA] = 0x00ffffff,
};

static const uint32_t data_masks[SFLASH_ACTIONCODE_MAX_ENUM] =
{
        [SFLASH_ACTIONCODE_ONLY]           = 0x00000000,
        [SFLASH_ACTIONCODE_1DATA]          = 0x000000ff,
        [SFLASH_ACTIONCODE_3ADDRESS]       = 0x00000000,
        [SFLASH_ACTIONCODE_3ADDRESS_1DATA] = 0x000000ff,
        [SFLASH_ACTIONCODE_3ADDRESS_4DATA] = 0xffffffff,
};

static const uint8_t data_bytes[SFLASH_ACTIONCODE_MAX_ENUM] =
{
        [SFLASH_ACTIONCODE_ONLY]           = 0,
        [SFLASH_ACTIONCODE_1DATA]          = 1,
        [SFLASH_ACTIONCODE_3ADDRESS]       = 0,
        [SFLASH_ACTIONCODE_3ADDRESS_1DATA] = 1,
        [SFLASH_ACTIONCODE_3ADDRESS_4DATA] = 4,
};

static uint ccrev;

static void sflash_get_capabilities( sflash_handle_t* handle );

static int generic_sflash_command(                               const sflash_handle_t* const handle,
                                                                 sflash_command_t             cmd,
                                                                 bcm43909_sflash_actioncode_t actioncode,
                                                                 unsigned long                device_address,
                            /*@null@*/ /*@observer@*/            const void* const            data_MOSI,
                            /*@null@*/ /*@out@*/ /*@dependent@*/ void* const                  data_MISO );

/*@access sflash_handle_t@*/ /* Lint: permit access to abstract sflash handle implementation */

int sflash_read_ID( const sflash_handle_t* handle, /*@out@*/ device_id_t* data_addr )
{
    /* The indirect SFLASH interface does not allow reading of 3 bytes - read 4 bytes instead and copy */
    uint32_t temp_data;
    int retval;

    retval = generic_sflash_command( handle, SFLASH_READ_JEDEC_ID, SFLASH_ACTIONCODE_3ADDRESS_4DATA, 0, NULL, &temp_data );

    memset( data_addr, 0, 3 );
    memcpy( data_addr, &temp_data, 3 );

    return retval;
}

int sflash_write_enable( const sflash_handle_t* const handle )
{
    if ( handle->write_allowed == SFLASH_WRITE_ALLOWED )
    {
        unsigned char status_register;
        unsigned char masked_status_register;
        int status;

        /* Send write-enable command */
        status = generic_sflash_command( handle, SFLASH_WRITE_ENABLE, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
        if ( status != 0 )
        {
            return status;
        }

        /* Check status register */
        if ( 0 != ( status = sflash_read_status_register( handle, &status_register ) ) )
        {
            return status;
        }

        /* Check if Block protect bits are set */
        masked_status_register = status_register & ( SFLASH_STATUS_REGISTER_WRITE_ENABLED |
                                                     SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_0 |
                                                     SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_1 |
                                                     SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_2 |
                                                     SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_3 );

        if ( masked_status_register != SFLASH_STATUS_REGISTER_WRITE_ENABLED )
        {
            /* Disable protection for all blocks */
            status_register = status_register & (unsigned char)(~( SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_0 | SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_1 | SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_2 |  SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_3 ));
            status = sflash_write_status_register( handle, status_register );
            if ( status != 0 )
            {
                return status;
            }

            /* Re-Enable writing */
            status = generic_sflash_command( handle, SFLASH_WRITE_ENABLE, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
            if ( status != 0 )
            {
                return status;
            }

            /* Check status register */
            if ( 0 != ( status = sflash_read_status_register( handle, &status_register ) ) )
            {
                return status;
            }
            status_register++;
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int sflash_chip_erase( const sflash_handle_t* const handle )
{
    int status = sflash_write_enable( handle );
    if ( status != 0 )
    {
        return status;
    }
    return generic_sflash_command( handle, SFLASH_CHIP_ERASE1, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
}

#include "wwd_assert.h"

int sflash_sector_erase ( const sflash_handle_t* const handle, unsigned long device_address )
{
    int retval;
    int status = sflash_write_enable( handle );
    if ( status != 0 )
    {
        return status;
    }
    retval = generic_sflash_command( handle, SFLASH_SECTOR_ERASE, SFLASH_ACTIONCODE_3ADDRESS, (device_address & 0x00FFFFFF), NULL, NULL );
    wiced_assert("error", retval == 0);
    return retval;
}

int sflash_read_status_register( const sflash_handle_t* const handle, /*@out@*/  /*@dependent@*/ unsigned char* const dest_addr )
{
    return generic_sflash_command( handle, SFLASH_READ_STATUS_REGISTER, SFLASH_ACTIONCODE_1DATA, 0, NULL, dest_addr );
}



int sflash_read( const sflash_handle_t* const handle, unsigned long device_address, /*@out@*/ /*@dependent@*/ void* data_addr, unsigned int size )
{
#ifdef SLFASH_43909_INDIRECT

    unsigned int i;
    int retval;
    for ( i = 0; i < size; i++ )
    {
        retval = generic_sflash_command( handle, SFLASH_READ, SFLASH_ACTIONCODE_3ADDRESS_1DATA, ( (device_address+i) & 0x00ffffff ), NULL, &((char*)data_addr)[i] );
        if ( retval != 0 )
        {
            return retval;
        }
    }

#else /* SLFASH_43909_INDIRECT */

    void* direct_address = (void*) ( SI_SFLASH + device_address );

    bcm43909_sflash_ctrl_reg_t ctrl = { .bits = { .opcode                     = SFLASH_READ,
                                                  .action_code                = SFLASH_ACTIONCODE_3ADDRESS_4DATA,
                                                  .use_four_byte_address_mode = 0,
                                                  .use_opcode_reg             = 1,
                                                  .mode_bit_enable            = 0,
                                                  .num_dummy_cycles           = 0,
                                                  .num_burst                  = 3,
                                                  .high_speed_mode            = 0,
                                                  .start_busy                 = 0,
                                                }
                                      };

    if ( ( handle->capabilities->supports_fast_read == 1 ) ||  ( handle->capabilities->supports_quad_read == 1 ) )
    {

        if ( handle->capabilities->supports_quad_read == 1 )
        {
#ifdef CHECK_QUAD_ENABLE_EVERY_READ
            unsigned char status_register;
            int status;
            /* Check status register */
            if ( 0 != ( status = sflash_read_status_register( handle, &status_register ) ) )
            {
                return status;
            }
            wiced_assert("", (status_register & SFLASH_STATUS_REGISTER_QUAD_ENABLE) != 0 );
            (void) status_register;
#endif /* ifdef CHECK_QUAD_ENABLE_EVERY_READ */

#ifndef FCBU_SWAP_SFLASH_BITS_2_3
            /* Both address and data in 4-bit mode */
            ctrl.bits.opcode = SFLASH_X4IO_READ;
            ctrl.bits.num_dummy_cycles = 6;

            if ( ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_ISSI_CQ ) ||
                 ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_ISSI_LP ) )
            {
                ctrl.bits.mode_bit_enable = 1;
                ctrl.bits.num_dummy_cycles = 4;
            }
#else
            /* Address in 1-bit and data in 4-bit mode */
            ctrl.bits.opcode = SFLASH_QUAD_READ;
            ctrl.bits.num_dummy_cycles = handle->capabilities->fast_dummy_cycles;
#endif
        }
        else
        {
            ctrl.bits.opcode = SFLASH_FAST_READ;
        }

        /* Set SPI flash clock to higher speed if possible */
        PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->fast_read_divider;

        ctrl.bits.high_speed_mode = ( handle->capabilities->fast_read_divider == 2 ) ? 1 : 0;
    }

    PLATFORM_CHIPCOMMON->sflash.control.raw = ctrl.raw;


    wiced_assert("Check match between highspeed mode and divider",
            ( PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider == 2 ) ==
            ( PLATFORM_CHIPCOMMON->sflash.control.bits.high_speed_mode == 1 ) );

    m2m_unprotected_blocking_dma_memcpy( data_addr, direct_address, size );


    /* Set SPI flash clock back to Backplane-Clock / 6 so that ROM bootloader will work -
     * required because 43909A0 does not reset the register properly. sflash control register does get reset, so ok to leave current value
     */
    PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->normal_read_divider;

#ifdef FCBU_SWAP_SFLASH_BITS_2_3
    /* On FCBU board, bits 2 and 3 are swapped, so the data needs to be processed to swap them back */
    if ( handle->capabilities->supports_quad_read == 1 )
    {
        unsigned int i;
        unsigned char* swap_ptr = (unsigned char*)data_addr;
        unsigned int unaligned_start = MIN(size, ((uint32_t)data_addr & 0x3) ? (4 - ((uint32_t)data_addr & 0x3)) : 0);
        unsigned int aligned_size = ( (unsigned int)(size-unaligned_start) & (~0x3ul) );
        unsigned int unaligned_end = (((uint32_t)data_addr+size) & 0x3);
        for( i = 0; i < unaligned_start; i++ )
        {
            unsigned char val = *swap_ptr;
            *swap_ptr = (unsigned char) ( ( val & 0x33 ) | ( (val >> 1) & 0x44 ) | ( (val << 1) & 0x88 ) );
            swap_ptr++;
        }
        for( i = 0; i < aligned_size; i+=sizeof(uint32_t) )
        {
            uint32_t val = *((uint32_t*)swap_ptr);
            *((uint32_t*)swap_ptr) = (uint32_t) ( ( val & 0x33333333 ) | ( (val >> 1) & 0x44444444 ) | ( (val << 1) & 0x88888888 ) );
            swap_ptr+= sizeof(uint32_t);
        }
        for( i = 0; i < unaligned_end; i++ )
        {
            unsigned char val = *swap_ptr;
            *swap_ptr = (unsigned char) ( ( val & 0x33 ) | ( (val >> 1) & 0x44 ) | ( (val << 1) & 0x88 ) );
            swap_ptr++;
        }
        wiced_assert("", swap_ptr==&((unsigned char*)data_addr)[size]);
    }
#endif /* FCBU_SWAP_SFLASH_BITS_2_3 */



#endif /* ifdef SLFASH_43909_INDIRECT */

    return 0;
}


int sflash_get_size( const sflash_handle_t* const handle, /*@out@*/ unsigned long* const size )
{
    *size = handle->capabilities->size;
    return 0;
}


static void sflash_get_capabilities( sflash_handle_t* handle )
{
    const sflash_capabilities_table_element_t* capabilities_element = sflash_capabilities_table;
    while ( ( capabilities_element->device_id != SFLASH_ID_DEFAULT ) &&
            ( capabilities_element->device_id != handle->device_id ) )
    {
        capabilities_element++;
    }

    handle->capabilities = &capabilities_element->capabilities;
}

static inline int is_quad_write_allowed( uint ccrev_in, const sflash_handle_t* const handle )
{
#if (PLATFORM_NO_SFLASH_WRITE == 0)
    return ( CHIP_SUPPORT_QUAD_ADDR_MODE( ccrev_in ) &&
             ( handle->capabilities->supports_quad_write == 1 ) );
#else
    UNUSED_PARAMETER( ccrev_in );
    UNUSED_PARAMETER( handle );
    return 0;
#endif
}

int sflash_write( const sflash_handle_t* const handle, unsigned long device_address, /*@observer@*/ const void* const data_addr, unsigned int size )
{
    int status;
    unsigned int write_size, num_burst;
    unsigned int max_write_size = handle->capabilities->max_write_size;
    int enable_before_every_write = handle->capabilities->write_enable_required_before_every_write;
    unsigned char* data_addr_ptr = (unsigned char*) data_addr;
    sflash_command_t opcode = SFLASH_WRITE;

    if ( handle->write_allowed != SFLASH_WRITE_ALLOWED )
    {
        return -1;
    }

    /* Some manufacturers support programming an entire page in one command. */
    if ( enable_before_every_write == 0 )
    {
        status = sflash_write_enable( handle );
        if ( status != 0 )
        {
            return status;
        }
    }

    if ( is_quad_write_allowed( ccrev, handle ) )
    {
        if ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_MACRONIX )
        {
            opcode = SFLASH_X4IO_WRITE;
        }
        else if ( ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_ISSI_CQ ) ||
                  ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_ISSI_LP ) )
        {
            opcode = SFLASH_QUAD_WRITE;
        }
        else
        {
            wiced_assert( "No opcode applied for quad write", 0 != 1 );
        }
    }

    /* Generic x-bytes-at-a-time write */
    while ( size > 0 )
    {
        write_size = ( size >= max_write_size )? max_write_size : size;

        /* All transmitted data must not go beyond the end of the current page in a write */
        write_size = MIN( max_write_size - (device_address % max_write_size), write_size );

#ifndef SLFASH_43909_INDIRECT
        if ( (write_size >= DIRECT_WRITE_BURST_LENGTH) && ((device_address & 0x03) == 0x00) && (((unsigned long)data_addr_ptr & 0x03) == 0x00) )
        {
            num_burst = MIN( MAX_NUM_BURST, write_size / DIRECT_WRITE_BURST_LENGTH );
            write_size = DIRECT_WRITE_BURST_LENGTH * num_burst;
        }
        else
#endif
        {
            num_burst = 0;
            write_size = ( write_size >= INDIRECT_DATA_4BYTE )? INDIRECT_DATA_4BYTE : INDIRECT_DATA_1BYTE;
        }

        if ( ( enable_before_every_write == 1 ) &&
             ( 0 != ( status = sflash_write_enable( handle ) ) ) )
        {
            return status;
        }

        if ( num_burst > 0 )
        {
            /* Use Direct backplane access */
            uint32_t i, *src_data_ptr, *dst_data_ptr;
            unsigned char status_register;
            uint32_t current_time, elapsed_time;

            bcm43909_sflash_ctrl_reg_t ctrl = { .bits = { .opcode                     = opcode,
                                                          .action_code                = SFLASH_ACTIONCODE_3ADDRESS_4DATA,
                                                          .use_four_byte_address_mode = 0,
                                                          .use_opcode_reg             = 1,
                                                          .mode_bit_enable            = 0,
                                                          .num_dummy_cycles           = 0,
                                                          .num_burst                  = num_burst & SFLASH_CTRL_NUMBURST_MASK,
                                                          .high_speed_mode            = 0,
                                                          .use_quad_address_mode      = ( opcode == SFLASH_X4IO_WRITE ) ? 1 : 0,
                                                          .start_busy                 = 0,
                                                        }
                                              };

            PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->normal_read_divider;
            PLATFORM_CHIPCOMMON->sflash.control.raw = ctrl.raw;

            src_data_ptr = (uint32_t *) data_addr_ptr;
            dst_data_ptr = (uint32_t *) ( SI_SFLASH + device_address );

            for ( i = 0; i < write_size / sizeof(uint32_t); i++ )
            {
                *dst_data_ptr = *src_data_ptr++;
            }

            /* Additional write starts to issue the transaction to the SFLASH */
            *dst_data_ptr = 0xFFFFFFFF;

            /* sflash state machine is still running. Do not change any bits in this register while this bit is high */
            current_time = host_rtos_get_time();
            while ( PLATFORM_CHIPCOMMON->sflash.control.bits.backplane_write_dma_busy == 1 )
            {
                elapsed_time = host_rtos_get_time() - current_time;
                if ( elapsed_time > SFLASH_POLLING_TIMEOUT )
                {
                    /* timeout */
                    return -1;
                }
            }

            /* write commands require waiting until chip is finished writing */
            current_time = host_rtos_get_time();
            do
            {
                status = sflash_read_status_register( handle, &status_register );
                if ( status != 0 )
                {
                    return status;
                }
                elapsed_time = host_rtos_get_time() - current_time;
                if ( elapsed_time > SFLASH_POLLING_TIMEOUT )
                {
                    /* timeout */
                    return -1;
                }
            } while( ( status_register & SFLASH_STATUS_REGISTER_BUSY ) != (unsigned char) 0 );

        }
        else
        {
            /* Use indirect sflash access */

            if ( write_size == INDIRECT_DATA_1BYTE )
            {
                status = generic_sflash_command( handle, opcode, SFLASH_ACTIONCODE_3ADDRESS_1DATA, (device_address & 0x00FFFFFF), data_addr_ptr, NULL );
            }
            else if ( write_size == INDIRECT_DATA_4BYTE )
            {
                status = generic_sflash_command( handle, opcode, SFLASH_ACTIONCODE_3ADDRESS_4DATA, (device_address & 0x00FFFFFF), data_addr_ptr, NULL );
            }
        }

        if ( status != 0 )
        {
            return status;
        }

        data_addr_ptr += write_size;
        device_address += write_size;
        size -= write_size;

    }

    return 0;
}

int sflash_write_status_register( const sflash_handle_t* const handle, unsigned char value )
{
    unsigned char status_register_val = value;
#ifdef SFLASH_SUPPORT_SST_PARTS
    /* SST parts require enabling writing to the status register */
    if ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_SST )
    {
        int status;
        if ( 0 != ( status = generic_sflash_command( handle, SFLASH_ENABLE_WRITE_STATUS_REGISTER, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL ) ) )
        {
            return status;
        }
    }
#endif /* ifdef SFLASH_SUPPORT_SST_PARTS */

#if ( defined(SFLASH_SUPPORT_MACRONIX_PARTS) || defined(SFLASH_SUPPORT_ISSI_PARTS) )
    /* Macronix and ISSI parts require enabling writing to the status register */
    if ( ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_MACRONIX ) ||
         ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_ISSI_CQ ) ||
         ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_ISSI_LP ) )
    {
        int status;
        /* Send write-enable command */
        status = generic_sflash_command( handle, SFLASH_WRITE_ENABLE, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
        if ( status != 0 )
        {
            return status;
        }
    }
#endif /* ifdef SFLASH_SUPPORT_MACRONIX_PARTS */

    return generic_sflash_command( handle, SFLASH_WRITE_STATUS_REGISTER, SFLASH_ACTIONCODE_1DATA, 0, &status_register_val, NULL );
}


int init_sflash( /*@out@*/ sflash_handle_t* const handle, /*@shared@*/ void* peripheral_id, sflash_write_allowed_t write_allowed_in )
{
    int status;
    device_id_t tmp_device_id;

    (void) peripheral_id;

    handle->write_allowed = write_allowed_in;
    handle->device_id     = 0;

    /* Sflash only works with divider=2 when sflash drive strength is max */
    platform_gci_chipcontrol( 8, 0, SFLASH_DRIVE_STRENGTH_MASK);

    /* Set to default capabilities */
    handle->capabilities = &sflash_capabilities_table[sizeof(sflash_capabilities_table)/sizeof(sflash_capabilities_table_element_t) - 1].capabilities;

    status = sflash_read_ID( handle, &tmp_device_id );
    if ( status != 0 )
    {
        return status;
    }

    handle->device_id = ( ((uint32_t) tmp_device_id.id[0]) << 16 ) +
                        ( ((uint32_t) tmp_device_id.id[1]) <<  8 ) +
                        ( ((uint32_t) tmp_device_id.id[2]) <<  0 );

    sflash_get_capabilities( handle );

    PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->normal_read_divider;

    if (  handle->capabilities->supports_quad_read == 1 )
    {
        unsigned char status_register;
        /* Check status register */
        if ( 0 != ( status = sflash_read_status_register( handle, &status_register ) ) )
        {
            return status;
        }

        if ( ( status_register & SFLASH_STATUS_REGISTER_QUAD_ENABLE ) == 0 )
        {
            status_register |= SFLASH_STATUS_REGISTER_QUAD_ENABLE;

            status = sflash_write_status_register( handle, status_register );
            if ( status != 0 )
            {
                return status;
            }
        }
    }

    if ( write_allowed_in == SFLASH_WRITE_ALLOWED )
    {
#if PLATFORM_NO_SFLASH_WRITE
        return -1;
#else
        /* Get chipc core rev to decide whether 4-bit write supported or not */
        ccrev = osl_get_corerev( CC_CORE_ID );

        /* Enable writing */
        if (0 != ( status = sflash_write_enable( handle ) ) )
        {
            return status;
        }
#endif
    }

    return 0;
}

int deinit_sflash( sflash_handle_t* const handle )
{
    UNUSED_PARAMETER( handle );
    return 0;
}

static inline int is_erase_command( sflash_command_t cmd )
{
    return ( ( cmd == SFLASH_CHIP_ERASE1           ) ||
             ( cmd == SFLASH_CHIP_ERASE2           ) ||
             ( cmd == SFLASH_SECTOR_ERASE          ) ||
             ( cmd == SFLASH_BLOCK_ERASE_MID       ) ||
             ( cmd == SFLASH_BLOCK_ERASE_LARGE     ) )? 1 : 0;
}

static inline int is_write_command( sflash_command_t cmd )
{
    return ( ( cmd == SFLASH_X4IO_WRITE            ) ||
             ( cmd == SFLASH_QUAD_WRITE            ) ||
             ( cmd == SFLASH_WRITE                 ) ||
             ( cmd == SFLASH_WRITE_STATUS_REGISTER ) ||
             ( is_erase_command( cmd )             ) )? 1 : 0;
}

static int generic_sflash_command(                               const sflash_handle_t* const handle,
                                                                 sflash_command_t             cmd,
                                                                 bcm43909_sflash_actioncode_t actioncode,
                                                                 unsigned long                device_address,
                            /*@null@*/ /*@observer@*/            const void* const            data_MOSI,
                            /*@null@*/ /*@out@*/ /*@dependent@*/ void* const                  data_MISO )
{

    uint32_t current_time, elapsed_time;
    bcm43909_sflash_ctrl_reg_t ctrl = { .bits = { .opcode                     = cmd,
                                                  .action_code                = actioncode,
                                                  .use_four_byte_address_mode = 0,
                                                  .use_opcode_reg             = 0,
                                                  .mode_bit_enable            = 0,
                                                  .num_dummy_cycles           = 0,
                                                  .num_burst                  = 3,
                                                  .high_speed_mode            = 0,
                                                  .use_quad_address_mode      = 0,
                                                  .start_busy                 = 1,
                                                }
                                       };

    PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->normal_read_divider;

    PLATFORM_CHIPCOMMON->sflash.address = device_address & address_masks[actioncode];
    if ( data_MOSI != NULL )
    {
        uint32_t data = *((uint32_t*)data_MOSI) & data_masks[actioncode];
        if ( actioncode == SFLASH_ACTIONCODE_3ADDRESS_4DATA )
        {
            data = ( ( ( data << 24 ) & ( 0xFF000000 ) ) |
                     ( ( data <<  8 ) & ( 0x00FF0000 ) ) |
                     ( ( data >>  8 ) & ( 0x0000FF00 ) ) |
                     ( ( data >> 24 ) & ( 0x000000FF ) ) );
        }
        PLATFORM_CHIPCOMMON->sflash.data = data ;
    }

    if ( cmd == SFLASH_X4IO_WRITE )
    {
        ctrl.bits.use_quad_address_mode = 1;
    }

    PLATFORM_CHIPCOMMON->sflash.control.raw = ctrl.raw;

    current_time = host_rtos_get_time();
    while ( PLATFORM_CHIPCOMMON->sflash.control.bits.start_busy == 1 )
    {
        elapsed_time = host_rtos_get_time() - current_time;
        if ( elapsed_time > SFLASH_POLLING_TIMEOUT )
        {
            /* timeout */
            return -1;
        }
    }

    if ( data_MISO != NULL )
    {
        uint32_t tmp = PLATFORM_CHIPCOMMON->sflash.data;
        memcpy( data_MISO, &tmp, data_bytes[actioncode] );
    }



    if ( is_write_command( cmd ) == 1 )
    {
        unsigned char status_register;
        uint32_t timeout_thresh;

        timeout_thresh = is_erase_command( cmd ) ? SFLASH_POLLING_ERASE_TIMEOUT : SFLASH_POLLING_TIMEOUT;
        current_time = host_rtos_get_time();

        /* write commands require waiting until chip is finished writing */

        do
        {
            int status = sflash_read_status_register( handle, &status_register );
            if ( status != 0 )
            {
                /*@-mustdefine@*/ /* Lint: do not need to define data_MISO due to failure */
                return status;
                /*@+mustdefine@*/
            }
            elapsed_time = host_rtos_get_time() - current_time;

            if ( elapsed_time > timeout_thresh )
            {
                /* timeout */
                return -1;
            }
        } while( ( status_register & SFLASH_STATUS_REGISTER_BUSY ) != (unsigned char) 0 );
    }


    /*@-mustdefine@*/ /* Lint: lint does not realise data_MISO was set */
    return 0;
    /*@+mustdefine@*/
}

/*@noaccess sflash_handle_t@*/
