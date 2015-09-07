
==============================
  EMW3165

  - ./make wwd.scan-EMW3165-FreeRTOS-LwIP-SDIO
  - sudo ~/stlink/st-flash write build/wwd_scan-EMW3165-FreeRTOS-LwIP-SDIO/binary/wwd_scan-EMW3165-FreeRTOS-LwIP-SDIO.bin 0x8000000

Note: My st-flash source code at ~/stlink.
==============================













=====================================================================
Broadcom WICED Software Development Kit 3.3.1 - README
=====================================================================

The WICED SDK provides a full compliment of application level APIs, 
libraries and tools needed to design & implement secure embedded wireless
networking applications. 

Major features of the WICED SDK include ...
  - Low-footprint embedded Wi-Fi Driver with Client (STA), softAP and Wi-Fi Direct
  - Wi-Fi <-> Bluetooth SmartBridge 
  - Various RTOS/TCP stack options including
    - ThreadX/NetX (IPv4), ThreadX/NetX Duo (IPv6), FreeRTOS/LwIP (IPv4)
  - Support for various Broadcom Wi-Fi & combo chips
    - BCM4390 Integrated Apps + Wi-Fi SoC
    - BCM43909 Integrated Apps + Wi-Fi SoC
    - BCM43362 Wi-Fi SoC
    - BCM43364 Wi-Fi SoC
    - BCM43341 Wi-Fi SoC
    - BCM43438 Wi-Fi SoC
    - BCM43341 Wi-Fi + Bluetooth combo SoC
  - Support for various MCU host platforms
    - ST Microelectronics : STM32F2xx, STM32F4xx
    - Atmel : AT91SAM4S16B
    - Freescale : K61
    - NXP : LPC17xx, LPC18xx
  - RTOS & Network abstraction layer with a simple API for UDP, TCP, HTTP, HTTPS communications
  - SSL/TLS Security Library integrated with an HTTPS library for secure web transactions
  - WICED Application Framework including Bootloader, OTA Upgrade and Factory Reset
  - Automated Wi-Fi Easy Setup using one of several methods
    - SoftAP & Secure HTTP server
    - Wi-Fi Protected Setup
    - Apple Wireless Accessory Configuration (WAC) Protocol
  - Simple API to provide access to MCU peripherals including UART, SPI, I2C, Timers, RTC, ADCs, DACs, etc
  - Support for multiple toolchains including GNU and IAR
  - Support for Apple AirPlay and HomeKit
 
The WICED SDK release is structured as follows:
  apps          : Example & Test Applications
  doc           : API & Reference Documentation
  include       : WICED API, constants, and defaults 
  libraries     : Bluetooth, daemons, drivers, file systems, inputs, and protocols
  platforms     : Evaluation board support package, including Eval Board and Module Schematics
  resources     : Binary and text based objects including scripts, images, and certificates
  tools         : Build tools, compilers, debugger, makefiles, programming tools etc.
  tools/drivers : Drivers for WICED evaluation boards
  WICED         : WICED core components (RTOS, Network Stack, Wi-Fi Driver, Security & Platform libraries)
  WICED/WWD     : The WICED Wi-Fi Driver (equivalent to the Wiced directory in previous SDK-1.x releases) 
  README.txt    : This file
  CHANGELOG.txt : A log of changes for each SDK revision
 

Getting Started
---------------------------------------------------------------------
If you are unfamiliar with the WICED SDK, please refer to the 
WICED Quickstart Guide located here: <WICED-SDK>/doc/WICED-QSG2xx-R.pdf
The WICED Quickstart Guide documents the process to setup a computer for
use with the WICED SDK, IDE and WICED Evaluation Board. 

The WICED SDK includes lots of sample applications in the <WICED-SDK>/Apps directory.
Applications included with the SDK are outlined below.

 apps/demo : Demonstration Applications
   - Applications demonstrating how to integrate various WICED API features 
   
 apps/snip : Application Snippets
   - Various applications to demonstrate usage of individual WICED APIs        

 apps/test : Test & Utility Applications
   - console      : Provides various test features including Iperf for throughput testing 
   - mfg_test     : Manufacturing Test application to enable radio performance and certification testing

 apps/waf  : WICED Application Framework
   - bootloader   : Bootloader application used in conjunction with the WICED Application Framework
   - sflash_write : Serial flash application used to write data into external serial flash
   
 apps/wwd : Wiced Wi-Fi Driver Applications to demonstrate advanced usage of the low layer Wi-Fi driver
    
To obtain a complete list of build commands and options, enter the following text in the
base WICED SDK directory on a command line:
$> make

