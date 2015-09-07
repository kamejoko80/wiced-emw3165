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
 */

#include "wiced_utilities.h"
#include <string.h>
#include "wiced_resource.h"
#include "wwd_debug.h"
#include "wiced_tcpip.h"
#include "platform_resource.h"
#include "wiced_wifi.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define IS_DIGIT(c) ((c >= '0') && (c <= '9'))

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

static uint8_t string_to_generic( const char* string, uint8_t str_length,  uint32_t* value_out, uint8_t is_unsigned, uint8_t is_hex )
{
    uint8_t nibble;
    uint8_t characters_processed = 0;

    if ( string == NULL )
    {
        return 0;
    }

    *value_out = 0;

    while ( ( characters_processed != str_length ) &&
            ( 0 == hexchar_to_nibble( *string, &nibble ) ) &&
            ( ( is_hex != 0 ) || ( nibble < 10 ) )
          )
    {
        if ( is_hex != 0 )
        {
            if ( ( ( *value_out > ( 0x7fffffff >> 4 ) ) && ( is_unsigned == 0 ) ) ||
                 ( *value_out > ( 0xffffffff >> 4 ) )
               )
            {
                break;
            }
            *value_out = ( *value_out << 4 ) + nibble;
        }
        else
        {
            if ( ( ( *value_out > ( 0x7fffffff / 10 ) ) && ( is_unsigned == 0 ) ) ||
                 ( *value_out > ( 0xffffffff / 10 ) )
               )
            {
                break;
            }
            *value_out = ( *value_out * 10 ) + nibble;
        }
        string++;
        characters_processed++;
    }

    return characters_processed;
}

/*!
 ******************************************************************************
 * Convert a decimal or hexidecimal string to an integer.
 *
 * @param[in] str  The string containing the value.
 *
 * @return    The value represented by the string.
 */

uint32_t generic_string_to_unsigned( const char* str )
{
    uint32_t val = 0;
    uint8_t is_hex = 0;

    if ( strncmp( str, "0x", 2 ) == 0 )
    {
        is_hex = 1;
        str += 2;
    }

    string_to_unsigned( str, (uint8_t)strlen(str), &val, is_hex );

    return val;
}

/**
 * Converts a decimal/hexidecimal string (with optional sign) to a signed long int
 * Better than strtol or atol or atoi because the return value indicates if an error occurred
 *
 * @param string[in]     : The string buffer to be converted
 * @param str_length[in] : The maximum number of characters to process in the string buffer
 * @param value_out[out] : The unsigned in that will receive value of the the decimal string
 * @param is_hex[in]     : 0 = Decimal string, 1 = Hexidecimal string
 *
 * @return the number of characters successfully converted (including sign).  i.e. 0 = error
 *
 */
uint8_t string_to_signed( const char* string, uint8_t str_length, int32_t* value_out, uint8_t is_hex )
{
    uint8_t characters_processed = 0;
    uint8_t retval;
    char    first_char;

    if ( string == NULL )
    {
        return 0;
    }

    first_char = *string;

    if ( ( first_char == '-' ) || ( *string == '+' ) )
    {
        characters_processed++;
        string++;
        str_length--;
    }

    retval = string_to_generic( string, str_length, (uint32_t*)value_out, 0, is_hex );
    if ( retval == 0 )
    {
        return 0;
    }

    if ( first_char == '-' )
    {
        *value_out = -(*value_out);
    }
    return (uint8_t) ( characters_processed + retval );
}


/**
 * Converts a decimal/hexidecimal string to an unsigned long int
 * Better than strtol or atol or atoi because the return value indicates if an error occurred
 *
 * @param string[in]     : The string buffer to be converted
 * @param str_length[in] : The maximum number of characters to process in the string buffer
 * @param value_out[out] : The unsigned in that will receive value of the the decimal string
 * @param is_hex[in]     : 0 = Decimal string, 1 = Hexidecimal string
 *
 * @return the number of characters successfully converted.  i.e. 0 = error
 *
 */
uint8_t string_to_unsigned( const char* string, uint8_t str_length, uint32_t* value_out, uint8_t is_hex )
{
    return string_to_generic( string, str_length,  value_out, 1, is_hex );
}

/**
 * Converts a unsigned long int to a decimal string
 *
 * @param value[in]      : The unsigned long to be converted
 * @param output[out]    : The buffer which will receive the decimal string
 * @param min_length[in] : the minimum number of characters to output (zero padding will apply if required).
 * @param max_length[in] : the maximum number of characters to output (up to 10 ). There must be space for terminating NULL.
 *
 * @note: A terminating NULL is added. Wnsure that there is space in the buffer for this.
 *
 * @return the number of characters returned (excluding terminating null)
 *
 */
uint8_t unsigned_to_decimal_string( uint32_t value, char* output, uint8_t min_length, uint8_t max_length )
{
    uint8_t digits_left;
    char buffer[ 10 ] = "0000000000";
    max_length = MIN( max_length, sizeof( buffer ) );
    digits_left = max_length;
    while ( ( value != 0 ) && ( digits_left != 0 ) )
    {
        --digits_left;
        buffer[ digits_left ] = (char) (( value % 10 ) + '0');
        value = value / 10;
    }

    digits_left = (uint8_t) MIN( ( max_length - min_length ), digits_left );
    memcpy( output, &buffer[ digits_left ], (size_t)( max_length - digits_left ) );

    /* Add terminating null */
    output[( max_length - digits_left )] = '\x00';

    return (uint8_t) ( max_length - digits_left );
}


