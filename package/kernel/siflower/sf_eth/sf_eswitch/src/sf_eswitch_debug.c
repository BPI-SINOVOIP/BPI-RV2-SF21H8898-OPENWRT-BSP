/*
* Description
*
* Copyright (C) 2016-2019 Qin.Xia <qin.xia@siflower.com.cn>
*
* Siflower software
*/

#include "sf_eswitch.h"
#include "sf_eswitch_debug.h"
#include "intel7084_src/src/gsw_sw_init.h"

extern int intel7084_get_port_link(struct switch_dev *dev,  int port,
        struct switch_port_link *link);


long sf_eswitch_debug_ioctl (struct file * fil, unsigned int a, unsigned long b){

	intel7084_mc_ioctl (a, b);
	return 0;
}

int sf_eswitch_debug_open(struct inode *inode, struct file *file)
{
	struct sfax8_debug_info *debug = NULL;

	debug = kzalloc(sizeof(struct sfax8_debug_info), GFP_KERNEL);
	if (!debug)
		return -ENOMEM;

	debug->i_private = inode->i_private;
	file->private_data = debug;

	return 0;
}

int sf_eswitch_debug_release(struct inode *inode, struct file *file)
{
	struct sfax8_debug_info *debug = file->private_data;

	if (!debug)
		return 0;

	file->private_data = NULL;
	kfree(debug);
	return 0;
}

ssize_t sf_eswitch_debug_read(struct file *file, char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct sfax8_debug_info *debug = file->private_data;
	struct sf_eswitch_priv *pesw_priv = debug->i_private;
	struct switch_port_link link;
#ifndef CONFIG_SWCONFIG
	struct switch_dev swdev;
#endif
	int i, max_port = 0, ret = 0;
	char buf[256] = {0};
	size_t read;

	if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082)
		max_port = INTEL_SWITCH_PORT_NUM;
	else if (pesw_priv->model == NF2507)
		max_port = 5;

	ret = sprintf(buf, "check phy link status\n");
	for(i = 0; i < max_port; i++){
		if (!check_port_in_portlist(pesw_priv, i))
			continue;
		memset(&link, 0, sizeof(link));
#ifdef CONFIG_SWCONFIG
		pesw_priv->pesw_api->ops->get_port_link(&pesw_priv->swdev, i, &link);
#else
		intel7084_get_port_link(&swdev, i , &link);
#endif
		ret += sprintf(buf+ret, "phy%d link %d speed %d duplex %d\n",
				i, link.link, link.speed, link.duplex);
	}

	// add switch type here
	ret += sprintf(buf+ret, "switch_type %d\n", pesw_priv->model);
	read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);
	return read;
}

static void print_help(void)
{
	printk(" Attention: all example should append with '>  /sys/kernel/debug/esw_debug'\n");
	printk(" read/write switch reg, no value for read, witch value for write\n");
	printk(" example: echo rwReg		[addr] [value] ,for realtek switch\n");
	printk(" example: echo rwReg		[addr] [shift] [size] [value] ,for intel switch\n");
	printk(" read/write switch phy reg, no value for read, witch value for write, support both intel/realtek switch\n");
	printk(" example: echo rwPHYReg		[port] [addr] [value]\n");
	printk(" read/write switch mmd reg, no value for read, witch value for write, only support intel switch now\n");
	printk(" example: echo rwMMDReg		[port] [addr] [value]\n");
	printk(" set switch port egress mode, 0 for org, 1 for keep, only support realtek switch now\n");
	printk(" example: echo rwPvid		[port] [pvid]\n");
	printk(" get switch port pvid, no pvid value for read, witch pvid for write\n");
	printk(" example: echo rwVlanPorts		[vlan id] [member_list] [untag_list]\n");
	printk(" set switch vlan ports, no member_list and untag_list for read, witch member_list and untag_list for write\n");
	printk(" example: echo setPortEgressMode	[port] [mode]\n");
	printk(" dump switch port tx/rx count, only support intel switch now\n");
	printk(" example: echo dumpSwitchCount		[port] \n");
	printk(" clear switch port tx/rx count, only support intel switch now\n");
	printk(" example: echo clearSwitchCount		[port] \n");
	printk(" enable software multicast function, only support intel switch now\n");
	printk(" example: echo enableMulticastFunc \n");
	printk(" port join/leave mc_ip group, only support intel switch now\n");
	printk(" example: echo setMulticastEntry	[port] [type] [mc_ip]\n");
	printk(" dump multicast entries, only support intel switch now\n");
	printk(" example: echo dumpMulticastEntry \n");
}

