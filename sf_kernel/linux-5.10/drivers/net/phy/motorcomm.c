// SPDX-License-Identifier: GPL-2.0+
/*
 * drivers/net/phy/motorcomm.c
 *
 * Driver for Motorcomm PHYs
 *
 * Author: yinghong.zhang<yinghong.zhang@motor-comm.com>
 *
 * Copyright (c) 2024 Motorcomm, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * Support : Motorcomm Phys:
 *        Giga phys: yt8511, yt8521, yt8531, yt8543, yt8614, yt8618
 *        100/10M Phys : yt8510, yt8512, yt8522
 *        Automotive 100M Phys : yt8010
 *        Automotive 1000M Phys: yt8011
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/of.h>
#include <linux/clk.h>
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif

/* for wol feature */
#include <linux/netdevice.h>

#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#define PROC_FILENAME "ytphydrv_ver"
#define YTPHY_LINUX_VERSION "2.2.41378"
#define BUFFER_SIZE 128
static struct proc_dir_entry *proc_file;

/********************************************
 **** configuration section begin ***********/

/* if system depends on ethernet packet to restore from sleep,
 * please define this macro to 1 otherwise, define it to 0.
 */
#define SYS_WAKEUP_BASED_ON_ETH_PKT     1

/* to enable system WOL feature of phy, please define this macro to 1
 * otherwise, define it to 0.
 */
#define YTPHY_WOL_FEATURE_ENABLE        0

/* some GMAC need clock input from PHY, for eg., 125M,
 * please enable this macro
 * by degault, it is set to 0
 * NOTE: this macro will need macro SYS_WAKEUP_BASED_ON_ETH_PKT to set to 1
 */
#define GMAC_CLOCK_INPUT_NEEDED         0

/* for YT8531 package A xtal init config */
#define YTPHY8531A_XTAL_INIT                (0)

/**** configuration section end ***********
 ******************************************/

/* no need to change below */
#define MOTORCOMM_PHY_ID_MASK           0xffffffff

#define PHY_ID_YT8010                   0x00000309
#define PHY_ID_YT8010AS                 0x4f51eb19
#define PHY_ID_YT8011                   0x4f51eb01
#define PHY_ID_YT8510                   0x00000109
#define PHY_ID_YT8511                   0x0000010a
#define PHY_ID_YT8512                   0x00000128
#define PHY_ID_YT8522                   0x4f51e928
#define PHY_ID_YT8521                   0x0000011a
#define PHY_ID_YT8531S                  0x4f51e91a
#define PHY_ID_YT8531                   0x4f51e91b
/* YT8543 phy driver disable(default) */
//#define YTPHY_YT8543_ENABLE
#ifdef YTPHY_YT8543_ENABLE
#define PHY_ID_YT8543                   0x0008011b
#endif
#define PHY_ID_YT8614                   0x4f51e899
#define PHY_ID_YT8614Q                  0x4f51e8a9
#define PHY_ID_YT8618                   0x4f51e889
#define PHY_ID_YT8821                   0x4f51ea19

#define REG_PHY_SPEC_STATUS             0x11
#define REG_DEBUG_ADDR_OFFSET           0x1e
#define REG_DEBUG_DATA                  0x1f
#define REG_MII_MMD_CTRL                0x0D    /* MMD access control register */
#define REG_MII_MMD_DATA                0x0E    /* MMD access data register    */

#define YT8512_EXTREG_LED0              0x40c0
#define YT8512_EXTREG_LED1              0x40c3

#define YT8512_EXTREG_SLEEP_CONTROL1    0x2027
#define YT8512_EXTENDED_COMBO_CONTROL1  0x4000
#define YT8512_10BT_DEBUG_LPBKS         0x200A

#define YT_SOFTWARE_RESET               0x8000

#define YT8512_LED0_ACT_BLK_IND         0x1000
#define YT8512_LED0_DIS_LED_AN_TRY      0x0001
#define YT8512_LED0_BT_BLK_EN           0x0002
#define YT8512_LED0_HT_BLK_EN           0x0004
#define YT8512_LED0_COL_BLK_EN          0x0008
#define YT8512_LED0_BT_ON_EN            0x0010
#define YT8512_LED1_BT_ON_EN            0x0010
#define YT8512_LED1_TXACT_BLK_EN        0x0100
#define YT8512_LED1_RXACT_BLK_EN        0x0200
#define YT8512_EN_SLEEP_SW_BIT          15

#define YT8522_TX_CLK_DELAY             0x4210
#define YT8522_ANAGLOG_IF_CTRL          0x4008
#define YT8522_DAC_CTRL                 0x2057
#define YT8522_INTERPOLATOR_FILTER_1    0x14
#define YT8522_INTERPOLATOR_FILTER_2    0x15
#define YT8522_EXTENDED_COMBO_CTRL_1    0x4000
#define YT8522_TX_DELAY_CONTROL         0x19
#define YT8522_EXTENDED_PAD_CONTROL     0x4001

#define YT8521_EXTREG_SLEEP_CONTROL1    0x27
#define YT8521_EN_SLEEP_SW_BIT          15

#define YTXXXX_SPEED_MODE               0xc000
#define YTXXXX_DUPLEX                   0x2000
#define YTXXXX_SPEED_MODE_BIT           14
#define YTXXXX_DUPLEX_BIT               13
#define YTXXXX_AUTO_NEGOTIATION_BIT     12
#define YTXXXX_ASYMMETRIC_PAUSE_BIT     11
#define YTXXXX_PAUSE_BIT                10
#define YTXXXX_LINK_STATUS_BIT          10

#define YT8821_SDS_ASYMMETRIC_PAUSE_BIT 8
#define YT8821_SDS_PAUSE_BIT            7

/* based on yt8521 wol feature config register */
#define YTPHY_UTP_INTR_REG              0x12
/* WOL Feature Event Interrupt Enable */
#define YTPHY_WOL_FEATURE_INTR          BIT(6)

/* Magic Packet MAC address registers */
#define YTPHY_WOL_FEATURE_MACADDR2_4_MAGIC_PACKET    0xa007
#define YTPHY_WOL_FEATURE_MACADDR1_4_MAGIC_PACKET    0xa008
#define YTPHY_WOL_FEATURE_MACADDR0_4_MAGIC_PACKET    0xa009

#define YTPHY_WOL_FEATURE_REG_CFG               0xa00a
#define YTPHY_WOL_FEATURE_TYPE_CFG              BIT(0)    /* WOL TYPE Config */
#define YTPHY_WOL_FEATURE_ENABLE_CFG            BIT(3)    /* WOL Enable Config */
#define YTPHY_WOL_FEATURE_INTR_SEL_CFG          BIT(6)    /* WOL Event Interrupt Enable Config */
#define YTPHY_WOL_FEATURE_WIDTH1_CFG            BIT(1)    /* WOL Pulse Width Config */
#define YTPHY_WOL_FEATURE_WIDTH2_CFG            BIT(2)    /* WOL Pulse Width Config */

#define YTPHY_REG_SPACE_UTP             0
#define YTPHY_REG_SPACE_FIBER           2

enum ytphy_wol_feature_trigger_type_e {
	YTPHY_WOL_FEATURE_PULSE_TRIGGER,
	YTPHY_WOL_FEATURE_LEVEL_TRIGGER,
	YTPHY_WOL_FEATURE_TRIGGER_TYPE_MAX
};

enum ytphy_wol_feature_pulse_width_e {
	YTPHY_WOL_FEATURE_672MS_PULSE_WIDTH,
	YTPHY_WOL_FEATURE_336MS_PULSE_WIDTH,
	YTPHY_WOL_FEATURE_168MS_PULSE_WIDTH,
	YTPHY_WOL_FEATURE_84MS_PULSE_WIDTH,
	YTPHY_WOL_FEATURE_PULSE_WIDTH_MAX
};

struct ytphy_wol_feature_cfg {
	bool enable;
	int type;
	int width;
};

#if (YTPHY_WOL_FEATURE_ENABLE)
#undef SYS_WAKEUP_BASED_ON_ETH_PKT
#define SYS_WAKEUP_BASED_ON_ETH_PKT     1
#endif

struct yt8xxx_priv {
	u8 polling_mode;
	u8 chip_mode;
};

/* polling mode */
#define YT_PHY_MODE_FIBER           1 //fiber mode only
#define YT_PHY_MODE_UTP             2 //utp mode only
#define YT_PHY_MODE_POLL            (YT_PHY_MODE_FIBER | YT_PHY_MODE_UTP)

static int ytxxxx_soft_reset(struct phy_device *phydev);

/* support automatically check polling mode for yt8521
 * for Fiber only system, please define YT8521_PHY_MODE_CURR 1
 * for UTP only system, please define YT8521_PHY_MODE_CURR 2
 * for combo system, please define YT8521_PHY_MODE_CURR 3
 */
#define YTPHY_861X_ABC_VER              0
#if (YTPHY_861X_ABC_VER)
static int yt8614_get_port_from_phydev(struct phy_device *phydev);
#endif

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
static int ytphy_config_init(struct phy_device *phydev)
{
	int val;

	val = phy_read(phydev, 3);

	return 0;
}
#endif


#if (KERNEL_VERSION(5, 5, 0) > LINUX_VERSION_CODE)
static inline void phy_lock_mdio_bus(struct phy_device *phydev)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	mutex_lock(&phydev->bus->mdio_lock);
#else
	mutex_lock(&phydev->mdio.bus->mdio_lock);
#endif
}

static inline void phy_unlock_mdio_bus(struct phy_device *phydev)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	mutex_unlock(&phydev->bus->mdio_lock);
#else
	mutex_unlock(&phydev->mdio.bus->mdio_lock);
#endif
}
#endif

#if (KERNEL_VERSION(4, 16, 0) > LINUX_VERSION_CODE)
static inline int __phy_read(struct phy_device *phydev, u32 regnum)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	struct mii_bus *bus = phydev->bus;
	int addr = phydev->addr;
	return bus->read(bus, phydev->addr, regnum);
#else
	struct mii_bus *bus = phydev->mdio.bus;
	int addr = phydev->mdio.addr;
#endif
	return bus->read(bus, addr, regnum);
}

static inline int __phy_write(struct phy_device *phydev, u32 regnum, u16 val)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	struct mii_bus *bus = phydev->bus;
	int addr = phydev->addr;
#else
	struct mii_bus *bus = phydev->mdio.bus;
	int addr = phydev->mdio.addr;
#endif
	return bus->write(bus, addr, regnum, val);
}
#endif

static int ytphy_read_ext(struct phy_device *phydev, u32 regnum)
{
	int ret;

	ret = __phy_write(phydev, REG_DEBUG_ADDR_OFFSET, regnum);
	if (ret < 0)
		return ret;

	ret = __phy_read(phydev, REG_DEBUG_DATA);
	if (ret < 0)
		return ret;

	return ret;
}

static int ytphy_write_ext(struct phy_device *phydev, u32 regnum, u16 val)
{
	int ret;

	ret = __phy_write(phydev, REG_DEBUG_ADDR_OFFSET, regnum);
	if (ret < 0)
		return ret;

	ret = __phy_write(phydev, REG_DEBUG_DATA, val);
	if (ret < 0)
		return ret;

	return ret;
}

__attribute__((unused)) static int ytphy_read_mmd(struct phy_device* phydev, u16 device, u16 reg)
{
	int val;

	phy_lock_mdio_bus(phydev);

	__phy_write(phydev, REG_MII_MMD_CTRL, device);
	__phy_write(phydev, REG_MII_MMD_DATA, reg);
	__phy_write(phydev, REG_MII_MMD_CTRL, device | 0x4000);
	val = __phy_read(phydev, REG_MII_MMD_DATA);
	if (val < 0) {
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
		dev_err(&phydev->dev, "error read mmd device(%u) reg (%u)\n", device, reg);
#else
		dev_err(&phydev->mdio.dev, "error read mmd device(%u) reg (%u)\n", device, reg);
#endif

		goto err_handle;
	}

err_handle:
	phy_unlock_mdio_bus(phydev);

	return val;
}

__attribute__((unused)) static int ytphy_write_mmd(struct phy_device* phydev, u16 device, u16 reg, u16 value)
{
	int ret = 0;

	phy_lock_mdio_bus(phydev);

	__phy_write(phydev, REG_MII_MMD_CTRL, device);
	__phy_write(phydev, REG_MII_MMD_DATA, reg);
	__phy_write(phydev, REG_MII_MMD_CTRL, device | 0x4000);
	__phy_write(phydev, REG_MII_MMD_DATA, value);

	phy_unlock_mdio_bus(phydev);

	return ret;
}


static int ytphy_soft_reset(struct phy_device *phydev)
{
	int ret = 0, val = 0;

	val = __phy_read(phydev, MII_BMCR);
	if (val < 0)
		return val;

	ret = __phy_write(phydev, MII_BMCR, val | BMCR_RESET);
	if (ret < 0)
		return ret;

	return ret;
}


