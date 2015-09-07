/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <string.h>
#include <stdlib.h>
#include "spi_flash.h"
#include "platform_config.h"
#include "platform_peripheral.h"
#include "wwd_assert.h"
#include "wiced_framework.h"
#include "wiced_apps_common.h"
#include "waf_platform.h"
#include "wwd_constants.h"
#include "platform_cmsis.h"

#define LPC183x_PART
/******************************************************
 *                    Constants
 ******************************************************/
#define LPC18XX_ENABLE_FLASH_WRITING_FAILS_BREAKS
#define APP_CODE_A_START_ADDR                   ( (uint32_t) &app_code_a_start_addr_loc )
#define APP_CODE_A_MAX_SIZE                     ( (uint32_t) &app_code_a_size_loc )
#define APP_CODE_B_START_ADDR                   ( (uint32_t) &app_code_b_start_addr_loc )
#define APP_CODE_B_MAX_SIZE                     ( (uint32_t) &app_code_b_size_loc )

#define APP_CODE_A_END_ADDR                     ( APP_CODE_A_START_ADDR + APP_CODE_A_MAX_SIZE )
#define APP_CODE_B_END_ADDR                     ( APP_CODE_B_START_ADDR + APP_CODE_B_MAX_SIZE )

#define LPC18XX_PAGE_SIZE                     ( 512 )
#define LPC18XX_START_BANK_SECTOR             ( 0 )
#define LPC18XX_END_BANK_SECTOR               ( 14 )

#define LPC18XX_BANKA_START_ADDRESS           ( 0x1A000000 )
#define LPC18XX_BANKB_START_ADDRESS           ( 0x1B000000 )
#define LPC18XX_GET_SECTOR_FROM_ADDRESS

#define LPC18XX_LOCAL_SRAM1_START_ADDRESS     ( 0x10000000 )
#define LPC18XX_LOCAL_SRAM2_START_ADDRESS     ( 0x10080000 )
#define LPC18XX_AHB_SRAM1_START_ADDRESS       ( 0x20000000 )
#define LPC18XX_AHB_SRAM2_START_ADDRESS       ( 0x2000C000 )

#ifdef LPC183x_PART
#define LPC18XX_LOCAL_SRAM1_SIZE              ( 32 * 1024 )
#define LPC18XX_LOCAL_SRAM1_END_ADDRESS       ( LPC18XX_LOCAL_SRAM1_START_ADDRESS + LPC18XX_LOCAL_SRAM1_SIZE )
#define LPC18XX_LOCAL_SRAM2_SIZE              ( 40 * 1024 )
#define LPC18XX_LOCAL_SRAM2_END_ADDRESS       ( LPC18XX_LOCAL_SRAM2_START_ADDRESS + LPC18XX_LOCAL_SRAM2_SIZE )
#define LPC18XX_AHB_SRAM1_SIZE                ( 32 * 1024 )
#define LPC18XX_AHB_SRAM1_END_ADDRESS         ( LPC18XX_AHB_SRAM1_START_ADDRESS + LPC18XX_AHB_SRAM1_SIZE )
#define LPC18XX_AHB_SRAM2_SIZE                ( 16 * 1024 )
#define LPC18XX_AHB_SRAM2_END_ADDRESS         ( LPC18XX_AHB_SRAM2_START_ADDRESS + LPC18XX_AHB_SRAM2_SIZE )
#endif /* LPC183x_PART */

#define IAP_LOCATION (*(const uint32_t*)((0x10400100)))
typedef void (*IAP)( uint32_t* command_args_ptr ,uint32_t* command_result_ptr );

/******************************************************
 *                    Macros
 ******************************************************/
#define APP_CODE_A_START_SECTOR               LPC18XX_BANKA_SECTOR(APP_CODE_A_START_ADDR)
#define APP_CODE_A_END_SECTOR                 LPC18XX_BANKA_SECTOR(APP_CODE_A_END_ADDR)

#define APP_CODE_B_START_SECTOR               (LPC18XX_BANKB_SECTOR(APP_CODE_B_START_ADDR) + LPC18XX_BANKB_START_SECTOR)
#define APP_CODE_B_END_SECTOR                 (LPC18XX_BANKB_SECTOR(APP_CODE_B_END_ADDR) + LPC18XX_BANKB_START_SECTOR)

