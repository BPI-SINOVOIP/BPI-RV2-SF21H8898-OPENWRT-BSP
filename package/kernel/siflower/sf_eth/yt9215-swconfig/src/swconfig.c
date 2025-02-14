#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/of_mdio.h>
#include <linux/gpio/consumer.h>
#include <linux/switch.h>
#include <linux/etherdevice.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <net/genetlink.h>

#ifdef CONFIG_SIFLOWER_MPW1_ESWITCH_LIGHT
#include "yt9215s_api.h"
#include "hal_ctrl.h"
#else
#include "hal_ctrl.h"
#include <cal_bprofile.h>
#include <cal_cmm.h>
#include <hal_mem.h>
#include <yt_init.h>
#include <yt_port_isolation.h>
#include <yt_exit.h>
#include <yt_port.h>
#include <yt_stat.h>
#include <yt_vlan.h>
#include <yt_lag.h>
#endif

#define YT9215S_NUM_PORTS 6
#define YT9215S_PHY_NUM 4
#define LAG_GROUPID_MAX_NUM 2

/**
 * The motorcomm sdk uses global variables for everything including
 * MDIO access. Providing an mdio access function using global
 * variables is the easiest way.
 */

#ifdef CONFIG_SIFLOWER_MPW1_ESWITCH_LIGHT
uint8_t gcal_inited = FALSE;
yt_hal_ctrl_t gHalCtrl[YT_UNIT_NUM];
char *_yt_errmsg[] = _YT_ERRMSG;
uint8_t yt_debug_level = YT_DEBUG_NONE;
char *_yt_prompt_msg[] = _YT_PROMPT_MSG;
#endif
extern yt_swDescp_t yt9215s_fib_swDescp[BOARD_MAX_UNIT_NUM];

struct yt9215_vlan {
	struct list_head list;
	u16 vid;
	u8 member_ports;
	u8 untagged_ports;
};

struct yt9215_mac_list {
        struct list_head node;
        yt_vlan_t vid;
        yt_mac_addr_t macaddr;
        yt_port_t port;
        uint16_t fdb_index;
};

#define SF_GENL_FAMILY_NAME		"DPS_NL"
#define MSGLEN				64
#define MAX_MSG_SIZE			(nla_total_size(MSGLEN) + NLMSG_LENGTH(GENL_HDRLEN))
#define GENL_NAMSIZ			16

struct genl_multicast_group group[1];

enum sf_genl_cmd {
        SF_GENL_CMD_UNSPEC = 0,         // DO NOT USE
        SF_GENL_CMD_A_MSG,
        NUM_SF_GENL_CMDS,
};

enum sf_genl_attr {
	SF_ETH_CMD_ATTR_UNSPEC = 0,
	SF_ETH_CMD_ATTR_DPS_PORT,            /* eth phy port*/
	SF_ETH_CMD_ATTR_DPS_LINK,            /* 0---link down  1---link up */
	SF_ETH_CMD_ATTR_DPS_MAC,
	SF_ETH_CMD_ATTR_DPS_VLAN,
	SF_ETH_CMD_ATTR_DPS_IFNAME,
	SF_ETH_CMD_ATTR_DPS_FLAG,
	__SF_ETH_CMD_ATTR_MAX,
};

#define SF_GENL_ATTR_MAX_VALID          (__SF_ETH_CMD_ATTR_MAX - 1)

static struct nla_policy sf_genl_policy[__SF_ETH_CMD_ATTR_MAX] = {
	[SF_ETH_CMD_ATTR_DPS_PORT] = { .type = NLA_U32 },
	[SF_ETH_CMD_ATTR_DPS_LINK] = { .type = NLA_U32 },
	[SF_ETH_CMD_ATTR_DPS_MAC]   = { .type = NLA_NUL_STRING },
	[SF_ETH_CMD_ATTR_DPS_VLAN] = { .type = NLA_U32 },
	[SF_ETH_CMD_ATTR_DPS_IFNAME] = { .type = NLA_NUL_STRING },
	[SF_ETH_CMD_ATTR_DPS_FLAG] = { .type = NLA_U8 },
};

static int sf_genl_tmp(struct sk_buff *skb, struct genl_info *info)
{
	return 0;
}

static const struct genl_ops sf_genl_ops[] = {
        {
                .cmd    = SF_GENL_CMD_A_MSG,
                .policy = sf_genl_policy,
                .doit   = sf_genl_tmp,
                .dumpit = NULL,
        },
};

static struct genl_family family = {
        .name           = SF_GENL_FAMILY_NAME,
        .version        = 1,
        .maxattr        = SF_GENL_ATTR_MAX_VALID,
        .netnsok        = false,
        .module         = ((struct module *)0),
        .ops            = sf_genl_ops,
        .n_ops          = ARRAY_SIZE(sf_genl_ops),
};

int sf_family_init(struct genl_family *sf_family, char *group_name)
{
	int err;

	memcpy(group[0].name, group_name, min_t(size_t, strlen(group_name)+1, GENL_NAMSIZ));
		family.mcgrps = group;
		family.n_mcgrps = ARRAY_SIZE(group);


	err = genl_register_family(&family);
        if (err) {
                printk("genl_register_family() failed, err=%d\n", err);
                return err;
        }

	return 0;
}

