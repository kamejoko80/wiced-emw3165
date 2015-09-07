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
 * Crypto Test Application
 *
 * This application demonstrates how to use crypto functions available
 * in the WICED crypto library
 *
 * Features demonstrated
 *  - AES encrypt/decrypt
 *  - MD5, SHA1 and SHA256 Hashing and HMAC functionality
 *
 * Application Instructions
 *   Connect a PC terminal to the serial port of the WICED Eval board,
 *   then build and download the application as described in the WICED
 *   Quick Start Guide
 *
 *   The app exercises various crypto functions and prints
 *   the results to the UART.
 *
 */

/*
    MD5 Test Vectors from RFC1321:

    A.5 Test suite

   The MD5 test suite (driver option "-x") should print the following
   results:

    MD5 test suite:
    MD5 ("") = d41d8cd98f00b204e9800998ecf8427e
    MD5 ("a") = 0cc175b9c0f1b6a831c399e269772661
    MD5 ("abc") = 900150983cd24fb0d6963f7d28e17f72
    MD5 ("message digest") = f96b697d7cb7938d525a2f31aaf161d0
    MD5 ("abcdefghijklmnopqrstuvwxyz") = c3fcd3d76192e4007dfb496cca67e13b
    MD5 ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") =
        d174ab98d277d9f5a5611c2c9f419d9f
    MD5 ("12345678901234567890123456789012345678901234567890123456789012345678901234567890") =
        57edf4a22be3c955ac49da2e2107b67a


    SHA-1 Test Vectors from FIPS180-2

    SHA1 ("abc") = a9993e36 4706816a ba3e2571 7850c26c 9cd0d89d
    SHA1 ("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") =
        84983e44 1c3bd26e baae4aa1 f95129e5 e54670f1
    SHA-1 (1,000,000 'a's) = 34aa973c d4c4daa4 f61eeb2b dbad2731 6534016f


    HMAC-MD5 test vectors from http://www.ietf.org/rfc/rfc2202.txt

    Test Vectors (Trailing '\0' of a character string not included in test):

    key =           0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b
    key_len =       16 bytes
    data =          "Hi There"
    data_len =      8  bytes
    digest =        0x9294727a3638bb1c13f48ef8158bfc9d

    key =           "Jefe"
    data =          "what do ya want for nothing?"
    data_len =      28 bytes
    digest =        0x750c783e6ab0b503eaa86e310a5db738

    key =           0xAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    key_len         16 bytes
    data =          0xDDDDDDDDDDDDDDDDDDDD...
                    ..DDDDDDDDDDDDDDDDDDDD...
                    ..DDDDDDDDDDDDDDDDDDDD...
                    ..DDDDDDDDDDDDDDDDDDDD...
                    ..DDDDDDDDDDDDDDDDDDDD
    data_len =      50 bytes
    digest =        0x56be34521d144c88dbb8c733f0e8b3f6

    test_case =     4
    key =           0x0102030405060708090a0b0c0d0e0f10111213141516171819
    key_len         25
    data =          0xcd repeated 50 times
    data_len =      50
    digest =        0x697eaf0aca3a3aea3a75164746ffaa79

    test_case =     5
    key =           0x0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c
    key_len =       16
    data =          "Test With Truncation"
    data_len =      20
    digest =        0x56461ef2342edc00f9bab995690efd4c
    digest-96       0x56461ef2342edc00f9bab995

    test_case =     6
    key =           0xaa repeated 80 times
    key_len =       80
    data =          "Test Using Larger Than Block-Size Key - Hash Key First"
    data_len =      54
    digest =        0x6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd

    test_case =     7
    key =           0xaa repeated 80 times
    key_len =       80
    data =          "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data"
    data_len =      73
    digest =        0x6f630fad67cda0ee1fb1f562db3aa53e


    HMAC-SHA1 test vectors from http://www.ietf.org/rfc/rfc2202.txt

    test_case =     1
    key =           0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b
    key_len =       20
    data =          "Hi There"
    data_len =      8
    digest =        0xb617318655057264e28bc0b6fb378c8ef146be00

    test_case =     2
    key =           "Jefe"
    key_len =       4
    data =          "what do ya want for nothing?"
    data_len =      28
    digest =        0xeffcdf6ae5eb2fa2d27416d5f184df9c259a7c79

    test_case =     3
    key =           0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
    key_len =       20
    data =          0xdd repeated 50 times
    data_len =      50
    digest =        0x125d7342b9ac11cd91a39af48aa17b4f63f175d3

    test_case =     4
    key =           0x0102030405060708090a0b0c0d0e0f10111213141516171819
    key_len =       25
    data =          0xcd repeated 50 times
    data_len =      50
    digest =        0x4c9007f4026250c6bc8414f9bf50c86c2d7235da

    test_case =     5
    key =           0x0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c
    key_len =       20
    data =          "Test With Truncation"
    data_len =      20
    digest =        0x4c1a03424b55e07fe7f27be1d58bb9324a9a5a04
    digest-96 =     0x4c1a03424b55e07fe7f27be1

    test_case =     6
    key =           0xaa repeated 80 times
    key_len =       80
    data =          "Test Using Larger Than Block-Size Key - Hash Key First"
    data_len =      54
    digest =        0xaa4ae5e15272d00e95705637ce8a3b55ed402112

    test_case =     7
    key =           0xaa repeated 80 times
    key_len =       80
    data =          "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data"
    data_len =      73
    digest =        0xe8e99d0f45237d786d6bbaa7965c7808bbff1a91
    data_len =      20
    digest =        0x4c1a03424b55e07fe7f27be1d58bb9324a9a5a04
    digest-96 =     0x4c1a03424b55e07fe7f27be1


    SHA-256 Example (MultiBlock Message) from FIPS 180-2 Appendix B2

    Message 448-bit (l = 448) ASCII string

    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"

    Result

    248d6a61 d20638b8 e5c02693 0c3e6039 a33ce459 64ff2167 f6ecedd4 19db06c1


    HMAC-SHA-256 examples from http://tools.ietf.org/html/rfc4868, 2.7.2.1.  SHA256 Authentication Test Vectors

    Test Case AUTH256-1:
    Key =          0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b
                   0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b  (32 bytes)

    Data =         4869205468657265                  ("Hi There")

    PRF-HMAC-SHA-256 = 198a607eb44bfbc69903a0f1cf2bbdc5
                       ba0aa3f3d9ae3c1c7a3b1696a0b68cf7

    HMAC-SHA-256-128 = 198a607eb44bfbc69903a0f1cf2bbdc5

    Test Case AUTH256-2:
    Key =          4a6566654a6566654a6566654a656665  ("JefeJefeJefeJefe")
                   4a6566654a6566654a6566654a656665  ("JefeJefeJefeJefe")

    Data =         7768617420646f2079612077616e7420  ("what do ya want ")
                   666f72206e6f7468696e673f          ("for nothing?")

    PRF-HMAC-SHA-256 = 167f928588c5cc2eef8e3093caa0e87c
                       9ff566a14794aa61648d81621a2a40c6

    HMAC-SHA-256-128 = 167f928588c5cc2eef8e3093caa0e87c

    Test Case AUTH256-3:
    Key =          aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
                   aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  (32 bytes)

    Data =         dddddddddddddddddddddddddddddddd
                   dddddddddddddddddddddddddddddddd
                   dddddddddddddddddddddddddddddddd
                   dddd                              (50 bytes)

    PRF-HMAC-SHA-256 = cdcb1220d1ecccea91e53aba3092f962
                       e549fe6ce9ed7fdc43191fbde45c30b0

    HMAC-SHA-256-128 = cdcb1220d1ecccea91e53aba3092f962

    Test Case AUTH256-4:
    Key =          0102030405060708090a0b0c0d0e0f10
                   1112131415161718191a1b1c1d1e1f20  (32 bytes)

    Data =         cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd
                   cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd
                   cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd
                   cdcd                              (50 bytes)

    PRF-HMAC-SHA-256 = 372efcf9b40b35c2115b1346903d2ef4
                       2fced46f0846e7257bb156d3d7b30d3f

    HMAC-SHA-256-128 = 372efcf9b40b35c2115b1346903d2ef4

*/