ssize_t sf_eswitch_debug_write(struct file *file, const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct sfax8_debug_info *debug = file->private_data;
	struct sf_eswitch_priv *pesw_priv = debug->i_private;
	unsigned int i = 0, ret = 0, last_i = 0, index_arg = 0;
	char str[5][20] = {'\0'};
	char buf[128] = {0};
	u32 address = 0;


	size_t len = min_t(size_t, count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	for(; i < len; i++){
		if (buf[i] == ' '){
			memcpy(str[index_arg], buf + last_i, i - last_i);
			last_i = (i + 1);
			index_arg++;
		}
	}
	memcpy(str[index_arg], buf + last_i, count - last_i);

	if (index_arg > 5)
		goto err_parsing;

	if (strncmp(str[0], "help", 4) == 0){
		print_help();
		return count;
	}  else if (strncmp(str[0], "renetlink", 9) == 0){
		unsigned int port = 0;
		ret = kstrtou32(str[1], 0, &port);
#ifdef CONFIG_SFAX8_GENL
		notify_link_event(pesw_priv, port, 1, "eth0");
#endif
	} else if (strncmp(str[0], "led", 3) == 0){
		unsigned int led_mode = 0;
		ret = kstrtou32(str[1], 0, &led_mode);
		if(led_mode == 0 || led_mode == 1){
			if(led_mode == 0)
				led_mode = 2;
			else
				led_mode = 0;
		}
		else{
			printk("0:led all off; 1:led normal");
			return count;
		}
		pesw_priv->pesw_api->led_init(led_mode);
	} else if (strncmp(str[0], "rwReg", 4) == 0){
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082) {
			unsigned int addr = 0, shift = 0, size = 0, value = 0;

			ret = kstrtou32(str[1], 0, &addr);
			ret = kstrtou32(str[2], 0, &shift);
			ret = kstrtou32(str[3], 0, &size);
			if (str[4][0] != '\0'){
				ret = kstrtou32(str[4], 0, &value);
				pesw_priv->pesw_api->setAsicReg(addr, value);
				printk("intel write addr:0x%08x value:0x%08x\n", addr, value);
			} else {
				pesw_priv->pesw_api->getAsicReg(addr, (u32 *)&value);
				printk("intel read addr:0x%08x value:0x%08x\n", addr, value);
			}
		}
	} else if (strncmp(str[0], "rwPHYReg", 8) == 0){
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082) {
			unsigned int port=0, addr=0, value=0;
			GSW_MDIO_data_t md;
			ret = kstrtou32(str[1], 0, &port);
			ret = kstrtou32(str[2], 0, &addr);
			md.nAddressDev = port;
			md.nAddressReg = addr;
			if (str[3][0] != '\0'){
				ret = kstrtou32(str[3], 0, &value);
				md.nData = value;
				intel7084_phy_wr(&md);
				printk("intel write phy:%d addr:0x%08x val:0x%08x\n",
						port, addr, value);
			} else {
				intel7084_phy_rd( &md);
				printk("intel read phy:%d addr:0x%08x val:0x%08x\n",
						port, addr, md.nData);
			}
		}
	} else if (strncmp(str[0], "rwMMDReg", 8) == 0){
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082) {
			GSW_MMD_data_t md;
			unsigned int port=0, addr=0, value=0;
			ret = kstrtou32(str[1], 0, &port);
			ret = kstrtou32(str[2], 0, &addr);
			md.nAddressDev = port;
			md.nAddressReg = addr;
			if (str[3][0] != '\0'){
				ret = kstrtou32(str[3], 0, &value);
				md.nData = value;
				intel7084_mmd_wr(&md);
				printk("intel mmd write phy:%d addr:0x%08x val:0x%08x\n",
						port, addr, value);
			} else {
				intel7084_mmd_rd(&md);
				printk("intel mmd read phy:%d addr:0x%08x val:0x%08x\n",
						port, addr, md.nData);
			}
		}
	}