struct yt9215_priv {
	struct delayed_work phy_monitor_work;
	int unit;
	struct mii_bus *mii_bus;
	struct mdio_device *mdiodev;
	struct device *dev;
	struct switch_dev swdev;

	int phy_status[YT9215S_PHY_NUM];
	int dev_num[YT9215S_PHY_NUM];

	struct mutex mac_lock;
	struct mutex dev_num_lock;
	struct list_head mac_list[BOARD_MAX_UNIT_NUM];

	// vlan configs
	bool vlan_enabled;
	struct list_head vlan_list;
	u16 vlan_pvid[YT9215S_NUM_PORTS];
};

static struct yt9215_priv yt_priv[BOARD_MAX_UNIT_NUM];
static int yt_swconfig_probed = 0;

static inline struct yt9215_priv *
swdev_to_yt9215(struct switch_dev *swdev)
{
	return container_of(swdev, struct yt9215_priv, swdev);
}

static uint32_t yt_smi0_cl22_write(uint8_t phyAddr, uint8_t regAddr, uint16_t regValue) {
	return mdiobus_write(yt_priv[0].mii_bus, phyAddr, regAddr, regValue);
}
static uint32_t yt_smi0_cl22_read(uint8_t phyAddr, uint8_t regAddr, uint16_t *pRegValue) {
	*pRegValue = mdiobus_read(yt_priv[0].mii_bus, phyAddr, regAddr);
	return 0;
}

#if BOARD_MAX_UNIT_NUM == 2
static uint32_t yt_smi1_cl22_write(uint8_t phyAddr, uint8_t regAddr, uint16_t regValue) {
	return mdiobus_write(yt_priv[1].mii_bus, phyAddr, regAddr, regValue);
}

static uint32_t yt_smi1_cl22_read(uint8_t phyAddr, uint8_t regAddr, uint16_t *pRegValue) {
	*pRegValue = mdiobus_read(yt_priv[1].mii_bus, phyAddr, regAddr);
	return 0;
}
#endif