#if (YTPHY8531A_XTAL_INIT)
static int yt8531a_xtal_init(struct phy_device *phydev)
{
	int ret = 0;
	int val = 0;
	bool state = false;

	msleep(50);

	do {
		ret = ytphy_write_ext(phydev, 0xa012, 0x88);
		if (ret < 0)
			return ret;

		msleep(100);

		val = ytphy_read_ext(phydev, 0xa012);
		if (val < 0)
			return val;

		usleep_range(10000, 20000);
	} while (val != 0x88);

	ret = ytphy_write_ext(phydev, 0xa012, 0xc8);
	if (ret < 0)
		return ret;

	return ret;
}
#endif

static int yt8010_soft_reset(struct phy_device *phydev)
{
	ytphy_soft_reset(phydev);

	return 0;
}

static int yt8010AS_soft_reset(struct phy_device *phydev)
{
	int ret = 0;

	/* sgmii */
	ytphy_write_ext(phydev, 0xe, 1);
	ret = ytphy_soft_reset(phydev);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xe, 0);
		return ret;
	}

	/* utp */
	ytphy_write_ext(phydev, 0xe, 0);
	ret = ytphy_soft_reset(phydev);
	if (ret < 0)
		return ret;

	return 0;
}

#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
static int yt8010_aneg_done(struct phy_device *phydev)
{
	int val = 0;

	val = phy_read(phydev, 0x1);
	val = phy_read(phydev, 0x1);

	return (val < 0) ? val : (val & BMSR_LSTATUS);
}
#endif

static int yt8010_config_init(struct phy_device *phydev)
{
	int val;

	phydev->autoneg = AUTONEG_DISABLE;

	ytphy_write_ext(phydev, 0x1023, 0xfc00);
	ytphy_write_ext(phydev, 0x101d, 0x12c0);
	val = ytphy_read_ext(phydev, 0x1000);
	val &= ~(1 << 7);
	ytphy_write_ext(phydev, 0x1000, val);
	ytphy_write_ext(phydev, 0x101d, 0x12c0);
	ytphy_write_ext(phydev, 0x101e, 0x1900);
	ytphy_write_ext(phydev, 0x101f, 0x1900);
	ytphy_write_ext(phydev, 0x4083, 0x4327);
	ytphy_write_ext(phydev, 0x4082, 0xc20);
	ytphy_soft_reset(phydev);

	return 0;
}

static int yt8010_config_aneg(struct phy_device *phydev)
{
	phydev->speed = SPEED_100;
	return 0;
}

static int yt8010_read_status(struct phy_device *phydev)
{
	int ret = 0;

	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	/* for 8010, no definition mii reg 0x04, 0x11, here force 100/full */
	phydev->speed = SPEED_100;
	phydev->duplex = DUPLEX_FULL;

	return 0;
}

static int yt8010AS_config_init(struct phy_device *phydev)
{
	phydev->autoneg = AUTONEG_DISABLE;

	ytphy_write_ext(phydev, 0x1009, 0x0);

	yt8010AS_soft_reset(phydev);

	return 0;
}

static int yt8011_probe(struct phy_device *phydev)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	struct device *dev = &phydev->dev;
#else
	struct device *dev = &phydev->mdio.dev;
#endif
	struct yt8xxx_priv *priv;
	int chip_config;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;

	/* ext reg 0x9030 bit0
	 * 0 = chip works in RGMII mode; 1 = chip works in SGMII mode
	 */
	chip_config = ytphy_read_ext(phydev, 0x9030);
	priv->chip_mode = chip_config & 0x1;

	return 0;
}

static int yt8011_soft_reset(struct phy_device *phydev)
{
	struct yt8xxx_priv *priv = phydev->priv;

	/* utp */
	ytphy_write_ext(phydev, 0x9000, 0x0);
	ytphy_soft_reset(phydev);

	if (priv->chip_mode)    /* sgmii */
	{
		ytphy_write_ext(phydev, 0x9000, 0x8000);
		ytphy_soft_reset(phydev);

		/* restore utp space */
		ytphy_write_ext(phydev, 0x9000, 0x0);
	}

	return 0;
}

static int yt8011_config_aneg(struct phy_device *phydev)
{
	phydev->speed = SPEED_1000;

	return 0;
}

#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
static int yt8011_aneg_done(struct phy_device *phydev)
{
	int link_utp = 0;

	/* UTP */
	ytphy_write_ext(phydev, 0x9000, 0);
	link_utp = !!(phy_read(phydev, REG_PHY_SPEC_STATUS) & (BIT(YTXXXX_LINK_STATUS_BIT)));

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s, phy addr: %d, link_utp: %d\n", __func__, phydev->addr, link_utp);
#else
	netdev_info(phydev->attached_dev, "%s, phy addr: %d, link_utp: %d\n", __func__, phydev->mdio.addr, link_utp);
#endif

	return !!(link_utp);
}
#endif

/* #define YT8011_RGMII_DVDDIO_3V3 */
/* #define YT8011_RGMII_DVDDIO_2V5 */
/* #define YT8011_RGMII_DVDDIO_1V8 */

static int yt8011_config_init(struct phy_device *phydev)
{
	struct yt8xxx_priv *priv = phydev->priv;

	phydev->autoneg = AUTONEG_DISABLE;

	/* UTP */
	ytphy_write_ext(phydev, 0x9000, 0x0);

	ytphy_write_ext(phydev, 0x1008, 0x2119);
	ytphy_write_ext(phydev, 0x1092, 0x712);
	ytphy_write_ext(phydev, 0x90bc, 0x6661);
	ytphy_write_ext(phydev, 0x90b9, 0x620b);
	ytphy_write_ext(phydev, 0x2001, 0x6418);
	ytphy_write_ext(phydev, 0x1019, 0x3712);
	ytphy_write_ext(phydev, 0x101a, 0x3713);
	ytphy_write_ext(phydev, 0x2015, 0x1012);
	ytphy_write_ext(phydev, 0x2005, 0x810);
	ytphy_write_ext(phydev, 0x2013, 0xff06);
	ytphy_write_ext(phydev, 0x1053, 0xf);
	ytphy_write_ext(phydev, 0x105e, 0xa46c);
	ytphy_write_ext(phydev, 0x1088, 0x002b);
	ytphy_write_ext(phydev, 0x1088, 0x002b);
	ytphy_write_ext(phydev, 0x1088, 0xb);
	ytphy_write_ext(phydev, 0x3008, 0x143);
	ytphy_write_ext(phydev, 0x3009, 0x1918);
	ytphy_write_ext(phydev, 0x9095, 0x1a1a);
	ytphy_write_ext(phydev, 0x9096, 0x1a10);
	ytphy_write_ext(phydev, 0x9097, 0x101a);
	ytphy_write_ext(phydev, 0x9098, 0x01ff);
	if (!(priv->chip_mode))    /* rgmii config */
	{
#if defined (YT8011_RGMII_DVDDIO_3V3)
		ytphy_write_ext(phydev, 0x9000, 0x8000);
		ytphy_write_ext(phydev, 0x0062, 0x0000);
		ytphy_write_ext(phydev, 0x9000, 0x0000);
		ytphy_write_ext(phydev, 0x9031, 0xb200);
		ytphy_write_ext(phydev, 0x903b, 0x0040);
		ytphy_write_ext(phydev, 0x903e, 0x3b3b);
		ytphy_write_ext(phydev, 0x903c, 0xf);
		ytphy_write_ext(phydev, 0x903d, 0x1000);
		ytphy_write_ext(phydev, 0x9038, 0x0000);
#elif defined (YT8011_RGMII_DVDDIO_2V5)
		ytphy_write_ext(phydev, 0x9000, 0x8000);
		ytphy_write_ext(phydev, 0x0062, 0x0000);
		ytphy_write_ext(phydev, 0x9000, 0x0000);
		ytphy_write_ext(phydev, 0x9031, 0xb200);
		ytphy_write_ext(phydev, 0x9111, 0x5);
		ytphy_write_ext(phydev, 0x9114, 0x3939);
		ytphy_write_ext(phydev, 0x9112, 0xf);
		ytphy_write_ext(phydev, 0x9110, 0x0);
		ytphy_write_ext(phydev, 0x9113, 0x10);
		ytphy_write_ext(phydev, 0x903d, 0x2);
#elif defined (YT8011_RGMII_DVDDIO_1V8)
		ytphy_write_ext(phydev, 0x9000, 0x8000);
		ytphy_write_ext(phydev, 0x0062, 0x0000);
		ytphy_write_ext(phydev, 0x9000, 0x0000);
		ytphy_write_ext(phydev, 0x9031, 0xb200);
		ytphy_write_ext(phydev, 0x9116, 0x6);
		ytphy_write_ext(phydev, 0x9119, 0x3939);
		ytphy_write_ext(phydev, 0x9117, 0xf);
		ytphy_write_ext(phydev, 0x9115, 0x0);
		ytphy_write_ext(phydev, 0x9118, 0x20);
		ytphy_write_ext(phydev, 0x903d, 0x3);
#endif
	}

	ytphy_soft_reset(phydev);

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d\n", __func__, phydev->addr);
#else
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d\n", __func__, phydev->mdio.addr);
#endif

	return 0;
}

static int ytxxxx_automotive_adjust_status(struct phy_device *phydev, int val)
{
	int speed_mode;
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	int speed = -1;
#else
	int speed = SPEED_UNKNOWN;
#endif

	speed_mode = (val & YTXXXX_SPEED_MODE) >> YTXXXX_SPEED_MODE_BIT;
	switch (speed_mode) {
		case 1:
			speed = SPEED_100;
			break;
		case 2:
			speed = SPEED_1000;
			break;
		default:
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
			speed = -1;
#else
			speed = SPEED_UNKNOWN;
#endif
			break;
	}

	phydev->speed = speed;
	phydev->duplex = DUPLEX_FULL;

	return 0;
}

static int yt8011_read_status(struct phy_device *phydev)
{
	int ret;
	int val;
	int link;
	int link_utp = 0;

	/* UTP */
	ret = ytphy_write_ext(phydev, 0x9000, 0x0);
	if (ret < 0)
		return ret;

	val = phy_read(phydev, REG_PHY_SPEC_STATUS);
	if (val < 0)
		return val;

	link = val & (BIT(YTXXXX_LINK_STATUS_BIT));
	if (link) {
		link_utp = 1;
		ytxxxx_automotive_adjust_status(phydev, val);
	} else {
		link_utp = 0;
	}

	if (link_utp) {
		if (phydev->link == 0)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media: UTP, mii reg 0x11 = 0x%x\n",
					__func__, phydev->addr, (unsigned int)val);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media: UTP, mii reg 0x11 = 0x%x\n",
				__func__, phydev->mdio.addr, (unsigned int)val);
#endif
		phydev->link = 1;
	} else {
		if (phydev->link == 1)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->addr);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->mdio.addr);
#endif

		phydev->link = 0;
	}

	if (link_utp)
		ytphy_write_ext(phydev, 0x9000, 0x0);

	return 0;
}

static int yt8512_led_init(struct phy_device *phydev)
{
	int ret;
	int val;
	int mask;

	val = ytphy_read_ext(phydev, YT8512_EXTREG_LED0);
	if (val < 0)
		return val;

	val |= YT8512_LED0_ACT_BLK_IND;

	mask = YT8512_LED0_DIS_LED_AN_TRY | YT8512_LED0_BT_BLK_EN |
		YT8512_LED0_HT_BLK_EN | YT8512_LED0_COL_BLK_EN |
		YT8512_LED0_BT_ON_EN;
	val &= ~mask;

	ret = ytphy_write_ext(phydev, YT8512_EXTREG_LED0, val);
	if (ret < 0)
		return ret;

	val = ytphy_read_ext(phydev, YT8512_EXTREG_LED1);
	if (val < 0)
		return val;

	val |= YT8512_LED1_BT_ON_EN;

	mask = YT8512_LED1_TXACT_BLK_EN | YT8512_LED1_RXACT_BLK_EN;
	val &= ~mask;

	ret = ytphy_write_ext(phydev, YT8512_EXTREG_LED1, val);

	return ret;
}

static int yt8512_probe(struct phy_device *phydev)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	struct device *dev = &phydev->dev;
#else
	struct device *dev = &phydev->mdio.dev;
#endif
	struct yt8xxx_priv *priv;
	int chip_config;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;

	chip_config = ytphy_read_ext(phydev, YT8512_EXTENDED_COMBO_CONTROL1);

	priv->chip_mode = (chip_config & (BIT(1) | BIT(0)));

	return 0;
}

static int yt8512_config_init(struct phy_device *phydev)
{
	int ret;
	int val;
	struct yt8xxx_priv *priv = phydev->priv;

	val = ytphy_read_ext(phydev, YT8512_10BT_DEBUG_LPBKS);
	if (val < 0)
		return val;

	val &= ~BIT(10);
	ret = ytphy_write_ext(phydev, YT8512_10BT_DEBUG_LPBKS, val);;
	if (ret < 0)
		return ret;

	if (!(priv->chip_mode))    /* MII mode */
	{
		val &= ~BIT(15);
		ret = ytphy_write_ext(phydev, YT8512_10BT_DEBUG_LPBKS, val);;
		if (ret < 0)
			return ret;
	}

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
	ret = ytphy_config_init(phydev);
#else
	ret = genphy_config_init(phydev);
#endif
	if (ret < 0)
		return ret;

	ret = yt8512_led_init(phydev);

	/* disable auto sleep */
	val = ytphy_read_ext(phydev, YT8512_EXTREG_SLEEP_CONTROL1);
	if (val < 0)
		return val;

	val &= (~BIT(YT8512_EN_SLEEP_SW_BIT));

	ret = ytphy_write_ext(phydev, YT8512_EXTREG_SLEEP_CONTROL1, val);
	if (ret < 0)
		return ret;

	ytphy_soft_reset(phydev);

	return ret;
}