To compile, download and run the Wi-Fi scan application on the Broadcom BCM943362WCD4 evaluation platform, 
enter the following text on a command line (a period character is used to reference applications 
in sub-directories) :
$> make snip.scan-BCM943362WCD4 download run

The default RTOS and Network Stack components are defined in the WICED configuration makefile  
at <WICED-SDK>/tools/makefiles/wiced_config.mk. The default I/O bus component is defined in the platform
makefile at <WICED-SDK>/platforms/<Platform>/<Platform>.mk. Defaults may be bypassed by specifying the 
component as part of the build string if desired as shown in the following example.
$> make snip.scan-BCM943362WCD4-FreeRTOS-LwIP-SDIO download run
       
Source code, headers and reference information for supported platforms are available 
in the <WICED-SDK>/platforms directory. Source code, headers, linker scripts etc that 
are common to all platforms are available in the <WICED-SDK>/WICED/platform directory.


Supported Features
---------------------------------------------------------------------
Wi-Fi & Bluetooth SmartBridge Features
 * Scan and associate to Wi-Fi access points
 * Authenticate to Wi-Fi Access Points with the following security types:
   Open, WEP-40, WEP-104, WPA (AES & TKIP), WPA2 (AES, TKIP & Mixed mode)
 * AP mode with support for security types : Open, WPA, WPA2
 * Concurrent AP & STA mode (AP mode limited to 3 concurrent connected clients)
 * Wi-Fi Direct
 * WPS 1.0 & 2.0 Enrollee & Registrar (Internal Registrar only)
 * Wi-Fi APIs : Network keep alive, packet filters
 * Host <-> Wi-Fi SDIO & SPI interface
 * Bluetooth SmartBridge with multiple connections including the
   following features: Whitelist, Bond Storage, Attribute Caching, 
   GATT Procedures, Configurable Maximum Concurrent Connections, Directed 
   Advertisements, Device address initialisation, Passkey entry
 * Host <-> Wi-Fi via Memory to Memory DMA engine

Bluetooth Features
 * A2DP v1.2 (Advanced Audio Distribution Profile)
   - A2DP Sink Functionality
   - SBC Decoder
 * AVRCP (Audio/Video Remote Control Profile)
   - AVRCP Controller v1.0
   - AVRCP Target v1.4 (Absolute Volume)
 *  Handsfree profile (Handsfree role)
   - HFP v1.6
   - Accept/Reject/End incoming call
   - Outgoing call – Last number dial
   - Support for inband ringtone
   - Two-way calling
   - Caller-ID support
 * Man-Machine-Interface via buttons
   - AVRCP play/pause/Skip-forward/Skip-backward
   - A2DP Volume Up/Down
   - Connect to previously connected device
   - HFP Accept/Reject/End incoming call and Last number dial
 * SDAP (Service Discovery Application Profile)
 * GAP (Generic Access Profile)

RTOS & Network Stack Support
 * FreeRTOS / LwIP    (full source)
 * ThreadX  / NetX    (object file; free for use with WICED *ONLY*)
 * ThreadX  / NetXDuo (object file; free for use with WICED *ONLY*)

Networking Features (IPv4 & IPv6)
 * ICMP (Ping)
 * ARP
 * TCP
 * UDP 
 * IGMP (Multicast)
 * IPv6 NDP, Multicast
 * DHCP (Client & Server)
 * DNS (Client & Redirect Server)
 * mDNS/DNS-SD Zeroconf Network Discovery (Broadcom Gedday)
 * TLS1.0/1.1/1.2 (object file with host abstraction layer; free for use with WICED *ONLY*)
 * HTTP / HTTPS (Client & Server)
 * SNTP
 * SMTP

Application Features
 * Apple AirPlay (requires Apple authentication co-processor; available to Apple MFi licensees *ONLY*) 
 * Apple HomeKit (available to Apple MFi licensees *ONLY*)
 * Bluetooth Audio
 * Peripheral interfaces
   * GPIO
   * Timer / PWM
   * UART
   * SPI
   * I2C
   * RTC (Real Time Clock)
 * Xively "Internet of Things" protocol

* WICED Application Framework
   * Bootloader
   * Device Configuration Table (region in flash to store AP, security credentials, TLS certs, serial number, Wi-Fi country code, etc)
   * OTA upgrade
   * Factory reset
   * Automated configuration via softAP & webserver
   * Apple Wireless Accessory Configuration (WAC) protocol (available to Apple MFi licensees *ONLY*)
   * System Monitor to manage the watchdog

Toolchains
 * GNU make
 * IAR