int notify_link_event(struct yt9215_priv *priv, int port, int updown, char *ifname, uint8_t *mac, uint16_t vlan_id, bool flag)
{
	struct sk_buff *skb;
	int ret = 0;
	void *msg_head;
	char macaddr[18] = {0};

	skb = genlmsg_new(MAX_MSG_SIZE, GFP_KERNEL);
	if (!skb)
		return -ENOMEM;
	msg_head = genlmsg_put(skb, 0, 0, &family, 0, SF_GENL_CMD_A_MSG);
	if (!msg_head) {
		printk("%s : add genlmsg header error!\n", __func__);
		ret = -ENOMEM;
		goto err;
	}
	ret = nla_put_u32(skb, SF_ETH_CMD_ATTR_DPS_PORT, port);
	if (ret < 0)
		goto err;

	ret = nla_put_u32(skb, SF_ETH_CMD_ATTR_DPS_LINK, updown);
	if (ret < 0)
		goto err;

	if (mac != NULL) {
		snprintf(macaddr, sizeof(macaddr), "%02x:%02x:%02x:%02x:%02x:%02x",
				 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		ret = nla_put_string(skb, SF_ETH_CMD_ATTR_DPS_MAC, macaddr);
		if (ret < 0)
			goto err;
	}

	ret = nla_put_u32(skb, SF_ETH_CMD_ATTR_DPS_VLAN, vlan_id);
	if (ret < 0)
		goto err;

	ret = nla_put_string(skb, SF_ETH_CMD_ATTR_DPS_IFNAME, ifname);
	if (ret < 0)
		goto err;

	ret = nla_put_u8(skb, SF_ETH_CMD_ATTR_DPS_FLAG, flag);
	if (ret < 0)
		goto err;

	genlmsg_end(skb, msg_head);
	ret = genlmsg_multicast(&family, skb, 0, 0, GFP_KERNEL);
	return 0;
err:
	nlmsg_free(skb);
	return ret;
}

int yt9215_check_phy_linkup(int unit, int port)
{
	yt_port_linkStatus_all_t status;
	int ret;
	ret = yt_port_link_status_all_get(unit, port, &status);
	if (ret)
		return ret;

	return (status.link_status == PORT_LINK_UP) ? PORT_LINK_UP : PORT_LINK_DOWN;
}

static void phy_monitor_work(struct work_struct *work)
{
        struct yt9215_priv *priv = container_of(work, struct yt9215_priv, phy_monitor_work.work);
	static int old_phy_status[BOARD_MAX_UNIT_NUM][YT9215S_PHY_NUM] = {0};
        int i;
	bool notify_easymesh_flag = true;

	for (i = 0; i < YT9215S_PHY_NUM; i++) {
                priv->phy_status[i] = yt9215_check_phy_linkup(priv->unit, i);

                if (old_phy_status[priv->unit][i] != priv->phy_status[i])
                        notify_link_event(priv, i, priv->phy_status[i], "eth0", NULL, 0, notify_easymesh_flag);

                old_phy_status[priv->unit][i] = priv->phy_status[i];
        }

        queue_delayed_work(system_wq, &priv->phy_monitor_work, 2*HZ);
}


static int yt9215_hw_apply(struct switch_dev *dev) {
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	struct yt9215_vlan *e;
	int i;

	if(!priv->vlan_enabled) {
		dev_info(priv->dev, "turn vlan off.\n");
		for(i=0;i<YT9215S_NUM_PORTS - 1;i++) {
			yt_port_mask_t member_mask;
			yt_port_mask_t untagged_mask;
			member_mask.portbits[0] = BIT(i) | BIT(YT9215S_NUM_PORTS - 1);
			untagged_mask.portbits[0] = 0;
			yt_vlan_port_set(priv->unit, i + 1, member_mask, untagged_mask);
			yt_vlan_port_egrTagMode_set(priv->unit, VLAN_TYPE_CVLAN, i, VLAN_TAG_MODE_KEEP_ALL);
		}
		yt_vlan_port_egrTagMode_set(priv->unit, VLAN_TYPE_CVLAN, YT9215S_NUM_PORTS - 1, VLAN_TAG_MODE_KEEP_ALL);
		return 0;
	}

	for(i=0;i<YT9215S_NUM_PORTS;i++) {
		yt_vlan_port_vidTypeSel_set(priv->unit, i, VLAN_TYPE_CVLAN);
		yt_vlan_port_egrTagMode_set(priv->unit, VLAN_TYPE_CVLAN, i, VLAN_TAG_MODE_ENTRY_BASED);
		yt_vlan_port_igrPvid_set(priv->unit, VLAN_TYPE_CVLAN, i, priv->vlan_pvid[i]);
		yt_vlan_port_igrFilter_enable_set(0, i, YT_ENABLE);
		yt_vlan_port_egrFilter_enable_set(0, i, YT_ENABLE);
	}

	list_for_each_entry(e, &priv->vlan_list, list) {
		yt_port_mask_t member_mask;
		yt_port_mask_t untagged_mask;
		member_mask.portbits[0] = e->member_ports;
		untagged_mask.portbits[0] = e->untagged_ports;
		yt_vlan_port_set(priv->unit, e->vid, member_mask, untagged_mask);
	}

	return 0;
}

static int yt9215_get_pvid(struct switch_dev *dev, int port, int *vlan) {
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	if(port >= YT9215S_NUM_PORTS)
		return -EINVAL;
	*vlan = priv->vlan_pvid[port];
	return 0;
}

static int yt9215_set_pvid(struct switch_dev *dev, int port, int vlan) {
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	if(port >= YT9215S_NUM_PORTS)
		return -EINVAL;
	priv->vlan_pvid[port] = vlan;
	return 0;
}

static int yt9215_get_ports(struct switch_dev *dev, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	u8 ports = 0;
	u8 untagged = 0;
	int i;
	struct yt9215_vlan *e;

	list_for_each_entry(e, &priv->vlan_list, list) {
		if(e->vid == val->port_vlan) {
			ports = e->member_ports;
			untagged = e->untagged_ports;
			break;
		}
	}

	val->len = 0;
	for (i = 0; i < YT9215S_NUM_PORTS; i++) {
		struct switch_port *p;

		if (!(ports & (1 << i)))
			continue;

		p = &val->value.ports[val->len++];
		p->id = i;
		if (untagged & (1 << i))
			p->flags = 0;
		else
			p->flags = (1 << SWITCH_PORT_FLAG_TAGGED);
	}
	return 0;
}

static int yt9215_set_ports(struct switch_dev *dev, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	struct yt9215_vlan *cur_vlan = NULL;
	int i;
	struct yt9215_vlan *e;

	list_for_each_entry(e, &priv->vlan_list, list) {
		if (e->vid == val->port_vlan) {
			cur_vlan = e;
			cur_vlan->member_ports = 0;
			cur_vlan->untagged_ports = 0;
			break;
		}
	}

	if (!cur_vlan) {
		cur_vlan = kzalloc(sizeof(*cur_vlan), GFP_KERNEL);
		if (!cur_vlan)
			return -ENOMEM;
		cur_vlan->vid = val->port_vlan;
		list_add_tail(&cur_vlan->list, &priv->vlan_list);
	}

	for (i = 0; i < val->len; i++) {
		struct switch_port *p = &val->value.ports[i];
		cur_vlan->member_ports |= (1 << p->id);
		if (!(p->flags & (1 << SWITCH_PORT_FLAG_TAGGED))) {
			cur_vlan->untagged_ports |= (1 << p->id);
			priv->vlan_pvid[p->id] = val->port_vlan;
		}
	}
	return 0;
}

static int yt9215_set_vlan(struct switch_dev *dev, const struct switch_attr *attr,
		struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	priv->vlan_enabled = !!val->value.i;
	return 0;
}

static int yt9215_get_vlan(struct switch_dev *dev, const struct switch_attr *attr,
		struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	val->value.i = priv->vlan_enabled;
	return 0;
}

static void yt9215_clear_vlans(struct yt9215_priv *priv) {
	struct yt9215_vlan *e, *n;
	list_for_each_entry_safe(e, n, &priv->vlan_list, list) {
		list_del(&e->list);
		kfree(e);
	}
}

static int yt9215_reset_switch(struct switch_dev *dev)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	int i;
	// clear hardware vlan table
	for (i = 1; i < YT_VLAN_ID_MAX; i++) {
		yt_port_mask_t member_mask;
		yt_port_mask_t untagged_mask;
		member_mask.portbits[0] = 0;
		untagged_mask.portbits[0] = 0;
		yt_vlan_port_set(priv->unit, i, member_mask, untagged_mask);
	}
	yt9215_clear_vlans(priv);
	return 0;
}