static int yt8512_read_status(struct phy_device *phydev)
{
	int ret;
	int val;
	int speed, speed_mode, duplex;

	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	genphy_read_status(phydev);

	val = phy_read(phydev, REG_PHY_SPEC_STATUS);
	if (val < 0)
		return val;

	duplex = (val & YTXXXX_DUPLEX) >> YTXXXX_DUPLEX_BIT;
	speed_mode = (val & YTXXXX_SPEED_MODE) >> YTXXXX_SPEED_MODE_BIT;
	switch (speed_mode) {
		case 0:
			speed = SPEED_10;
			break;
		case 1:
			speed = SPEED_100;
			break;
		case 2:
		case 3:
		default:
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
			speed = -1;
#else
			speed = SPEED_UNKNOWN;
#endif
			break;
	}

	phydev->speed = speed;
	phydev->duplex = duplex;

	return 0;
}

static int yt8522_read_status(struct phy_device *phydev)
{
	int val;
	int speed, speed_mode, duplex;

	genphy_read_status(phydev);

	val = phy_read(phydev, REG_PHY_SPEC_STATUS);
	if (val < 0)
		return val;

	if ((val & BIT(10)) >> YTXXXX_LINK_STATUS_BIT) {    /* link up */
		duplex = (val & BIT(13)) >> YTXXXX_DUPLEX_BIT;
		speed_mode = (val & (BIT(15) | BIT(14))) >> YTXXXX_SPEED_MODE_BIT;
		switch (speed_mode) {
			case 0:
				speed = SPEED_10;
				break;
			case 1:
				speed = SPEED_100;
				break;
			case 2:
			case 3:
			default:
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
				speed = -1;
#else
				speed = SPEED_UNKNOWN;
#endif
				break;
		}

		phydev->link = 1;
		phydev->speed = speed;
		phydev->duplex = duplex;

		return 0;
	}

	phydev->link = 0;

	return 0;
}

static int yt8522_probe(struct phy_device *phydev)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	struct device *dev = &phydev->dev;
#else
	struct device *dev = &phydev->mdio.dev;
#endif
	struct yt8xxx_priv *priv;
	int chip_config;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;

	chip_config = ytphy_read_ext(phydev, YT8522_EXTENDED_COMBO_CTRL_1);

	priv->chip_mode = (chip_config & (BIT(1) | BIT(0)));

	return 0;
}

static int yt8522_config_init(struct phy_device *phydev)
{
	int ret;
	int val;
	struct yt8xxx_priv *priv = phydev->priv;

	val = ytphy_read_ext(phydev, YT8522_EXTENDED_COMBO_CTRL_1);
	if (val < 0)
		return val;

	if (0x2 == (priv->chip_mode))       /* RMII2 mode */
	{
		val |= BIT(4);
		ret = ytphy_write_ext(phydev, YT8522_EXTENDED_COMBO_CTRL_1, val);;
		if (ret < 0)
			return ret;

		ret = ytphy_write_ext(phydev, YT8522_TX_DELAY_CONTROL, 0x9f);
		if (ret < 0)
			return ret;

		ret = ytphy_write_ext(phydev, YT8522_EXTENDED_PAD_CONTROL, 0x81d4);
		if (ret < 0)
			return ret;
	}
	else if (0x3 == (priv->chip_mode))  /* RMII1 mode */
	{
		val |= BIT(4);
		ret = ytphy_write_ext(phydev, YT8522_EXTENDED_COMBO_CTRL_1, val);;
		if (ret < 0)
			return ret;
	}

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
	ret = ytphy_config_init(phydev);
#else
	ret = genphy_config_init(phydev);
#endif
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, YT8522_TX_CLK_DELAY, 0);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, YT8522_ANAGLOG_IF_CTRL, 0xbf2a);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, YT8522_DAC_CTRL, 0x297f);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, YT8522_INTERPOLATOR_FILTER_1, 0x1FE);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, YT8522_INTERPOLATOR_FILTER_2, 0x1FE);
	if (ret < 0)
		return ret;

	/* disable auto sleep */
	val = ytphy_read_ext(phydev, YT8512_EXTREG_SLEEP_CONTROL1);
	if (val < 0)
		return val;

	val &= (~BIT(YT8512_EN_SLEEP_SW_BIT));

	ret = ytphy_write_ext(phydev, YT8512_EXTREG_SLEEP_CONTROL1, val);
	if (ret < 0)
		return ret;

	ytphy_soft_reset(phydev);

	return 0;
}

static int yt8521_probe(struct phy_device *phydev)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	struct device *dev = &phydev->dev;
#else
	struct device *dev = &phydev->mdio.dev;
#endif
	struct yt8xxx_priv *priv;
	int chip_config;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;

	chip_config = ytphy_read_ext(phydev, 0xa001);

	priv->chip_mode = chip_config & 0x7;
	switch (priv->chip_mode) {
		case 1:    //fiber<>rgmii
		case 4:
		case 5:
			priv->polling_mode = YT_PHY_MODE_FIBER;
			break;
		case 2:    //utp/fiber<>rgmii
		case 6:
		case 7:
			priv->polling_mode = YT_PHY_MODE_POLL;
			break;
		case 3:    //utp<>sgmii
		case 0:    //utp<>rgmii
		default:
			priv->polling_mode = YT_PHY_MODE_UTP;
			break;
	}

	return 0;
}

#if GMAC_CLOCK_INPUT_NEEDED
static int ytphy_mii_rd_ext(struct mii_bus *bus, int phy_id, u32 regnum)
{
	int ret;
	int val;

	ret = bus->write(bus, phy_id, REG_DEBUG_ADDR_OFFSET, regnum);
	if (ret < 0)
		return ret;

	val = bus->read(bus, phy_id, REG_DEBUG_DATA);

	return val;
}

static int ytphy_mii_wr_ext(struct mii_bus *bus
		int phy_id,
		u32 regnum,
		u16 val)
{
	int ret;

	ret = bus->write(bus, phy_id, REG_DEBUG_ADDR_OFFSET, regnum);
	if (ret < 0)
		return ret;

	ret = bus->write(bus, phy_id, REG_DEBUG_DATA, val);

	return ret;
}

static int yt8511_config_dis_txdelay(struct mii_bus *bus, int phy_id)
{
	int ret;
	int val;

	/* disable auto sleep */
	val = ytphy_mii_rd_ext(bus, phy_id, 0x27);
	if (val < 0)
		return val;

	val &= (~BIT(15));

	ret = ytphy_mii_wr_ext(bus, phy_id, 0x27, val);
	if (ret < 0)
		return ret;

	/* enable RXC clock when no wire plug */
	val = ytphy_mii_rd_ext(bus, phy_id, 0xc);
	if (val < 0)
		return val;

	/* ext reg 0xc b[7:4]
	 * Tx Delay time = 150ps * N - 250ps
	 */
	val &= ~(0xf << 4);
	ret = ytphy_mii_wr_ext(bus, phy_id, 0xc, val);

	return ret;
}

static int yt8511_config_out_125m(struct mii_bus *bus, int phy_id)
{
	int ret;
	int val;

	/* disable auto sleep */
	val = ytphy_mii_rd_ext(bus, phy_id, 0x27);
	if (val < 0)
		return val;

	val &= (~BIT(15));

	ret = ytphy_mii_wr_ext(bus, phy_id, 0x27, val);
	if (ret < 0)
		return ret;

	/* enable RXC clock when no wire plug */
	val = ytphy_mii_rd_ext(bus, phy_id, 0xc);
	if (val < 0)
		return val;

	/* ext reg 0xc.b[2:1]
	 * 00-----25M from pll;
	 * 01---- 25M from xtl;(default)
	 * 10-----62.5M from pll;
	 * 11----125M from pll(here set to this value)
	 */
	val |= (3 << 1);
	ret = ytphy_mii_wr_ext(bus, phy_id, 0xc, val);

#ifdef YT_8511_INIT_TO_MASTER
	/* for customer, please enable it based on demand.
	 * configure to master
	 */

	/* master/slave config reg*/
	val = bus->read(bus, phy_id, 0x9);
	/* to be manual config and force to be master */
	val |= (0x3<<11);
	/* take effect until phy soft reset */
	ret = bus->write(bus, phy_id, 0x9, val);
	if (ret < 0)
		return ret;
#endif

	return ret;
}

static int yt8511_config_init(struct phy_device *phydev)
{
	int ret;

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
	ret = ytphy_config_init(phydev);
#else
	ret = genphy_config_init(phydev);
#endif

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d\n", __func__, phydev->addr);
#else
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d\n", __func__, phydev->mdio.addr);
#endif

	ytphy_soft_reset(phydev);

	return ret;
}
#endif /* GMAC_CLOCK_INPUT_NEEDED */

#if (YTPHY_WOL_FEATURE_ENABLE)
static int ytphy_switch_reg_space(struct phy_device *phydev, int space)
{
	int ret;

	if (space == YTPHY_REG_SPACE_UTP)
		ret = ytphy_write_ext(phydev, 0xa000, 0);
	else
		ret = ytphy_write_ext(phydev, 0xa000, 2);

	return ret;
}

static int ytphy_wol_feature_enable_cfg(struct phy_device *phydev,
		struct ytphy_wol_feature_cfg wol_cfg)
{
	int ret = 0;
	int val = 0;

	val = ytphy_read_ext(phydev, YTPHY_WOL_FEATURE_REG_CFG);
	if (val < 0)
		return val;

	if (wol_cfg.enable) {
		val |= YTPHY_WOL_FEATURE_ENABLE_CFG;

		if (wol_cfg.type == YTPHY_WOL_FEATURE_LEVEL_TRIGGER) {
			val &= ~YTPHY_WOL_FEATURE_TYPE_CFG;
			val &= ~YTPHY_WOL_FEATURE_INTR_SEL_CFG;
		} else if (wol_cfg.type == YTPHY_WOL_FEATURE_PULSE_TRIGGER) {
			val |= YTPHY_WOL_FEATURE_TYPE_CFG;
			val |= YTPHY_WOL_FEATURE_INTR_SEL_CFG;

			if (wol_cfg.width == YTPHY_WOL_FEATURE_84MS_PULSE_WIDTH) {
				val &= ~YTPHY_WOL_FEATURE_WIDTH1_CFG;
				val &= ~YTPHY_WOL_FEATURE_WIDTH2_CFG;
			} else if (wol_cfg.width == YTPHY_WOL_FEATURE_168MS_PULSE_WIDTH) {
				val |= YTPHY_WOL_FEATURE_WIDTH1_CFG;
				val &= ~YTPHY_WOL_FEATURE_WIDTH2_CFG;
			} else if (wol_cfg.width == YTPHY_WOL_FEATURE_336MS_PULSE_WIDTH) {
				val &= ~YTPHY_WOL_FEATURE_WIDTH1_CFG;
				val |= YTPHY_WOL_FEATURE_WIDTH2_CFG;
			} else if (wol_cfg.width == YTPHY_WOL_FEATURE_672MS_PULSE_WIDTH) {
				val |= YTPHY_WOL_FEATURE_WIDTH1_CFG;
				val |= YTPHY_WOL_FEATURE_WIDTH2_CFG;
			}
		}
	} else {
		val &= ~YTPHY_WOL_FEATURE_ENABLE_CFG;
		val &= ~YTPHY_WOL_FEATURE_INTR_SEL_CFG;
	}

	ret = ytphy_write_ext(phydev, YTPHY_WOL_FEATURE_REG_CFG, val);
	if (ret < 0)
		return ret;

	return 0;
}

static void ytphy_wol_feature_get(struct phy_device *phydev,
		struct ethtool_wolinfo *wol)
{
	int val = 0;

	wol->supported = WAKE_MAGIC;
	wol->wolopts = 0;

	phy_lock_mdio_bus(phydev);
	val = ytphy_read_ext(phydev, YTPHY_WOL_FEATURE_REG_CFG);
	phy_unlock_mdio_bus(phydev);
	if (val < 0)
		return;

	if (val & YTPHY_WOL_FEATURE_ENABLE_CFG)
		wol->wolopts |= WAKE_MAGIC;

	//return;
}

static int ytphy_wol_feature_set(struct phy_device *phydev,
		struct ethtool_wolinfo *wol)
{
	int ret, curr_reg_space, val;
	struct ytphy_wol_feature_cfg wol_cfg;
	struct net_device *p_attached_dev = phydev->attached_dev;

	memset(&wol_cfg, 0, sizeof(struct ytphy_wol_feature_cfg));
	phy_lock_mdio_bus(phydev);
	curr_reg_space = ytphy_read_ext(phydev, 0xa000);
	if (curr_reg_space < 0) {
		ret = curr_reg_space;
		goto err_handle;
	}

