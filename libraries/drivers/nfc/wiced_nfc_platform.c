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
 */
#include "nfc_host.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* NFC_TASK declarations */
#define NFC_TASK_STR            ((int8_t *) "NFC_TASK")
#define NFC_TASK_STACK_SIZE     0x800
#define NFC_TASK_THREAD_PRI     6

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

static uint32_t nfc_task_stack[(NFC_TASK_STACK_SIZE+3)/4];


/******************************************************
 *               Function Definitions
 ******************************************************/

/* Init hardware used by NFC */
void wiced_nfc_InitHW(void)
{
    /* Initialize GPIOs */
    UPIO_Init( NULL );
    GKI_delay( 200 );

    USERIAL_Init( NULL );
}

/* Create NFC task */
void wiced_nfc_CreateTasks (void)
{
    GKI_create_task( (void (*)(uint32_t))nfc_task, nfc_task_value, NFC_TASK_STR, (uint16_t *) ( (uint8_t *) nfc_task_stack + NFC_TASK_STACK_SIZE ), sizeof( nfc_task_stack ), NFC_TASK_THREAD_PRI );
}

/* NFC  stack booting */
int nfc_fwk_boot_entry( void )
{
    /* Initialize hardware */
    wiced_nfc_InitHW( );

    /* Initialize OS */
    GKI_init( );

    /* Enable interrupts */
    GKI_enable( );

    /* Create tasks */
    wiced_nfc_CreateTasks( );

    HAL_NfcInitialize( );

    /* Start tasks */
    GKI_run( 0 );

    return 0;
}


