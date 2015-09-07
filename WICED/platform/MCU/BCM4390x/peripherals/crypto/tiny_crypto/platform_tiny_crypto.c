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
 * Broadcom SPU-M Interface
 */

#include <stdint.h>
#include "typedefs.h"
#include <siutils.h>
#include <platform_peripheral.h>
#include "wiced_osl.h"
#include <crypto_core.h>
#include <crypto_api.h>
#include "cr4.h"
#include "platform_cache.h"
#include "../platform_map.h"
#include "hndsoc.h"
#include <sbhnddma.h>
#include <platform_toolchain.h>

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#if defined( WICED_DCACHE_WTHROUGH ) && defined( PLATFORM_L1_CACHE_SHIFT )
#define CRYPTO_CACHE_WRITE_THROUGH      1
#else
#define CRYPTO_CACHE_WRITE_THROUGH      0
#endif /* defined( WICED_DCACHE_WTHROUGH ) && defined( PLATFORM_L1_CACHE_SHIFT ) */

#define CRYPTO_MEMCPY_DCACHE_CLEAN( ptr, size) platform_dcache_clean_range( ptr, size )

#define SPUM_MAX_PAYLOAD_SIZE           PLATFORM_L1_CACHE_ROUND_UP(16 * 1024)
#define WORDSIZE                        4 /* 4 Bytes */
#define CRYPTO_REG_ADDR                 PLATFORM_CRYPTO_REGBASE(0)
#define MAX_DMA_BUFFER_SIZE             (D64_CTRL2_BC_USABLE_MASK + 1)

/* SPU-M processes max 64KB data at a time
 * The max no of DMA descriptors for packet size of 64K
 * 1(header) +  4 (16K per DMA desc) + 1 (Hashoutput) + 1 (status)
 */
#define MAX_DMADESCS_PER_SPUMPKT        7
#define SPUM_STATUS_SIZE                WORDSIZE
#define OUTPUT_HDR_SIZE                 12
#define OUTPUT_HDR_CACHE_ALIGNED_SIZE   PLATFORM_L1_CACHE_ROUND_UP(12)
#define HWCRYPTO_UNUSED                 0

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    MH_IDX = 0,
    EH_IDX,
    SCTX1_IDX,
    SCTX2_IDX,
    SCTX3_IDX,
    HASH_KEY0_IDX,
    HASH_KEY1_IDX,
    HASH_KEY2_IDX,
    HASH_KEY3_IDX,
    HASH_KEY4_IDX,
    HASH_KEY5_IDX,
    HASH_KEY6_IDX,
    HASH_KEY7_IDX,
    CRYPT_KEY0_IDX,
    CRYPT_KEY1_IDX,
    CRYPT_KEY2_IDX,
    CRYPT_KEY3_IDX,
    CRYPT_KEY4_IDX,
    CRYPT_KEY5_IDX,
    CRYPT_KEY6_IDX,
    CRYPT_KEY7_IDX,
    CRYPT_IV0_IDX,
    CRYPT_IV1_IDX,
    CRYPT_IV2_IDX,
    CRYPT_IV3_IDX,
    BDESC_MAC_IDX,
    BDESC_CRYPTO_IDX,
    BDESC_IV_IDX,
    BD_SIZE_IDX,
    SPUM_HDR_SIZE,
} spu_combo_hdr_index_t;

typedef enum
{
    HDR=0,
    PAYLOAD,
    STATUS,
} dmadesc_index_t;

typedef struct bdesc_header
{
    uint32_t mac;
    uint32_t crypt;
    uint32_t iv;

} bdesc_header_t;

typedef struct bd_header
{
    uint32_t size;
} bd_header_t;

typedef struct dma_list
{
    uint32_t addr;
    uint32_t len;
} dma_list_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

