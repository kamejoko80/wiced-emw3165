/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef _CRYPTO_API_H
#define _CRYPTO_API_H

#include "typedefs.h"
#include <sbhnddma.h>
#include <platform_cache_def.h>

/******************************************************
 *                      Macros
 ******************************************************/

#define CRYPTO_DESCRIPTOR_ALIGNMENT     16
#if !defined( WICED_DCACHE_WTHROUGH ) && defined( PLATFORM_L1_CACHE_SHIFT )
#define CRYPTO_CACHE_WRITE_BACK         1
#define CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT  MAX(CRYPTO_DESCRIPTOR_ALIGNMENT, PLATFORM_L1_CACHE_BYTES)
#else
#define CRYPTO_CACHE_WRITE_BACK         0
#define CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT   CRYPTO_DESCRIPTOR_ALIGNMENT
#endif /* !defined( WICED_DCACHE_WTHROUGH ) && defined( PLATFORM_L1_CACHE_SHIFT ) */

/*****************************************************
 *                    Constants
 ******************************************************/

#define AES128_IV_LEN                   16
#define AES_BLOCK_SZ                    16
#define AES128_KEY_LEN                  16
#define HMAC256_KEY_LEN                 32
#define SHA256_HASH_SIZE                32
#define HMAC256_OUTPUT_SIZE             32
#define HMAC256_INNERHASHCONTEXT_SIZE   64
#define HMAC256_OUTERHASHCONTEXT_SIZE   64
#define HMAC_BLOCK_SIZE                 64
#define HMAC_IPAD                       0x36
#define HMAC_OPAD                       0x5C
#define HWCRYPTO_MAX_PAYLOAD_SIZE       (64*1023)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    OUTBOUND    = 0,
    INBOUND     = 1
} sctx_inbound_t;

typedef enum
{
    ENCR_AUTH = 0,
    AUTH_ENCR = 1
} sctx_order_t;

typedef enum
{
    CRYPT_NULL  = 0,
    RC4         = 1,
    DES         = 2,
    _3DES       = 3,
    AES         = 4
} sctx_crypt_algo_t;

typedef enum
{
    ECB = 0,
    CBC = 1,
    OFB = 2,
    CFB = 3,
    CTR = 4,
    CCM = 5,
    GCM = 6,
    XTS = 7
} sctx_crypt_mode_t;

typedef enum
{
    NULL_   = 0,
    MD5     = 1,
    SHA1    = 2,
    SHA224  = 3,
    SHA256  = 4,
    AES_H   = 5,
    FHMAC   = 6
} sctx_hash_algo_t;

typedef enum
{
    HASH=0,
    CTXT=1,
    HMAC=2,
    CCM_H=5,
    GCM_H=6
} sctx_hash_mode_t;

typedef enum
{
    HASH_FULL   = 0,
    HASH_INIT   = 1,
    HASH_UPDT   = 2,
    HASH_FINAL  = 3
} sctx_hash_optype_t;

typedef enum
{
    HWCRYPTO_ENCR_ALG_NONE = 0,
    HWCRYPTO_ENCR_ALG_AES_128,
    HWCRYPTO_ENCR_ALG_AES_192,
    HWCRYPTO_ENCR_ALG_AES_256,
    HWCRYPTO_ENCR_ALG_DES,
    HWCRYPTO_ENCR_ALG_3DES,
    HWCRYPTO_ENCR_ALG_RC4_INIT,
    HWCRYPTO_ENCR_ALG_RC4_UPDT
} hwcrypto_encr_alg_t;

typedef enum
{
    HWCRYPTO_ENCR_MODE_NONE = 0,
    HWCRYPTO_ENCR_MODE_CBC,
    HWCRYPTO_ENCR_MODE_ECB ,
    HWCRYPTO_ENCR_MODE_CTR,
    HWCRYPTO_ENCR_MODE_CCM = 5,
    HWCRYPTO_ENCR_MODE_CMAC,
    HWCRYPTO_ENCR_MODE_OFB,
    HWCRYPTO_ENCR_MODE_CFB,
    HWCRYPTO_ENCR_MODE_GCM,
    HWCRYPTO_ENCR_MODE_XTS
} hwcrypto_encr_mode_t;

typedef enum
{
      HWCRYPTO_AUTH_ALG_NULL = 0,
      HWCRYPTO_AUTH_ALG_MD5 ,
      HWCRYPTO_AUTH_ALG_SHA1,
      HWCRYPTO_AUTH_ALG_SHA224,
      HWCRYPTO_AUTH_ALG_SHA256,
      HWCRYPTO_AUTH_ALG_AES
} hwcrypto_auth_alg_t;

typedef enum
{
      HWCRYPTO_AUTH_MODE_HASH = 0,
      HWCRYPTO_AUTH_MODE_CTXT,
      HWCRYPTO_AUTH_MODE_HMAC,
      HWCRYPTO_AUTH_MODE_FHMAC,
      HWCRYPTO_AUTH_MODE_CCM,
      HWCRYPTO_AUTH_MODE_GCM,
      HWCRYPTO_AUTH_MODE_XCBCMAC,
      HWCRYPTO_AUTH_MODE_CMAC
} hwcrypto_auth_mode_t;