	/* Switch to phy UTP page */
	ret = ytphy_switch_reg_space(phydev, YTPHY_REG_SPACE_UTP);
	if (ret < 0)
		goto err_handle;

	if (wol->wolopts & WAKE_MAGIC) {
		/* Enable the WOL feature interrupt */
		val = __phy_read(phydev, YTPHY_UTP_INTR_REG);
		val |= YTPHY_WOL_FEATURE_INTR;
		ret = __phy_write(phydev, YTPHY_UTP_INTR_REG, val);
		if (ret < 0)
			goto err_handle;

		/* Set the WOL feature config */
		wol_cfg.enable = true;
		wol_cfg.type = YTPHY_WOL_FEATURE_PULSE_TRIGGER;
		wol_cfg.width = YTPHY_WOL_FEATURE_672MS_PULSE_WIDTH;
		ret = ytphy_wol_feature_enable_cfg(phydev, wol_cfg);
		if (ret < 0)
			goto err_handle;

		/* Store the device address for the magic packet */
		ret = ytphy_write_ext(phydev, YTPHY_WOL_FEATURE_MACADDR2_4_MAGIC_PACKET,
				((p_attached_dev->dev_addr[0] << 8) |
				 p_attached_dev->dev_addr[1]));
		if (ret < 0)
			goto err_handle;
		ret = ytphy_write_ext(phydev, YTPHY_WOL_FEATURE_MACADDR1_4_MAGIC_PACKET,
				((p_attached_dev->dev_addr[2] << 8) |
				 p_attached_dev->dev_addr[3]));
		if (ret < 0)
			goto err_handle;
		ret = ytphy_write_ext(phydev, YTPHY_WOL_FEATURE_MACADDR0_4_MAGIC_PACKET,
				((p_attached_dev->dev_addr[4] << 8) |
				 p_attached_dev->dev_addr[5]));
		if (ret < 0)
			goto err_handle;
	} else {
		wol_cfg.enable = false;
		wol_cfg.type = YTPHY_WOL_FEATURE_TRIGGER_TYPE_MAX;
		wol_cfg.width = YTPHY_WOL_FEATURE_PULSE_WIDTH_MAX;
		ret = ytphy_wol_feature_enable_cfg(phydev, wol_cfg);
		if (ret < 0)
			goto err_handle;
	}

	/* Recover to previous register space page */
	ret = ytphy_switch_reg_space(phydev, curr_reg_space);

err_handle:
	phy_unlock_mdio_bus(phydev);
	return ret;
}
#endif /*(YTPHY_WOL_FEATURE_ENABLE)*/



static int yt8521_config_init(struct phy_device *phydev)
{
	int ret;
	int val;

	struct yt8xxx_priv *priv = phydev->priv;

#if (YTPHY_WOL_FEATURE_ENABLE)
	struct ethtool_wolinfo wol;

	/* set phy wol enable */
	memset(&wol, 0x0, sizeof(struct ethtool_wolinfo));
	wol.wolopts |= WAKE_MAGIC;
	ytphy_wol_feature_set(phydev, &wol);
#endif

	phydev->irq = PHY_POLL;

	ytphy_write_ext(phydev, 0xa000, 0);
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
	ret = ytphy_config_init(phydev);
#else
	ret = genphy_config_init(phydev);
#endif
	if (ret < 0)
		return ret;

	/* disable auto sleep */
	val = ytphy_read_ext(phydev, YT8521_EXTREG_SLEEP_CONTROL1);
	if (val < 0)
		return val;

	val &= (~BIT(YT8521_EN_SLEEP_SW_BIT));

	ret = ytphy_write_ext(phydev, YT8521_EXTREG_SLEEP_CONTROL1, val);
	if (ret < 0)
		return ret;

	/* enable RXC clock when no wire plug */
	val = ytphy_read_ext(phydev, 0xc);
	if (val < 0)
		return val;
	val &= ~(1 << 12);
	ret = ytphy_write_ext(phydev, 0xc, val);
	if (ret < 0)
		return ret;

	ytxxxx_soft_reset(phydev);

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d, chip mode = %d, polling mode = %d\n",
			__func__, phydev->addr, priv->chip_mode, priv->polling_mode);
#else
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d, chip mode = %d, polling mode = %d\n",
			__func__, phydev->mdio.addr, priv->chip_mode, priv->polling_mode);
#endif
	return ret;
}

/* for fiber mode, there is no 10M speed mode and
 * this function is for this purpose.
 */
static int ytxxxx_adjust_status(struct phy_device *phydev, int val, int is_utp)
{
	int speed_mode, duplex;
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	int speed = -1;
#else
	int speed = SPEED_UNKNOWN;
#endif

	if (is_utp)
		duplex = (val & YTXXXX_DUPLEX) >> YTXXXX_DUPLEX_BIT;
	else
		duplex = 1;
	speed_mode = (val & YTXXXX_SPEED_MODE) >> YTXXXX_SPEED_MODE_BIT;
	switch (speed_mode) {
		case 0:
			if (is_utp)
				speed = SPEED_10;
			break;
		case 1:
			speed = SPEED_100;
			break;
		case 2:
			speed = SPEED_1000;
			break;
		case 3:
			break;
		default:
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
			speed = -1;
#else
			speed = SPEED_UNKNOWN;
#endif
			break;
	}

	phydev->speed = speed;
	phydev->duplex = duplex;

	return 0;
}

/* for fiber mode, when speed is 100M, there is no definition for
 * autonegotiation, and this function handles this case and return
 * 1 per linux kernel's polling.
 */
static int yt8521_aneg_done(struct phy_device *phydev)
{
	int link_fiber = 0, link_utp = 0;

	/* reading Fiber */
	ytphy_write_ext(phydev, 0xa000, 2);
	link_fiber = !!(phy_read(phydev, REG_PHY_SPEC_STATUS) & (BIT(YTXXXX_LINK_STATUS_BIT)));

	/* reading UTP */
	ytphy_write_ext(phydev, 0xa000, 0);
	if (!link_fiber)
		link_utp = !!(phy_read(phydev, REG_PHY_SPEC_STATUS) & (BIT(YTXXXX_LINK_STATUS_BIT)));

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s, phy addr: %d, link_fiber: %d, link_utp: %d\n",
			__func__, phydev->addr, link_fiber, link_utp);
#else
	netdev_info(phydev->attached_dev, "%s, phy addr: %d, link_fiber: %d, link_utp: %d\n",
			__func__, phydev->mdio.addr, link_fiber, link_utp);
#endif
	return !!(link_fiber | link_utp);
}

static int yt8521_read_status(struct phy_device *phydev)
{
	int ret;
	int val_utp, val_fiber;
	int yt8521_fiber_latch_val;
	int yt8521_fiber_curr_val;
	int link;
	int link_utp = 0, link_fiber = 0;
	struct yt8xxx_priv *priv = phydev->priv;

	if(priv->polling_mode != YT_PHY_MODE_FIBER) {
		/* reading UTP */
		ret = ytphy_write_ext(phydev, 0xa000, 0);
		if (ret < 0)
			return ret;

		val_utp = phy_read(phydev, REG_PHY_SPEC_STATUS);
		if (val_utp < 0)
			return val_utp;

		link = val_utp & (BIT(YTXXXX_LINK_STATUS_BIT));
		if (link) {
			link_utp = 1;
			ytxxxx_adjust_status(phydev, val_utp, 1);
		} else {
			link_utp = 0;
		}
	}

	if (priv->polling_mode != YT_PHY_MODE_UTP) {
		/* reading Fiber */
		ret = ytphy_write_ext(phydev, 0xa000, 2);
		if (ret < 0) {
			ytphy_write_ext(phydev, 0xa000, 0);
			return ret;
		}

		val_fiber = phy_read(phydev, REG_PHY_SPEC_STATUS);
		if (val_fiber < 0) {
			ytphy_write_ext(phydev, 0xa000, 0);
			return val_fiber;
		}

		//note: below debug information is used to check multiple PHy ports.

		/* for fiber, from 1000m to 100m, there is not link down from 0x11,
		 * and check reg 1 to identify such case this is important for Linux
		 * kernel for that, missing linkdown event will cause problem.
		 */
		yt8521_fiber_latch_val = phy_read(phydev, MII_BMSR);
		yt8521_fiber_curr_val = phy_read(phydev, MII_BMSR);
		link = val_fiber & (BIT(YTXXXX_LINK_STATUS_BIT));
		if (link && yt8521_fiber_latch_val != yt8521_fiber_curr_val) {
			link = 0;
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, fiber link down detect, latch = %04x, curr = %04x\n",
					__func__, phydev->addr, yt8521_fiber_latch_val, yt8521_fiber_curr_val);
#else
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, fiber link down detect, latch = %04x, curr = %04x\n",
					__func__, phydev->mdio.addr, yt8521_fiber_latch_val, yt8521_fiber_curr_val);
#endif
		}

		if (link) {
			link_fiber = 1;
			ytxxxx_adjust_status(phydev, val_fiber, 0);
		} else {
			link_fiber = 0;
		}
	}

	if (link_utp || link_fiber) {
		if (phydev->link == 0)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media: %s, mii reg 0x11 = 0x%x\n",
					__func__, phydev->addr, (link_utp && link_fiber) ? "UNKNOWN MEDIA" : (link_utp ? "UTP" : "Fiber"),
					link_utp ? (unsigned int)val_utp : (unsigned int)val_fiber);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media: %s, mii reg 0x11 = 0x%x\n",
				__func__, phydev->mdio.addr, (link_utp && link_fiber) ? "UNKNOWN MEDIA" : (link_utp ? "UTP" : "Fiber"),
				link_utp ? (unsigned int)val_utp : (unsigned int)val_fiber);
#endif
		phydev->link = 1;
	} else {
		if (phydev->link == 1)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->addr);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->mdio.addr);
#endif
		phydev->link = 0;
	}

	if (priv->polling_mode != YT_PHY_MODE_FIBER) {    //utp or combo
		if (link_fiber)
			ytphy_write_ext(phydev, 0xa000, 2);
		else
			ytphy_write_ext(phydev, 0xa000, 0);
	}
	return 0;
}

static int yt8521_suspend(struct phy_device *phydev)
{
#if !(SYS_WAKEUP_BASED_ON_ETH_PKT)
	int value;

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_lock(&phydev->lock);
#else
	/* no need lock in 4.19 */
#endif
	struct yt8xxx_priv *priv = phydev->priv;

	if (priv->polling_mode != YT_PHY_MODE_FIBER) {
		ytphy_write_ext(phydev, 0xa000, 0);
		value = phy_read(phydev, MII_BMCR);
		phy_write(phydev, MII_BMCR, value | BMCR_PDOWN);
	}

	if (priv->polling_mode != YT_PHY_MODE_UTP) {
		ytphy_write_ext(phydev, 0xa000, 2);
		value = phy_read(phydev, MII_BMCR);
		phy_write(phydev, MII_BMCR, value | BMCR_PDOWN);
	}

	ytphy_write_ext(phydev, 0xa000, 0);

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_unlock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif
#endif /*!(SYS_WAKEUP_BASED_ON_ETH_PKT)*/

	return 0;
}

static int yt8521_resume(struct phy_device *phydev)
{
	int value, ret;

	/* disable auto sleep */
	value = ytphy_read_ext(phydev, YT8521_EXTREG_SLEEP_CONTROL1);
	if (value < 0)
		return value;

	value &= (~BIT(YT8521_EN_SLEEP_SW_BIT));

	ret = ytphy_write_ext(phydev, YT8521_EXTREG_SLEEP_CONTROL1, value);
	if (ret < 0)
		return ret;

#if !(SYS_WAKEUP_BASED_ON_ETH_PKT)
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_lock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif
	struct yt8xxx_priv *priv = phydev->priv;

	if (priv->polling_mode != YT_PHY_MODE_FIBER) {
		ytphy_write_ext(phydev, 0xa000, 0);
		value = phy_read(phydev, MII_BMCR);
		phy_write(phydev, MII_BMCR, value & ~BMCR_PDOWN);
	}

	if (priv->polling_mode != YT_PHY_MODE_UTP) {
		ytphy_write_ext(phydev, 0xa000, 2);
		value = phy_read(phydev, MII_BMCR);
		phy_write(phydev, MII_BMCR, value & ~BMCR_PDOWN);

		ytphy_write_ext(phydev, 0xa000, 0);
	}

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_unlock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif
#endif /*!(SYS_WAKEUP_BASED_ON_ETH_PKT)*/

	return 0;
}

