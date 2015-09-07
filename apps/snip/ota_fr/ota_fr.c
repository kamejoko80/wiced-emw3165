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
 *
 * Over-The-Air (OTA) Upgrade & Factory Reset Application
 *
 * ------------------------------------------------------
 * PLEASE read the following documentation before trying
 * this application!
 * ------------------------------------------------------
 *
 * This application demonstrates how to use the WICED build system with a WICED development
 * board to demonstrate the OTA upgrade and factory reset capability.
 *
 * Features demonstrated
 *  - WICED OTA Upgrade
 *  - Factory Reset process
 *
 * WICED Multi-Application Support:
 * =================================
 * As of WICED-SDK 3.1.1, WICED Application Framework (WAF) supports loading and storing of multiple
 * application binaries in the external serial flash. Up to 8 binaries are supported. The first
 * five binaries are reserved for internal usage and the last three binaries are free for users to use.
 * The binaries are organised as follows:
 *  - Factory reset application (FR_APP)
 *  - DCT backup image (DCT_IMAGE)
 *  - OTA upgrade application (OTA_APP)
 *  - Resources file system (FILESYSTEM_IMAGE)
 *  - WIFI firmware (WIFI_FIRMWARE)
 *  - Application 0 (APP0)
 *  - Application 1 (APP1)
 *  - Application 2 (APP2)
 *
 * OTA Snippet Application:
 * =========================
 * This snippet application demonstrates how to use WICED multi-application support to perform
 * factory reset and OTA upgrade. The following steps assume you have a BCM943362WCD4 WICED evaluation board
 * (a BCM943362WCD4 WICED module on a WICED evaluation board). If your board is different, substitute
 * BCM943362WCD4 for your platform name.
 *
 * Prepare the WICED evaluation board for OTA upgrade
 *     1. Build the snip.ota_fr application to function as your factory reset and OTA
 *        application
 *     2. Notice that the factory reset application (FR_APP) is set in <WICED-SDK>/apps/snip/ota_fr/ota_fr.mk
 *        to point to the snip.ota_fr application elf file.
 *     3. Run the following make target to download your production and factory reset applications
 *        to the board:
 *            make snip.ota_fr-BCM943362WCD4 download download_apps run
 *     4. Build an application that will upgrade the current application.
 *        For this example we will build the snip.scan application:
 *            make snip.scan-BCM943362WCD4
 *
 * Upgrade the application running on the WICED evaluation board
 *   After carefully completing the above steps, the WICED evaluation board is ready to for an OTA upgrade.
 *   'Loading OTA upgrade app' log message is displayed in the terminal when the OTA upgrade application is ready.
 *   Work through the following steps:
 *   - Using the Wi-Fi connection manager on your computer, search for, and connect to,
 *     the Wi-Fi AP called : Wiced_Device
 *   - Open a web browser and enter the IP address of the eval board: 192.168.10.1 (default)
 *   - After a short period, the WICED Webserver OTA Upgrade webpage appears
 *   - Click 'Choose File' and navigate to the file
 *     <WICED-SDK>/build/snip_scan-BCM943362WCD4/Binary/snip_scan-BCM943362WCD4.stripped.elf
 *     (this is the snip.scan application binary file that was created in step 2 above)
 *   - Click 'Open' (the dialogue box disappears)
 *   - Click 'Start upgrade' to begin the upgrade process
 *      - The progress bar within the webpage indicates the upgrade progress
 *      - The webpage displays 'Transfer completed, WICED device is rebooting now' when the
 *        process completes
 *   - With the upgrade complete, the snip.scan application runs and Wi-Fi scan results are regularly
 *     printed to the terminal
 *
 * Perform factory reset on the WICED evaluation board
 *   To perform factory reset on the WICED evaluation board, work through the following steps:
 *   - Push and hold the SW1 button THEN momentarily press and release the Reset button.
 *     The D1 LED flashes quickly to indicate factory reset will occur *IF* SW1 is held
 *     for a further 5 seconds. Continue to hold SW1.
 *   - After the copy process is complete, the WICED evaluation board reboots and runs the factory
 *     reset (OTA_FR) application. Observe the log messages at the terminal to confirm the factory reset
 *     is completed successfully.
 *
 */

#include "wiced.h"
#include "wwd_debug.h"
#include "wiced_framework.h"
#include "wiced_ota_server.h"

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
 *               Variable Definitions
 ******************************************************/
static const wiced_ip_setting_t device_init_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS(192, 168, 10,  1) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS(255, 255, 255, 0) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS(192, 168, 10,  1) ),
};

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
#ifndef GPIO_LED_NOT_SUPPORTED
    uint32_t i;
#endif
    wiced_init( );

    WPRINT_APP_INFO( ( "Hi, I'm the Production App (ota_fr).\r\n" ) );

#ifndef GPIO_LED_NOT_SUPPORTED
    WPRINT_APP_INFO( ( "Watch while I toggle some LEDs ...\r\n" ) );

    for ( i = 0; i < 15; i++ )
    {
        wiced_gpio_output_high( WICED_LED1 );
        wiced_gpio_output_low( WICED_LED2 );
        wiced_rtos_delay_milliseconds( 300 );
        wiced_gpio_output_low( WICED_LED1 );
        wiced_gpio_output_high( WICED_LED2 );
        wiced_rtos_delay_milliseconds( 300 );
    }
#endif

    WPRINT_APP_INFO( ( "Time for an upgrade. OTA upgrade starting ...\r\n" ) );

    /* Bringup the network interface */
    wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &device_init_ip_settings );
    wiced_ota_server_start( WICED_AP_INTERFACE );
    while ( 1 )
    {
        wiced_rtos_delay_milliseconds( 100 );
    }
}