static void hwcrypto_compute_sha256hmac_inner_outer_hashcontext( uint8_t* key, uint32_t keylen, uint8_t i_key_pad[ ], uint8_t o_key_pad[ ] );
static void hwcrypto_unprotected_blocking_dma_transfer( int txendindex, int rxendindex, dma64dd_t input_descriptors[ ], dma64dd_t output_descriptors[ ] );
static uint32_t hwcrypto_split_dma_data( uint32_t size, uint8_t *src, dma64dd_t *dma_descriptor );
static void hwcrypto_dcache_clean_dma_input( int txendindex, int rxendindex, dma64dd_t input_desc[ ], dma64dd_t output_desc[ ] );
static void hwcrypto_dcache_invalidate_dma_output( int rxendindex, dma64dd_t output_desc[ ] );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static volatile crypto_regs_t* cryptoreg;

/******************************************************
 *               Function Definitions
 ******************************************************/

static void hwcrypto_core_enable( void )
{
    osl_wrapper_enable( (void*) PLATFORM_CRYPTO_MASTER_WRAPPER_REGBASE(0x0) );
}

/* Initialize the Crypto Core */
void platform_hwcrypto_init( void )
{
    hwcrypto_core_enable( );
    cryptoreg = (crypto_regs_t *) CRYPTO_REG_ADDR;

    /* We are using 32 bit integers(To program The SPU-M packet) and
     * Char Array (Input payload), this is Mixed mode .
     * See Appendix H on Endianness Support for details
     */
    cryptoreg->spum_ctrl.raw = ( SPUM_BIGENDIAN << SPUM_OUTPUT_FIFO_SHIFT ) | ( SPUM_BIGENDIAN << SPUM_INPUT_FIFO_SHIFT );
}