static int yt8531S_config_init(struct phy_device *phydev)
{
	int ret = 0, val = 0;

#if (YTPHY8531A_XTAL_INIT)
	ret = yt8531a_xtal_init(phydev);
	if (ret < 0)
		return ret;
#endif
	ret = ytphy_write_ext(phydev, 0xa023, 0x4031);
	if (ret < 0)
		return ret;

	ytphy_write_ext(phydev, 0xa000, 0x0);
	val = ytphy_read_ext(phydev, 0xf);

	if(0x31 != val && 0x32 != val)
	{
		ret = ytphy_write_ext(phydev, 0xa071, 0x9007);
		if (ret < 0)
			return ret;

		ret = ytphy_write_ext(phydev, 0x52, 0x231d);
		if (ret < 0)
			return ret;

		ret = ytphy_write_ext(phydev, 0x51, 0x04a9);
		if (ret < 0)
			return ret;

		ret = ytphy_write_ext(phydev, 0x57, 0x274c);
		if (ret < 0)
			return ret;

		ret = ytphy_write_ext(phydev, 0xa006, 0x10d);
		if (ret < 0)
			return ret;

		if(0x500 == val)
		{
			val = ytphy_read_ext(phydev, 0xa001);
			if((0x30 == (val & 0x30)) || (0x20 == (val & 0x30)))
			{
				ret = ytphy_write_ext(phydev, 0xa010, 0xabff);
				if (ret < 0)
					return ret;
			}
		}
	}

	ret = yt8521_config_init(phydev);
	if (ret < 0)
		return ret;

	ytphy_write_ext(phydev, 0xa000, 0x0);

	return ret;
}

static int yt8531_config_init(struct phy_device *phydev)
{
	int ret = 0, val = 0;

#if (YTPHY8531A_XTAL_INIT)
	ret = yt8531a_xtal_init(phydev);
	if (ret < 0)
		return ret;
#endif

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
	ret = ytphy_config_init(phydev);
#else
	ret = genphy_config_init(phydev);
#endif
	if (ret < 0)
		return ret;

	val = ytphy_read_ext(phydev, 0xf);
	if(0x31 != val && 0x32 != val)
	{
		ret = ytphy_write_ext(phydev, 0x52, 0x231d);
		if (ret < 0)
			return ret;

		ret = ytphy_write_ext(phydev, 0x51, 0x04a9);
		if (ret < 0)
			return ret;

		ret = ytphy_write_ext(phydev, 0x57, 0x274c);
		if (ret < 0)
			return ret;

		if(0x500 == val)
		{
			val = ytphy_read_ext(phydev, 0xa001);
			if((0x30 == (val & 0x30)) || (0x20 == (val & 0x30)))
			{
				ret = ytphy_write_ext(phydev, 0xa010, 0xabff);
				if (ret < 0)
					return ret;
			}
		}
	}

	ytphy_soft_reset(phydev);

	return 0;
}

#if (KERNEL_VERSION(5, 0, 21) < LINUX_VERSION_CODE)
static void ytphy_link_change_notify(struct phy_device *phydev)
{
	int adv;
	adv = phy_read(phydev, MII_ADVERTISE);

	if (adv < 0)
		return;

	linkmode_mod_bit(ETHTOOL_LINK_MODE_10baseT_Half_BIT,
			phydev->advertising, (adv & ADVERTISE_10HALF));
	linkmode_mod_bit(ETHTOOL_LINK_MODE_10baseT_Full_BIT,
			phydev->advertising, (adv & ADVERTISE_10FULL));
	linkmode_mod_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT,
			phydev->advertising, (adv & ADVERTISE_100HALF));
	linkmode_mod_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT,
			phydev->advertising, (adv & ADVERTISE_100FULL));

	adv = phy_read(phydev, MII_CTRL1000);

	if (adv < 0)
		return;

	linkmode_mod_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT,
			phydev->advertising, (adv & ADVERTISE_1000HALF));
	linkmode_mod_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
			phydev->advertising, (adv & ADVERTISE_1000FULL));
}
#endif

#ifdef YTPHY_YT8543_ENABLE
static int yt8543_config_init(struct phy_device *phydev)
{
	int ret = 0, val = 0;

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
	ret = ytphy_config_init(phydev);
#else
	ret = genphy_config_init(phydev);
#endif
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x403c, 0x286);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xdc, 0x855c);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xdd, 0x6040);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x40e, 0xf00);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x40f, 0xf00);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x411, 0x5030);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x1f, 0x110a);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x20, 0xc06);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x40d, 0x1f);
	if (ret < 0)
		return ret;

	val = ytphy_read_ext(phydev, 0xa088);
	if (val < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xa088,
			((val & 0xfff0) | BIT(4)) | (((ytphy_read_ext(phydev, 0xa015) & 0x3c)) >> 2));
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xa183, 0x1918);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xa184, 0x1818);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xa186, 0x2018);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xa189, 0x3894);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xa187, 0x3838);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xa18b, 0x1918);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xa18c, 0x1818);
	if (ret < 0)
		return ret;

	ytphy_soft_reset(phydev);

	return 0;
}

static int yt8543_read_status(struct phy_device *phydev)
{
	int val;
	int link;
	int link_utp = 0;

	genphy_read_status(phydev);

	val = phy_read(phydev, REG_PHY_SPEC_STATUS);
	if (val < 0)
		return val;

	link = val & (BIT(YTXXXX_LINK_STATUS_BIT));
	if (link) {
		link_utp = 1;
		ytxxxx_adjust_status(phydev, val, 1);
	} else {
		link_utp = 0;
	}

	if (link_utp) {
		if (phydev->link == 0)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media: UTP, mii reg 0x11 = 0x%x\n",
					__func__, phydev->addr, (unsigned int)val);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media: UTP, mii reg 0x11 = 0x%x\n",
				__func__, phydev->mdio.addr, (unsigned int)val);
#endif
		phydev->link = 1;
	} else {
		if (phydev->link == 1)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->addr);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->mdio.addr);
#endif

		phydev->link = 0;
	}

	return 0;
}
#endif

static int yt8614_probe(struct phy_device *phydev)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	struct device *dev = &phydev->dev;
#else
	struct device *dev = &phydev->mdio.dev;
#endif
	struct yt8xxx_priv *priv;
	int chip_config;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;

	phy_lock_mdio_bus(phydev);
	chip_config = ytphy_read_ext(phydev, 0xa007);
	phy_unlock_mdio_bus(phydev);

	priv->chip_mode = chip_config & 0xf;
	switch (priv->chip_mode) {
		case 8:        //4'b1000, Fiber x4 + Copper x4
		case 12:    //4'b1100, QSGMII x1 + Combo x4 mode;
		case 13:    //4'b1101, QSGMII x1 + Combo x4 mode;
			priv->polling_mode = (YT_PHY_MODE_FIBER | YT_PHY_MODE_UTP);
			break;
		case 14:    //4'b1110, QSGMII x1 + SGMII(MAC) x4 mode;
		case 11:    //4'b1011, QSGMII x1 + Fiber x4 mode;
			priv->polling_mode = YT_PHY_MODE_FIBER;
			break;
		case 9:        //4'b1001, Reserved.
		case 10:    //4'b1010, QSGMII x1 + Copper x4 mode
		case 15:    //4'b1111, SGMII(PHY) x4 + Copper x4 mode
		default:
			priv->polling_mode = YT_PHY_MODE_UTP;
			break;
	}

	return 0;
}

static int yt8614Q_probe(struct phy_device *phydev)
{
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	struct device *dev = &phydev->dev;
#else
	struct device *dev = &phydev->mdio.dev;
#endif
	struct yt8xxx_priv *priv;
	int chip_config;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;

	chip_config = ytphy_read_ext(phydev, 0xa007);

	priv->chip_mode = chip_config & 0xf;
	switch (priv->chip_mode) {
		case 0x1:    //4'b0001, QSGMII to 1000BASE-X or 100BASE-FX x 4
		default:
			priv->polling_mode = YT_PHY_MODE_FIBER;
			break;
	}

	return 0;
}

static int yt8614Q_config_aneg(struct phy_device *phydev)
{
	//do nothing

	return 0;
}

static int yt8618_soft_reset(struct phy_device *phydev)
{
	int ret;

	ytphy_write_ext(phydev, 0xa000, 0);
	ret = ytphy_soft_reset(phydev);
	if (ret < 0)
		return ret;

	return 0;
}

static int __yt8614_soft_reset(struct phy_device *phydev)
{
	int ret;

	/* qsgmii */
	ytphy_write_ext(phydev, 0xa000, 2);
	ret = ytphy_soft_reset(phydev);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		return ret;
	}

	/* sgmii */
	ytphy_write_ext(phydev, 0xa000, 3);
	ret = ytphy_soft_reset(phydev);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		return ret;
	}

	/* utp */
	ytphy_write_ext(phydev, 0xa000, 0);
	ret = ytphy_soft_reset(phydev);
	if (ret < 0)
		return ret;

	return 0;
}

static int yt8614_soft_reset(struct phy_device *phydev)
{
	int ret;

	phy_lock_mdio_bus(phydev);
	ret = __yt8614_soft_reset(phydev);
	phy_unlock_mdio_bus(phydev);

	return ret;
}

static int yt8614Q_soft_reset(struct phy_device *phydev)
{
	int ret;

	/* qsgmii */
	ytphy_write_ext(phydev, 0xa000, 2);
	ret = ytphy_soft_reset(phydev);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		return ret;
	}

	/* sgmii */
	ytphy_write_ext(phydev, 0xa000, 3);
	ret = ytphy_soft_reset(phydev);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		return ret;
	}

	return 0;
}

static int yt8618_config_init(struct phy_device *phydev)
{
	int ret;
	int val;
	unsigned int retries = 12;
#if (YTPHY_861X_ABC_VER)
	int port = 0;
#endif

	phydev->irq = PHY_POLL;

#if (YTPHY_861X_ABC_VER)
	port = yt8614_get_port_from_phydev(phydev);
#endif

	ytphy_write_ext(phydev, 0xa000, 0);
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
	ret = ytphy_config_init(phydev);
#else
	ret = genphy_config_init(phydev);
#endif
	if (ret < 0)
		return ret;

	/* for utp to optimize signal */
	ret = ytphy_write_ext(phydev, 0x41, 0x33);
	if (ret < 0)
		return ret;
	ret = ytphy_write_ext(phydev, 0x42, 0x66);
	if (ret < 0)
		return ret;
	ret = ytphy_write_ext(phydev, 0x43, 0xaa);
	if (ret < 0)
		return ret;
	ret = ytphy_write_ext(phydev, 0x44, 0xd0d);
	if (ret < 0)
		return ret;

#if (YTPHY_861X_ABC_VER)
	if ((port == 2) || (port == 5)) {
		ret = ytphy_write_ext(phydev, 0x57, 0x2929);
		if (ret < 0)
			return ret;
	}
#endif

	val = phy_read(phydev, MII_BMCR);
	phy_write(phydev, MII_BMCR, val | BMCR_RESET);
	do {
		msleep(50);
		ret = phy_read(phydev, MII_BMCR);
		if (ret < 0)
			return ret;
	} while ((ret & BMCR_RESET) && --retries);
	if (ret & BMCR_RESET)
		return -ETIMEDOUT;

	/* for QSGMII optimization */
	ytphy_write_ext(phydev, 0xa000, 0x02);

	ret = ytphy_write_ext(phydev, 0x3, 0x4F80);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		return ret;
	}
	ret = ytphy_write_ext(phydev, 0xe, 0x4F80);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		return ret;
	}

	yt8618_soft_reset(phydev);

	ytphy_write_ext(phydev, 0xa000, 0);

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d\n", __func__, phydev->addr);
#else
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d\n", __func__, phydev->mdio.addr);
#endif
	return ret;
}

#if (YTPHY_861X_ABC_VER)
static int yt8614_get_port_from_phydev(struct phy_device *phydev)
{
	int tmp = ytphy_read_ext(phydev, 0xa0ff);
	int phy_addr = 0;

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	phy_addr = (unsigned int)phydev->addr;
#else
	phy_addr = (unsigned int)phydev->mdio.addr;
#endif

	if ((phy_addr - tmp) < 0) {
		ytphy_write_ext(phydev, 0xa0ff, phy_addr);
		tmp = phy_addr;
	}

	return (phy_addr - tmp);
}
#endif

static int yt8614_config_init(struct phy_device *phydev)
{
	int ret = 0;
	int val;
	unsigned int retries = 12;
#if (YTPHY_861X_ABC_VER)
	int port = 0;
#endif
	struct yt8xxx_priv *priv = phydev->priv;

	phydev->irq = PHY_POLL;


#if (YTPHY_861X_ABC_VER)
	port = yt8614_get_port_from_phydev(phydev);
#endif

	phy_lock_mdio_bus(phydev);
	ytphy_write_ext(phydev, 0xa000, 0);
	phy_unlock_mdio_bus(phydev);

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
	ret = ytphy_config_init(phydev);
#else
	ret = genphy_config_init(phydev);
#endif
	if (ret < 0)
		return ret;

	phy_lock_mdio_bus(phydev);
	/* for utp to optimize signal */
	ret = ytphy_write_ext(phydev, 0x41, 0x33);
	if (ret < 0)
		goto err_handle;
	ret = ytphy_write_ext(phydev, 0x42, 0x66);
	if (ret < 0)
		goto err_handle;
	ret = ytphy_write_ext(phydev, 0x43, 0xaa);
	if (ret < 0)
		goto err_handle;
	ret = ytphy_write_ext(phydev, 0x44, 0xd0d);
	if (ret < 0)
		goto err_handle;

#if (YTPHY_861X_ABC_VER)
	if (port == 2) {
		ret = ytphy_write_ext(phydev, 0x57, 0x2929);
		if (ret < 0)
			goto err_handle;
	}
#endif
	phy_unlock_mdio_bus(phydev);

	/* soft reset to take config effect */
	val = phy_read(phydev, MII_BMCR);
	phy_write(phydev, MII_BMCR, val | BMCR_RESET);
	do {
		msleep(50);
		ret = phy_read(phydev, MII_BMCR);
		if (ret < 0)
			return ret;
	} while ((ret & BMCR_RESET) && --retries);
	if (ret & BMCR_RESET)
		return -ETIMEDOUT;

	phy_lock_mdio_bus(phydev);
	/* for QSGMII optimization */
	ytphy_write_ext(phydev, 0xa000, 0x02);
	ret = ytphy_write_ext(phydev, 0x3, 0x4F80);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		goto err_handle;
	}
	ret = ytphy_write_ext(phydev, 0xe, 0x4F80);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		goto err_handle;
	}

	/* for SGMII optimization */
	ytphy_write_ext(phydev, 0xa000, 0x03);
	ret = ytphy_write_ext(phydev, 0x3, 0x2420);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		goto err_handle;
	}
	ret = ytphy_write_ext(phydev, 0xe, 0x24a0);
	if (ret < 0) {
		ytphy_write_ext(phydev, 0xa000, 0);
		goto err_handle;
	}

	__yt8614_soft_reset(phydev);

	/* back up to utp*/
	ytphy_write_ext(phydev, 0xa000, 0);

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d, chip mode: %d, polling mode = %d\n", __func__, phydev->addr, priv->chip_mode, priv->polling_mode);
#else
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d, chip mode: %d, polling mode = %d\n", __func__, phydev->mdio.addr, priv->chip_mode, priv->polling_mode);
#endif