#include <stdlib.h>
#include "wiced.h"
#include "wiced_security.h"
#include "wwd_debug.h"
#include "string.h"
#include "wiced_time.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AES_128_CBC_SALT (uint8_t*)"\xC1\x5F\x27\x03\x26\x64\x2D\x72"
#define AES_128_CBC_KEY  (uint8_t*)"\xBB\x41\xFA\x96\x2E\x5E\xDD\xF4\xAB\x54\x0B\xD6\x0B\x57\xA8\x8A"
#define AES_128_CBC_IV   (uint8_t*)"\x4A\xB6\x86\x6D\x0D\x9E\x8E\xE7\x99\x31\x64\xB3\x88\x5A\xA7\xE6"
#define NUM_OF_HMAC_MD5_TESTS 7
#define NUM_OF_HMAC_SHA1_TESTS 7
#define NUM_OF_HMAC_SHA256_TESTS 4
#define MD5_LENGTH      16              /* Length of the MD5 hash */
#define SHA1_LENGTH     20              /* Length of the SHA1 hash */
#define SHA256_LENGTH   32              /* Length of the SHA256 hash */

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

void sw_md5_test(void);
void sw_sha1_test(void);
void sw_md5_hmac_test(void);
void sw_sha1_hmac_test(void);
void sw_sha256_test(void);
void sw_hmac_sha256_test(void);
void dump_bytes(const uint8_t* bptr, uint32_t len);

