// SPDX-License-Identifier: GPL-2.0
/*
 * RISC-V optimized GHASH routines
 *
 * Copyright (C) 2023 VRULL GmbH
 * Copyright (C) 2024 Siflower Communications Ltd
 * Author: Heiko Stuebner <heiko.stuebner@vrull.eu>
 * 	   Qingfang Deng <qingfang.deng@siflower.com.cn>
 *
 * Based on https://lore.kernel.org/linux-riscv/20230709154243.1582671-4-heiko@sntech.de/ ,
 * but translated to C.
 */

#include <linux/types.h>
#include <linux/err.h>
#include <linux/crypto.h>
#include <linux/module.h>
#include <asm/clmul.h>
#include <asm/unaligned.h>
#include <crypto/ghash.h>
#include <crypto/internal/hash.h>

struct riscv64_ghash_ctx {
	u128 htable;
};

struct riscv64_ghash_desc_ctx {
	u64 shash[2];
	u8 buffer[GHASH_DIGEST_SIZE];
	int bytes;
};

/* Reverse the bit order for each byte in x */
static inline u64 bitrev8x8(u64 x)
{
	x = ((x & 0xF0F0F0F0F0F0F0F0ULL) >> 4) | ((x & 0x0F0F0F0F0F0F0F0FULL) << 4);
	x = ((x & 0xCCCCCCCCCCCCCCCCULL) >> 2) | ((x & 0x3333333333333333ULL) << 2);
	x = ((x & 0xAAAAAAAAAAAAAAAAULL) >> 1) | ((x & 0x5555555555555555ULL) << 1);
	return x;
}

static int riscv64_ghash_init(struct shash_desc *desc)
{
	struct riscv64_ghash_desc_ctx *dctx = shash_desc_ctx(desc);

	dctx->bytes = 0;
	memset(dctx->shash, 0, GHASH_DIGEST_SIZE);
	return 0;
}

static void gcm_init_rv64i_zbc(u128 Htable[1], const u8 key[GHASH_BLOCK_SIZE])
{
	u64 h0 = get_unaligned((const u64 *)key + 0);
	u64 h1 = get_unaligned((const u64 *)key + 1);

	h0 = bitrev8x8(h0);
	h1 = bitrev8x8(h1);
	Htable->a = h0;
	Htable->b = h1;
}

static void gcm_ghash_rv64i_zbc(u64 Xi[2], const u128 Htable[1], const u8 *inp,
				size_t len)
{
	u64 t0, t1, x0, x1, y0, y1, z0, z1, z2, z3;
	const u64 polymod = 0x87;

	// Load Xi and bit-reverse it
	x0 = Xi[0];
	x1 = Xi[1];
	x0 = bitrev8x8(x0);
	x1 = bitrev8x8(x1);

	// Load the key (already bit-reversed)
	y0 = Htable->a;
	y1 = Htable->b;

	do {
		// Load the input data, bit-reverse them, and XOR them with Xi
		t0 = get_unaligned((const u64 *)inp + 0);
		t1 = get_unaligned((const u64 *)inp + 1);
		inp += GHASH_BLOCK_SIZE;
		len -= GHASH_BLOCK_SIZE;
		t0 = bitrev8x8(t0);
		t1 = bitrev8x8(t1);
		x0 ^= t0;
		x1 ^= t1;

		// Multiplication (without Karatsuba)
		z3 = clmulh(x1, y1);
		z2 = clmul(x1, y1);
		t1 = clmulh(x0, y1);
		z1 = clmul(x0, y1);
		z2 ^= t1;
		t1 = clmulh(x1, y0);
		t0 = clmul(x1, y0);
		z2 ^= t1;
		z1 ^= t0;
		t1 = clmulh(x0, y0);
		z0 = clmul(x0, y0);
		z1 ^= t1;

		// Reduction with clmul
		t1 = clmulh(z3, polymod);
		t0 = clmul(z3, polymod);
		z2 ^= t1;
		z1 ^= t0;
		t1 = clmulh(z2, polymod);
		t0 = clmul(z2, polymod);
		x1 = z1 ^ t1;
		x0 = z0 ^ t0;
	} while (len);

	// Bit-reverse final Xi back and store it
	x0 = bitrev8x8(x0);
	x1 = bitrev8x8(x1);
	Xi[0] = x0;
	Xi[1] = x1;
}