#define SECTOR_FROM_ADDRESS(address) \
    ( (IS_ADDRESS_IN_BANKA(address)) ? ( LPC18XX_BANKA_SECTOR(address) ) : ( LPC18XX_BANKB_SECTOR(address) + LPC18XX_BANKB_START_SECTOR  ) )

#define IS_ADDRESS_IN_LOCAL_SRAM1_RANGE(address) ( address >= LPC18XX_LOCAL_SRAM1_START_ADDRESS && address < ( LPC18XX_LOCAL_SRAM1_START_ADDRESS + LPC18XX_LOCAL_SRAM1_SIZE ) )
#define IS_ADDRESS_IN_LOCAL_SRAM2_RANGE(address) ( address >= LPC18XX_LOCAL_SRAM2_START_ADDRESS && address < ( LPC18XX_LOCAL_SRAM2_START_ADDRESS + LPC18XX_LOCAL_SRAM2_SIZE ) )
#define IS_ADDRESS_IN_AHB_SRAM1_RANGE(address)   ( address >= LPC18XX_AHB_SRAM1_START_ADDRESS   && address < ( LPC18XX_AHB_SRAM1_START_ADDRESS + LPC18XX_AHB_SRAM1_SIZE ) )
#define IS_ADDRESS_IN_AHB_SRAM2_RANGE(address)   ( address >= LPC18XX_AHB_SRAM2_START_ADDRESS   && address < ( LPC18XX_AHB_SRAM2_START_ADDRESS + LPC18XX_AHB_SRAM2_SIZE ) )
#define IS_ADDRESS_IN_LOCAL_SRAM_RANGE(address)  ( IS_ADDRESS_IN_LOCAL_SRAM1_RANGE(address) ? ( ( IS_ADDRESS_IN_LOCAL_SRAM2_RANGE(address) ) ? ( 1 ) : ( 0 ) ) : ( 0 ) )
#define IS_ADDRESS_IN_AHB_SRAM_RANGE(address)    ( IS_ADDRESS_IN_AHB_SRAM1_RANGE(address)   ? ( ( IS_ADDRESS_IN_AHB_SRAM2_RANGE(address)   ) ? ( 1 ) : ( 0 ) ) : ( 0 ) )
#define IS_ADDRESS_IN_SRAM_RANGE(address)        ( IS_ADDRESS_IN_LOCAL_SRAM_RANGE(address)  ? ( ( IS_ADDRESS_IN_AHB_SRAM_RANGE(address)    ) ? ( 1 ) : ( 0 ) ) : ( 0 ) )

#define IAP_MAX_COMMAND_ARGS (6)
#define IAP_MAX_RESULTS      (5)

#ifdef LPC18XX_ENABLE_FLASH_WRITING_FAILS_BREAKS
#define LPC18XX_FLASH_WRITING_ERROR_INDICATE() __asm("bkpt")
#else /* LPC18XX_ENABLE_FLASH_WRITING_FAILS_BREAKS */
#define LPC18XX_FLASH_WRITING_ERROR_INDICATE()
#endif /* LPC18XX_ENABLE_FLASH_WRITING_FAILS_BREAKS */
/******************************************************
 *                   Enumerations
 ******************************************************/
enum iap_command_ids
{
    IAP_INIT                                 = 49,
    IAP_PREPARE_SECTORS                      = 50,
    IAP_COPY_RAM_TO_FLASH                    = 51,
    IAP_ERASE_SECTORS                        = 52,
    IAP_BLANK_CHECK_SECTORS                  = 53,
    IAP_READ_PART_ID                         = 54,
    IAP_READ_BOOT_CODE_VERSION               = 55,
    IAP_READ_DEVICE_SERIAL_NUMBER            = 56,
    IAP_COMPARE                              = 57,
    IAP_REINVOKE_ISP                         = 58,
    IAP_ERASE_PAGE                           = 59,
    IAP_SET_ACTIVE_BOOT_FLASH_BANK           = 60
};

