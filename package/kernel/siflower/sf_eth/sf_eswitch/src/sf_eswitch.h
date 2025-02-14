#ifndef _SF_ESWITCH_H_
#define _SF_ESWITCH_H_

#include <linux/switch.h>
#include <linux/phy.h>

#define SF_VERSION              "1.0"

#define NF2507_ID				0x1619
#define RTK8367C_ID				0x6367
#define INTEL7084_ID			0x2003
#define INTEL7082_ID			0x3003
/* Description: 'PHY Address' */
#define PHY_ADDR_0_ADDR_OFFSET	0xF415
#define PHY_ADDR_0_ADDR_SHIFT	0
#define PHY_ADDR_0_ADDR_SIZE	5

#define SWITCH_PORT_LIST				0xf
#define RGMII_PORT0						0x5
#define RTK_PHY_PORT_NUM				5
#define INTEL_SWITCH_PORT_NUM			7
#define INTEL_PHY_PORT_NUM				5

#define PHY_CTRL_ENABLE_POWER_DOWN					(1 << 11)
#define PHY_IDENTIFY_1					0x02
#define PHY_IDENTIFY_2					0x03

// extern spinlock_t	mdio_lock;
extern struct mutex op_switch_lock;
// #define SF_MDIO_LOCK()				spin_lock(&mdio_lock)
// #define SF_MDIO_UNLOCK()			spin_unlock(&mdio_lock)
#define SF_MDIO_LOCK()				mutex_lock(&op_switch_lock);
#define SF_MDIO_UNLOCK()			mutex_unlock(&op_switch_lock);

enum sf_eswitch_model{
	UNKNOWN = 0,
	RTK8367C,
	INTEL7084,
	INTEL7082,
	NF2507
};

enum led_mode{
	LED_NORMAL = 0, // led on when link 10/100/1000Mbps, blink when tx/rx
	LED_ALL_ON,
	LED_ALL_OFF,
	LED_ALL_BLINK
};

struct vlan_entry {
	u16 vid;
	u32 member;
	struct list_head entry_list;
};

struct sf_eswitch_priv {
	struct device    *dev;
	struct mii_bus    *bus;
	struct switch_dev	swdev;

	int model;
	int port_list;
	u32 phy_status;
	struct sf_eswitch_api_t *pesw_api;

#ifdef CONFIG_DEBUG_FS
	struct dentry     *esw_debug;
#endif

	int (*init)(struct mdio_device *pdev);
	int (*deinit)(struct mdio_device *pdev);
	unsigned char (*init_swdev)(struct mdio_device *pdev, struct mii_bus* pmii_bus);
	void (*deinit_swdev)(struct mdio_device *pdev);
	void (*write_phy)(struct sf_eswitch_priv* priv, int phyNo, int phyReg, int phyData) ;
	unsigned int (*read_phy)(struct sf_eswitch_priv* priv , int phyNo, int phyReg);
};

struct sf_eswitch_api_t {
	struct switch_dev_ops *ops;
	void (*vender_init)( struct sf_eswitch_priv *eswitch_priv);
	void (*vender_deinit)( struct sf_eswitch_priv *eswitch_priv);
	void (*led_init)( int led_mode);
	void (*ifg_init)(void);
	void (*enable_all_phy)(struct sf_eswitch_priv *pesw_priv);
	void (*disable_all_phy)(void);
	int (*check_phy_linkup)(int port);
	u32 (*get_cpu_port_rx_mib)(void);
	int (*set_cpu_port_self_mirror)(struct sf_eswitch_priv *pesw_priv, int port, int enable);
	int (*getAsicReg)(unsigned int reg, unsigned int *pValue);
	int (*setAsicReg)(unsigned int reg, unsigned int pValue);
	int (*getAsicPHYReg)(unsigned int phyNo, unsigned int phyAddr, unsigned int *pRegData);
	int (*setAsicPHYReg)(unsigned int phyNo, unsigned int phyAddr, unsigned int pRegData);
	int (*set_rgmii)(bool enable);
	int (*irq_init)(int type);
	int (*irq_ack)(void);
	void (*dumpmac)(char macaddr[], int port);
};

static inline bool check_port_in_portlist(struct sf_eswitch_priv *pesw_priv, int port)
{
	return BIT(port) & pesw_priv->port_list;
}

#endif