/******************************************************
 *               Variable Definitions
 ******************************************************/

char* secret_information = "This is secret!!";
aes_context_t  context_aes;
sha1_context context_sha1;
sha2_context context_sha2;
md5_context  context_md5;

int pass_count = 0, fail_count = 0;

char *md5_test_vectors[] = {
    "",
    "a",
    "abc",
    "message digest",
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
    NULL
};

uint8_t md5_expected_results[] = {
    0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
    0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e,
    0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
    0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61,
    0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
    0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72,
    0xf9, 0x6b, 0x69, 0x7d, 0x7c, 0xb7, 0x93, 0x8d,
    0x52, 0x5a, 0x2f, 0x31, 0xaa, 0xf1, 0x61, 0xd0,
    0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00,
    0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b,
    0xd1, 0x74, 0xab, 0x98, 0xd2, 0x77, 0xd9, 0xf5,
    0xa5, 0x61, 0x1c, 0x2c, 0x9f, 0x41, 0x9d, 0x9f,
    0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55,
    0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a,
};

char *sha1_test_vectors[] = {
    "abc",
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
    NULL
};

uint8_t sha1_expected_results[] = {
    0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a,
    0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c,
    0x9c, 0xd0, 0xd8, 0x9d, 0x84, 0x98, 0x3e, 0x44,
    0x1c, 0x3b, 0xd2, 0x6e, 0xba, 0xae, 0x4a, 0xa1,
    0xf9, 0x51, 0x29, 0xe5, 0xe5, 0x46, 0x70, 0xf1
};

char *sha256_test_vectors[] = {
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
    NULL
};

uint8_t sha256_expected_results[] = {
    0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
    0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
    0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
    0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1,
};

/* For hmac test cases the key and/or data may be a hex or character string. If a hex value is in use then the char string for that key or data field will be null. */

struct hmac_md5_test_case
{
    int num;
    uint8_t hex_key[128];
    char *char_key;
    int key_len;
    uint8_t hex_data[128];
    char *char_data;
    int data_len;
    uint8_t digest[MD5_LENGTH];
} hmac_md5_test_cases[NUM_OF_HMAC_MD5_TESTS] =
{
    {1,
    {0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b}, NULL, 16,
    {}, "Hi There", 8,
    {0x92, 0x94, 0x72, 0x7a, 0x36, 0x38, 0xbb, 0x1c, 0x13, 0xf4, 0x8e, 0xf8, 0x15, 0x8b, 0xfc, 0x9d}},
    {2,
    {}, "Jefe", 4,
    {}, "what do ya want for nothing?", 28,
    {0x75, 0x0c, 0x78, 0x3e, 0x6a, 0xb0, 0xb5, 0x03, 0xea, 0xa8, 0x6e, 0x31, 0x0a, 0x5d, 0xb7, 0x38}},
    {3,
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}, NULL, 16,
    {0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
     0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
     0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
     0xdd, 0xdd}, NULL, 50,
    {0x56, 0xbe, 0x34, 0x52, 0x1d, 0x14, 0x4c, 0x88, 0xdb, 0xb8, 0xc7, 0x33, 0xf0, 0xe8, 0xb3, 0xf6}},
    {4,
    {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
     0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19}, NULL, 25,
    {0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
     0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
     0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
     0xcd, 0xcd}, NULL, 50,
    {0x69, 0x7e, 0xaf, 0x0a, 0xca, 0x3a, 0x3a, 0xea, 0x3a, 0x75, 0x16, 0x47, 0x46, 0xff, 0xaa, 0x79}},
    {5,
    {0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c}, NULL, 16,
    {}, "Test With Truncation", 20,
    {0x56, 0x46, 0x1e, 0xf2, 0x34, 0x2e, 0xdc, 0x00, 0xf9, 0xba, 0xb9, 0x95, 0x69, 0x0e, 0xfd, 0x4c}},
    {6,
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}, NULL, 80,
    {}, "Test Using Larger Than Block-Size Key - Hash Key First", 54,
    {0x6b, 0x1a, 0xb7, 0xfe, 0x4b, 0xd7, 0xbf, 0x8f, 0x0b, 0x62, 0xe6, 0xce, 0x61, 0xb9, 0xd0, 0xcd}},
    {7,
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}, NULL, 80,
    {}, "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data", 73,
    {0x6f, 0x63, 0x0f, 0xad, 0x67, 0xcd, 0xa0, 0xee, 0x1f, 0xb1, 0xf5, 0x62, 0xdb, 0x3a, 0xa5, 0x3e}}
};