err_handle:
	phy_unlock_mdio_bus(phydev);
	return ret;
}

static int yt8614Q_config_init(struct phy_device *phydev)
{
	struct yt8xxx_priv *priv = phydev->priv;

	phydev->irq = PHY_POLL;

	ytphy_write_ext(phydev, 0xa056, 0x7);

	yt8614Q_soft_reset(phydev);

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d, chip mode: %d, polling mode = %d\n", __func__, phydev->addr, priv->chip_mode, priv->polling_mode);
#else
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d, chip mode: %d, polling mode = %d\n", __func__, phydev->mdio.addr, priv->chip_mode, priv->polling_mode);
#endif
	return 0;
}

static int yt8618_aneg_done(struct phy_device *phydev)
{
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
	return genphy_aneg_done(phydev);
#else
	return 1;
#endif
}

static int yt8614_aneg_done(struct phy_device *phydev)
{
	int link_fiber = 0, link_utp = 0;
	struct yt8xxx_priv *priv = phydev->priv;

	phy_lock_mdio_bus(phydev);
	if (priv->polling_mode & YT_PHY_MODE_FIBER) {
		/* reading Fiber */
		ytphy_write_ext(phydev, 0xa000, 3);
		link_fiber = !!(__phy_read(phydev, REG_PHY_SPEC_STATUS) & (BIT(YTXXXX_LINK_STATUS_BIT)));
	}

	if (priv->polling_mode & YT_PHY_MODE_UTP) {
		/* reading UTP */
		ytphy_write_ext(phydev, 0xa000, 0);
		link_utp = !!(__phy_read(phydev, REG_PHY_SPEC_STATUS) & (BIT(YTXXXX_LINK_STATUS_BIT)));
	}
	phy_unlock_mdio_bus(phydev);

	return !!(link_fiber | link_utp);
}

static int yt8614Q_aneg_done(struct phy_device *phydev)
{
	int link_fiber = 0;
	struct yt8xxx_priv *priv = phydev->priv;

	if (priv->polling_mode & YT_PHY_MODE_FIBER) {
		/* reading Fiber */
		ytphy_write_ext(phydev, 0xa000, 3);
		link_fiber = !!(phy_read(phydev, REG_PHY_SPEC_STATUS) & (BIT(YTXXXX_LINK_STATUS_BIT)));
	}

	return !!(link_fiber);
}

static int yt8614_read_status(struct phy_device *phydev)
{
	int ret;
	int val, yt8614_fiber_latch_val, yt8614_fiber_curr_val;
	int link;
	int link_utp = 0, link_fiber = 0;
	struct yt8xxx_priv *priv = phydev->priv;

	phy_lock_mdio_bus(phydev);
	if (priv->polling_mode & YT_PHY_MODE_UTP) {
		/* switch to utp and reading regs  */
		ret = ytphy_write_ext(phydev, 0xa000, 0);
		if (ret < 0)
			goto err_handle;

		phy_unlock_mdio_bus(phydev);
		ret = genphy_read_status(phydev);
		if (ret < 0)
			return ret;

		phy_lock_mdio_bus(phydev);
		link_utp = phydev->link;
	}

	if (priv->polling_mode & YT_PHY_MODE_FIBER) {
		/* reading Fiber/sgmii */
		ret = ytphy_write_ext(phydev, 0xa000, 3);
		if (ret < 0)
			goto err_handle;

		val = __phy_read(phydev, REG_PHY_SPEC_STATUS);
		if (val < 0) {
			ret = val;
			goto err_handle;
		}

		/* for fiber, from 1000m to 100m, there is not link down from 0x11,
		 * and check reg 1 to identify such case
		 */
		yt8614_fiber_latch_val = __phy_read(phydev, MII_BMSR);
		yt8614_fiber_curr_val = __phy_read(phydev, MII_BMSR);
		link = val & (BIT(YTXXXX_LINK_STATUS_BIT));
		if (link && yt8614_fiber_latch_val != yt8614_fiber_curr_val) {
			link = 0;
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, fiber link down detect, latch = %04x, curr = %04x\n",
					__func__, phydev->addr, yt8614_fiber_latch_val, yt8614_fiber_curr_val);
#else
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, fiber link down detect, latch = %04x, curr = %04x\n",
					__func__, phydev->mdio.addr, yt8614_fiber_latch_val, yt8614_fiber_curr_val);
#endif
		}

		if (link) {
			link_fiber = 1;
			ytxxxx_adjust_status(phydev, val, 0);
		} else {
			link_fiber = 0;
		}
	}

	if (link_utp || link_fiber) {
		if (phydev->link == 0)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media %s\n",
					__func__, phydev->addr, (link_utp && link_fiber) ? "both UTP and Fiber" : (link_utp ? "UTP" : "Fiber"));
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media %s\n",
				__func__, phydev->mdio.addr, (link_utp && link_fiber) ? "both UTP and Fiber" : (link_utp ? "UTP" : "Fiber"));
#endif
		phydev->link = 1;
	} else {
		if (phydev->link == 1)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->addr);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->mdio.addr);
#endif
		phydev->link = 0;
	}

	if (priv->polling_mode & YT_PHY_MODE_UTP) {
		if (link_utp)
			ytphy_write_ext(phydev, 0xa000, 0);
	}

err_handle:
	phy_unlock_mdio_bus(phydev);
	return ret;
}

static int yt8614Q_read_status(struct phy_device *phydev)
{
	int ret;
	int val, yt8614Q_fiber_latch_val, yt8614Q_fiber_curr_val;
	int link;
	int link_fiber = 0;
	struct yt8xxx_priv *priv = phydev->priv;

	if (priv->polling_mode & YT_PHY_MODE_FIBER) {
		/* reading Fiber/sgmii */
		ret = ytphy_write_ext(phydev, 0xa000, 3);
		if (ret < 0)
			return ret;

		val = phy_read(phydev, REG_PHY_SPEC_STATUS);
		if (val < 0)
			return val;

		/* for fiber, from 1000m to 100m, there is not link down from 0x11,
		 * and check reg 1 to identify such case
		 */
		yt8614Q_fiber_latch_val = phy_read(phydev, MII_BMSR);
		yt8614Q_fiber_curr_val = phy_read(phydev, MII_BMSR);
		link = val & (BIT(YTXXXX_LINK_STATUS_BIT));
		if (link && yt8614Q_fiber_latch_val != yt8614Q_fiber_curr_val) {
			link = 0;
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, fiber link down detect, latch = %04x, curr = %04x\n",
					__func__, phydev->addr, yt8614Q_fiber_latch_val, yt8614Q_fiber_curr_val);
#else
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, fiber link down detect, latch = %04x, curr = %04x\n",
					__func__, phydev->mdio.addr, yt8614Q_fiber_latch_val, yt8614Q_fiber_curr_val);
#endif
		}

		if (link) {
			link_fiber = 1;
			ytxxxx_adjust_status(phydev, val, 0);
		} else {
			link_fiber = 0;
		}
	}

	if (link_fiber) {
		if (phydev->link == 0)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media Fiber\n",
					__func__, phydev->addr);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media Fiber\n",
				__func__, phydev->mdio.addr);
#endif
		phydev->link = 1;
	} else {
		if (phydev->link == 1)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->addr);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->mdio.addr);
#endif
		phydev->link = 0;
	}

	return 0;
}

static int yt8618_read_status(struct phy_device *phydev)
{
	int ret;
	int val;
	int link;
	int link_utp = 0;

	ret = ytphy_write_ext(phydev, 0xa000, 0);
	if (ret < 0)
		return ret;

	genphy_read_status(phydev);

	val = phy_read(phydev, REG_PHY_SPEC_STATUS);
	if (val < 0)
		return val;

	link = val & (BIT(YTXXXX_LINK_STATUS_BIT));
	if (link) {
		link_utp = 1;
		ytxxxx_adjust_status(phydev, val, 1);
	} else {
		link_utp = 0;
	}

	if (link_utp)
		phydev->link = 1;
	else
		phydev->link = 0;

	return 0;
}

static int yt8618_suspend(struct phy_device *phydev)
{
#if !(SYS_WAKEUP_BASED_ON_ETH_PKT)
	int value;

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_lock(&phydev->lock);
#else
	/* no need lock in 4.19 */
#endif

	ytphy_write_ext(phydev, 0xa000, 0);
	value = phy_read(phydev, MII_BMCR);
	phy_write(phydev, MII_BMCR, value | BMCR_PDOWN);

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_unlock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif
#endif /*!(SYS_WAKEUP_BASED_ON_ETH_PKT)*/

	return 0;
}

static int yt8618_resume(struct phy_device *phydev)
{
#if !(SYS_WAKEUP_BASED_ON_ETH_PKT)
	int value;

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_lock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif

	ytphy_write_ext(phydev, 0xa000, 0);
	value = phy_read(phydev, MII_BMCR);
	phy_write(phydev, MII_BMCR, value & ~BMCR_PDOWN);

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_unlock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif
#endif /*!(SYS_WAKEUP_BASED_ON_ETH_PKT)*/

	return 0;
}

static int yt8614_suspend(struct phy_device *phydev)
{
#if !(SYS_WAKEUP_BASED_ON_ETH_PKT)
	int value;

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_lock(&phydev->lock);
#else
	/* no need lock in 4.19 */
#endif

	phy_lock_mdio_bus(phydev);
	ytphy_write_ext(phydev, 0xa000, 0);
	value = __phy_read(phydev, MII_BMCR);
	__phy_write(phydev, MII_BMCR, value | BMCR_PDOWN);

	ytphy_write_ext(phydev, 0xa000, 3);
	value = __phy_read(phydev, MII_BMCR);
	__phy_write(phydev, MII_BMCR, value | BMCR_PDOWN);

	ytphy_write_ext(phydev, 0xa000, 0);
	phy_unlock_mdio_bus(phydev);

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_unlock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif
#endif /*!(SYS_WAKEUP_BASED_ON_ETH_PKT)*/

	return 0;
}

static int yt8614Q_suspend(struct phy_device *phydev)
{
#if !(SYS_WAKEUP_BASED_ON_ETH_PKT)
	int value;

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_lock(&phydev->lock);
#else
	/* no need lock in 4.19 */
#endif

	ytphy_write_ext(phydev, 0xa000, 3);
	value = phy_read(phydev, MII_BMCR);
	phy_write(phydev, MII_BMCR, value | BMCR_PDOWN);

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_unlock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif
#endif /*!(SYS_WAKEUP_BASED_ON_ETH_PKT)*/

	return 0;
}

static int yt8614_resume(struct phy_device *phydev)
{
#if !(SYS_WAKEUP_BASED_ON_ETH_PKT)
	int value;

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_lock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif

	phy_lock_mdio_bus(phydev);
	ytphy_write_ext(phydev, 0xa000, 0);
	value = __phy_read(phydev, MII_BMCR);
	__phy_write(phydev, MII_BMCR, value & ~BMCR_PDOWN);

	ytphy_write_ext(phydev, 0xa000, 3);
	value = __phy_read(phydev, MII_BMCR);
	__phy_write(phydev, MII_BMCR, value & ~BMCR_PDOWN);

	ytphy_write_ext(phydev, 0xa000, 0);
	phy_unlock_mdio_bus(phydev);

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_unlock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif
#endif /* !(SYS_WAKEUP_BASED_ON_ETH_PKT) */

	return 0;
}

static int yt8614Q_resume(struct phy_device *phydev)
{
#if !(SYS_WAKEUP_BASED_ON_ETH_PKT)
	int value;

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_lock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif

	ytphy_write_ext(phydev, 0xa000, 3);
	value = phy_read(phydev, MII_BMCR);
	phy_write(phydev, MII_BMCR, value & ~BMCR_PDOWN);

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	mutex_unlock(&phydev->lock);
#else
	/* no need lock/unlock in 4.19 */
#endif
#endif /* !(SYS_WAKEUP_BASED_ON_ETH_PKT) */

	return 0;
}