static int yt9215_get_port_link(struct switch_dev *dev, int port, struct switch_port_link *link)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	yt_port_linkStatus_all_t status;
	yt_enable_t aneg_enable;
	int ret;
	if (port > YT9215S_NUM_PORTS)
		return -EINVAL;
	ret = yt_port_link_status_all_get(priv->unit, port, &status);
	if (ret)
		return ret;
	ret = yt_port_macAutoNeg_enable_get(priv->unit, port, &aneg_enable);
	if (ret)
		return ret;
	link->link = status.link_status == PORT_LINK_UP;
	link->duplex = status.link_duplex == PORT_DUPLEX_FULL;
	link->aneg = aneg_enable == YT_ENABLE;
	link->tx_flow = status.tx_fc_en;
	link->rx_flow = status.rx_fc_en;
	switch (status.link_speed) {
		case PORT_SPEED_10M:
			link->speed = SWITCH_PORT_SPEED_10;
			break;
    		case PORT_SPEED_100M:
			link->speed = SWITCH_PORT_SPEED_100;
			break;
    		case PORT_SPEED_1000M:
			link->speed = SWITCH_PORT_SPEED_1000;
			break;
		default:
			link->speed = SWITCH_PORT_SPEED_UNKNOWN;
			break;
	}

	return 0;
}

static int yt9215_set_port_link(struct switch_dev *dev, int port, struct switch_port_link *link)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	yt_port_force_ctrl_t port_ctrl = {
		.rx_fc_en = true,
		.tx_fc_en = true,
	};
	int ret = 0;
	if (port > YT9215S_NUM_PORTS)
		return -EINVAL;

	switch (link->speed) {
		case SWITCH_PORT_SPEED_10:
			if(link->duplex)
				port_ctrl.speed_dup = PORT_SPEED_DUP_10FULL;
			else
				port_ctrl.speed_dup = PORT_SPEED_DUP_10HALF;
			break;
		case SWITCH_PORT_SPEED_100:
			if(link->duplex)
				port_ctrl.speed_dup = PORT_SPEED_DUP_100FULL;
			else
				port_ctrl.speed_dup = PORT_SPEED_DUP_100HALF;
			break;
		case SWITCH_PORT_SPEED_1000:
			port_ctrl.speed_dup = PORT_SPEED_DUP_1000FULL;
			break;
		default:
			port_ctrl.speed_dup = PORT_SPEED_DUP_END;
			break;
	}

	yt_port_mac_force_set(priv->unit, port, port_ctrl);

	ret = yt_port_macAutoNeg_enable_set(priv->unit, port, link->aneg);

	return ret;
}

static int yt9215_sw_reset_mibs(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val) {
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	return yt_stat_mib_clear_all(priv->unit);
}

static int yt9215_sw_reset_port_mibs(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val) {
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	if (val->port_vlan >= YT9215S_NUM_PORTS)
		return -EINVAL;
	return yt_stat_mib_clear(priv->unit, val->port_vlan);
}

static int yt9215_set_lag_hash(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	uint8_t hash_mask;

	if ((u8)val->value.i >= AGG_HASH_SEL_MAX)
		return -EINVAL;

	if (val->port_vlan >= YT9215S_NUM_PORTS) {
		return -EINVAL;
	} else {
        	sf_yt_lag_hash_sel_get(priv->unit, &hash_mask);
		hash_mask = hash_mask | 1 << val->value.i;
	}

	return sf_yt_lag_hash_sel_set(priv->unit, hash_mask);
}

static int yt9215_get_lag_hash(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	uint8_t hash_mask;

	sf_yt_lag_hash_sel_get(priv->unit, &hash_mask);
	val->value.i = hash_mask;
	printk("The hashmask is 0x%x.\n", val->value.i);
 	return 0;
}

static int yt9215_get_port_fdb_uc_entry(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	l2_ucastMacAddr_info_t pUcastMac;
	uint16_t fdb_index = 0, next_fdb_index;
	static char buf[64];
	int len = 0;
	yt_port_t port;
	yt_vlan_t vid;

	if (val->port_vlan >= YT9215S_NUM_PORTS)
		return -EINVAL;

	memset(&pUcastMac, 0, sizeof(pUcastMac));

	while ( fdb_index <= 4096 ) {
		next_fdb_index = 0;
		fal_tiger_l2_fdb_uc_withindex_getnext(priv->unit, fdb_index, &next_fdb_index, &pUcastMac);
		if (next_fdb_index) {
			fdb_index = next_fdb_index;
			port = pUcastMac.port;
			vid = pUcastMac.vid;
			if (val->port_vlan == port) {
				printk("The mac:%pM vlan_id:%d is at the yt9215s's port:%d\n",
					pUcastMac.macaddr.addr, vid, port);
					notify_link_event(priv, port, priv->phy_status[port], "eth0", pUcastMac.macaddr.addr, vid, false);
			}
		fdb_index++;
		} else {
			break;
		}
	}
	len += snprintf(buf + len, sizeof(buf) - len, "These are the port %d of yt9215s's fdb entry.\n", val->port_vlan);

	val->value.s = buf;
	val->len = len;

 	return 0;
}