struct hmac_sha1_test_case {
    int num;
    uint8_t hex_key[128];
    char *char_key;
    int key_len;
    uint8_t hex_data[128];
    char *char_data;
    int data_len;
    uint8_t digest[SHA1_LENGTH];
} hmac_sha1_test_cases[NUM_OF_HMAC_SHA1_TESTS] = {
    {1,
    {0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b}, NULL, 20,
    {}, "Hi There", 8,
    {0xb6, 0x17, 0x31, 0x86, 0x55, 0x05, 0x72, 0x64, 0xe2, 0x8b, 0xc0, 0xb6, 0xfb, 0x37, 0x8c, 0x8e, 0xf1, 0x46, 0xbe, 0x00}},
    {2,
    {}, "Jefe", 4,
    {}, "what do ya want for nothing?", 28,
    {0xef, 0xfc, 0xdf, 0x6a, 0xe5, 0xeb, 0x2f, 0xa2, 0xd2, 0x74, 0x16, 0xd5, 0xf1, 0x84, 0xdf, 0x9c, 0x25, 0x9a, 0x7c, 0x79}},
    {3,
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}, NULL, 20,
    {0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd}, NULL, 50,
    {0x12, 0x5d, 0x73, 0x42, 0xb9, 0xac, 0x11, 0xcd, 0x91, 0xa3, 0x9a, 0xf4, 0x8a, 0xa1, 0x7b, 0x4f, 0x63, 0xf1, 0x75, 0xd3}},
    {4,
    {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
     0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19}, NULL, 25,
    {0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
     0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
     0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
     0xcd, 0xcd}, NULL, 50,
    {0x4c, 0x90, 0x07, 0xf4, 0x02, 0x62, 0x50, 0xc6, 0xbc, 0x84, 0x14, 0xf9, 0xbf, 0x50, 0xc8, 0x6c, 0x2d, 0x72, 0x35, 0xda}},
    {5,
    {0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c}, NULL, 20,
    {}, "Test With Truncation", 20,
    {0x4c, 0x1a, 0x03, 0x42, 0x4b, 0x55, 0xe0, 0x7f, 0xe7, 0xf2, 0x7b, 0xe1, 0xd5, 0x8b, 0xb9, 0x32, 0x4a, 0x9a, 0x5a, 0x04}},
    {6,
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}, NULL, 80,
    {}, "Test Using Larger Than Block-Size Key - Hash Key First", 54,
    {0xaa, 0x4a, 0xe5, 0xe1, 0x52, 0x72, 0xd0, 0x0e, 0x95, 0x70, 0x56, 0x37, 0xce, 0x8a, 0x3b, 0x55, 0xed, 0x40, 0x21, 0x12}},
    {7,
    {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}, NULL, 80,
    {}, "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data", 73,
    {0xe8, 0xe9, 0x9d, 0x0f, 0x45, 0x23, 0x7d, 0x78, 0x6d, 0x6b, 0xba, 0xa7, 0x96, 0x5c, 0x78, 0x08, 0xbb, 0xff, 0x1a, 0x91}}
};


