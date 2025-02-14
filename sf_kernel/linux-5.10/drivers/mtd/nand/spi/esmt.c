// SPDX-License-Identifier: GPL-2.0
/*
 * Author:
 *	Chuanhong Guo <gch981213@gmail.com>
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mtd/spinand.h>
#define SPINAND_MFR_ESMT			0xC8

static SPINAND_OP_VARIANTS(read_cache_variants,
		SPINAND_PAGE_READ_FROM_CACHE_QUADIO_OP(0, 2, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_DUALIO_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X2_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(false, 0, 1, NULL, 0));

static SPINAND_OP_VARIANTS(write_cache_variants,
		SPINAND_PROG_LOAD_X4(true, 0, NULL, 0),
		SPINAND_PROG_LOAD(true, 0, NULL, 0));

static SPINAND_OP_VARIANTS(update_cache_variants,
		SPINAND_PROG_LOAD_X4(false, 0, NULL, 0),
		SPINAND_PROG_LOAD(false, 0, NULL, 0));

static int f50l1g41lb_variant2_ooblayout_ecc(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	region->offset = 64;
	region->length = 64;

	return 0;
}

static int f50l1g41lb_variant2_ooblayout_free(struct mtd_info *mtd, int section,
					struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;
	
	/* Reserve 1 bytes for the BBM. */
	region->offset = 1;
	region->length = 63;

	return 0;
}

//designed by tian.kang
static const struct mtd_ooblayout_ops f50l1g41lb_OOB_OPS = {
	.ecc = f50l1g41lb_variant2_ooblayout_ecc, 
	.free = f50l1g41lb_variant2_ooblayout_free,
};

static const struct spinand_info f50l1g41lb_spinand_table[] = {
	//designed by tian.kang
	SPINAND_INFO("F50L1G41LB",
			SPINAND_ID(SPINAND_READID_METHOD_OPCODE_ADDR, 0x01),
			//want to be understood
			NAND_MEMORG(1,2048,64,64,1024,20,1,1,1),
			NAND_ECCREQ(1,512),
			SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
			       			&write_cache_variants,
					       	&update_cache_variants),
			0,
			SPINAND_ECCINFO(&f50l1g41lb_OOB_OPS,NULL))
};

static const struct spinand_manufacturer_ops f50l1g41lb_spinand_manuf_ops = {
};

const struct spinand_manufacturer esmt_spinand_manufacturer = {
	.id = SPINAND_MFR_ESMT,
	.name = "ESMT",
	.chips = f50l1g41lb_spinand_table,
	.nchips = ARRAY_SIZE(f50l1g41lb_spinand_table),
	.ops = &f50l1g41lb_spinand_manuf_ops,
};