platform_result_t platform_hwcrypto_execute( crypto_cmd_t cmd )
{

    uint8_t     *spum_header_ptr;
    uint32_t    sctx_size;
    uint32_t    spum_header_size;
    uint32_t    total_size;
    uint32_t    output_size;
    uint32_t    status_index;
    uint32_t    hash_output_index;
    uint32_t    num_payload_descriptors;
    uint32_t    num_input_descriptors;
    uint32_t    num_output_descriptors;
    bd_header_t *bd;
    bdesc_header_t *bdesc;
    volatile dma64dd_t input_descriptors[ MAX_DMADESCS_PER_SPUMPKT ] ALIGNED(CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT) =
    { { 0 } };
    volatile dma64dd_t output_descriptors[ MAX_DMADESCS_PER_SPUMPKT + 1 ] ALIGNED(CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT) =
    { { 0 } };
    volatile dma64dd_t hash_descriptor ALIGNED(CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT)=
    { 0 };
    uint8_t status[ PLATFORM_L1_CACHE_BYTES ] ALIGNED(CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT) =
    { 0 };
    uint32_t spum_header[ SPUM_HDR_SIZE * WORDSIZE ] =
    { 0 };
    uint8 output_header[ OUTPUT_HDR_CACHE_ALIGNED_SIZE ] ALIGNED(CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT);


    total_size = ( ( cmd.crypt_size > cmd.hash_size ) ? cmd.crypt_size : cmd.hash_size );

    if ( total_size > HWCRYPTO_MAX_PAYLOAD_SIZE )
        return PLATFORM_ERROR;

    output_size = total_size;
    spum_header[ MH_IDX ] = MH_SCTX | MH_BDESC | MH_BD;
    spum_header[ EH_IDX ] = HWCRYPTO_UNUSED;

    spum_header[ SCTX2_IDX ] = (unsigned) ( ( cmd.inbound << SCTX2_INBOUND_SHIFT ) |
                                                     ( cmd.order << SCTX2_ORDER_SHIFT ) |
                                                     ( cmd.crypt_algo << SCTX2_CRYPT_ALGO_SHIFT ) |
                                                     ( cmd.crypt_mode << SCTX2_CRYPT_MODE_SHIFT ) |
                                                     ( cmd.crypt_optype << SCTX2_CRYPT_OPTYPE_SHIFT ) |
                                                     ( cmd.hash_algo << SCTX2_HASH_ALGO_SHIFT ) |
                                                     ( cmd.hash_mode << SCTX2_HASH_MODE_SHIFT ) |
                                                     ( cmd.hash_optype << SCTX2_HASH_OPTYPE_SHIFT ) );

    sctx_size = SCTX1_SIZE + SCTX2_SIZE + SCTX3_SIZE;

    spum_header_ptr = (uint8_t *) spum_header + ( ( SCTX3_IDX + 1 ) * WORDSIZE );

    if ( 0 != cmd.hash_algo )
    {
        if ( 0 != cmd.hash_key_len )
        {
            memcpy( spum_header_ptr, cmd.hash_key, cmd.hash_key_len );
            sctx_size       += cmd.hash_key_len;
            spum_header_ptr += cmd.hash_key_len;
        }

        spum_header[ SCTX3_IDX ] |= (uint32_t) ( cmd.icv_size << SCTX3_ICV_SHIFT );

        /* If the user has provided separate buffer for storing
         * hash output, instead of using the End of crypt_output
         * to store it
         */
        if ( cmd.hash_output != NULL )
        {
            /* Additional descriptor for Hash output */
            hash_descriptor.addrlow = (uint32_t) cmd.hash_output;
            hash_descriptor.ctrl2   = (uint32_t) ( cmd.icv_size * WORDSIZE );
        }
        else
        {
            output_size += (uint32_t) ( cmd.icv_size * WORDSIZE );
        }

    }

    if ( 0 != cmd.crypt_algo )
    {
        if ( 0 != cmd.crypt_key_len )
        {
            memcpy( spum_header_ptr, cmd.crypt_key, cmd.crypt_key_len );
            spum_header_ptr += cmd.crypt_key_len;
            sctx_size       += cmd.crypt_key_len;
        }

        if ( 0 != cmd.crypt_iv_len )
        {
            memcpy( spum_header_ptr, cmd.crypt_iv, cmd.crypt_iv_len );
            spum_header_ptr += cmd.crypt_iv_len;
            sctx_size       += cmd.crypt_iv_len;
            spum_header[ SCTX3_IDX ] |= ( SCTX_IV << SCTX3_IV_FLAGS_SHIFT );
        }

    }

    bdesc           = (bdesc_header_t *) spum_header_ptr;
    bdesc->mac      = (unsigned) ( cmd.hash_size & 0xffff );
    bdesc->crypt    = (unsigned) ( cmd.crypt_size & 0xffff );
    bdesc->iv       = HWCRYPTO_UNUSED;
    spum_header_ptr += sizeof(bdesc_header_t);

    bd = (bd_header_t *) spum_header_ptr;
    bd->size = ( (unsigned) ( total_size << 16 ) & 0xffff0000 ) | ( cmd.prev_len & 0xffff );

    spum_header_size = MH_SIZE + ECH_SIZE + sctx_size + sizeof(bdesc_header_t) + sizeof(bd_header_t);
    spum_header[ SCTX1_IDX ] = ( sctx_size ) / WORDSIZE;

    input_descriptors[ HDR ].ctrl1      = D64_CTRL1_SOF;
    input_descriptors[ HDR ].addrlow    = (uint32_t) spum_header;
    input_descriptors[ HDR ].ctrl2      = spum_header_size;

    num_payload_descriptors             = hwcrypto_split_dma_data( total_size, (uint8_t*) cmd.source, (dma64dd_t *) ( &input_descriptors[ PAYLOAD ] ) );
    status_index                        = PAYLOAD + num_payload_descriptors;
    num_input_descriptors               = status_index + 1;

    input_descriptors[ status_index ].addrlow   = (uint32_t) &status;
    input_descriptors[ status_index ].ctrl2     = SPUM_STATUS_SIZE;
    input_descriptors[ status_index ].ctrl1     = D64_CTRL1_EOF;

    output_descriptors[ HDR ].ctrl1     = D64_CTRL1_SOF;
    output_descriptors[ HDR ].addrlow   = (uint32_t) output_header;
    output_descriptors[ HDR ].ctrl2     = OUTPUT_HDR_SIZE;

    num_payload_descriptors             = hwcrypto_split_dma_data( output_size, (uint8_t *) cmd.output, (dma64dd_t *) ( &output_descriptors[ PAYLOAD ] ) );
    hash_output_index                   = PAYLOAD + num_payload_descriptors;

    if ( hash_descriptor.ctrl2 != 0 )
    {
        output_descriptors[ hash_output_index ] = hash_descriptor;
        status_index = hash_output_index + 1;
    }
    else
    {
        status_index = PAYLOAD + num_payload_descriptors;
    }

    num_output_descriptors = status_index + 1;

    output_descriptors[ status_index ].addrlow  = (uint32_t) &status;
    output_descriptors[ status_index ].ctrl2    = SPUM_STATUS_SIZE;
    output_descriptors[ status_index ].ctrl1    = D64_CTRL1_EOF;

    hwcrypto_unprotected_blocking_dma_transfer( (int) num_input_descriptors, (int) num_output_descriptors, input_descriptors, output_descriptors );

    return PLATFORM_SUCCESS;
}