struct hmac_sha256_test_case {
    int num;
    char label[32];
    uint8_t hex_key[128];
    char* char_key;
    int key_len;
    uint8_t hex_data[128];
    char* char_data;
    int data_len;
    uint8_t digest[32];
} hmac_sha256_test_cases[NUM_OF_HMAC_SHA256_TESTS] = {
    {
        .num = 1,
        .label = "Test Case AUTH256-1",
        .hex_key = {
                0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
            },
        .char_key = NULL,
        .key_len = 32,
        .hex_data = {
                0x48, 0x69, 0x20, 0x54, 0x68, 0x65, 0x72, 0x65,
            },
        .char_data = "Hi There",
        .data_len = 8,
        .digest = {
                0x19, 0x8a, 0x60, 0x7e, 0xb4, 0x4b, 0xfb, 0xc6,
                0x99, 0x03, 0xa0, 0xf1, 0xcf, 0x2b, 0xbd, 0xc5,
                0xba, 0x0a, 0xa3, 0xf3, 0xd9, 0xae, 0x3c, 0x1c,
                0x7a, 0x3b, 0x16, 0x96, 0xa0, 0xb6, 0x8c, 0xf7
            },
    },
    {
        .num = 2,
        .label = "Test Case AUTH256-2",
        .hex_key = {
                0x4a, 0x65, 0x66, 0x65, 0x4a, 0x65, 0x66, 0x65,
                0x4a, 0x65, 0x66, 0x65, 0x4a, 0x65, 0x66, 0x65,
                0x4a, 0x65, 0x66, 0x65, 0x4a, 0x65, 0x66, 0x65,
                0x4a, 0x65, 0x66, 0x65, 0x4a, 0x65, 0x66, 0x65,
            },
        .char_key = "JefeJefeJefeJefeJefeJefeJefeJefe",
        .key_len = 32,
        .hex_data = {
                0x77, 0x68, 0x61, 0x74, 0x20, 0x64, 0x6f, 0x20,
                0x79, 0x61, 0x20, 0x77, 0x61, 0x6e, 0x74, 0x20,
                0x66, 0x6f, 0x72, 0x20, 0x6e, 0x6f, 0x74, 0x68,
                0x69, 0x6e, 0x67, 0x3f
            },
        .char_data = "what do ya want for nothing?",
        .data_len = 28,
        .digest = {
                0x16, 0x7f, 0x92, 0x85, 0x88, 0xc5, 0xcc, 0x2e,
                0xef, 0x8e, 0x30, 0x93, 0xca, 0xa0, 0xe8, 0x7c,
                0x9f, 0xf5, 0x66, 0xa1, 0x47, 0x94, 0xaa, 0x61,
                0x64, 0x8d, 0x81, 0x62, 0x1a, 0x2a, 0x40, 0xc6,
            },
    },
    {
        .num = 3,
        .label = "Test Case AUTH256-3",
        .hex_key = {
                0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
                0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
                0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
                0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
            },
        .char_key = NULL,
        .key_len = 32,
        .hex_data = {
                0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
                0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
                0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
                0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
                0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
                0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
                0xdd, 0xdd,
            },
        .char_data = NULL,
        .data_len = 50,
        .digest = {
                0xcd, 0xcb, 0x12, 0x20, 0xd1, 0xec, 0xcc, 0xea,
                0x91, 0xe5, 0x3a, 0xba, 0x30, 0x92, 0xf9, 0x62,
                0xe5, 0x49, 0xfe, 0x6c, 0xe9, 0xed, 0x7f, 0xdc,
                0x43, 0x19, 0x1f, 0xbd, 0xe4, 0x5c, 0x30, 0xb0,
            },
    },
    {
        .num = 4,
        .label = "Test Case AUTH256-4",
        .hex_key = {
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
                0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
            },
        .char_key = NULL,
        .key_len = 32,
        .hex_data = {
                0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
                0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
                0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
                0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
                0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
                0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
                0xcd, 0xcd,
            },
        .char_data = NULL,
        .data_len = 50,
        .digest = {
                0x37, 0x2e, 0xfc, 0xf9, 0xb4, 0x0b, 0x35, 0xc2,
                0x11, 0x5b, 0x13, 0x46, 0x90, 0x3d, 0x2e, 0xf4,
                0x2f, 0xce, 0xd4, 0x6f, 0x08, 0x46, 0xe7, 0x25,
                0x7b, 0xb1, 0x56, 0xd3, 0xd7, 0xb3, 0x0d, 0x3f,
            },
    },
};


/******************************************************
 *               Function Definitions
 ******************************************************/


void sw_md5_test(void)
{
    md5_context md5_ctx;
    wiced_time_t t1, t2;
    uint8_t hash_value[MD5_LENGTH];
    char **test_vector_ptr = md5_test_vectors;
    int i = 0;

    WPRINT_APP_INFO( ( "\nSoftware MD5 Hash Tests\n" ) );

    while (test_vector_ptr[i] != NULL)
    {
        wiced_time_get_time( &t1 );

        md5_starts( &md5_ctx );
        md5_update( &md5_ctx, (unsigned char *)test_vector_ptr[i], strlen(test_vector_ptr[i]) );
        md5_finish( &md5_ctx, (unsigned char *)hash_value );

        wiced_time_get_time( &t2 );
        t2 = t2 - t1;

        WPRINT_APP_INFO( ( "\nMD5(%s) =  \n", test_vector_ptr[i] ) );
        dump_bytes( hash_value, MD5_LENGTH );
        if (memcmp(hash_value, &md5_expected_results[i * MD5_LENGTH], MD5_LENGTH) == 0)
        {
            WPRINT_APP_INFO( ( "MD5 Success\n" ) );
            pass_count++;
        }
        else
        {
           WPRINT_APP_INFO( ( "MD5 Failure\n" ) );
           fail_count++;
        }
        WPRINT_APP_INFO( ( "Time for MD5 hash feed and peek = %u ms\n", (unsigned int)t2 ) );
        i++;
    }
}