enum iap_status_codes
{
    IAP_CMD_SUCCESS                                = 0x0000,
    IAP_INVALID_COMMAND                            = 0x0001,
    IAP_SRC_ADDR_ERROR                             = 0x0002,
    IAP_DST_ADDR_ERROR                             = 0x0003,
    IAP_SRC_ADDR_NOT_MAPPED                        = 0x0004,
    IAP_DST_ADDR_NOT_MAPPED                        = 0x0005,
    IAP_COUNT_ERROR                                = 0x0006,
    IAP_INVALID_SECTOR                             = 0x0007,
    IAP_SECTOR_NOT_BLANK                           = 0x0008,
    IAP_SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION    = 0x0009,
    IAP_COMPARE_ERROR                              = 0x000A,
    IAP_BUSY                                       = 0x000B,
    IAP_PARAM_ERROR                                = 0x000C,
    IAP_ADDR_ERROR                                 = 0x000D,
    IAP_ADDR_NOT_MAPPED                            = 0x000E,
    IAP_CMD_LOCKED                                 = 0x000F,
    IAP_INVALID_CODE                               = 0x0010,
    IAP_INVALID_BAUD_RATE                          = 0x0011,
    IAP_INVALID_STOP_BIT                           = 0x0012,
    IAP_CODE_READ_PROTECTION_ENABLED               = 0x0013,
    IAP_INVALID_FLASH_UNIT                         = 0x0014,
    IAP_USER_CODE_CHECKSUM                         = 0x0015,
    IAP_ERROR_SETTING_ACTIVE_PARTITION             = 0x0016
};

enum iap_erase_sectors_args
{
    COMMAND_CODE_INDEX_IN_ERASE_SECTORS        = 0,
    START_SECTOR_NUMBER_INDEX_IN_ERASE_SECTORS = 1,
    END_SECTOR_NUMBER_INDEX_IN_ERASE_SECTORS   = 2,
    CPU_CLOCK_FREQUENCY_INDEX_IN_ERASE_SECTORS = 3,
    FLASH_BANK_INDEX_IN_ERASE_SECTORS          = 4,
};

enum iap_prepare_sectors_args
{
    COMMAND_CODE_INDEX_IN_PREPARE_SECTORS        = 0,
    START_SECTOR_NUMBER_INDEX_IN_PREPARE_SECTORS = 1,
    END_SECTOR_NUMBER_INDEX_IN_PREPARE_SECTORS   = 2,
    FLASH_BANK_INDEX_IN_PREPARE_SECTORS          = 3,
};

enum iap_copy_from_ram_to_flash_args
{
    COMMAND_CODE_INDEX_IN_COPY_RAM_TO_FLASH        = 0,
    DST_INDEX                                      = 1,
    SRC_INDEX                                      = 2,
    NUMBER_OF_BYTES_TO_WRITE                       = 3,
    CPU_CLOCK_FREQUENCY_INDEX_IN_COPY_RAM_TO_FLASH = 4,
};

enum iap_erase_pages_args
{
    COMMAND_CODE_INDEX_IN_ERASE_PAGES        = 0,
    START_PAGE_ADDRESS = 1,
    END_PAGE_ADDRESS   = 2,
    CPU_CLOCK_FREQUENCY_INDEX_IN_ERASE_PAGES = 3
};

typedef enum
{
    LPC18XX_ERASE_PAGE_BEFORE_WRITE,
    LPC18XX_DO_NOT_ERASE_PAGE_BEFORE_WRITE
} lpc18xx_page_write_type_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static platform_result_t platform_unlock_sectors ( uint16_t start_sector, uint16_t end_sector );
static platform_result_t platform_write_page     ( uint32_t address, uint8_t* data, uint32_t size );
static platform_result_t lpc18xx_init_iap        ( void );

/******************************************************
 *               Variable Definitions
 ******************************************************/

/* Entry point of the application. This symbol comes from linker */
extern void* app_code_a_start_addr_loc;
extern void* app_code_a_size_loc;
extern void* app_code_b_start_addr_loc;
extern void* app_code_b_size_loc;