static int riscv64_zbc_ghash_setkey(struct crypto_shash *tfm,
				    const u8 *key,
				    unsigned int keylen)
{
	struct riscv64_ghash_ctx *ctx = crypto_tfm_ctx(crypto_shash_tfm(tfm));

	if (keylen != GHASH_BLOCK_SIZE)
		return -EINVAL;

	gcm_init_rv64i_zbc(&ctx->htable, key);

	return 0;
}

static int riscv64_zbc_ghash_update(struct shash_desc *desc,
				    const u8 *src, unsigned int srclen)
{
	unsigned int len;
	struct riscv64_ghash_ctx *ctx = crypto_tfm_ctx(crypto_shash_tfm(desc->tfm));
	struct riscv64_ghash_desc_ctx *dctx = shash_desc_ctx(desc);

	if (dctx->bytes) {
		if (dctx->bytes + srclen < GHASH_DIGEST_SIZE) {
			memcpy(dctx->buffer + dctx->bytes, src,
				srclen);
			dctx->bytes += srclen;
			return 0;
		}
		memcpy(dctx->buffer + dctx->bytes, src,
			GHASH_DIGEST_SIZE - dctx->bytes);

		gcm_ghash_rv64i_zbc(dctx->shash, &ctx->htable,
				dctx->buffer, GHASH_DIGEST_SIZE);

		src += GHASH_DIGEST_SIZE - dctx->bytes;
		srclen -= GHASH_DIGEST_SIZE - dctx->bytes;
		dctx->bytes = 0;
	}
	len = srclen & ~(GHASH_DIGEST_SIZE - 1);

	if (len) {
		gcm_ghash_rv64i_zbc(dctx->shash, &ctx->htable,
				src, len);
		src += len;
		srclen -= len;
	}

	if (srclen) {
		memcpy(dctx->buffer, src, srclen);
		dctx->bytes = srclen;
	}
	return 0;
}

static int riscv64_zbc_ghash_final(struct shash_desc *desc, u8 out[GHASH_DIGEST_SIZE])
{
	int i;
	struct riscv64_ghash_ctx *ctx = crypto_tfm_ctx(crypto_shash_tfm(desc->tfm));
	struct riscv64_ghash_desc_ctx *dctx = shash_desc_ctx(desc);

	if (dctx->bytes) {
		for (i = dctx->bytes; i < GHASH_DIGEST_SIZE; i++)
			dctx->buffer[i] = 0;
		gcm_ghash_rv64i_zbc(dctx->shash, &ctx->htable,
				dctx->buffer, GHASH_DIGEST_SIZE);
		dctx->bytes = 0;
	}
	memcpy(out, dctx->shash, GHASH_DIGEST_SIZE);
	return 0;
}

struct shash_alg riscv64_zbc_ghash_alg = {
	.digestsize = GHASH_DIGEST_SIZE,
	.init = riscv64_ghash_init,
	.update = riscv64_zbc_ghash_update,
	.final = riscv64_zbc_ghash_final,
	.setkey = riscv64_zbc_ghash_setkey,
	.descsize = sizeof(struct riscv64_ghash_desc_ctx)
		    + sizeof(struct ghash_desc_ctx),
	.base = {
		 .cra_name = "ghash",
		 .cra_driver_name = "riscv64_zbc_ghash",
		 .cra_priority = 250,
		 .cra_blocksize = GHASH_BLOCK_SIZE,
		 .cra_ctxsize = sizeof(struct riscv64_ghash_ctx),
		 .cra_module = THIS_MODULE,
	},
};

static int __init riscv64_ghash_mod_init(void)
{
	return crypto_register_shash(&riscv64_zbc_ghash_alg);
}

static void __exit riscv64_ghash_mod_fini(void)
{
	crypto_unregister_shash(&riscv64_zbc_ghash_alg);
}

module_init(riscv64_ghash_mod_init);
module_exit(riscv64_ghash_mod_fini);

MODULE_DESCRIPTION("GHASH hash function (CLMUL accelerated)");
MODULE_AUTHOR("Heiko Stuebner <heiko.stuebner@vrull.eu>");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CRYPTO("ghash");