static int yt9215_get_info_fdb_uc_entry(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	l2_ucastMacAddr_info_t pUcastMac;
	static char buf[64];
	int len = 0;
	uint16_t fdb_index = 0, next_fdb_index;
	yt_port_t port;
	yt_vlan_t vid;

	memset(&pUcastMac, 0, sizeof(pUcastMac));

	while ( fdb_index <= 4096 ) {
		next_fdb_index = 0;
		fal_tiger_l2_fdb_uc_withindex_getnext(priv->unit, fdb_index, &next_fdb_index, &pUcastMac);
		if (next_fdb_index) {
			fdb_index = next_fdb_index;
			port = pUcastMac.port;
			vid = pUcastMac.vid;
			printk("The mac:%pM vlan_id:%d is at the yt9215s's port:%d, the index is %d\n",
					pUcastMac.macaddr.addr, vid, port, next_fdb_index );
		fdb_index++;
		} else {
			break;
		}
	}
	len += snprintf(buf + len, sizeof(buf) - len, "These are the yt9215s's fdb entry.\n");

	val->value.s = buf;
	val->len = len;

 	return 0;
}

static int yt9215_get_info_lag_group(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	yt_link_agg_group_t yt_p_laginfo;
	static char buf[128];
	int len = 0;

	memset(&yt_p_laginfo, 0, sizeof(yt_p_laginfo));
	yt_lag_group_info_get(priv->unit, 0, &yt_p_laginfo);
	len += snprintf(buf + len, sizeof(buf) - len, "The portmask of the lag group with groupid 0 is 0x%x.\n", yt_p_laginfo.member_portmask);

	memset(&yt_p_laginfo, 0, sizeof(yt_p_laginfo));
	yt_lag_group_info_get(priv->unit, 1, &yt_p_laginfo);
	len += snprintf(buf + len, sizeof(buf) - len, "The portmask of the lag group with groupid 1 is 0x%x.\n", yt_p_laginfo.member_portmask);

	val->value.s = buf;
	val->len = len;
 	return 0;
}

static int yt9215_set_port_lag_group(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	yt_link_agg_group_t yt_p_laginfo;
	yt_port_mask_t portmask;

	if ((u8)val->value.i >= LAG_GROUPID_MAX_NUM)
		return -EINVAL;

	if (val->port_vlan >= YT9215S_NUM_PORTS) {
		return -EINVAL;
	} else {
        	if (val->value.i == 0) {
			yt_lag_group_info_get(priv->unit, 0, &yt_p_laginfo);
        		CMM_SET_MEMBER_PORT(portmask, val->port_vlan);
			portmask.portbits[0] = portmask.portbits[0] | yt_p_laginfo.member_portmask;
		} else {
			yt_lag_group_info_get(priv->unit, 1, &yt_p_laginfo);
			CMM_SET_MEMBER_PORT(portmask, val->port_vlan);
			portmask.portbits[0] = portmask.portbits[0] | yt_p_laginfo.member_portmask;
		}
	}

	return yt_lag_group_port_set(priv->unit, (u8)val->value.i, portmask);
}

static int yt9215_get_port_lag_group(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	yt_link_agg_group_t yt_p_laginfo;
	yt_port_mask_t portmask;

	yt_lag_group_info_get(priv->unit, 0, &yt_p_laginfo);
	CMM_SET_MEMBER_PORT(portmask, val->port_vlan);
	if (portmask.portbits[0] & yt_p_laginfo.member_portmask) {
		val->value.i = 0;
	} else {
		yt_lag_group_info_get(priv->unit, 1, &yt_p_laginfo);
		if (portmask.portbits[0] & yt_p_laginfo.member_portmask)
			val->value.i = 1;
		else
			val->value.i = -1;
	}
 	return 0;
}

static int yt9215_reset_lag_group(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val) {
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	yt_port_mask_t portmask;

	portmask.portbits[0] = 0;
	yt_lag_group_port_set(priv->unit, 0, portmask);
	yt_lag_group_port_set(priv->unit, 1, portmask);
	sf_yt_lag_hash_sel_set(priv->unit, (uint8_t)0);
	return 0;
}

static int yt9215_set_all_phy_enable(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	int i;

	for (i = 0; i < YT9215S_PHY_NUM; i++)
		yt_port_enable_set(priv->unit, i, val->value.i);

	return 0;
}

static int yt9215_set_phy_enable(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val)
{
	struct yt9215_priv *priv = swdev_to_yt9215(dev);

	if (val->port_vlan >= YT9215S_PHY_NUM)
		return -EINVAL;

	yt_port_enable_set(priv->unit, val->port_vlan, val->value.i);

	return 0;
}