void iap_entry( uint32_t* arguments_ptr, uint32_t* result_ptr )
{
    IAP fptr = (IAP)IAP_LOCATION;
    WICED_DISABLE_INTERRUPTS();
    fptr(arguments_ptr, result_ptr);
    WICED_ENABLE_INTERRUPTS();
}
/******************************************************
 *               Function Definitions
 ******************************************************/

#if defined ( __ICCARM__ )

static inline void __jump_to( uint32_t addr )
{
    __asm( "MOV R1, #0x00000001" );
    __asm( "ORR R0, R1, #0" ); /* Last bit of jump address indicates whether destination is Thumb or ARM code */
    __asm( "BX R0" );
}

#elif defined ( __GNUC__ )

__attribute__( ( always_inline ) ) static __INLINE void __jump_to( uint32_t addr )
{
    addr |= 0x00000001;  /* Last bit of jump address indicates whether destination is Thumb or ARM code */
  __ASM volatile ("BX %0" : : "r" (addr) );
}

#endif

void platform_start_app( uint32_t entry_point )
{

    /* Simulate a reset for the app: */
    /*   Switch to Thread Mode, and the Main Stack Pointer */
    /*   Change the vector table offset address to point to the app vector table */
    /*   Set other registers to reset values (esp LR) */
    /*   Jump to the reset vector */

    if ( entry_point == 0 )
    {
        uint32_t* vector_table = (uint32_t*) APP_CODE_A_START_ADDR;
        entry_point = vector_table[ 1 ];
    }

    __asm( "MOV LR,        #0xFFFFFFFF" );
    __asm( "MOV R1,        #0x01000000" );
    __asm( "MSR APSR_nzcvq,     R1" );
    __asm( "MOV R1,        #0x00000000" );
    __asm( "MSR PRIMASK,   R1" );
    __asm( "MSR FAULTMASK, R1" );
    __asm( "MSR BASEPRI,   R1" );
    __asm( "MSR CONTROL,   R1" );

    /*  Now rely on the app crt0 to load VTOR / Stack pointer

     SCB->VTOR = vector_table_address; - Change the vector table to point to app vector table
     __set_MSP( *stack_ptr ); */

    __jump_to( entry_point );

}