/**
 * Converts a signed long int to a decimal string
 *
 * @param value[in]      : The signed long to be converted
 * @param output[out]    : The buffer which will receive the decimal string
 * @param min_length[in] : the minimum number of characters to output (zero padding will apply if required)
 * @param max_length[in] : the maximum number of characters to output (up to 10 ). There must be space for terminating NULL.
 *
 * @note: A terminating NULL is added. Wnsure that there is space in the buffer for this.
 *
 * @return the number of characters returned.
 *
 */
uint8_t signed_to_decimal_string( int32_t value, char* output, uint8_t min_length, uint8_t max_length )
{
    uint8_t retval = 0;
    if ( ( value < 0 ) && ( max_length > 0 ) )
    {
        *output = '-';
        output++;
        max_length--;
        value = -value;
        retval++;
    }
    retval = (uint8_t) ( retval + unsigned_to_decimal_string( (uint32_t)value, output, min_length, max_length ) );
    return retval;
}




/**
 * Converts a unsigned long int to a hexidecimal string
 *
 * @param value[in]      : The unsigned long to be converted
 * @param output[out]    : The buffer which will receive the hexidecimal string
 * @param min_length[in] : the minimum number of characters to output (zero padding will apply if required)
 * @param max_length[in] : the maximum number of characters to output (up to 8 ) There must be space for terminating NULL.
 *
 * @note: A terminating NULL is added. Wnsure that there is space in the buffer for this.
 * @note: No leading '0x' is added.
 *
 * @return the number of characters returned.
 *
 */
uint8_t unsigned_to_hex_string( uint32_t value, char* output, uint8_t min_length, uint8_t max_length )
{
    uint8_t digits_left;
    char buffer[ 8 ] = "00000000";
    max_length = MIN( max_length, sizeof( buffer ) );
    digits_left = max_length;
    while ( ( value != 0 ) && ( digits_left != 0 ) )
    {
        --digits_left;
        buffer[ digits_left ] = nibble_to_hexchar( value & 0x0000000F );
        value = value >> 4;
    }

    digits_left = (uint8_t) MIN( ( max_length - min_length ), digits_left );
    memcpy( output, &buffer[ digits_left ], (size_t)( max_length - digits_left ) );

    /* Add terminating null */
    output[( max_length - digits_left )] = '\x00';

    return (uint8_t) ( max_length - digits_left );
}

wiced_result_t wiced_tcp_stream_write_resource( wiced_tcp_stream_t* tcp_stream, const resource_hnd_t* res_id )
{
    const void*   data;
    uint32_t   res_size;
    wiced_result_t    result;
    uint32_t pos = 0;

    do
    {
        resource_result_t resource_result = resource_get_readonly_buffer ( res_id, pos, 0x7fffffff, &res_size, &data );
        if ( resource_result != RESOURCE_SUCCESS )
        {
            return result;
        }

        result = wiced_tcp_stream_write( tcp_stream, data, (uint16_t) res_size );
        resource_free_readonly_buffer( res_id, data );
        if ( result != WICED_SUCCESS )
        {
            return result;
        }
        pos += res_size;
    } while ( res_size > 0 );

    return result;
}

int is_digit_str( const char* str )
{
    int res = 0;
    int i = 0;

    if ( str != NULL )
    {
        i = (int)strlen(str);
        res = 1;
        while ( i > 0 )
        {
            if ( !IS_DIGIT(*str) )
            {
                res = 0;
                break;
            }
            str++;
            i--;
        }
    }

    return res;
}


/*!
 ******************************************************************************
 * Prints partial details of a scan result on a single line
 *
 * @param[in] record  A pointer to the wiced_scan_result_t record
 *
 */