/*
 * DMA engine can process max 16K data at a time,
 * if size is > 64K , split it to chunks of 16K
 */
static uint32_t hwcrypto_split_dma_data( uint32_t size, uint8_t *src, dma64dd_t *dma_descriptor )
{
    uint32_t dma_size;
    uint32_t i = 0;

    while ( size != 0 )
    {
        dma_size = MIN(size, MAX_DMA_BUFFER_SIZE);

        dma_descriptor[ i ].addrlow = (uint32_t) src;
        dma_descriptor[ i ].ctrl2   = dma_size;
        size    -= dma_size;
        src     += dma_size;
        i++ ;
    }

    return i;
}

/*
 * AES128 CBC decryption
 *
 * @Assumption : dest is aligned to CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT
 */
platform_result_t platform_hwcrypto_aes128cbc_decrypt( uint8_t *key, uint32_t keysize, uint8_t *iv, uint32_t size, uint8_t *src, uint8_t *dest )
{
    crypto_cmd_t cmd =
    { 0 };

    cmd.crypt_key       = key;
    cmd.crypt_key_len   = keysize;
    cmd.crypt_iv        = iv;
    cmd.crypt_iv_len    = AES128_IV_LEN;
    cmd.source          = src;
    cmd.output          = dest;
    cmd.crypt_size      = size;
    cmd.inbound         = INBOUND;
    cmd.order           = AUTH_ENCR;
    cmd.crypt_algo      = AES;
    cmd.crypt_mode      = CBC;

    platform_hwcrypto_execute( cmd );
    memcpy( iv, (uint8_t*) ( src + size - AES128_IV_LEN ), AES_BLOCK_SZ );

    return PLATFORM_SUCCESS;
}

/*
 * Incremental SHA256 HASH (Maximum total data size 4MB)
 *
 * HWcrypto Outputs the PAYLOAD + HASH,
 * ctx->payload_buffer has the output payload
 * ctx->state has the HASH result
 *
 * @assumption : Output is aligned to CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT
 */
platform_result_t platform_hwcrypto_sha256_incremental( sha256_context_t *ctx, uint8_t *source, uint32_t length)
{
    crypto_cmd_t cmd =
    { 0 };

    cmd.source      = source;
    cmd.output      = ctx->payload_buffer;
    cmd.hash_output = ctx->state;
    cmd.hash_size   = length;
    cmd.hash_algo   = SHA256;
    cmd.hash_optype = ctx->hash_optype;


    if ( cmd.hash_optype == HASH_UPDT || cmd.hash_optype == HASH_FINAL )
    {
        cmd.prev_len        = ctx->prev_len;
        cmd.hash_key        = ctx->state;
        cmd.hash_key_len    = SHA256_HASH_SIZE;
    }

    cmd.icv_size = SHA256_HASH_SIZE / WORDSIZE;

    platform_hwcrypto_execute( cmd );

    ctx->prev_len += length / BD_PREVLEN_BLOCKSZ;

    return PLATFORM_SUCCESS;
}