static int ytxxxx_soft_reset(struct phy_device *phydev)
{
	int ret, val;

	phy_lock_mdio_bus(phydev);
	val = ytphy_read_ext(phydev, 0xa001);
	ytphy_write_ext(phydev, 0xa001, (val & ~0x8000));

	ret = ytphy_write_ext(phydev, 0xa000, 0);
	phy_unlock_mdio_bus(phydev);

	return ret;
}

static int yt8821_init(struct phy_device *phydev)
{
	int ret = 0;

	ret = ytphy_write_ext(phydev, 0xa000, 0x2);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x23, 0x8605);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0xa000, 0x0);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x34e, 0x8080);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x4d2, 0x5200);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x4d3, 0x5200);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x372, 0x5a3c);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x374, 0x7c6c);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x336, 0xaa0a);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x340, 0x3022);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x36a, 0x8000);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x4b3, 0x7711);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x4b5, 0x2211);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x56, 0x20);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x56, 0x3f);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x97, 0x380c);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x660, 0x112a);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x450, 0xe9);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x466, 0x6464);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x467, 0x6464);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x468, 0x6464);
	if (ret < 0)
		return ret;

	ret = ytphy_write_ext(phydev, 0x469, 0x6464);
	if (ret < 0)
		return ret;

	return ret;
}

static int yt8821_config_init(struct phy_device *phydev)
{
	int ret, val;

	phydev->irq = PHY_POLL;

	phy_lock_mdio_bus(phydev);
	val = ytphy_read_ext(phydev, 0xa001);
	if (phydev->interface == PHY_INTERFACE_MODE_SGMII)
	{
		val &= ~(BIT(0));
		val &= ~(BIT(1));
		val &= ~(BIT(2));
		ret = ytphy_write_ext(phydev, 0xa001, val);
		if (ret < 0)
			goto err_handle;

		ret = ytphy_write_ext(phydev, 0xa000, 2);
		if (ret < 0)
			goto err_handle;

		val = __phy_read(phydev, MII_BMCR);
		val |= BIT(YTXXXX_AUTO_NEGOTIATION_BIT);
		__phy_write(phydev, MII_BMCR, val);

		ret = ytphy_write_ext(phydev, 0xa000, 0x0);
		if (ret < 0)
			goto err_handle;
	}
#if (KERNEL_VERSION(4, 10, 17) < LINUX_VERSION_CODE)
	else if (phydev->interface == PHY_INTERFACE_MODE_2500BASEX)
	{
		val |= BIT(0);
		val &= ~(BIT(1));
		val &= ~(BIT(2));
		ret = ytphy_write_ext(phydev, 0xa001, val);
		if (ret < 0)
			goto err_handle;

		ret = ytphy_write_ext(phydev, 0xa000, 0x0);
		if (ret < 0)
			goto err_handle;

		val = __phy_read(phydev, MII_ADVERTISE);
		val |= BIT(YTXXXX_PAUSE_BIT);                   //Pause
		val |= BIT(YTXXXX_ASYMMETRIC_PAUSE_BIT);        //Asymmetric_Pause
		__phy_write(phydev, MII_ADVERTISE, val);

		ret = ytphy_write_ext(phydev, 0xa000, 2);
		if (ret < 0)
			goto err_handle;

		val = __phy_read(phydev, MII_ADVERTISE);
		val |= BIT(YT8821_SDS_PAUSE_BIT);               //Pause
		val |= BIT(YT8821_SDS_ASYMMETRIC_PAUSE_BIT);    //Asymmetric_Pause
		__phy_write(phydev, MII_ADVERTISE, val);

		val = __phy_read(phydev, MII_BMCR);
		val &= (~BIT(YTXXXX_AUTO_NEGOTIATION_BIT));
		__phy_write(phydev, MII_BMCR, val);

		ret = ytphy_write_ext(phydev, 0xa000, 0x0);
		if (ret < 0)
			goto err_handle;
	}
#endif
	else
	{
		val |= BIT(0);
		val &= ~(BIT(1));
		val |= BIT(2);
		ret = ytphy_write_ext(phydev, 0xa001, val);
		if (ret < 0)
			goto err_handle;
	}

	ret = yt8821_init(phydev);
	if (ret < 0)
		goto err_handle;

	/* disable auto sleep */
	val = ytphy_read_ext(phydev, YT8521_EXTREG_SLEEP_CONTROL1);
	if (val < 0)
		goto err_handle;

	val &= (~BIT(YT8521_EN_SLEEP_SW_BIT));

	ret = ytphy_write_ext(phydev, YT8521_EXTREG_SLEEP_CONTROL1, val);
	if (ret < 0)
		goto err_handle;

	phy_unlock_mdio_bus(phydev);
	/* soft reset */
	ytxxxx_soft_reset(phydev);

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d\n", __func__, phydev->addr);
#else
	netdev_info(phydev->attached_dev, "%s done, phy addr: %d\n", __func__, phydev->mdio.addr);
#endif
	return ret;

err_handle:
	phy_unlock_mdio_bus(phydev);
	return ret;
}

#if (KERNEL_VERSION(6, 0, 19) < LINUX_VERSION_CODE)
static int yt8821_get_rate_matching(struct phy_device *phydev,
		phy_interface_t iface)
{
	int val;

	phy_lock_mdio_bus(phydev);
	val = ytphy_read_ext(phydev, 0xa001);
	phy_unlock_mdio_bus(phydev);
	if (val < 0)
		return val;

	if (val & (BIT(2) | BIT(1) | BIT(0)))
		return RATE_MATCH_PAUSE;

	return RATE_MATCH_NONE;
}
#endif

static int yt8821_aneg_done(struct phy_device *phydev)
{
	int link_utp = 0;

	/* reading UTP */
	phy_lock_mdio_bus(phydev);
	ytphy_write_ext(phydev, 0xa000, 0);
	link_utp = !!(__phy_read(phydev, REG_PHY_SPEC_STATUS) & (BIT(10)));
	phy_unlock_mdio_bus(phydev);

#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
	netdev_info(phydev->attached_dev, "%s, phy addr: %d, link_utp: %d\n",
			__func__, phydev->addr, link_utp);
#else
	netdev_info(phydev->attached_dev, "%s, phy addr: %d, link_utp: %d\n",
			__func__, phydev->mdio.addr, link_utp);
#endif

	return !!(link_utp);
}
static int yt8821_adjust_status(struct phy_device *phydev, int val)
{
	int speed_mode, duplex;
	int speed_mode_bit15_14, speed_mode_bit9;
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
	int speed = -1;
#else
	int speed = SPEED_UNKNOWN;
#endif

	duplex = (val & YTXXXX_DUPLEX) >> YTXXXX_DUPLEX_BIT;

	/* Bit9-Bit15-Bit14 speed mode 100---2.5G; 010---1000M; 001---100M; 000---10M */
	speed_mode_bit15_14 = (val & YTXXXX_SPEED_MODE) >> YTXXXX_SPEED_MODE_BIT;
	speed_mode_bit9 = (val & BIT(9)) >> 9;
	speed_mode = (speed_mode_bit9 << 2) | speed_mode_bit15_14;
	switch (speed_mode) {
		case 0:
			speed = SPEED_10;
			break;
		case 1:
			speed = SPEED_100;
			break;
		case 2:
			speed = SPEED_1000;
			break;
		case 4:
			speed = SPEED_2500;
			break;
		default:
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
			speed = -1;
#else
			speed = SPEED_UNKNOWN;
#endif
			break;
	}

	phydev->speed = speed;
	phydev->duplex = duplex;

	return 0;
}

static int yt8821_read_status(struct phy_device *phydev)
{
	int ret;
	int val;
	int link;
	int link_utp = 0;

	/* reading UTP */
	phy_lock_mdio_bus(phydev);
	ret = ytphy_write_ext(phydev, 0xa000, 0);
	if (ret < 0)
		goto err_handle;

	phy_unlock_mdio_bus(phydev);
	genphy_read_status(phydev);

	val = phy_read(phydev, REG_PHY_SPEC_STATUS);
	if (val < 0)
		return val;

	link = val & (BIT(YTXXXX_LINK_STATUS_BIT));
	if (link) {
		link_utp = 1;
		yt8821_adjust_status(phydev, val);    /* speed(2500), duplex */
	} else {
		link_utp = 0;
	}

	if (link_utp) {
		if (phydev->link == 0)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media: UTP, mii reg 0x11 = 0x%x\n",
					__func__, phydev->addr, (unsigned int)val);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link up, media: UTP, mii reg 0x11 = 0x%x\n",
				__func__, phydev->mdio.addr, (unsigned int)val);
#endif
		phydev->link = 1;
	} else {
		if (phydev->link == 1)
#if (KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE)
			netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->addr);
#else
		netdev_info(phydev->attached_dev, "%s, phy addr: %d, link down\n", __func__, phydev->mdio.addr);
#endif

		phydev->link = 0;
	}

	phy_lock_mdio_bus(phydev);
	if (link_utp)
		ytphy_write_ext(phydev, 0xa000, 0);

#if (KERNEL_VERSION(4, 10, 17) < LINUX_VERSION_CODE)
	val = ytphy_read_ext(phydev, 0xa001);
	if ((val & (BIT(2) | BIT(1) | BIT(0))) == 0x0)
	{
		switch (phydev->speed)
		{
			case SPEED_2500:
				phydev->interface = PHY_INTERFACE_MODE_2500BASEX;
				break;
			case SPEED_1000:
			case SPEED_100:
			case SPEED_10:
				phydev->interface = PHY_INTERFACE_MODE_SGMII;
				break;
		}
	}
#endif

err_handle:
	phy_unlock_mdio_bus(phydev);
	return ret;
}

#if (KERNEL_VERSION(5, 0, 21) < LINUX_VERSION_CODE)
static int yt8821_get_features(struct phy_device *phydev)
{
	linkmode_mod_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->supported, 1);
	return genphy_read_abilities(phydev);
}
#endif

static int ytxxxx_suspend(struct phy_device *phydev)
{
	int value = 0;
	int wol_enabled = 0;

#if (YTPHY_WOL_FEATURE_ENABLE)
	value = phy_read(phydev, YTPHY_UTP_INTR_REG);
	wol_enabled = value & YTPHY_WOL_FEATURE_INTR;
#endif

	if (!wol_enabled)
	{
		value = phy_read(phydev, MII_BMCR);
		phy_write(phydev, MII_BMCR, value | BMCR_PDOWN);
	}

	return 0;
}

static int ytxxxx_resume(struct phy_device *phydev)
{
	int value;

	value = phy_read(phydev, MII_BMCR);
	value &= ~BMCR_PDOWN;
	value &= ~BMCR_ISOLATE;

	phy_write(phydev, MII_BMCR, value);

	return 0;
}

