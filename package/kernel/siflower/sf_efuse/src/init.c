#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include "efuse.h"

#define DEVICE_NAME ("sf_efuse")
#define NODE_NAME ("node")

struct sf_data* global_sf_data = NULL;
struct dentry *root_dentry, *sub_dentry;
void *base_addr, *ef_base, *ef_reg_timing1, *ef_reg_data;

static int map_addr_init(void)
{
	int ret = 0;
	void *addr = (void *)ioremap(SIFLOWER_EFUSE_BASE, 0x20);
	base_addr = addr;

	if (IS_ERR_OR_NULL((void *)base_addr))
		return -EFAULT;

	ef_base = (void *)(base_addr + EFUSE_REG_RD_CFG_RDAT_OFFSET);
	ef_reg_timing1 = (void *)(base_addr + EFUSE_REG_TIMING_1_OFFSET);
	ef_reg_data = (void *)(base_addr + EFUSE_REG_RD_DATA_OFFSET);

	return ret;
}

static void map_addr_remove(void)
{
	iounmap(base_addr);
}

static void efuse_init(void)
{
	u32 reg;

	reg = (u32)readl(ef_reg_timing1);
	reg |= 1;
	writel(reg, ef_reg_timing1);
}

static int efuse_single_read_trigger(u32 addr)
{
	u32 timeout = 0;
	u32 reg;

	writel(FIELD_PREP(GENMASK(17, 16), EF_SINGLE_READ) |
	       FIELD_PREP(GENMASK(10, 0), addr), ef_base);

	/* wait for access successful bit */
	do
	{
		if (timeout >= 100000)
			return -EIO;

		timeout++;
		reg = readl(ef_base);
	} while (EF_ACCESS_SUCCESSFUL != FIELD_GET(GENMASK(25, 24), reg));

	return 0;
}

static unsigned int get_efuse(u32 offset)
{
	u32 reg, ret;

	ret = efuse_single_read_trigger(offset / 32);

	if(ret == 0)
		reg = (readl(ef_reg_data) >> (offset % 32));
	else
		reg = -EIO;

	return reg;
}

static unsigned char hex_to_char(unsigned char hex)
{
	if(!isalnum(hex))
		hex = 0x3f;
	return hex;
}

static int global_show(struct seq_file *m, void *data)
{
	struct sf_data *data_param = m->private;

	seq_printf(m, "\nChip_Version: %x\n", data_param->Chip_Version);
	seq_printf(m, "Wafer_LOT_ID: %c", data_param->Wafer_LOT_ID_0);
	seq_printf(m, "%c", data_param->Wafer_LOT_ID_1);
	seq_printf(m, "%c", data_param->Wafer_LOT_ID_2);
	seq_printf(m, "%c", data_param->Wafer_LOT_ID_3);
	seq_printf(m, "%c", data_param->Wafer_LOT_ID_4);
	seq_printf(m, "%c", data_param->Wafer_LOT_ID_5);
	seq_printf(m, "%c\n", data_param->Wafer_LOT_ID_6);

	seq_printf(m, "Wafer_Num: %d\n", data_param->Wafer_no);

	seq_printf(m, "Chip_XY:(%d", data_param->X);
	seq_printf(m, ",%d)\n", data_param->Y);

	seq_printf(m, "FLG_CP: 0x%x\n", data_param->FLG_EFUSE_W_CP);
	seq_printf(m, "FLG_FT: 0x%x\n", data_param->FLG_EFUSE_W_FT);

	seq_printf(m, "ASSY_LOT_ID: %c", data_param->Assembly_LOT_ID_0);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_1);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_2);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_3);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_4);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_5);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_6);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_7);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_8);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_9);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_10);
	seq_printf(m, "%c", data_param->Assembly_LOT_ID_11);
	seq_printf(m, "%c\n\n", data_param->Assembly_LOT_ID_12);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(global);