/*
 * SPU-M does not support SHA-256 HMAC of > 64K , but it supports SHA-256 Hash for > 64K data
 * So HMAC is computed by using Hash as described below.
 * HMAC = Hash(key XOR opad || HASH(key XOR ipad || data))
 * (|| -> append, ipad -> 64 byte array of 0x3C, opad -> 64 byte array of 0x5c )
 * InnexHashContext -> key XOR ipad
 * OuterHashContext -> key XOR opad
 *
 * For Details Refer to :
 * APPENDIX B: Summary of Hash Modes and
 * 3.4.1 : Code authentication Using Incremental Hash operation
 */

/*
 * HWcrypto Outputs the PAYLOAD + HASH RESULT
 * ctx->payload_buffer has the output payload
 * output has the HASH result in case of init/updt/final
 *
 * @Assumption : ctx is aligned to CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT
 */

platform_result_t platform_hwcrypto_sha256_hmac_init( sha256_context_t *ctx, uint8_t *hmac_key, uint32_t hmac_key_len )
{
    crypto_cmd_t cmd =
    { 0 };

    /* Form the ipad  */
    /* TODO append/truncate HMAC key to make it 256bits */

    memset( ctx->i_key_pad, 0, HMAC256_INNERHASHCONTEXT_SIZE );
    memset( ctx->o_key_pad, 0, HMAC256_INNERHASHCONTEXT_SIZE );

    hwcrypto_compute_sha256hmac_inner_outer_hashcontext( hmac_key, hmac_key_len, ctx->i_key_pad, ctx->o_key_pad );

    cmd.hash_algo   = SHA256;
    cmd.source      = ctx->i_key_pad;
    cmd.output      = ctx->payload_buffer;
    cmd.hash_output = ctx->state;
    cmd.hash_size   = HMAC256_INNERHASHCONTEXT_SIZE;
    cmd.hash_optype = HASH_INIT;
    cmd.icv_size    = SHA256_HASH_SIZE / WORDSIZE;

    platform_hwcrypto_execute( cmd );

    ctx->prev_len   = HMAC256_INNERHASHCONTEXT_SIZE / BD_PREVLEN_BLOCKSZ;

    return PLATFORM_SUCCESS;

}

/* @Assumption : input_len is multiple of 64 */
platform_result_t platform_hwcrypto_sha256_hmac_update( sha256_context_t *ctx, uint8_t *input, uint32_t input_len )
{
    crypto_cmd_t cmd =
    { 0 };


    cmd.hash_algo   = SHA256;
    cmd.source      = input;
    cmd.output      = ctx->payload_buffer;
    cmd.hash_output = ctx->state;
    cmd.hash_size   = input_len;
    cmd.hash_key    = ctx->state;
    cmd.hash_key_len= SHA256_HASH_SIZE;
    cmd.hash_optype = HASH_UPDT;
    cmd.icv_size    = SHA256_HASH_SIZE / WORDSIZE;
    cmd.prev_len    = ctx->prev_len;

    platform_hwcrypto_execute( cmd );

    ctx->prev_len   += input_len / BD_PREVLEN_BLOCKSZ;

    return PLATFORM_SUCCESS;

}