void sw_sha1_test(void)
{
    sha1_context sha1_ctx;
    wiced_time_t t1, t2;
    uint8_t hash_value[SHA1_LENGTH];
    char **test_vector_ptr = sha1_test_vectors;
    int i = 0;

    WPRINT_APP_INFO( ( "\nSoftware SHA-1 Hash Tests\n" ) );

    while (test_vector_ptr[i] != NULL)
    {
        wiced_time_get_time( &t1 );

        sha1_starts( &sha1_ctx );
        sha1_update( &sha1_ctx, (unsigned char *)test_vector_ptr[i], strlen(test_vector_ptr[i]) );
        sha1_finish( &sha1_ctx, (unsigned char *)hash_value );

        wiced_time_get_time( &t2 );
        t2 = t2 - t1;

        WPRINT_APP_INFO( ( "\nSHA-1(%s) =  \n", test_vector_ptr[i] ) );
        dump_bytes( hash_value, SHA1_LENGTH );
        if (memcmp(hash_value, &sha1_expected_results[i * SHA1_LENGTH], SHA1_LENGTH) == 0)
        {
            WPRINT_APP_INFO( ( "SHA-1 Success\n" ) );
            pass_count++;
        }
        else
        {
           WPRINT_APP_INFO( ( "SHA-1 Failure\n" ) );
           fail_count++;
        }
        WPRINT_APP_INFO( ( "Time for SHA-1 hash feed and peek = %u ms\n", (unsigned int)t2 ) );
        i++;
    }


    /* A.3 SHA-1 Example (Long Message) */

    char long_test_vector [] =
    {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" /* 100 'a's */
    };

    uint8_t long_expected_result [] = {
        0x34, 0xaa, 0x97, 0x3c, 0xd4, 0xc4, 0xda, 0xa4,
        0xf6, 0x1e, 0xeb, 0x2b, 0xdb, 0xad, 0x27, 0x31,
        0x65, 0x34, 0x01, 0x6f
    };

    wiced_time_get_time( &t1 );

    sha1_starts( &sha1_ctx );

    for (i = 0; i < 10000; i++)
    {
        sha1_update( &sha1_ctx, (unsigned char *)&long_test_vector, strlen(long_test_vector) );
    }
    sha1_finish( &sha1_ctx, (unsigned char *)hash_value );

    wiced_time_get_time( &t2 );
    t2 = t2 - t1;

    WPRINT_APP_INFO( ( "\nSHA-1 of one million 'a's =\n" ) );
    dump_bytes( hash_value, SHA1_LENGTH );

    if (memcmp(hash_value, long_expected_result, SHA1_LENGTH) == 0)
    {
        WPRINT_APP_INFO( ( "SHA-1 Success\n" ) );
        pass_count++;
    }
    else
    {
       WPRINT_APP_INFO( ( "SHA-1 Failure\n" ) );
       fail_count++;
    }

    WPRINT_APP_INFO( ( "Time for SHA-1 hash feed and peek for one million 'a's = %u ms\n", (unsigned int)t2 ) );
}


void sw_md5_hmac_test(void)
{
    md5_context md5_hmac_ctx;
    wiced_time_t t1, t2, i;
    uint8_t result[MD5_LENGTH];

    WPRINT_APP_INFO( ( "\nSoftware HMAC-MD5 Tests\n" ) );

    for (i = 0; i < NUM_OF_HMAC_MD5_TESTS; i++)
    {
        WPRINT_APP_INFO( ( "\nHMAC-MD5 Test %d\n", hmac_md5_test_cases[i].num ) );
        if (hmac_md5_test_cases[i].char_key)
        {
            WPRINT_APP_INFO( ( "Key: %s\n", hmac_md5_test_cases[i].char_key ) );
            md5_hmac_starts( &md5_hmac_ctx, (unsigned char *)hmac_md5_test_cases[i].char_key, hmac_md5_test_cases[i].key_len );
        }
        else
        {
            WPRINT_APP_INFO( ( "Key: \n" ) );
            dump_bytes( hmac_md5_test_cases[i].hex_key, hmac_md5_test_cases[i].key_len );
            md5_hmac_starts( &md5_hmac_ctx, (unsigned char *)hmac_md5_test_cases[i].hex_key, hmac_md5_test_cases[i].key_len );
        }

        if (hmac_md5_test_cases[i].char_data)
        {
            WPRINT_APP_INFO( ( "Data: %s\n", hmac_md5_test_cases[i].char_data ) );
            wiced_time_get_time( &t1 );
            md5_hmac_update( &md5_hmac_ctx, (unsigned char *)hmac_md5_test_cases[i].char_data, hmac_md5_test_cases[i].data_len );
        }
        else
        {
            WPRINT_APP_INFO( ( "Data: \n" ) );
            dump_bytes( hmac_md5_test_cases[i].hex_data, hmac_md5_test_cases[i].data_len );
            wiced_time_get_time( &t1 );
            md5_hmac_update( &md5_hmac_ctx, (unsigned char *)hmac_md5_test_cases[i].hex_data, hmac_md5_test_cases[i].data_len );
        }

        md5_hmac_finish( &md5_hmac_ctx, (unsigned char *) result );
        wiced_time_get_time( &t2 );
        t2 = t2 - t1;

        WPRINT_APP_INFO( ( "Result: \n" ) );
        dump_bytes( result, MD5_LENGTH );

        if (memcmp(result, hmac_md5_test_cases[i].digest, MD5_LENGTH) == 0)
        {
            WPRINT_APP_INFO( ( "HMAC-MD5 Success\n" ) );
            pass_count++;
        }
        else
        {
           WPRINT_APP_INFO( ( "HMAC-MD5 Failure\n" ) );
           fail_count++;
        }

        WPRINT_APP_INFO( ( "Time for HMAC-MD5 hash feed and done = %u ms\n", (unsigned int)t2 ) );
    }
}