#define MIB_PR_UINT32(CNT) len += snprintf(buf + len, sizeof(buf) - len, "\t%-22s : %u\n", #CNT, yt_cnt.CNT)
#define MIB_PR_UINT64(CNT) len += snprintf(buf + len, sizeof(buf) - len, "\t%-22s : %llu\n", #CNT, yt_cnt.CNT)
static int yt9215_get_port_mib(struct switch_dev *dev, const struct switch_attr *attr, struct switch_val *val) {
	struct yt9215_priv *priv = swdev_to_yt9215(dev);
	int ret;
	static char buf[4096];
	yt_stat_mib_port_cnt_t yt_cnt;
	int len = 0;

	if (val->port_vlan >= YT9215S_NUM_PORTS)
		return -EINVAL;

	ret = yt_stat_mib_port_get(priv->unit, val->port_vlan, &yt_cnt);
	if (ret)
		return ret;

	// FIXME: snprintf is probably unsafe...
	len += snprintf(buf + len, sizeof(buf) - len, "Port %d MIB counters\n", val->port_vlan);
	MIB_PR_UINT32(RX_BROADCAST);
	MIB_PR_UINT32(RX_PAUSE);
	MIB_PR_UINT32(RX_MULTICAST);
	MIB_PR_UINT32(RX_FCS_ERR);
	MIB_PR_UINT32(RX_ALIGNMENT_ERR);
	MIB_PR_UINT32(RX_UNDERSIZE);
	MIB_PR_UINT32(RX_FRAGMENT);
	MIB_PR_UINT32(RX_64B);
	MIB_PR_UINT32(RX_65_127B);
	MIB_PR_UINT32(RX_128_255B);
	MIB_PR_UINT32(RX_256_511B);
	MIB_PR_UINT32(RX_512_1023B);
	MIB_PR_UINT32(RX_1024_1518B);
	MIB_PR_UINT32(RX_JUMBO);
	MIB_PR_UINT64(RX_OKBYTE);
	MIB_PR_UINT64(RX_NOT_OKBYTE);
	MIB_PR_UINT32(RX_OVERSIZE);
	MIB_PR_UINT32(RX_DISCARD);
	MIB_PR_UINT32(TX_BROADCAST);
	MIB_PR_UINT32(TX_PAUSE);
	MIB_PR_UINT32(TX_MULTICAST);
	MIB_PR_UINT32(TX_UNDERSIZE);
	MIB_PR_UINT32(TX_64B);
	MIB_PR_UINT32(TX_65_127B);
	MIB_PR_UINT32(TX_128_255B);
	MIB_PR_UINT32(TX_256_511B);
	MIB_PR_UINT32(TX_512_1023B);
	MIB_PR_UINT32(TX_1024_1518B);
	MIB_PR_UINT32(TX_JUMBO);
	MIB_PR_UINT64(TX_OKBYTE);
	MIB_PR_UINT32(TX_COLLISION);
	MIB_PR_UINT32(TX_EXCESSIVE_COLLISION);
	MIB_PR_UINT32(TX_MULTI_COLLISION);
	MIB_PR_UINT32(TX_SINGLE_COLLISION);
	MIB_PR_UINT32(TX_OK_PKT);
	MIB_PR_UINT32(TX_DEFER);
	MIB_PR_UINT32(TX_LATE_COLLISION);
	MIB_PR_UINT32(RX_OAM_COUNTER);
	MIB_PR_UINT32(TX_OAM_COUNTER);
	MIB_PR_UINT32(RX_UNICAST);
	MIB_PR_UINT32(TX_UNICAST);

	val->value.s = buf;
	val->len = len;
	return 0;
}


static struct switch_attr yt9215_globals[] = {
	{
		.type = SWITCH_TYPE_INT,
		.name = "enable_vlan",
		.description = "Enable VLAN mode",
		.set = yt9215_set_vlan,
		.get = yt9215_get_vlan,
		.max = 1
	}, {
		.type = SWITCH_TYPE_NOVAL,
		.name = "reset_mib",
		.description = "Reset all MIB counters",
		.set = yt9215_sw_reset_mibs,
	}, {
		.type = SWITCH_TYPE_STRING,
		.name = "lag_group",
		.description = "Get the lag_groups' information",
		.get = yt9215_get_info_lag_group,
	}, {
		.type = SWITCH_TYPE_INT,
		.name = "enable_port",
		.description = "Set all port phy enable",
		.set = yt9215_set_all_phy_enable,
	}, {
		.type = SWITCH_TYPE_NOVAL,
		.name = "reset_lag",
		.description = "Reset both lag groups",
		.set = yt9215_reset_lag_group,
	}, {
		.type = SWITCH_TYPE_INT,
		.name = "lag_hash",
		.description = "Set the lag_groups' hash mask and get it",
		.get = yt9215_get_lag_hash,
		.set = yt9215_set_lag_hash,
	}, {
		.type = SWITCH_TYPE_STRING,
		.name = "fdb_entry",
		.description = "Get the switch's fdb unicast entry",
		.get = yt9215_get_info_fdb_uc_entry,
	}
};

static struct switch_attr yt9215_port[] = {
	{
		.type = SWITCH_TYPE_STRING,
		.name = "mib",
		.description = "Get MIB counters for port",
		.get = yt9215_get_port_mib,
		.set = NULL,
	}, {
		.type = SWITCH_TYPE_NOVAL,
		.name = "reset_mib",
		.description = "Reset single port MIB counters",
		.set = yt9215_sw_reset_port_mibs,
	}, {
		.type = SWITCH_TYPE_INT,
		.name = "lag_group",
		.description = "Set the lag_groups and know which lag_group the port is in",
		.get = yt9215_get_port_lag_group,
		.set = yt9215_set_port_lag_group,
	}, {
		.type = SWITCH_TYPE_INT,
		.name = "enable_port",
		.description = "Set the port phy enable",
		.set = yt9215_set_phy_enable,
	},{
		.type = SWITCH_TYPE_STRING,
		.name = "fdb_entry",
		.description = "Get the switch's each port's fdb unicast entry",
		.get = yt9215_get_port_fdb_uc_entry,
	}
};

static struct switch_attr yt9215_vlan[] = {
};