platform_result_t platform_hwcrypto_sha256_hmac_final( sha256_context_t *ctx, uint8_t *input, uint32_t input_len, uint8_t * output )
{
    crypto_cmd_t cmd =
    { 0 };
    uint8_t store_buffer[ HMAC256_OUTERHASHCONTEXT_SIZE + HMAC256_OUTPUT_SIZE ] =
    { 0 };

    cmd.hash_algo       = SHA256;
    cmd.source          = input;
    cmd.output          = ctx->payload_buffer;
    cmd.hash_output     = ctx->state;
    cmd.hash_size       = input_len;
    cmd.hash_key        = ctx->state;
    cmd.hash_key_len    = SHA256_HASH_SIZE;
    cmd.hash_optype     = HASH_FINAL;
    cmd.icv_size        = SHA256_HASH_SIZE / WORDSIZE;
    cmd.prev_len        = ctx->prev_len;

    platform_hwcrypto_execute( cmd );

    /* Compute HMAC = HASH(key XOR opad || innerHash) */
    memcpy( &store_buffer[ 0 ], ctx->o_key_pad, HMAC256_OUTERHASHCONTEXT_SIZE );
    memcpy( &store_buffer[ HMAC256_OUTERHASHCONTEXT_SIZE ], cmd.hash_output, HMAC256_OUTPUT_SIZE );
    platform_hwcrypto_sha256_hash( store_buffer, ( HMAC256_OUTERHASHCONTEXT_SIZE + HMAC256_OUTPUT_SIZE ), ctx->payload_buffer, output );

    return PLATFORM_SUCCESS;

}
/*
 * SHA256 HASH on data size < 64 K (HWCRYPTO_MAX_PAYLOAD_SIZE)
 *
 * @Assumption
 * Size is < HWCRYPTO_MAX_PAYLOAD_SIZE
 * output is aligned to CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT
 */
platform_result_t platform_hwcrypto_sha256_hash(uint8_t *source, uint32_t size, uint8_t *output_payload_buffer, uint8_t *hash_output)
{
    crypto_cmd_t cmd =
    { 0 };

    cmd.source      = source;
    cmd.output      = output_payload_buffer;
    cmd.hash_output = hash_output;
    cmd.hash_size   = size;
    cmd.hash_algo   = SHA256;
    cmd.icv_size    = SHA256_HASH_SIZE / WORDSIZE;

    platform_hwcrypto_execute( cmd );

    return PLATFORM_SUCCESS;
}

/*
 * SHA256 HMAC on data size < 64 K (HWCRYPTO_MAX_PAYLOAD_SIZE)
 *
 * @Assumption
 * Size is < HWCRYPTO_MAX_PAYLOAD_SIZE
 * payload_buffer and hash_output are aligned to CRYPTO_OPTIMIZED_DESCRIPTOR_ALIGNMENT
 */
platform_result_t platform_hwcrypto_sha256_hmac( uint8_t *hmac_key, uint32_t keysize, uint8_t *source, uint32_t size,
        uint8_t *payload_buffer, uint8_t *hash_output )
{
    platform_result_t result;
    uint32_t        hmac_key_copy_size;
    crypto_cmd_t    cmd = { 0 };
    uint8           hmac_key_32[HMAC256_KEY_LEN] = {'\0'};

    /* Append / Truncate key to make it 256 bits */
    hmac_key_copy_size = (keysize > HMAC256_KEY_LEN) ? HMAC256_KEY_LEN : keysize;
    memcpy(hmac_key_32, hmac_key, hmac_key_copy_size);

    cmd.source      = source;
    cmd.output      = payload_buffer;
    cmd.hash_output = hash_output;
    cmd.hash_size   = size;
    cmd.hash_algo   = SHA256;
    cmd.hash_mode   = HMAC;
    cmd.hash_key    = hmac_key_32;
    cmd.hash_key_len= HMAC256_KEY_LEN;
    cmd.icv_size    = SHA256_HASH_SIZE / WORDSIZE;

    result = platform_hwcrypto_execute( cmd );

    return result;
}

/* Combo AES128 CBC decryption + SHA256HMAC Authentication
 * For data size < 64K (HWCRYPTO_MAX_PAYLOAD_SIZE)
 */