void print_scan_result( wiced_scan_result_t* record )
{
    WPRINT_APP_INFO( ( "%5s ", ( record->bss_type == WICED_BSS_TYPE_ADHOC ) ? "Adhoc" : "Infra" ) );
    WPRINT_APP_INFO( ( "%02X:%02X:%02X:%02X:%02X:%02X ", record->BSSID.octet[0], record->BSSID.octet[1], record->BSSID.octet[2], record->BSSID.octet[3], record->BSSID.octet[4], record->BSSID.octet[5] ) );
    WPRINT_APP_INFO( ( " %d ", record->signal_strength ) );
    if ( record->max_data_rate < 100000 )
    {
        WPRINT_APP_INFO( ( " %.1f ", (double) (record->max_data_rate / 1000.0) ) );
    }
    else
    {
        WPRINT_APP_INFO( ( "%.1f ", (double) (record->max_data_rate / 1000.0) ) );
    }
    WPRINT_APP_INFO( ( " %3d  ", record->channel ) );
    WPRINT_APP_INFO( ( "%-15s ", ( record->security == WICED_SECURITY_OPEN             ) ? "Open                 " :
                                 ( record->security == WICED_SECURITY_WEP_PSK          ) ? "WEP                  " :
                                 ( record->security == WICED_SECURITY_WPA_TKIP_PSK     ) ? "WPA  TKIP  PSK       " :
                                 ( record->security == WICED_SECURITY_WPA_AES_PSK      ) ? "WPA  AES   PSK       " :
                                 ( record->security == WICED_SECURITY_WPA_MIXED_PSK    ) ? "WPA  Mixed PSK       " :
                                 ( record->security == WICED_SECURITY_WPA2_AES_PSK     ) ? "WPA2 AES   PSK       " :
                                 ( record->security == WICED_SECURITY_WPA2_TKIP_PSK    ) ? "WPA2 TKIP  PSK       " :
                                 ( record->security == WICED_SECURITY_WPA2_MIXED_PSK   ) ? "WPA2 Mixed PSK       " :
                                 ( record->security == WICED_SECURITY_WPA_TKIP_ENT     ) ? "WPA  TKIP  Enterprise" :
                                 ( record->security == WICED_SECURITY_WPA_AES_ENT      ) ? "WPA  AES   Enterprise" :
                                 ( record->security == WICED_SECURITY_WPA_MIXED_ENT    ) ? "WPA  Mixed Enterprise" :
                                 ( record->security == WICED_SECURITY_WPA2_TKIP_ENT    ) ? "WPA2 TKIP  Enterprise" :
                                 ( record->security == WICED_SECURITY_WPA2_AES_ENT     ) ? "WPA2 AES   Enterprise" :
                                 ( record->security == WICED_SECURITY_WPA2_MIXED_ENT   ) ? "WPA2 Mixed Enterprise" :
                                                                                         "Unknown              " ) );
    WPRINT_APP_INFO( ( " %-32s ", record->SSID.value ) );
    WPRINT_APP_INFO( ( "\n" ) );
}

/*
 ******************************************************************************
 * Convert an ipv4 string to a uint32_t.
 *
 * @param     arg  The string containing the value.
 * @param     arg  The structure which will receive the IP address
 *
 * @return    0 if read successfully
 */
int str_to_ip( const char* arg, wiced_ip_address_t* address )
{
    uint32_t* addr = &address->ip.v4;
    uint8_t num = 0;

    arg--;

    *addr = 0;

    do
    {
        uint32_t tmp_val = 0;
        *addr = *addr << 8;
        string_to_unsigned( ++arg, 3, &tmp_val, 0 );
        *addr += (uint32_t) tmp_val;
        while ( ( *arg != '\x00' ) && ( *arg != '.' ) )
        {
            arg++;
        }
        num++;
    } while ( ( num < 4 ) && ( *arg != '\x00' ) );
    if ( num == 4 )
    {

        address->version = WICED_IPV4;
        return 0;
    }
    return -1;
}


void format_wep_keys( char* wep_key_ouput, const char* wep_key_data, uint8_t* wep_key_length, wep_key_format_t wep_key_format )
{
    int              a;
    uint8_t          wep_key_entry_size;
    wiced_wep_key_t* wep_key = (wiced_wep_key_t*)wep_key_ouput;

    /* Setup WEP key 0 */
    wep_key[0].index  = 0;

    if ( wep_key_format == WEP_KEY_HEX_FORMAT )
    {
        wep_key[0].length = *wep_key_length >> 1;
        for ( a = 0; a < wep_key[0].length; ++a )
        {
            uint8_t nibble1 = 0;
            uint8_t nibble2 = 0;
            if ( hexchar_to_nibble( wep_key_data[a*2],     &nibble1 ) == -1 ||
                 hexchar_to_nibble( wep_key_data[a*2 + 1], &nibble2 ) == -1    )
            {
                WPRINT_APP_INFO( ( "Error - invalid hex character function: %s line: %u ", __FUNCTION__, __LINE__ ) );
            }
            wep_key[0].data[a] = (uint8_t)(( nibble1 << 4 ) | nibble2);
        }
    }
    else
    {
        wep_key[0].length = *wep_key_length;
        memcpy( wep_key[0].data, wep_key_data, *wep_key_length );
    }

    /* Calculate the size of each WEP key entry */
    wep_key_entry_size = (uint8_t) ( 2 + *wep_key_length );

    /* Duplicate WEP key 0 for keys 1 to 3 */
    wep_key = (wiced_wep_key_t*)((char*)wep_key + wep_key_entry_size);
    memcpy( wep_key, wep_key_ouput, wep_key_entry_size );
    wep_key->index = 1;

    wep_key = (wiced_wep_key_t*)((char*)wep_key + wep_key_entry_size);
    memcpy( wep_key, wep_key_ouput, wep_key_entry_size );
    wep_key->index = 2;

    wep_key = (wiced_wep_key_t*)((char*)wep_key + wep_key_entry_size);
    memcpy( wep_key, wep_key_ouput, wep_key_entry_size );
    wep_key->index = 3;

    *wep_key_length = (uint8_t) ( 4 * wep_key_entry_size );
}