static const struct switch_dev_ops yt9215_ops = {
	.attr_global = {
		.attr = yt9215_globals,
		.n_attr = ARRAY_SIZE(yt9215_globals),
	},
	.attr_port = {
		.attr = yt9215_port,
		.n_attr = ARRAY_SIZE(yt9215_port),
	},
	.attr_vlan = {
		.attr = yt9215_vlan,
		.n_attr = ARRAY_SIZE(yt9215_vlan),
	},
	.get_port_pvid = yt9215_get_pvid,
	.set_port_pvid = yt9215_set_pvid,
	.get_vlan_ports = yt9215_get_ports,
	.set_vlan_ports = yt9215_set_ports,
	.apply_config = yt9215_hw_apply,
	.reset_switch = yt9215_reset_switch,
	.get_port_link = yt9215_get_port_link,
	.set_port_link = yt9215_set_port_link,
};

static int yt9215_init_all(void) {
	yt_port_force_ctrl_t yt_port_ctrl = {
		.speed_dup = PORT_SPEED_DUP_2500FULL,
		.rx_fc_en = true,
		.tx_fc_en = true,
	};
	int err;
	int i, j;

	yt9215s_fib_swDescp[0].sw_access.swreg_acc_method = SWCHIP_ACC_SMI;
	yt9215s_fib_swDescp[0].sw_access.controller.smi_controller.smi_read = yt_smi0_cl22_read;
	yt9215s_fib_swDescp[0].sw_access.controller.smi_controller.smi_write = yt_smi0_cl22_write;
	yt9215s_fib_swDescp[0].sw_access.controller.smi_controller.switchId = 0x0;

#if BOARD_MAX_UNIT_NUM == 2
	yt9215s_fib_swDescp[1].sw_access.swreg_acc_method = SWCHIP_ACC_SMI;
	yt9215s_fib_swDescp[1].sw_access.controller.smi_controller.smi_read = yt_smi1_cl22_read;
	yt9215s_fib_swDescp[1].sw_access.controller.smi_controller.smi_write = yt_smi1_cl22_write;
	yt9215s_fib_swDescp[1].sw_access.controller.smi_controller.switchId = 0x0;
#endif

	err = yt_init();
	if (err) {
		printk("init error %d\n", err);
		return -1;
	}

	err = sf_family_init(&family, "updown");
	if(err)
		return err;

	for (i = 0; i < yt_swconfig_probed; i++) {
		memset(yt_priv[i].phy_status, 0, sizeof(yt_priv[i].phy_status));
		memset(yt_priv[i].dev_num, 0, sizeof(yt_priv[i].dev_num));

		INIT_DELAYED_WORK(&yt_priv[i].phy_monitor_work, phy_monitor_work);

		yt_port_enable_set(i, 5, YT_ENABLE);
		yt_port_extif_mode_set(i, 5, YT_EXTIF_MODE_BX2500);
		yt_port_mac_force_set(i, 5, yt_port_ctrl);

		//set TX IPG to avoid packet drop
		hal_mem32_write(i, 0x81000, 0xb00e5858);
		hal_mem32_write(i, 0x82000, 0xb00e5858);
		hal_mem32_write(i, 0x83000, 0xb00e5858);
		hal_mem32_write(i, 0x84000, 0xb00e5858);
		hal_mem32_write(i, 0x85000, 0xb00e5858);
		hal_mem32_write(i, 0x89000, 0xb00e5858);
		hal_mem32_write(i, 0x8a000, 0xb00e5858);

		//init 2.5G serdes to support eye pattern
		hal_mem32_write(i, 0x8008c, 0x27a);
		hal_mem32_write(i, 0x80028, 0x1);
		hal_mem32_write(i, 0xf0004, 0x800);
		HALSWDRV_FUNC(i)->switch_intif_write(i, 0x8, 0x1e, 0xa0);

		hal_mem32_write(i, 0xf0004, 0x800);
		HALSWDRV_FUNC(i)->switch_intif_write(i, 0x8, 0x1f, 0x8c00);

		hal_mem32_write(i, 0xf0004, 0x800);
		HALSWDRV_FUNC(i)->switch_intif_write(i, 0x8, 0x1e, 0xa2);

		hal_mem32_write(i, 0xf0004, 0x800);
		HALSWDRV_FUNC(i)->switch_intif_write(i, 0x8, 0x1f, 0x112);

		hal_mem32_write(i, 0xf0004, 0x800);
		HALSWDRV_FUNC(i)->switch_intif_write(i, 0x8, 0x1e, 0xa1);

		hal_mem32_write(i, 0xf0004, 0x800);
		HALSWDRV_FUNC(i)->switch_intif_write(i, 0x8, 0x1f, 0xf8ce);

		yt_stat_mib_enable_set(i, YT_ENABLE);

		for (j = 0; j < YT9215S_NUM_PORTS; j++) {
			yt_vlan_t vid;
			yt_vlan_port_igrPvid_get(i, VLAN_TYPE_CVLAN, j, &vid);
			yt_priv[i].vlan_pvid[j] = vid;
		}

		yt9215_reset_switch(&yt_priv[i].swdev);
		yt9215_hw_apply(&yt_priv[i].swdev);

		err = register_switch(&yt_priv[i].swdev, NULL);
		if (err)
			return err;

		queue_delayed_work(system_wq, &yt_priv[i].phy_monitor_work, 2*HZ);
	}

	return 0;
}