static struct phy_driver ytphy_drvs[] = {
	{
		.phy_id         = PHY_ID_YT8010,
		.name           = "YT8010 100M Automotive Ethernet",
		.phy_id_mask    = MOTORCOMM_PHY_ID_MASK,
		.features       = PHY_BASIC_FEATURES,
		.flags          = PHY_POLL,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
		.soft_reset     = yt8010_soft_reset,
#endif
		.config_aneg    = yt8010_config_aneg,
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
		.aneg_done      = yt8010_aneg_done,
#endif
		.config_init    = yt8010_config_init,
		.read_status    = yt8010_read_status,
	}, {
		.phy_id         = PHY_ID_YT8010AS,
		.name           = "YT8010AS 100M Automotive Ethernet",
		.phy_id_mask    = MOTORCOMM_PHY_ID_MASK,
		.features       = PHY_BASIC_FEATURES,
		.flags          = PHY_POLL,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
		.soft_reset     = yt8010AS_soft_reset,
#endif
		.config_aneg    = yt8010_config_aneg,
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
		.aneg_done      = yt8010_aneg_done,
#endif
		.config_init    = yt8010AS_config_init,
		.read_status    = yt8010_read_status,
	}, {
		.phy_id         = PHY_ID_YT8011,
			.name           = "YT8011 Automotive Gigabit Ethernet",
			.phy_id_mask    = MOTORCOMM_PHY_ID_MASK,
			.features       = PHY_GBIT_FEATURES,
			.flags          = PHY_POLL,
			.probe          = yt8011_probe,
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
#else
			.soft_reset     = yt8011_soft_reset,
#endif
			.config_aneg    = yt8011_config_aneg,
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
			.aneg_done      = yt8011_aneg_done,
#endif
			.config_init    = yt8011_config_init,
			.read_status    = yt8011_read_status,
	}, {
		.phy_id         = PHY_ID_YT8510,
			.name           = "YT8510 100M Ethernet",
			.phy_id_mask    = MOTORCOMM_PHY_ID_MASK,
			.features       = PHY_BASIC_FEATURES,
			.flags          = PHY_POLL,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
			.soft_reset     = ytphy_soft_reset,
#endif
			.config_aneg    = genphy_config_aneg,
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
			.config_init    = ytphy_config_init,
#else
			.config_init    = genphy_config_init,
#endif
			.read_status    = genphy_read_status,
	}, {
		.phy_id         = PHY_ID_YT8511,
			.name           = "YT8511 Gigabit Ethernet",
			.phy_id_mask    = MOTORCOMM_PHY_ID_MASK,
			.features       = PHY_GBIT_FEATURES,
			.flags          = PHY_POLL,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
			.soft_reset     = ytphy_soft_reset,
#endif
			.config_aneg    = genphy_config_aneg,
#if GMAC_CLOCK_INPUT_NEEDED
			.config_init    = yt8511_config_init,
#else
#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE) || (KERNEL_VERSION(5, 3, 0) < LINUX_VERSION_CODE)
			.config_init    = ytphy_config_init,
#else
			.config_init    = genphy_config_init,
#endif
#endif
			.read_status    = genphy_read_status,
			.suspend        = genphy_suspend,
			.resume         = genphy_resume,
	}, {
		.phy_id         = PHY_ID_YT8512,
			.name           = "YT8512 100M Ethernet",
			.phy_id_mask    = MOTORCOMM_PHY_ID_MASK,
			.features       = PHY_BASIC_FEATURES,
			.flags          = PHY_POLL,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
			.soft_reset     = ytphy_soft_reset,
#endif
			.config_aneg    = genphy_config_aneg,
			.config_init    = yt8512_config_init,
			.probe          = yt8512_probe,
			.read_status    = yt8512_read_status,
			.suspend        = genphy_suspend,
			.resume         = genphy_resume,
	}, {
		.phy_id         = PHY_ID_YT8522,
			.name           = "YT8522 100M Ethernet",
			.phy_id_mask    = MOTORCOMM_PHY_ID_MASK,
			.features       = PHY_BASIC_FEATURES,
			.flags          = PHY_POLL,
			.probe          = yt8522_probe,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
			.soft_reset     = ytphy_soft_reset,
#endif
			.config_aneg    = genphy_config_aneg,
			.config_init    = yt8522_config_init,
			.read_status    = yt8522_read_status,
			.suspend        = genphy_suspend,
			.resume         = genphy_resume,
	}, {
		.phy_id         = PHY_ID_YT8521,
			.name           = "YT8521 Ethernet",
			.phy_id_mask    = MOTORCOMM_PHY_ID_MASK,
			.features       = PHY_GBIT_FEATURES,
			.flags          = PHY_POLL,
			.probe          = yt8521_probe,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
			.soft_reset     = ytxxxx_soft_reset,
#endif
			.config_aneg    = genphy_config_aneg,
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
			.aneg_done      = yt8521_aneg_done,
#endif
			.config_init    = yt8521_config_init,
			.read_status    = yt8521_read_status,
			.suspend        = yt8521_suspend,
			.resume         = yt8521_resume,
#if (YTPHY_WOL_FEATURE_ENABLE)
			.get_wol        = &ytphy_wol_feature_get,
			.set_wol        = &ytphy_wol_feature_set,
#endif
	}, {
		/* same as 8521 */
		.phy_id        = PHY_ID_YT8531S,
			.name          = "YT8531S Ethernet",
			.phy_id_mask   = MOTORCOMM_PHY_ID_MASK,
			.features      = PHY_GBIT_FEATURES,
			.flags         = PHY_POLL,
			.probe         = yt8521_probe,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
			.soft_reset    = ytxxxx_soft_reset,
#endif
			.config_aneg   = genphy_config_aneg,
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
			.aneg_done     = yt8521_aneg_done,
#endif
			.config_init   = yt8531S_config_init,
			.read_status   = yt8521_read_status,
			.suspend       = yt8521_suspend,
			.resume        = yt8521_resume,
#if (YTPHY_WOL_FEATURE_ENABLE)
			.get_wol       = &ytphy_wol_feature_get,
			.set_wol       = &ytphy_wol_feature_set,
#endif
	}, {
		/* same as 8511 */
		.phy_id        = PHY_ID_YT8531,
			.name          = "YT8531 Gigabit Ethernet",
			.phy_id_mask   = MOTORCOMM_PHY_ID_MASK,
			.features      = PHY_GBIT_FEATURES,
			.flags         = PHY_POLL,
			.config_aneg   = genphy_config_aneg,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
			.soft_reset    = ytphy_soft_reset,
#endif
			.config_init   = yt8531_config_init,
			.read_status   = genphy_read_status,
#if (KERNEL_VERSION(5, 0, 21) < LINUX_VERSION_CODE)
			.link_change_notify = ytphy_link_change_notify,
#endif
			.suspend       = genphy_suspend,
			.resume        = genphy_resume,
#if (YTPHY_WOL_FEATURE_ENABLE)
			.get_wol       = &ytphy_wol_feature_get,
			.set_wol       = &ytphy_wol_feature_set,
#endif
	},
#ifdef YTPHY_YT8543_ENABLE
	{
		.phy_id        = PHY_ID_YT8543,
		.name          = "YT8543 Dual Port Gigabit Ethernet",
		.phy_id_mask   = MOTORCOMM_PHY_ID_MASK,
		.features      = PHY_GBIT_FEATURES,
		.flags         = PHY_POLL,
		.config_aneg   = genphy_config_aneg,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
		.soft_reset    = ytphy_soft_reset,
#endif
		.config_init   = yt8543_config_init,
		.read_status   = yt8543_read_status,
		.suspend       = ytxxxx_suspend,
		.resume        = ytxxxx_resume,
#if (YTPHY_WOL_FEATURE_ENABLE)
		.get_wol       = &ytphy_wol_feature_get,
		.set_wol       = &ytphy_wol_feature_set,
#endif
	},
#endif
	{
		.phy_id        = PHY_ID_YT8618,
		.name          = "YT8618 Ethernet",
		.phy_id_mask   = MOTORCOMM_PHY_ID_MASK,
		.features      = PHY_GBIT_FEATURES,
		.flags         = PHY_POLL,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
		.soft_reset    = yt8618_soft_reset,
#endif
		.config_aneg   = genphy_config_aneg,
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
		.aneg_done     = yt8618_aneg_done,
#endif
		.config_init   = yt8618_config_init,
		.read_status   = yt8618_read_status,
		.suspend       = yt8618_suspend,
		.resume        = yt8618_resume,
	},
	{
		.phy_id        = PHY_ID_YT8614,
		.name          = "YT8614 Ethernet",
		.phy_id_mask   = MOTORCOMM_PHY_ID_MASK,
		.features      = PHY_GBIT_FEATURES,
		.flags         = PHY_POLL,
		.probe         = yt8614_probe,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
		.soft_reset    = yt8614_soft_reset,
#endif
		.config_aneg   = genphy_config_aneg,
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
		.aneg_done     = yt8614_aneg_done,
#endif
		.config_init   = yt8614_config_init,
		.read_status   = yt8614_read_status,
		.suspend       = yt8614_suspend,
		.resume        = yt8614_resume,
	},
	{
		.phy_id        = PHY_ID_YT8614Q,
		.name          = "YT8614Q Ethernet",
		.phy_id_mask   = MOTORCOMM_PHY_ID_MASK,
		.features      = PHY_GBIT_FEATURES,
		.flags         = PHY_POLL,
		.probe         = yt8614Q_probe,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
		.soft_reset    = yt8614Q_soft_reset,
#endif
		.config_aneg   = yt8614Q_config_aneg,
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
		.aneg_done     = yt8614Q_aneg_done,
#endif
		.config_init   = yt8614Q_config_init,
		.read_status   = yt8614Q_read_status,
		.suspend       = yt8614Q_suspend,
		.resume        = yt8614Q_resume,
	},
	{
		.phy_id        = PHY_ID_YT8821,
		.name          = "YT8821 2.5Gbps Ethernet",
		.phy_id_mask   = MOTORCOMM_PHY_ID_MASK,
#if (KERNEL_VERSION(5, 1, 0) > LINUX_VERSION_CODE)
		.features      = PHY_GBIT_FEATURES,
#endif
		.flags         = PHY_POLL,
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
#else
		.soft_reset    = ytxxxx_soft_reset,
#endif
		.config_aneg   = genphy_config_aneg,
#if (KERNEL_VERSION(3, 14, 79) < LINUX_VERSION_CODE)
		.aneg_done     = yt8821_aneg_done,
#endif
#if (KERNEL_VERSION(5, 0, 21) < LINUX_VERSION_CODE)
		.get_features  = yt8821_get_features,
#endif
		.config_init   = yt8821_config_init,
#if (YTPHY_WOL_FEATURE_ENABLE)
		.set_wol       = &ytphy_wol_feature_set,
		.get_wol       = &ytphy_wol_feature_get,
#endif
#if (KERNEL_VERSION(6, 0, 19) < LINUX_VERSION_CODE)
		.get_rate_matching  = yt8821_get_rate_matching,
#endif
		.read_status   = yt8821_read_status,
		.suspend       = ytxxxx_suspend,
		.resume        = ytxxxx_resume,
	},
};

#if (KERNEL_VERSION(5, 6, 0) > LINUX_VERSION_CODE)
#include <linux/fs.h>
static ssize_t proc_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	char buffer[BUFFER_SIZE];
	int len;

	len = snprintf(buffer, BUFFER_SIZE, "Driver Version: %s\n", YTPHY_LINUX_VERSION);

	if (*ppos > 0 || count < len)
		return 0;

	if (copy_to_user(user_buf, buffer, len))
		return -EFAULT;

	*ppos = len;
	return len;
}

static const struct file_operations proc_file_ops = {
	.read = proc_read,
};
#else
static ssize_t proc_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	char buffer[BUFFER_SIZE];
	int len;

	len = snprintf(buffer, BUFFER_SIZE, "Driver Version: %s\n", YTPHY_LINUX_VERSION);

	if (*ppos > 0 || count < len)
		return 0;

	if (copy_to_user(user_buf, buffer, len))
		return -EFAULT;

	*ppos = len;
	return len;
}

static const struct proc_ops proc_file_ops = {
	.proc_read = proc_read,
};
#endif

#if (KERNEL_VERSION(4, 0, 0) > LINUX_VERSION_CODE)
static int ytphy_drivers_register(struct phy_driver *phy_drvs, int size)
{
	int i, j;
	int ret;

	for (i = 0; i < size; i++) {
		ret = phy_driver_register(&phy_drvs[i]);
		if (ret)
			goto err;
	}

	return 0;

err:
	for (j = 0; j < i; j++)
		phy_driver_unregister(&phy_drvs[j]);

	return ret;
}

static void ytphy_drivers_unregister(struct phy_driver *phy_drvs, int size)
{
	int i;

	for (i = 0; i < size; i++)
		phy_driver_unregister(&phy_drvs[i]);
}

static int __init ytphy_init(void)
{
	proc_file = proc_create(PROC_FILENAME, 0444, NULL, &proc_file_ops);
	if (!proc_file) {
		pr_err("Failed to create /proc/%s\n", PROC_FILENAME);
		return -ENOMEM;
	}
	return ytphy_drivers_register(ytphy_drvs, ARRAY_SIZE(ytphy_drvs));
}

static void __exit ytphy_exit(void)
{
	proc_remove(proc_file);
	pr_info("/proc/%s removed\n", PROC_FILENAME);

	ytphy_drivers_unregister(ytphy_drvs, ARRAY_SIZE(ytphy_drvs));
}

module_init(ytphy_init);
module_exit(ytphy_exit);
#else
/* for linux 4.x */
//module_phy_driver(ytphy_drvs);
static int __init phy_module_init(void)
{
	proc_file = proc_create(PROC_FILENAME, 0444, NULL, &proc_file_ops);
	if (!proc_file) {
		pr_err("Failed to create /proc/%s\n", PROC_FILENAME);
		return -ENOMEM;
	}

	return phy_drivers_register(ytphy_drvs, ARRAY_SIZE(ytphy_drvs), THIS_MODULE);
}
static void __exit phy_module_exit(void)
{
	proc_remove(proc_file);
	pr_info("/proc/%s removed\n", PROC_FILENAME);

	phy_drivers_unregister(ytphy_drvs, ARRAY_SIZE(ytphy_drvs));
}

module_init(phy_module_init);
module_exit(phy_module_exit);
#endif

MODULE_DESCRIPTION("Motorcomm PHY driver");
MODULE_VERSION(YTPHY_LINUX_VERSION);        //for modinfo xxxx.ko in userspace
MODULE_AUTHOR("Leilei Zhao");
MODULE_LICENSE("GPL");

static struct mdio_device_id __maybe_unused motorcomm_tbl[] = {
	{ PHY_ID_YT8010, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8010AS, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8510, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8511, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8512, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8522, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8521, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8531S, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8531, MOTORCOMM_PHY_ID_MASK },
#ifdef YTPHY_YT8543_ENABLE
	{ PHY_ID_YT8543, MOTORCOMM_PHY_ID_MASK },
#endif
	{ PHY_ID_YT8614, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8614Q, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8618, MOTORCOMM_PHY_ID_MASK },
	{ PHY_ID_YT8821, MOTORCOMM_PHY_ID_MASK },
	{ }
};

MODULE_DEVICE_TABLE(mdio, motorcomm_tbl);