typedef enum
{
      HWCRYPTO_AUTH_OPTYPE_FULL = 0,
      HWCRYPTO_AUTH_OPTYPE_INIT,
      HWCRYPTO_AUTH_OPTYPE_UPDATE,
      HWCRYPTO_AUTH_OPTYPE_FINAL,
      HWCRYPTO_AUTH_OPTYPE_HMAC_HASH
} hwcrypto_auth_optype_t;

typedef enum
{
    HWCRYPTO_CIPHER_MODE_NULL = 0,
    HWCRYPTO_CIPHER_MODE_ENCRYPT,
    HWCRYPTO_CIPHER_MODE_DECRYPT,
    HWCRYPTO_CIPHER_MODE_AUTHONLY
} hwcrypto_cipher_mode_t;

typedef enum
{
    HWCRYPTO_CIPHER_ORDER_NULL = 0,
    HWCRYPTO_CIPHER_ORDER_AUTH_CRYPT,
    HWCRYPTO_CIPHER_ORDER_CRYPT_AUTH
} hwcrypto_cipher_order_t;

typedef enum
{
    HWCRYPTO_CIPHER_OPTYPE_RC4_OPTYPE_INIT = 0,
    HWCRYPTO_CIPHER_OPTYPE_RC4_OPTYPE_UPDT,
    HWCRYPTO_CIPHER_OPTYPE_DES_OPTYPE_K56,
    HWCRYPTO_CIPHER_OPTYPE_3DES_OPTYPE_K168EDE,
    HWCRYPTO_CIPHER_OPTYPE_AES_OPTYPE_K128,
    HWCRYPTO_CIPHER_OPTYPE_AES_OPTYPE_K192,
    HWCRYPTO_CIPHER_OPTYPE_AES_OPTYPE_K256
} hwcrypto_cipher_optype_t;

typedef enum
{
    HWCRYPTO_HASHMODE_HASH_HASH = 0,
    HWCRYPTO_HASHMODE_HASH_CTXT,
    HWCRYPTO_HASHMODE_HASH_HMAC,
    HWCRYPTO_HASHMODE_HASH_FHMAC,
    HWCRYPTO_HASHMODE_AES_HASH_XCBC_MAC,
    HWCRYPTO_HASHMODE_AES_HASH_CMAC,
    HWCRYPTO_HASHMODE_AES_HASH_CCM,
    HWCRYPTO_HASHMODE_AES_HASH_GCM
}  hwcrypto_hash_mode_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct crypto_cmd
{
    uint8_t   *source;
    uint32_t  crypt_size;
    uint32_t  hash_size;
    uint8_t   *hash_key;
    uint32_t  hash_key_len;
    uint8_t   *crypt_key;
    uint32_t  crypt_key_len;
    uint8_t   *crypt_iv;
    uint32_t  crypt_iv_len;
    uint8_t   *output;
    uint8_t   *hash_output;
    uint8_t   crypt_algo;
    uint8_t   crypt_mode;
    uint8_t   crypt_optype;
    uint8_t   hash_algo;
    uint8_t   hash_mode;
    uint8_t   hash_optype;
    uint8_t   order;
    uint8_t   inbound;
    uint8_t   icv_size;
    uint32_t  prev_len;

} crypto_cmd_t;


typedef struct
{
    uint8_t     state[ SHA256_HASH_SIZE ];
    uint8_t     *payload_buffer;
    uint8_t     hash_optype;
    uint32_t    prev_len;
    uint8_t     i_key_pad[ HMAC256_INNERHASHCONTEXT_SIZE ];
    uint8_t     o_key_pad[ HMAC256_INNERHASHCONTEXT_SIZE ];
} sha256_context_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

platform_result_t platform_hwcrypto_execute(crypto_cmd_t cmd);
platform_result_t platform_hwcrypto_aes128cbc_decrypt( uint8_t *key, uint32_t keysize, uint8_t *iv, uint32_t size,
        uint8_t *src, uint8_t *dest );
platform_result_t platform_hwcrypto_sha256_hash(uint8_t *source, uint32_t size, uint8_t *output_payload_buffer, uint8_t *hash_output);
platform_result_t platform_hwcrypto_sha256_incremental(sha256_context_t *ctx, uint8_t *source, uint32_t length);
void platform_hwcrypto_init( void );
platform_result_t platform_hwcrypto_sha256_hmac_final(sha256_context_t *ctx, uint8_t *input, uint32_t input_len, uint8_t * output );
platform_result_t platform_hwcrypto_sha256_hmac_update(sha256_context_t *ctx, uint8_t *input, uint32_t input_len );
platform_result_t platform_hwcrypto_sha256_hmac_init(sha256_context_t *ctx, uint8_t *hmac_key, uint32_t hmac_key_len );
platform_result_t platform_hwcrypto_aescbc_decrypt_sha256_hmac(uint8_t *crypt_key, uint8_t *crypt_iv, uint32_t crypt_size,
        uint32_t auth_size, uint8_t *hmac_key, uint32_t hmac_key_len, uint8_t *src, uint8_t *crypt_dest, uint8_t *hash_dest);
platform_result_t platform_hwcrypto_sha256_hmac( uint8_t *hmac_key, uint32_t keysize, uint8_t *source, uint32_t size,
        uint8_t *output_payload_buffer, uint8_t *hash_output );

#endif  /*_CRYPTO_API_H */