platform_result_t platform_hwcrypto_aescbc_decrypt_sha256_hmac(uint8_t *crypt_key, uint8_t *crypt_iv, uint32_t crypt_size,
        uint32_t auth_size, uint8_t *hmac_key, uint32_t hmac_key_len, uint8_t *src, uint8_t *crypt_dest, uint8_t *hash_dest)
{
    uint32_t        hmac_key_copy_size;
    uint8           hmac_key_32[HMAC256_KEY_LEN] = {'\0'};
    crypto_cmd_t  cmd = {0};
    platform_result_t result;

    /* Append / Truncate key to make it 256 bits */
    hmac_key_copy_size = (hmac_key_len > HMAC256_KEY_LEN) ? HMAC256_KEY_LEN : hmac_key_len;
    memcpy(hmac_key_32, hmac_key, hmac_key_copy_size);

    cmd.crypt_key       = crypt_key;
    cmd.crypt_key_len   = AES128_KEY_LEN;
    cmd.crypt_iv        = crypt_iv;
    cmd.crypt_iv_len    = AES128_IV_LEN;
    cmd.hash_key        = hmac_key_32;
    cmd.hash_key_len    = HMAC256_KEY_LEN;
    cmd.source          = src;
    cmd.output          = crypt_dest;
    cmd.hash_output     = hash_dest;
    cmd.crypt_size      = crypt_size;
    cmd.hash_size       = auth_size;
    cmd.inbound         = INBOUND;
    cmd.order           = AUTH_ENCR;
    cmd.crypt_algo      = AES;
    cmd.crypt_mode      = CBC;
    cmd.hash_algo       = SHA256;
    cmd.hash_mode       = HMAC;
    cmd.icv_size        = SHA256_HASH_SIZE/WORDSIZE;

    result = platform_hwcrypto_execute(cmd);

    return result;
}

/*
 * Invalidate/Flush DMA descriptors and the data they are pointing to
 */
static void hwcrypto_dcache_clean_dma_input( int txendindex, int rxendindex, dma64dd_t input_desc[ ], dma64dd_t output_desc[ ] )
{
    int i;

    CRYPTO_MEMCPY_DCACHE_CLEAN( (void*)&input_desc[0], (unsigned)(sizeof(input_desc[0])*(unsigned)(txendindex)) );
    CRYPTO_MEMCPY_DCACHE_CLEAN( (void*)&output_desc[0], (unsigned)(sizeof(output_desc[0])*(unsigned)(rxendindex)) );

    /* Flush the DMA data to main memory to ensure DMA picks them up */
    for ( i = 0; i < txendindex; i++ )
    {
        CRYPTO_MEMCPY_DCACHE_CLEAN((void*)input_desc[i].addrlow, input_desc[i].ctrl2);
    }

#if CRYPTO_CACHE_WRITE_BACK
    for (i = 0; i < rxendindex; i++)
    {
        /* Make sure the output buffers are PLATFORM_L1_CACHE_LINE ALIGNED */
        wiced_assert( "check alignment of hwcrypto destination address", ( ( output_desc[i].addrlow & PLATFORM_L1_CACHE_LINE_MASK ) == 0 ) );
        platform_dcache_inv_range( (uint32_t*)output_desc[i].addrlow, output_desc[i].ctrl2 );
    }
#endif

}

/*
 * Cache Invalidate the DMA output data
 */
static void hwcrypto_dcache_invalidate_dma_output( int rxendindex, dma64dd_t output_desc[ ] )
{
#if CRYPTO_CACHE_WRITE_THROUGH
    int i;
    /* Prepares the cache for reading the finished DMA data. */
    for (i = 0; i < rxendindex; i++)
    {
        platform_dcache_inv_range( (uint32_t*)output_desc[i].addrlow, output_desc[i].ctrl2 );
    }
#else
    UNUSED_PARAMETER( rxendindex );
    UNUSED_PARAMETER( output_desc );
#endif
}

/*
 * Make sure that the destination address and size is cache line aligned (For Write back cache)
 */