static int sf_eswitch_probe(struct mdio_device *mdiodev) {
	struct yt9215_priv *priv = &yt_priv[yt_swconfig_probed];
	struct switch_dev *swdev = &priv->swdev;
	priv->unit = yt_swconfig_probed;

	yt_swconfig_probed++;
	if (yt_swconfig_probed > BOARD_MAX_UNIT_NUM) {
		dev_err(&mdiodev->dev, "too many switches.\n");
		return -EINVAL;
	}

	yt9215s_fib_swDescp[priv->unit].sw_access.controller.smi_controller.phyAddr = mdiodev->addr;

	priv->mii_bus = mdiodev->bus;
	priv->dev = &mdiodev->dev;
	priv->mdiodev = mdiodev;

	mutex_init(&priv->mac_lock);

	priv->vlan_enabled = false;
	INIT_LIST_HEAD(&priv->vlan_list);
	INIT_LIST_HEAD(&priv->mac_list[priv->unit]);

	swdev->name = "Motorcomm YT9215S";
	swdev->ports = YT9215S_NUM_PORTS;
	swdev->cpu_port = YT9215S_NUM_PORTS - 1;
	swdev->vlans = YT_VLAN_ID_MAX;
	swdev->ops = &yt9215_ops;
	swdev->alias = "YT9215S";

	dev_set_drvdata(&mdiodev->dev, priv);

	if (yt_swconfig_probed == BOARD_MAX_UNIT_NUM)
		return yt9215_init_all();
	return 0;
}

static void sf_eswitch_remove(struct mdio_device *mdiodev) {
	struct yt9215_priv *priv = dev_get_drvdata(&mdiodev->dev);
	unregister_switch(&priv->swdev);
	yt9215_clear_vlans(priv);
}

static const struct of_device_id eswitch_match[] = {
    { .compatible = "motorcomm,yt9215s" },
    { /* sentinel */ },
};

static struct mdio_driver sf_eswitch_driver = {
	.mdiodrv.driver = {
		.name = "yt9215s",
		.of_match_table = eswitch_match,
	},
	.probe = sf_eswitch_probe,
	.remove = sf_eswitch_remove,
};

static ssize_t smi_write_proc(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char *str, *unit_str, *cmd, *value;
	char tmpbuf[128] = {0};
	u32 unit = 0;
	uint32_t regAddr = 0;
	uint32_t regData = 0;
	uint32_t rData = 0;
	if(count >= sizeof(tmpbuf))
		goto error;
	if(!buffer || copy_from_user(tmpbuf, buffer, count) != 0)
		return 0;
	if(yt_swconfig_probed < BOARD_MAX_UNIT_NUM) {
		pr_err("switch isn't all probed yet.\n");
		return 0;
	}
	if (count > 0)
	{
		str = tmpbuf;
		unit_str = strsep(&str, "\t \n");
		if (!unit_str) {
			goto error;
		} else if(unit_str[0] < '0' || unit_str[0] > '9') {
			unit = 0;
			cmd = unit_str;
		} else {
			unit = simple_strtoul(unit_str, &unit_str, 10);
			cmd = strsep(&str, "\t \n");
			if (unit >= yt_swconfig_probed)
				goto error;
		}
		if (!cmd)
			goto error;

		if (strcmp(cmd, "write") == 0) {
			value = strsep(&str, "\t \n");
			if (!value)
				goto error;
			regAddr = simple_strtoul(value, &value, 16);

			value = strsep(&str, "\t \n");
			if (!value)
				goto error;
			regData = simple_strtoul(value, &value, 16);
			dev_info(yt_priv[unit].dev, "write regAddr = 0x%x regData = 0x%x\n", regAddr, regData);
			hal_mem32_write(unit, regAddr, regData);
		}
		else if (strcmp(cmd, "read") == 0)
		{
			value = strsep(&str, "\t \n");
			if (!value)
			{
				goto error;
			}
			regAddr = simple_strtoul(value, &value, 16);
			hal_mem32_read(unit, regAddr, &rData);
			dev_info(yt_priv[unit].dev, "read regAddr = 0x%x regData = 0x%x\n", regAddr, rData);
		}
		else
		{
			goto error;
		}
	}
	return count;

	error:
	printk("usage: \n");
	printk(" <unit> read {regaddr}: for example, echo 0 read 0xd0004 > /proc/yt_reg\n");
	printk(" <unit> write {regaddr} {regdata}: for example; echo 0 write 0xd0004 0x680 > /proc/yt_reg\n");
	return -EFAULT;
}

static struct proc_dir_entry *smi_proc;
static const struct proc_ops smi_proc_fops = {
	.proc_read = NULL,
	.proc_write = smi_write_proc,
};

static int __init mdio_module_init(void)
{
	int ret;
	ret = mdio_driver_register(&sf_eswitch_driver);
	if(ret)
		return ret;
	smi_proc = proc_create("yt_reg", 0666, NULL, &smi_proc_fops);
	return 0;
}
module_init(mdio_module_init);
static void __exit mdio_module_exit(void)
{
	mdio_driver_unregister(&sf_eswitch_driver);
	proc_remove(smi_proc);
	yt_exit(0); /* this unit parameter isn't used. */
}
module_exit(mdio_module_exit)

/* Why the heck is this SDK proprietary!? */
MODULE_LICENSE("GPL");