#ifdef CONFIG_SWCONFIG
	else if (strncmp(str[0], "rwPvid", 6) == 0){
		unsigned int port=0, pvid=0;
		ret = kstrtou32(str[1], 0, &port);
		ret = kstrtou32(str[2], 0, &pvid);
		if (str[2][0] != '\0'){
			pesw_priv->pesw_api->ops->set_port_pvid(&pesw_priv->swdev, port, pvid);
			printk("set port%d pvid:%d\n", port, pvid);
		} else{
			pesw_priv->pesw_api->ops->get_port_pvid(&pesw_priv->swdev, port, &pvid);
			printk("get port%d pvid:%d\n", port, pvid);
		}
	} else if (strncmp(str[0], "rwVlanPorts", 11) == 0){
		struct switch_val val;
		struct switch_port p[5] = {0};
		unsigned int vid=0, mbr_list = 0, untag_list = 0, index = 0;
		ret = kstrtou32(str[1], 0, &vid);
		if (str[3][0] != '\0'){
			ret = kstrtou32(str[2], 0, &mbr_list);
			ret = kstrtou32(str[3], 0, &untag_list);
			for (i = 0; i < 7; i++) {
				if (mbr_list & (1 << i))
				{
					if (i == 6)
						p[index].id = 5;
					else
						p[index].id = i;

					if (!(untag_list & (1 << i)))
						p[index].flags = BIT(SWITCH_PORT_FLAG_TAGGED);
					index++;
				}
			}

			val.port_vlan = vid;
			val.len = index;
			val.value.ports = p;
			pesw_priv->pesw_api->ops->set_vlan_ports(&pesw_priv->swdev, &val);
			printk("set vid:%d member_list:0x%x untag_list:0x%x\n", vid, mbr_list, untag_list);
		} else{
			val.port_vlan = vid;
			val.value.ports = p;
			pesw_priv->pesw_api->ops->get_vlan_ports(&pesw_priv->swdev, &val);
			for (i = 0; i < val.len; i++) {
				if (val.value.ports[i].id == 5)
					mbr_list |= (1 << 6);
				else
					mbr_list |= (1 << val.value.ports[i].id);

				if (!(val.value.ports[i].flags & BIT(SWITCH_PORT_FLAG_TAGGED)))
				{
					if (val.value.ports[i].id == 5)
						untag_list |= (1 << 6);
					else
						untag_list |= (1 << val.value.ports[i].id);
				}
			}
			printk("get vid:%d member_list:0x%x untag_list:0x%x\n", vid, mbr_list, untag_list);
		}
	}
#endif
	else if (strncmp(str[0], "dumpSwitchCount", 15) == 0){
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082) {
			unsigned int port=0;
			GSW_RMON_Port_cnt_t parm;
			ret = kstrtou32(str[1], 0, &port);
			parm.nPortId = port;
			intel7084_count_rd(&parm);
			printk("===== get switch port:%d counter =====\n", port);
			printk("nRxGoodPkts:   %-10d nTxGoodPkts:     %-10d\n", parm.nRxGoodPkts, parm.nTxGoodPkts);
			printk("nRxUnicastPkts:%-10d nRxBroadcastPkts:%-10d nRxMulticastPkts:%-10d\n",
					parm.nRxUnicastPkts, parm.nRxBroadcastPkts, parm.nRxMulticastPkts);
			printk("nTxUnicastPkts:%-10d nTxBroadcastPkts:%-10d nTxMulticastPkts:%-10d\n",
					parm.nTxUnicastPkts, parm.nTxBroadcastPkts, parm.nTxMulticastPkts);
		}
	} else if (strncmp(str[0], "clearSwitchCount", 16) == 0){
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082) {
			unsigned int port=0, mode=0;
			GSW_RMON_clear_t mp;
			ret = kstrtou32(str[1], 0, &port);
			ret = kstrtou32(str[2], 0, &mode);
			mp.nRmonId = port;
			mp.eRmonType = mode;
			intel7084_count_clear(&mp);
			printk("clear switch counter type:%d port:%d\n", mode, port);
		}
	} else if (strncmp(str[0], "enableMulticastFunc", 19) == 0){
		//enable software multicast function
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082)
			intel7084_multicast_set();
	} else if (strncmp(str[0], "setMulticastEntry", 17) == 0){
		//port join/leave mc_ip group
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082) {
			u32 port,mc_ip;
			u8 type;
			ret = kstrtou32(str[1], 0, &port);
			ret = kstrtou8(str[2], 0, &type);
			ret = kstrtou32(str[3], 0, &mc_ip);
			intel7084_multicast_entry_set(port, type, mc_ip);
		}
	} else if (strncmp(str[0], "dumpMulticastEntry", 18) == 0){
		//read multicast entries
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082)
			intel7084_multicast_entry_get();
	} else if (strncmp(str[0], "enableBridgeRedirect", sizeof(str[0])) == 0) {
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082) {
			int err = intel7084_bridge_redirect_ip(str[1]);

			if (err < 0)
				return err;
		} else {
			return -EOPNOTSUPP;
		}
	} else if (strncmp(str[0], "disableBridgeRedirect", sizeof(str[0])) == 0) {
		if (pesw_priv->model == INTEL7084 || pesw_priv->model == INTEL7082) {
			int err = intel7084_bridge_redirect_disable();

			if (err < 0)
				return err;
		} else {
			return -EOPNOTSUPP;
		}
	}
	else {
		printk("command not support!!!\n");
	}

	return count;

err_parsing:
	printk("parsing Error,please check your input!\n");
	return count;
}