Hardware Platforms
 BCM43362
   * BCM943362WCD4  : Broadcom WICED Module with STM32F205 MCU mounted on BCM9WCD1EVAL1
   * BCM943362WCD6  : Broadcom WICED Module with STM32F415 MCU mounted on BCM9WCD1EVAL1
   * BCM943362WCD8  : Broadcom WICED Module with Atmel SAM4S16B MCU mounted on BCM9WCD1EVAL1
   * BCM9WCDPLUS114 : WICED+ Eval Board (includes BCM43362+STM32F205 WICED+ Module and BCM20702 Bluetooth module)
   * BCM9WCD1AUDIO  : Broadcom WICED Audio Evaluation Board (includes BCM43362, STM32F415, WM8533 audio DAC, and BCM20702 Bluetooth module)
 BCM943364
   * BCM943364WCD1  : Broadcom WICED Module with STM32F215 MCU mounted on BCM9WCD1EVAL1
   * BCM943364WCDA  : Broadcom WICED Module with Atmel SAM4S16B MCU mounted on BCM9WCD1EVAL1
 BCM943341
   * BCM943341WCD1  : Broadcom BCM43341-based WICED Module with STM32F417 MCU mounted on BCM9WCD5EVAL1
 BCM4390
   * BCM94390WCD2   : Broadcom BCM4390 SiP-based WICED Module on BCM9WCD3EVAL1
 BCM43909
   * BCM943909WCD1  : Broadcom BCM43909 SiP-based WICED Module on BCM943909WCDEVAL_1


Known Limitations & Notes
---------------------------------------------------------------------

 * Features not yet supported in WICED-SDK-3.3.1
   - IAR Embedded Workspace native support

   * Platform Restrictions:
       BCM943341WCD1
         - Wi-Fi Direct not support
         - SPI bus not supported
       BCM4390WCD2
         - Wi-Fi Direct not supported
         - WPS may timeout with some access points
     
   * Wi-Fi Powersave (1)
       The WLAN chip requires an external 32kHz sleep clock input during powersave.
       Platforms that do not support Wi-Fi powersave (per the table above) are
       not capable of driving the WLAN sleep clock. An external 32kHz clock is 
       required for these platforms.

 * libc does not include support for printing uint64_t (long long)
   
 * RTOS detection may cause OpenOCD to crash in the following situation:
     GDB has an elf containing a known RTOS loaded, and your app is using the 
     RTOS memory for other purposes. This situation may occur while debugging 
     the bootloader
     SOLUTION : Remove " -rtos auto " from the <WICED-SDK>/Tools/OpenCD/OpenOCD 
                .cfg file that matches your hardware debugger 
                (ie. BCM9WCD1EVAL1.cfg for WICED Evaluation Boards) 

 * AP mode when running with Wi-Fi Direct is limited to 2 STA clients
 
Tools
---------------------------------------------------------------------
The GNU ARM toolchain is from Yagarto, http://yagarto.de

Programming and debugging is enabled by OpenOCD, http://openocd.berlios.de

The standard WICED Evaluation board (BCM9WCD1EVAL1) provides two physical 
programming/debug interfaces for the STM32 host microprocessor: USB-JTAG and direct 
JTAG. The WICED Evaluation board driver additionally provides a single USB-serial 
port for debug printing or UART console purposes.

The USB-JTAG interface is enabled by the libftdi/libusb open source driver,
http://intra2net.com/en/developer/libftdi. 
The direct JTAG interface works with third party JTAG programmers including 
Segger, IAR J-Link and Olimex ARM-USB-TINY series. OpenOCD works with the libftdi/libusb 
USB-JTAG driver shipped with the WICED SDK and commercially available JTAG drivers 
available from third party vendors.

Building, programming and debugging of applications is achieved using either a 
command line interface or the WICED IDE as described in the Quickstart Guide.

Instructions to use the IAR toolchain are provided in a README located in the 
following directory: <WICED-SDK>/Tools/IAR 

                     
WICED Technical Support
---------------------------------------------------------------------
WICED support is available on the Broadcom forum at http://forum.broadcom.com/forum.php 

Sign-up is a two-step process: Firstly, sign up to the general Broadcom support 
forum, then apply to be a member of the WICED User Group. Be sure to identify 
yourself to the forum moderator, access to the WICED forum is restricted to 
bona-fide WICED customers only.

Broadcom provides customer access to a wide range of additional information, including 
technical documentation, schematic diagrams, product bill of materials, PCB layout 
information, and software updates through its customer support portal. For a CSP account, 
contact your Broadcom Sales or Engineering support representative.

                     
Further Information
---------------------------------------------------------------------
Further information about WICED and the WICED Development System is
available on the WICED website at http://broadcom.com/wiced or
by e-mailing Broadcom support at support@broadcom.com
