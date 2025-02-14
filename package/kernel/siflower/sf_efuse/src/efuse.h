#include <asm/io.h>
#include <asm/types.h>
#include <linux/bitops.h>
#include <linux/kernel.h>

#define EFUSE_OP_AREA_1		(0 * 32)
#define EFUSE_OP_AREA_2		(1 * 32)
#define EFUSE_OP_AREA_3		(2 * 32)
#define EFUSE_OP_AREA_4		(3 * 32)
#define EFUSE_OP_AREA_5		(4 * 32)
#define EFUSE_OP_AREA_6		(5 * 32)
#define EFUSE_OP_AREA_7		(6 * 32)

#define SIFLOWER_EFUSE_BASE	0xce02c00
#define EFUSE_REG_RD_CFG_RDAT_OFFSET	0x0
#define EFUSE_REG_RD_DATA_OFFSET	0x4
#define EFUSE_REG_TIMING_1_OFFSET	0x10

enum efuse_trigger_mode {
	EF_FULL_READ	= 0b00,
	EF_SINGLE_READ	= 0b01,
	EF_SINGLE_WRITE	= 0b10,
};

enum efuse_trigger_state {
	EF_NO_RESULT = 0b00,
	EF_ACCESS_SUCCESSFUL = 0b01,
	EF_RESERVED = 0b10,
	EF_WRITE_FAIL = 0b11,
};

struct sf_data {
	u8 FLG_EFUSE_W_CP;
	u8 FLG_EFUSE_W_FT;
	int X;
	int Y;
	int Wafer_no;
	unsigned char Wafer_LOT_ID_0;
	unsigned char Wafer_LOT_ID_1;
	unsigned char Wafer_LOT_ID_2;
	unsigned char Wafer_LOT_ID_3;
	unsigned char Wafer_LOT_ID_4;
	unsigned char Wafer_LOT_ID_5;
	unsigned char Wafer_LOT_ID_6;
	unsigned char Assembly_LOT_ID_0;
	unsigned char Assembly_LOT_ID_1;
	unsigned char Assembly_LOT_ID_2;
	unsigned char Assembly_LOT_ID_3;
	unsigned char Assembly_LOT_ID_4;
	unsigned char Assembly_LOT_ID_5;
	unsigned char Assembly_LOT_ID_6;
	unsigned char Assembly_LOT_ID_7;
	unsigned char Assembly_LOT_ID_8;
	unsigned char Assembly_LOT_ID_9;
	unsigned char Assembly_LOT_ID_10;
	unsigned char Assembly_LOT_ID_11;
	unsigned char Assembly_LOT_ID_12;
	u8 Chip_Version;
};