void sw_sha1_hmac_test(void)
{
    sha1_context sha1_hmac_ctx;
    wiced_time_t t1, t2, i;
    uint8_t result[SHA1_LENGTH];

    WPRINT_APP_INFO( ( "\nSoftware HMAC-SHA1 Tests\n" ) );

    for (i = 0; i < NUM_OF_HMAC_SHA1_TESTS; i++)
    {
        WPRINT_APP_INFO( ( "\nHMAC-SHA1 Test %d\n", hmac_sha1_test_cases[i].num ) );
        if (hmac_sha1_test_cases[i].char_key)
        {
            WPRINT_APP_INFO( ( "Key: %s\n", hmac_sha1_test_cases[i].char_key ) );
            sha1_hmac_starts( &sha1_hmac_ctx, (unsigned char *)hmac_sha1_test_cases[i].char_key, hmac_sha1_test_cases[i].key_len );
        }
        else
        {
            WPRINT_APP_INFO( ( "Key: \n" ) );
            dump_bytes( hmac_sha1_test_cases[i].hex_key, hmac_sha1_test_cases[i].key_len );
            sha1_hmac_starts( &sha1_hmac_ctx, (unsigned char *)hmac_sha1_test_cases[i].hex_key, hmac_sha1_test_cases[i].key_len );
        }

        if (hmac_sha1_test_cases[i].char_data)
        {
            WPRINT_APP_INFO( ( "Data: %s\n", hmac_sha1_test_cases[i].char_data ) );
            wiced_time_get_time( &t1 );
            sha1_hmac_update( &sha1_hmac_ctx, (unsigned char *)hmac_sha1_test_cases[i].char_data, hmac_sha1_test_cases[i].data_len );
        }
        else
        {
            WPRINT_APP_INFO( ( "Data: \n" ) );
            dump_bytes( hmac_sha1_test_cases[i].hex_data, hmac_sha1_test_cases[i].data_len );
            wiced_time_get_time( &t1 );
            sha1_hmac_update( &sha1_hmac_ctx, (unsigned char *)hmac_sha1_test_cases[i].hex_data, hmac_sha1_test_cases[i].data_len );
        }

        sha1_hmac_finish( &sha1_hmac_ctx, (unsigned char *)result );
        wiced_time_get_time( &t2 );
        t2 = t2 - t1;

        WPRINT_APP_INFO( ( "Result: \n" ) );
        dump_bytes( result, SHA1_LENGTH );

        if (memcmp(result, hmac_sha1_test_cases[i].digest, SHA1_LENGTH) == 0)
        {
            WPRINT_APP_INFO( ( "HMAC-SHA1 Success\n" ) );
            pass_count++;
        }
        else
        {
           WPRINT_APP_INFO( ( "HMAC-SHA1 Failure\n" ) );
           fail_count++;
        }

        WPRINT_APP_INFO( ( "Time for HMAC-SHA1 hash feed and done = %u ms\n", (unsigned int)t2 ) );
    }
}


void sw_sha256_test(void)
{
    wiced_time_t t1, t2;
    sha2_context sha256_ctx;
    uint8_t hash_value[SHA256_LENGTH];

    char **test_vector_ptr = sha256_test_vectors;
    int i = 0;

    WPRINT_APP_INFO( ( "\nSoftware SHA-256 Hash Test(s)\n" ) );

    while (test_vector_ptr[i] != NULL)
    {
        wiced_time_get_time( &t1 );

        sha2_starts( &sha256_ctx, 0 );
        sha2_update( &sha256_ctx, (unsigned char *)test_vector_ptr[i], strlen(test_vector_ptr[i]) );
        sha2_finish( &sha256_ctx, (unsigned char *)hash_value );

        wiced_time_get_time( &t2 );
        t2 = t2 - t1;

        WPRINT_APP_INFO( ( "\nSHA-256(%s) =  \n", test_vector_ptr[i] ) );
        dump_bytes( hash_value, SHA256_LENGTH );
        if (memcmp(hash_value, &sha256_expected_results[i * SHA256_LENGTH], SHA256_LENGTH) == 0)
        {
            WPRINT_APP_INFO( ( "SHA-256 Success\n" ) );
            pass_count++;
        }
        else
        {
            WPRINT_APP_INFO( ( "SHA-256 Failure\n" ) );
           fail_count++;
        }

        WPRINT_APP_INFO( ( "Time for SHA-256 hash process and done = %u ms\n", (unsigned int)t2 ) );
        i++;
    }
}

