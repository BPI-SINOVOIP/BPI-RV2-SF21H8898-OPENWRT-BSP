// SPDX-License-Identifier: GPL-2.0-only

#include <linux/init.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/mfd/syscon.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <dt-bindings/reset/siflower,sf21a6826-reset.h>
#include <dt-bindings/reset/siflower,sf24h8891-reset.h>

#define	SF21A6826_SOFT_RESET		0xC0
#define SF24H8891_SOFT_RESET		0xD0

struct sf_reset_param {
	int (*reset_shift)(unsigned long id);
	u32 soft_reset;
	u32 reset_max;
};

struct sf21a6826_reset_data {
	struct reset_controller_dev	rcdev;
	struct regmap			*regmap;
	const struct sf_reset_param	*param;
};

static inline int sf21a6826_reset_shift(unsigned long id)
{
	switch (id) {
	case SF21A6826_RESET_GIC:
	case SF21A6826_RESET_AXI:
	case SF21A6826_RESET_AHB:
	case SF21A6826_RESET_APB:
	case SF21A6826_RESET_IRAM:
        return id + 1;
	case SF21A6826_RESET_NPU:
	case SF21A6826_RESET_DDR_CTL:
	case SF21A6826_RESET_DDR_PHY:
	case SF21A6826_RESET_DDR_PWR_OK_IN:
	case SF21A6826_RESET_DDR_CTL_APB:
	case SF21A6826_RESET_DDR_PHY_APB:
        return id + 2;
	case SF21A6826_RESET_USB:
        return id + 8;
	case SF21A6826_RESET_PVT:
	case SF21A6826_RESET_SERDES_CSR:
        return id + 11;
	case SF21A6826_RESET_CRYPT_CSR:
	case SF21A6826_RESET_CRYPT_APP:
	case SF21A6826_RESET_NPU2DDR_ASYNCBRIDGE:
	case SF21A6826_RESET_IROM:
		return id + 14;
	default:
		return -EINVAL;
	}
}

static inline int sf24h8891_reset_shift(unsigned long id)
{
	if(id < 0 || id > SF24H8891_RESET_MAX)
		return -EINVAL;

	return id;
}

struct sf_reset_param sf21a6826_reset_param = {
	.reset_shift = sf21a6826_reset_shift,
	.soft_reset = SF21A6826_SOFT_RESET,
	.reset_max = SF21A6826_RESET_MAX,
};

struct sf_reset_param sf24h8891_reset_param = {
	.reset_shift = sf24h8891_reset_shift,
	.soft_reset = SF24H8891_SOFT_RESET,
	.reset_max = SF24H8891_RESET_MAX,
};

static int sf21a6826_reset_assert(struct reset_controller_dev *rcdev,
				  unsigned long id)
{
	struct sf21a6826_reset_data *rd;
	int shift;
	u32 mask;

	rd = container_of(rcdev, struct sf21a6826_reset_data, rcdev);

	shift = rd->param->reset_shift(id);
	mask = BIT(shift);
	return regmap_update_bits(rd->regmap, rd->param->soft_reset,
				mask, 0);
}

static int sf21a6826_reset_deassert(struct reset_controller_dev *rcdev,
				    unsigned long id)
{
	struct sf21a6826_reset_data *rd;
	int shift;
	u32 mask;

	rd = container_of(rcdev, struct sf21a6826_reset_data, rcdev);

	shift = rd->param->reset_shift(id);
	mask = BIT(shift);
	return regmap_update_bits(rd->regmap, rd->param->soft_reset,
				mask, mask);
}

static int sf21a6826_reset_status(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	struct sf21a6826_reset_data *rd;
	int shift, ret;
	u32 mask;
	u32 reg;

	rd = container_of(rcdev, struct sf21a6826_reset_data, rcdev);
	ret = regmap_read(rd->regmap, rd->param->soft_reset, &reg);
	if (ret)
		return ret;

	shift = rd->param->reset_shift(id);
	mask = BIT(shift);
	return !!(reg & mask);
}

static const struct reset_control_ops sf21a6826_reset_ops = {
	.assert		= sf21a6826_reset_assert,
	.deassert	= sf21a6826_reset_deassert,
	.status		= sf21a6826_reset_status,
};

static int sf21a6826_reset_probe(struct platform_device *pdev)
{
	struct sf21a6826_reset_data *rd;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *node;

	rd = devm_kzalloc(dev, sizeof(*rd), GFP_KERNEL);
	if (!rd)
		return -ENOMEM;

	node = of_parse_phandle(np, "siflower,crm", 0);
	rd->regmap = syscon_node_to_regmap(node);

	if (IS_ERR(rd->regmap))
		return PTR_ERR(rd->regmap);

	rd->param = device_get_match_data(&pdev->dev);
	if (!rd->param)
		return -ENODEV;

	rd->rcdev.owner = THIS_MODULE;
	rd->rcdev.nr_resets = rd->param->reset_max + 1;
	rd->rcdev.ops = &sf21a6826_reset_ops;
	rd->rcdev.of_node = np;

	return devm_reset_controller_register(dev, &rd->rcdev);
}

static const struct of_device_id sf21a6826_reset_dt_ids[] = {
	{
		.compatible = "siflower,sf21a6826-reset",
		.data = &sf21a6826_reset_param,
	},
	{
		.compatible = "siflower,sf24h8891-reset",
		.data = &sf24h8891_reset_param,
	},
	{ /* sentinel */ }
};

static struct platform_driver sf21a6826_reset_driver = {
	.probe	= sf21a6826_reset_probe,
	.driver = {
		.name		= "sf21a6826-reset",
		.of_match_table	= sf21a6826_reset_dt_ids,
	},
};
builtin_platform_driver(sf21a6826_reset_driver);