static int __init sf_efuse_init(void)
{
	u32 val[7] = {0};
	int i = 0, map = 0, ret = -ENOENT;
	struct sf_data *data_param;

	data_param = kzalloc(sizeof(struct sf_data), GFP_KERNEL);
	global_sf_data = data_param;

	if(!global_sf_data)
		goto Fail_data_param;

	map = map_addr_init();
	if(map)
		goto Fail_map;

	efuse_init();

	/* read register and set data into array */
	//param1
	val[0] = get_efuse(EFUSE_OP_AREA_1);
	global_sf_data->FLG_EFUSE_W_CP = (u8)(FIELD_GET(GENMASK(7, 0), val[0]));
	global_sf_data->FLG_EFUSE_W_FT = (u8)(FIELD_GET(GENMASK(15, 8), val[0]));
	global_sf_data->X = (int)(FIELD_GET(GENMASK(23, 16), val[0]));
	global_sf_data->Y = (int)(FIELD_GET(GENMASK(31, 24), val[0]));

	//param2
	val[1] = get_efuse(EFUSE_OP_AREA_2);
	global_sf_data->Wafer_no = (int)(FIELD_GET(GENMASK(7, 0), val[1]));
	global_sf_data->Wafer_LOT_ID_0 = hex_to_char((FIELD_GET(GENMASK(15, 8), val[1])));
	global_sf_data->Wafer_LOT_ID_1 = hex_to_char((FIELD_GET(GENMASK(23, 16), val[1])));
	global_sf_data->Wafer_LOT_ID_2 = hex_to_char((FIELD_GET(GENMASK(31, 24), val[1])));

	//param3
	val[2] = get_efuse(EFUSE_OP_AREA_3);
	global_sf_data->Wafer_LOT_ID_3 = hex_to_char((FIELD_GET(GENMASK(7, 0), val[2])));
	global_sf_data->Wafer_LOT_ID_4 = hex_to_char((FIELD_GET(GENMASK(15, 8), val[2])));
	global_sf_data->Wafer_LOT_ID_5 = hex_to_char((FIELD_GET(GENMASK(23, 16), val[2])));
	global_sf_data->Wafer_LOT_ID_6 = hex_to_char((FIELD_GET(GENMASK(31, 24), val[2])));

	//param4
	val[3] = get_efuse(EFUSE_OP_AREA_4);
	global_sf_data->Assembly_LOT_ID_0 = hex_to_char((FIELD_GET(GENMASK(7, 0), val[3])));
	global_sf_data->Assembly_LOT_ID_1 = hex_to_char((FIELD_GET(GENMASK(15, 8), val[3])));
	global_sf_data->Assembly_LOT_ID_2 = hex_to_char((FIELD_GET(GENMASK(23, 16), val[3])));
	global_sf_data->Assembly_LOT_ID_3 = hex_to_char((FIELD_GET(GENMASK(31, 24), val[3])));

	//param5
	val[4] = get_efuse(EFUSE_OP_AREA_5);
	global_sf_data->Assembly_LOT_ID_4 = hex_to_char((FIELD_GET(GENMASK(7, 0), val[4])));
	global_sf_data->Assembly_LOT_ID_5 = hex_to_char((FIELD_GET(GENMASK(15, 8), val[4])));
	global_sf_data->Assembly_LOT_ID_6 = hex_to_char((FIELD_GET(GENMASK(23, 16), val[4])));
	global_sf_data->Assembly_LOT_ID_7 = hex_to_char((FIELD_GET(GENMASK(31, 24), val[4])));

	//param6
	val[5] = get_efuse(EFUSE_OP_AREA_6);
	global_sf_data->Assembly_LOT_ID_8 = hex_to_char((FIELD_GET(GENMASK(7, 0), val[5])));
	global_sf_data->Assembly_LOT_ID_9 = hex_to_char((FIELD_GET(GENMASK(15, 8), val[5])));
	global_sf_data->Assembly_LOT_ID_10 = hex_to_char((FIELD_GET(GENMASK(23, 16), val[5])));
	global_sf_data->Assembly_LOT_ID_11 = hex_to_char((FIELD_GET(GENMASK(31, 24), val[5])));

	//param7
	val[6] = get_efuse(EFUSE_OP_AREA_7);
	global_sf_data->Assembly_LOT_ID_12 = hex_to_char((FIELD_GET(GENMASK(7, 0), val[6])));
	global_sf_data->Chip_Version = (FIELD_GET(GENMASK(15, 8), val[6]));


	for(i = 0; i < 7; i++){
		if(val[i] == -EIO){
			printk(KERN_ERR "read efuse timeout !\n");
			goto Fail_map;
		}
	}

	root_dentry = debugfs_create_dir(DEVICE_NAME, NULL);
	if (!IS_ERR(root_dentry))
	{
		sub_dentry = debugfs_create_file(NODE_NAME, 0644, root_dentry, data_param, &global_fops);
		if(IS_ERR(sub_dentry)){
			ret = PTR_ERR(sub_dentry);
			goto Fail_sub_dentry;
		}
	}else{
		ret = PTR_ERR(root_dentry);
		goto Fail_root_dentry;
	}

	return 0;

Fail_sub_dentry:
	debugfs_remove(sub_dentry);
Fail_root_dentry:
	debugfs_remove(root_dentry);
Fail_map:
	map_addr_remove();
Fail_data_param:
	kfree(global_sf_data);
	return ret;
}

static void __exit sf_efuse_exit(void)
{
	printk("sf_efuse_exit\n");
	debugfs_remove(sub_dentry);
	debugfs_remove(root_dentry);
	kfree(global_sf_data);
	map_addr_remove();
}

MODULE_LICENSE("GPL");
module_init(sf_efuse_init);
module_exit(sf_efuse_exit);