void sw_hmac_sha256_test(void)
{
    wiced_time_t t1, t2, i;
    uint8_t result[SHA256_LENGTH];
    unsigned char *key_ptr, *data_ptr;
    sha2_context hmac_sha256_ctx;

    for (i = 0; i < NUM_OF_HMAC_SHA256_TESTS; i++)
    {
        WPRINT_APP_INFO( ( "\n%s\n", hmac_sha256_test_cases[i].label ) );
        if (hmac_sha256_test_cases[i].char_key)
        {
            WPRINT_APP_INFO( ( "Key: %s\n", hmac_sha256_test_cases[i].char_key ) );
            key_ptr = (unsigned char *)hmac_sha256_test_cases[i].char_key;
        }
        else
        {
            WPRINT_APP_INFO( ( "Key:" ) );
            dump_bytes( hmac_sha256_test_cases[i].hex_key, hmac_sha256_test_cases[i].key_len );
            key_ptr = (unsigned char *)hmac_sha256_test_cases[i].hex_key;
        }

        if (hmac_sha256_test_cases[i].char_data)
        {
            WPRINT_APP_INFO( ( "Data: %s\n", hmac_sha256_test_cases[i].char_data ) );
            data_ptr = (unsigned char *)hmac_sha256_test_cases[i].char_data;
        }
        else
        {
            WPRINT_APP_INFO( ( "Data:" ) );
            dump_bytes( hmac_sha256_test_cases[i].hex_data, hmac_sha256_test_cases[i].data_len);
            data_ptr = (unsigned char *)hmac_sha256_test_cases[i].hex_data;
        }

        wiced_time_get_time( &t1 );

        sha2_hmac_starts( &hmac_sha256_ctx, key_ptr, hmac_sha256_test_cases[i].key_len, 0 );
        sha2_hmac_update( &hmac_sha256_ctx, data_ptr, hmac_sha256_test_cases[i].data_len );
        sha2_hmac_finish( &hmac_sha256_ctx, (unsigned char *)result );

        wiced_time_get_time( &t2 );
        t2 = t2 - t1;

        WPRINT_APP_INFO( ( "Result:" ) );
        dump_bytes( result, SHA256_LENGTH);

        if (memcmp(result, hmac_sha256_test_cases[i].digest, SHA256_LENGTH) == 0)
        {
            WPRINT_APP_INFO( ( "%s Success\n", hmac_sha256_test_cases[i].label ) );
            pass_count++;
        }
        else
        {
            WPRINT_APP_INFO( ( "Expected:" ) );
            dump_bytes( hmac_sha256_test_cases[i].digest, SHA256_LENGTH);
            WPRINT_APP_INFO( ( "%s Failure\n", hmac_sha256_test_cases[i].label ) );
            fail_count++;
        }

        WPRINT_APP_INFO( ( "Time for %s hash feed and done = %u ms\n", hmac_sha256_test_cases[i].label, (unsigned int) t2 ) );
    }
}


void application_start( )
{
    uint8_t output[256];

    WPRINT_APP_INFO( ( "\r\nOriginal data:\n%s\n", secret_information ) );

    /* AES */
    memset(&context_aes, 0, sizeof(context_aes));
    aes_setkey_enc(&context_aes, AES_128_CBC_KEY, 128);
    aes_crypt_cbc(&context_aes, AES_ENCRYPT, 16, AES_128_CBC_IV, (unsigned char*)secret_information, output);
    WPRINT_APP_INFO( ( "After AES encryption:\n%s\n", output ) );

    aes_setkey_dec(&context_aes, AES_128_CBC_KEY, 128);
    aes_crypt_cbc(&context_aes, AES_DECRYPT, 16, AES_128_CBC_IV, output, output);
    output[16] = '\0';
    WPRINT_APP_INFO( ( "After AES decryption:\n%s\n", output ) );

    /* Hash and HMAC tests */
    sw_md5_test();
    sw_sha1_test();
    sw_sha256_test();
    sw_md5_hmac_test();
    sw_sha1_hmac_test();
    sw_hmac_sha256_test();

    WPRINT_APP_INFO( ( "\nPassed %d tests\n", pass_count ) );
    WPRINT_APP_INFO( ( "Failed %d tests\n", fail_count ) );

}


void dump_bytes(const uint8_t* bptr, uint32_t len)
{
    int i = 0;

    for (i = 0; i < len; )
    {
        if ((i & 0x0f) == 0)
        {
            WPRINT_APP_INFO( ( "\n" ) );
        }
        else if ((i & 0x07) == 0)
        {
            WPRINT_APP_INFO( (" ") );
        }
        WPRINT_APP_INFO( ( "%02x ", bptr[i++] ) );
    }
    WPRINT_APP_INFO( ( "\n" ) );
}