static void hwcrypto_unprotected_blocking_dma_transfer( int txendindex, int rxendindex, dma64dd_t input_desc[ ], dma64dd_t output_desc[ ] )
{

    uint32_t indmasize = 0;
    uint32_t outdmasize = 0;
    int i;

    /* Setup DMA channel receiver */
    cryptoreg->dmaregs.rx.addrhigh = 0;
    cryptoreg->dmaregs.rx.addrlow = ( (uint32_t) &output_desc[ 0 ] ); // Receive descriptor table address
    cryptoreg->dmaregs.rx.ptr = ( (uint32_t) &output_desc[ rxendindex - 1 ] + sizeof(dma64dd_t) ) & 0xffff; // needs to be lower 16 bits of descriptor address
    cryptoreg->dmaregs.rx.control = D64_XC_PD | ( ( 2 << D64_XC_BL_SHIFT ) & D64_XC_BL_MASK ) | ( ( 1 << D64_XC_PC_SHIFT ) & D64_XC_PC_MASK );

    cryptoreg->dmaregs.tx.addrhigh = 0;
    cryptoreg->dmaregs.tx.addrlow = ( (uint32_t) &input_desc[ 0 ] ); // Transmit descriptor table address
    cryptoreg->dmaregs.tx.ptr = ( (uint32_t) &input_desc[ txendindex - 1 ] + sizeof(dma64dd_t) ) & 0xffff; // needs to be lower 16 bits of descriptor address
    cryptoreg->dmaregs.tx.control = D64_XC_PD | ( ( 2 << D64_XC_BL_SHIFT ) & D64_XC_BL_MASK ) | ( ( 1 << D64_XC_PC_SHIFT ) & D64_XC_PC_MASK ) | D64_XC_LE;

    hwcrypto_dcache_clean_dma_input( txendindex, rxendindex, input_desc, output_desc );

    /* Fire off the DMA */
    cryptoreg->dmaregs.tx.control |= D64_XC_XE;
    cryptoreg->dmaregs.rx.control |= D64_XC_XE;

    for ( i = 0; i < txendindex; i++ )
    {
        indmasize += input_desc[ i ].ctrl2;
    }

    for ( i = 0; i < rxendindex; i++ )
    {
        outdmasize += output_desc[ i ].ctrl2;
    }

    cryptoreg->indmasize.raw = ( indmasize ) / WORDSIZE;
    cryptoreg->outdmasize.raw = ( outdmasize ) / WORDSIZE;
    cryptoreg->cryptocontrol.raw = 0x01;

    /* Wait till DMA has finished */
    while ( ( ( cryptoreg->dmaregs.tx.status0 & D64_XS0_XS_MASK ) == D64_XS0_XS_ACTIVE ) || ( ( cryptoreg->dmaregs.rx.status0 & D64_XS0_XS_MASK ) == D64_XS0_XS_ACTIVE ) )
    {
    }

    /* Switch off the DMA */
    cryptoreg->dmaregs.tx.control &= (unsigned) ( ~D64_XC_XE );
    cryptoreg->dmaregs.rx.control &= (unsigned) ( ~D64_XC_XE );
    hwcrypto_dcache_invalidate_dma_output( rxendindex, output_desc );

    /* Indicate empty table. Otherwise core is not freeing PMU resources. */
    cryptoreg->dmaregs.tx.ptr = cryptoreg->dmaregs.tx.addrlow & 0xFFFF;
}

/* For incremental HMAC operations calculate the InnerHashContext (key XOR ipad)
 * and OuterHashContext (key XOR opad)
 */
static void hwcrypto_compute_sha256hmac_inner_outer_hashcontext( uint8_t* key, uint32_t keylen, uint8_t i_key_pad[ ], uint8_t o_key_pad[ ] )
{
    int i = 0;

    memcpy( i_key_pad, key, keylen );
    memcpy( o_key_pad, key, keylen );

    for ( i = 0; i < HMAC_BLOCK_SIZE; i++ )
    {
        i_key_pad[ i ] ^= HMAC_IPAD;
        o_key_pad[ i ] ^= HMAC_OPAD;
    }
}