static platform_result_t platform_unlock_sectors ( uint16_t start_sector, uint16_t end_sector )
{
    uint32_t status_code[ IAP_MAX_RESULTS ];
    uint32_t iap_prepare_sectors_args[ IAP_MAX_COMMAND_ARGS ];

    memset(status_code, 0x00, sizeof(status_code));
    memset(iap_prepare_sectors_args, 0x00, sizeof(iap_prepare_sectors_args));

    iap_prepare_sectors_args[ COMMAND_CODE_INDEX_IN_PREPARE_SECTORS ]        = IAP_PREPARE_SECTORS;
    iap_prepare_sectors_args[ START_SECTOR_NUMBER_INDEX_IN_PREPARE_SECTORS ] = ( start_sector >= LPC18XX_BANKB_START_SECTOR ) ? ( start_sector - LPC18XX_BANKB_START_SECTOR ) : ( start_sector );
    iap_prepare_sectors_args[ END_SECTOR_NUMBER_INDEX_IN_PREPARE_SECTORS ]   = ( end_sector   >= LPC18XX_BANKB_START_SECTOR ) ? ( end_sector - LPC18XX_BANKB_START_SECTOR ) : ( end_sector );
    iap_prepare_sectors_args[ FLASH_BANK_INDEX_IN_PREPARE_SECTORS ]          = ( start_sector >= LPC18XX_BANKB_START_SECTOR ) ? ( 1 ) : ( 0 );

    iap_entry( &iap_prepare_sectors_args[ 0 ], &status_code[ 0 ] );
    wiced_assert("", status_code[ 0 ] == IAP_CMD_SUCCESS);
    if ( status_code[ 0 ] != IAP_CMD_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    return PLATFORM_SUCCESS;
}

static platform_result_t platform_write_page ( uint32_t address, uint8_t* data, uint32_t size )
{
    uint32_t iap_copy_ram_to_flash_args[ IAP_MAX_COMMAND_ARGS ];
    uint32_t status_code[ IAP_MAX_RESULTS ];

    memset( status_code, 0x00, sizeof(status_code) );
    memset( iap_copy_ram_to_flash_args, 0x00, sizeof(iap_copy_ram_to_flash_args) );

    iap_copy_ram_to_flash_args[ COMMAND_CODE_INDEX_IN_COPY_RAM_TO_FLASH ]        = IAP_COPY_RAM_TO_FLASH;
    iap_copy_ram_to_flash_args[ DST_INDEX ]                                      = address;
    iap_copy_ram_to_flash_args[ SRC_INDEX ]                                      = (uint32_t) data;
    iap_copy_ram_to_flash_args[ NUMBER_OF_BYTES_TO_WRITE ]                       = size;
    iap_copy_ram_to_flash_args[ CPU_CLOCK_FREQUENCY_INDEX_IN_COPY_RAM_TO_FLASH ] = CPU_CLOCK_HZ / 1000;

    iap_entry( &iap_copy_ram_to_flash_args[ 0 ], &status_code[ 0 ] );
    wiced_assert("", status_code[ 0 ] == IAP_CMD_SUCCESS);
    if ( status_code[ 0 ] != IAP_CMD_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    return PLATFORM_SUCCESS;
}

platform_result_t platform_erase_flash( uint16_t start_sector, uint16_t end_sector )
{
    platform_result_t result                = PLATFORM_SUCCESS;
    uint32_t          iap_init_command_args[ IAP_MAX_COMMAND_ARGS ];
    uint32_t          iap_erase_sectors_args[ IAP_MAX_COMMAND_ARGS ];
    uint32_t          status_code[ IAP_MAX_RESULTS ];

    /* Init IAP  */
    memset(status_code, 0x00, sizeof(status_code));
    iap_init_command_args[ 0 ] = IAP_INIT;
    iap_entry( &iap_init_command_args[0], &status_code[ 0 ] );
    wiced_assert("", status_code[ 0 ] == IAP_CMD_SUCCESS);
    if ( status_code[ 0 ] != IAP_CMD_SUCCESS )
    {
        return PLATFORM_ERROR;
    }
    /* Unlock sectors */
    result = platform_unlock_sectors( start_sector, end_sector );
    if ( result != PLATFORM_SUCCESS )
    {
        return result;
    }

    memset( status_code, 0x00, sizeof(status_code) );
    iap_erase_sectors_args[ COMMAND_CODE_INDEX_IN_ERASE_SECTORS ]        = IAP_ERASE_SECTORS;
    iap_erase_sectors_args[ START_SECTOR_NUMBER_INDEX_IN_ERASE_SECTORS ] = ( start_sector >= LPC18XX_BANKB_START_SECTOR ) ? ( start_sector - LPC18XX_BANKB_START_SECTOR ) : ( start_sector );
    iap_erase_sectors_args[ END_SECTOR_NUMBER_INDEX_IN_ERASE_SECTORS ]   = ( end_sector   >= LPC18XX_BANKB_START_SECTOR ) ? ( end_sector - LPC18XX_BANKB_START_SECTOR )   : ( end_sector );
    iap_erase_sectors_args[ CPU_CLOCK_FREQUENCY_INDEX_IN_ERASE_SECTORS ] = CPU_CLOCK_HZ / 1000;
    iap_erase_sectors_args[ FLASH_BANK_INDEX_IN_ERASE_SECTORS ]          = ( end_sector   >= LPC18XX_BANKB_START_SECTOR ) ? ( 1 ) : (0);
    iap_entry( &iap_erase_sectors_args[ 0 ], &status_code[ 0 ] );
    if ( status_code[ 0 ] != IAP_CMD_SUCCESS )
    {
        return PLATFORM_ERROR;
    }
    return PLATFORM_SUCCESS;
}

platform_result_t lpc18xx_erase_page( uint32_t address )
{
    uint32_t erase_page_args[ IAP_MAX_COMMAND_ARGS ];
    uint32_t status_code[ IAP_MAX_RESULTS ];

    memset( status_code,     0x00, sizeof(status_code) );
    memset( erase_page_args, 0x00, sizeof(erase_page_args) );

    erase_page_args[ COMMAND_CODE_INDEX_IN_ERASE_PAGES ]        = IAP_ERASE_PAGE;
    erase_page_args[ START_PAGE_ADDRESS ]                             = address;
    erase_page_args[ END_PAGE_ADDRESS ]                               = address;
    erase_page_args[ CPU_CLOCK_FREQUENCY_INDEX_IN_ERASE_PAGES ] = CPU_CLOCK_HZ / 1000;

    iap_entry( &erase_page_args[ 0 ], &status_code[ 0 ] );
    wiced_assert("", status_code[ 0 ] == IAP_CMD_SUCCESS);
    if ( status_code[ 0 ] != IAP_CMD_SUCCESS )
    {
        return PLATFORM_ERROR;
    }
    return PLATFORM_SUCCESS;
}

static platform_result_t lpc18xx_init_iap ( void )
{
    uint32_t iap_init_command_args [IAP_MAX_COMMAND_ARGS];
    uint32_t status_code [ IAP_MAX_RESULTS ];

    memset( status_code,           0x00, sizeof(status_code) );
    memset( iap_init_command_args, 0x00, sizeof(iap_init_command_args) );
    iap_init_command_args[ 0 ] = IAP_INIT;

    iap_entry( &iap_init_command_args[ 0 ], &status_code[ 0 ] );
    wiced_assert("", status_code[ 0 ] == IAP_CMD_SUCCESS);
    if ( status_code[ 0 ] != IAP_CMD_SUCCESS )
    {
        return PLATFORM_ERROR;
    }

    return PLATFORM_SUCCESS;
}

platform_result_t lpc18xx_write_page( uint32_t page_address, uint8_t* write_page_buffer, lpc18xx_page_write_type_t write_type )
{
    platform_result_t result = PLATFORM_SUCCESS;

    /* Find which sector is going to affected by this write and unlock it */
    result = platform_unlock_sectors( SECTOR_FROM_ADDRESS(page_address), SECTOR_FROM_ADDRESS(page_address) );
    wiced_assert("", result == PLATFORM_SUCCESS);
    if ( result != PLATFORM_SUCCESS )
    {
        LPC18XX_FLASH_WRITING_ERROR_INDICATE();
        return result;
    }

    if ( write_type == LPC18XX_ERASE_PAGE_BEFORE_WRITE )
    {
        /* Erase page and unlock sectors */
        result = lpc18xx_erase_page(page_address);
        wiced_assert("", result == PLATFORM_SUCCESS);
        if ( result != PLATFORM_SUCCESS )
        {
            LPC18XX_FLASH_WRITING_ERROR_INDICATE();
            return result;
        }
        result = platform_unlock_sectors( SECTOR_FROM_ADDRESS(page_address), SECTOR_FROM_ADDRESS(page_address) );
        wiced_assert("", result == PLATFORM_SUCCESS);
        if ( result != PLATFORM_SUCCESS )
        {
            LPC18XX_FLASH_WRITING_ERROR_INDICATE();
            return result;
        }
    }

    /* Write page to the flash memory */
    result = platform_write_page( page_address, write_page_buffer, LPC18XX_PAGE_SIZE );
    wiced_assert("", result == PLATFORM_SUCCESS);
    if ( result != PLATFORM_SUCCESS )
    {
        LPC18XX_FLASH_WRITING_ERROR_INDICATE();
        return result;
    }
    return PLATFORM_SUCCESS;
}

platform_result_t platform_write_flash_chunk( uint32_t address, const void* data, uint32_t size )
{
    platform_result_t result          = PLATFORM_SUCCESS;
    uint32_t          start_offset    = 0;
    uint32_t          end_offset      = 0;
    uint8_t*          data_ptr        = (uint8_t*) data;
    uint16_t          written_first   = 0;
    uint16_t          written_last    = 0;
    uint8_t           write_page_buffer   [ LPC18XX_PAGE_SIZE ];
    uint8_t           compare_page_buffer [ LPC18XX_PAGE_SIZE ];
    uint32_t          bytes_remaining;
    uint8_t*          write_address;
    uint8_t*          read_address;

    /* Init IAP */
    result = lpc18xx_init_iap();
    if ( result != PLATFORM_SUCCESS )
    {
        return result;
    }

    /* Set mask on all bytes in the page write buffer */
    memset( write_page_buffer, 0xFF, sizeof(write_page_buffer) );
    start_offset = address & ( LPC18XX_PAGE_SIZE - 1 );

    if ( start_offset != 0 )
    {
        uint32_t start_of_page_address = address - start_offset;

        /* Read first incomplete page to the buffer which will used to check the flash page integrity */
        memcpy( compare_page_buffer, (uint8_t*) start_of_page_address, LPC18XX_PAGE_SIZE );

        /* Copy new data from the start_offset in the page */
        memcpy( &write_page_buffer  [ start_offset ], data, MIN( size, LPC18XX_PAGE_SIZE - start_offset) );
        memcpy( &compare_page_buffer[ start_offset ], data, MIN( size, LPC18XX_PAGE_SIZE - start_offset) );

        /* calculate the number of bytes which will be written during this page write */
        written_first = MIN(size, (LPC18XX_PAGE_SIZE - start_offset));

        /* Write page to the flash memory */
        result = lpc18xx_write_page( start_of_page_address, write_page_buffer, LPC18XX_DO_NOT_ERASE_PAGE_BEFORE_WRITE );
        if ( result != PLATFORM_SUCCESS )
        {
            LPC18XX_FLASH_WRITING_ERROR_INDICATE();
            return result;
        }

        /* Check flash page integrity. The writing of the current data may have caused the modification of the
         * data which was written to this page before but was masked in the current write */
        if ( memcmp( (char *)start_of_page_address, compare_page_buffer, LPC18XX_PAGE_SIZE ) != 0 )
        {
            result = lpc18xx_write_page( start_of_page_address, compare_page_buffer, LPC18XX_ERASE_PAGE_BEFORE_WRITE );
            if ( memcmp( (char *)start_of_page_address, compare_page_buffer, LPC18XX_PAGE_SIZE ) != 0 )
            {
                /* Writing to LPC18xx internal flash has failed. The contents of the buffer and
                 * the data read directly from the internal FLASH do not match */
                LPC18XX_FLASH_WRITING_ERROR_INDICATE();
                return PLATFORM_ERROR;
            }
        }

        if ( written_first == size )
        {
            return PLATFORM_SUCCESS;
        }
    }


    /* If the end address is not aligned to the page size, read full page to the temporary buffer and
     * copy unaligned number of bytes  */
    end_offset   = ( address + size ) & ( LPC18XX_PAGE_SIZE - 1 );

    /* Set mask on all bytes in the page write buffer */
    memset( write_page_buffer, 0xFF, sizeof(write_page_buffer) );

    if ( end_offset != 0 )
    {
        uint32_t start_of_page_address = ( address + size ) - end_offset;

        /* Read last incomplete page to the buffer which will used to check the flash page integrity */
        memcpy( compare_page_buffer, (uint8_t*) start_of_page_address, LPC18XX_PAGE_SIZE );

        /* Copy new data from the start of the page up to the end_offset */
        memcpy( write_page_buffer,   &data_ptr[ size - end_offset ], end_offset );
        memcpy( compare_page_buffer, &data_ptr[ size - end_offset ], end_offset );

        /* Write page to the flash memory */
        result = lpc18xx_write_page( start_of_page_address, write_page_buffer, LPC18XX_DO_NOT_ERASE_PAGE_BEFORE_WRITE );
        if ( result != PLATFORM_SUCCESS )
        {
            LPC18XX_FLASH_WRITING_ERROR_INDICATE();
            return result;
        }

        /* Check flash page integrity. The writing of the current data may have caused the modification of the
         * data which was written to this page previously but was masked in the current write */
        if ( memcmp( (char *)start_of_page_address, compare_page_buffer, LPC18XX_PAGE_SIZE ) != 0 )
        {
            result = lpc18xx_write_page( start_of_page_address, compare_page_buffer, LPC18XX_ERASE_PAGE_BEFORE_WRITE );
            if ( memcmp( (char *)start_of_page_address, compare_page_buffer, LPC18XX_PAGE_SIZE ) != 0 )
            {
                /* Writing to LPC18xx internal flash has failed. The contents of the buffer and
                 * the data read directly from the internal FLASH do not match */
                LPC18XX_FLASH_WRITING_ERROR_INDICATE();
                return PLATFORM_ERROR;
            }
        }
        written_last = end_offset ;
    }

    write_address   = (uint8_t*) ( ( start_offset != 0 ) ? ( address + ( LPC18XX_PAGE_SIZE - start_offset ) ) : ( address ) );
    read_address    = ( start_offset != 0 ) ? &data_ptr[ LPC18XX_PAGE_SIZE - start_offset ] : data_ptr;
    bytes_remaining = size - ( written_first + written_last );

    while ( bytes_remaining != 0 )
    {
        if ( IS_ADDRESS_IN_SRAM_RANGE((uint32_t)read_address) == 0 )
        {
            /* Use temporary buffer allocated in SRAM if the read address belongs to SRAM */
            memcpy( write_page_buffer, read_address, LPC18XX_PAGE_SIZE );

            /* Write page to the flash memory */
            result = lpc18xx_write_page( (uint32_t) write_address, write_page_buffer, LPC18XX_DO_NOT_ERASE_PAGE_BEFORE_WRITE );
        }
        else
        {
            /* Write page to the flash memory */
            result = lpc18xx_write_page( (uint32_t) write_address, read_address, LPC18XX_DO_NOT_ERASE_PAGE_BEFORE_WRITE );
        }

        if ( result != PLATFORM_SUCCESS )
        {
            LPC18XX_FLASH_WRITING_ERROR_INDICATE();
            return result;
        }

        bytes_remaining -= LPC18XX_PAGE_SIZE;
        write_address   += LPC18XX_PAGE_SIZE;
        read_address    += LPC18XX_PAGE_SIZE;
    }

    return PLATFORM_SUCCESS;
}

void platform_erase_app_area( uint32_t physical_address, uint32_t size )
{
    UNUSED_PARAMETER(physical_address);
    UNUSED_PARAMETER(size);

    if ( IS_ADDRESS_IN_SRAM_RANGE(physical_address) == WICED_FALSE )
    {
        if ( physical_address == (uint32_t)DCT1_START_ADDR )
        {
            platform_erase_flash( PLATFORM_DCT_COPY1_START_SECTOR, PLATFORM_DCT_COPY1_END_SECTOR );
        }
        else
        {
            platform_erase_flash( APP_CODE_A_START_SECTOR, APP_CODE_A_END_SECTOR );
            if ( ( physical_address + size ) > LPC18XX_BANKB_START_ADDRESS )
            {
                platform_erase_flash( APP_CODE_B_START_SECTOR, APP_CODE_B_END_SECTOR );
            }
        }
    }
}

/* The function would copy data from serial flash to internal flash.
 * The function assumes that the program area is already erased (for now).
 * TODO: Adding erasing the required area
 */

static wiced_result_t platform_copy_app_to_iflash( const image_location_t* app_header_location, uint32_t offset, uint32_t physical_address, uint32_t size )
{
    int i = 0;
    uint8_t buff[ LPC18XX_PAGE_SIZE ];

    while ( size > 0 )
    {
        uint32_t write_size = MIN( sizeof(buff), size);
        wiced_apps_read( app_header_location, buff, offset, write_size );
        platform_write_flash_chunk( (uint32_t) physical_address, buff, write_size );
        if ( memcmp( (char *)physical_address, buff, write_size ) != 0 )
        {
            LPC18XX_FLASH_WRITING_ERROR_INDICATE();
            return PLATFORM_ERROR;
        }

        offset           += write_size;
        physical_address += write_size;
        size             -= write_size;
        i++;
    }
    return PLATFORM_SUCCESS;
}

void platform_load_app_chunk( const image_location_t* app_header_location, uint32_t offset, void* physical_address, uint32_t size )
{
    /* This function makes an assumption that the application area has been erased previously  */
    if ( IS_ADDRESS_IN_SRAM_RANGE((uint32_t)physical_address) == 0 )
    {
        platform_copy_app_to_iflash( app_header_location, offset, (uint32_t) physical_address, size );
    }
    else
    {
        wiced_apps_read( app_header_location, physical_address, offset, size );
    }
}
