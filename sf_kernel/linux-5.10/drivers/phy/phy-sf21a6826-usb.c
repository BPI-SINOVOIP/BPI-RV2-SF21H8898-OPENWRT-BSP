/*
 * SIFLOWER sf21a6826 USB PHY driver
 *
 * Copyright (C) 2015 Google, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

#define SF21A6826_USB_PHY_RESET_OFFSET		0xC

struct sf21a6826_usb_phy {
	struct device *dev;
	struct clk *phy_clk;
	struct clk *bus_clk;
	void __iomem *base;
};

static int sf21a6826_usb_phy_power_on(struct phy *phy)
{
	struct sf21a6826_usb_phy *p_phy = phy_get_drvdata(phy);
	int ret;

	ret = clk_prepare_enable(p_phy->phy_clk);
	if (ret < 0) {
		dev_err(p_phy->dev, "Failed to enable PHY clock: %d\n", ret);
		return ret;
	}

	writel(0, p_phy->base + SF21A6826_USB_PHY_RESET_OFFSET);
	/* Without this delay, dwc2 soft reset will stuck */
	usleep_range(500, 1000);

	return ret;
}

static int sf21a6826_usb_phy_power_off(struct phy *phy)
{
	struct sf21a6826_usb_phy *p_phy = phy_get_drvdata(phy);
	clk_disable_unprepare(p_phy->phy_clk);

	return 0;
}

static int sf21a6826_usb_phy_exit(struct phy *phy)
{
	struct sf21a6826_usb_phy *p_phy = phy_get_drvdata(phy);
	writel(0x7, p_phy->base + SF21A6826_USB_PHY_RESET_OFFSET);

	return 0;
}

static const struct phy_ops sf21a6826_usb_phy_ops = {
	.power_on = sf21a6826_usb_phy_power_on,
	.power_off = sf21a6826_usb_phy_power_off,
	.exit = sf21a6826_usb_phy_exit,
	.owner = THIS_MODULE,
};

static int sf21a6826_usb_phy_probe(struct platform_device *pdev)
{
	struct sf21a6826_usb_phy *p_phy;
	struct phy_provider *provider;
	struct phy *phy;

	p_phy = devm_kzalloc(&pdev->dev, sizeof(*p_phy), GFP_KERNEL);
	if (!p_phy)
		return -ENOMEM;

	p_phy->dev = &pdev->dev;
	platform_set_drvdata(pdev, p_phy);

	p_phy->phy_clk = devm_clk_get(p_phy->dev, "usb_phy_clk");
	if (IS_ERR(p_phy->phy_clk))
		return dev_err_probe(p_phy->dev, PTR_ERR(p_phy->phy_clk),
				     "Failed to get usb_phy clock.\n");

	p_phy->base = devm_platform_ioremap_resource(pdev, 0);

	phy = devm_phy_create(p_phy->dev, NULL, &sf21a6826_usb_phy_ops);
	if (IS_ERR(phy))
		return dev_err_probe(p_phy->dev, PTR_ERR(phy),
				     "Failed to create PHY.\n");

	phy_set_drvdata(phy, p_phy);

	provider = devm_of_phy_provider_register(p_phy->dev, of_phy_simple_xlate);
	if (IS_ERR(provider))
		return dev_err_probe(p_phy->dev, PTR_ERR(provider),
				     "Failed to register PHY provider.\n");

	return 0;
}

static const struct of_device_id sf21a6826_usb_phy_of_match[] = {
	{ .compatible = "siflower,sf21a6826-usb-phy", },
	{ },
};
MODULE_DEVICE_TABLE(of, sf21a6826_usb_phy_of_match);

static struct platform_driver sf21a6826_usb_phy_driver = {
	.probe		= sf21a6826_usb_phy_probe,
	.driver		= {
		.name	= "sf21a6826-usb-phy",
		.of_match_table = sf21a6826_usb_phy_of_match,
	},
};
module_platform_driver(sf21a6826_usb_phy_driver);

MODULE_AUTHOR("Ziying Wu <ziying.wu@siflower.com.cn>");
MODULE_DESCRIPTION("Siflower SF21A6826 USB2.0 PHY driver");
MODULE_LICENSE("GPL");
