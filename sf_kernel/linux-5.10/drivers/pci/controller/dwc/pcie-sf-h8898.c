// SPDX-License-Identifier: GPL-2.0
//
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/reset.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include "../../../base/base.h"

#include "pcie-designware.h"

#define SF_PCIE_MAX_TIMEOUT	10000

#define AHB_SYSM_OFFSET		0x10000

#define AHB_SYSM_REG(n)		(AHB_SYSM_OFFSET + 0x4 * n)
#define CFG_PIPE_RSTN		1

#define ELBI_REG(n)			(0x4 * n)
#define APP_LTSSM_ENABLE	23

#define TOPSYS_RSTN_CFG		0xC0
#define SERDES_CSR_SW_RST_N	24

#define TOPSYS_LVDS_CFG		0xE8

#define TOPSYS_LVDS_CFG1	0x120

#define BIAS_EN			20
#define RXEN			3
#define TXEN			2

#define to_sf_pcie(x)	dev_get_drvdata((x)->dev)

static bool phy_init_done = false;
static DEFINE_MUTEX(pcie_phy_lock);
int volt[3] = { 33, 0, 28 };

enum pcie_device_type {
	PCIE_EP = 0,
	PCIE_RC = 4,
	PCIE_UNKNOW,
};

enum pcie_lane_mode {
	PCIE0_LANE0_PCIE1_LANE1,
	PCIE0_LANE0_LANE1,
	PCIE1_LANE0_PCIE0_LANE1,
	PCIE1_LANE0_LANE1,
};

struct sf_pcie;

struct sf_pcie_ops {
	int (*init)(struct sf_pcie *sf_pcie);
	void (*deinit)(struct sf_pcie *sf_pcie);
	void (*ltssm_enable)(struct sf_pcie *sf_pcie);
};

struct sf_pcie {
	struct dw_pcie			*pci;
	void __iomem			*elbi;
	void __iomem			*iatu;
	void __iomem			*dma;
	struct clk			*csr_clk;
	struct clk			*ref_clk;
	struct clk			*phy_clk;
	const struct sf_pcie_ops	*pcie_ops;
	struct regmap			*topsys;
	struct regmap			*ahbsys;
	struct irq_domain		*irq_domain;
	u64				irq_enable_bits;
	raw_spinlock_t			irq_enable_lock;
	raw_spinlock_t			irq_mask_lock;
	u32				ctrl_id;
	u32				reset_ms;
	struct gpio_desc		*reset_gpio;
	bool				link_state;
	enum pcie_device_type		device_type;
	enum pcie_lane_mode		lane_mode;
	u32				firmware_bypass;
};



void sf_pcie_phy0_cr_write(struct sf_pcie *sf_pcie, u32 data, u32 addr)
{
	u32 val, check = 0, count = 0;
	val = ((0xffff & addr) << 16) | (0xffff & data);
	regmap_write(sf_pcie->ahbsys, AHB_SYSM_REG(0xB), val);
	usleep_range(1, 1);
	while(count<0x10){
		regmap_read(sf_pcie->ahbsys, AHB_SYSM_REG(0xC), &check);
		check &= 0x1;
		if(check)
			break;
		count++;
		usleep_range(1, 1);
	}
}

static u32 sf_pcie_phy0_cr_read(struct sf_pcie *sf_pcie, u32 addr)
{
	u32 val, check = 0, count = 0;
	regmap_write(sf_pcie->ahbsys, AHB_SYSM_REG(0xD), addr);
	usleep_range(1, 1);
	while (count < 0x10) {
		regmap_read(sf_pcie->ahbsys, AHB_SYSM_REG(0xE), &check);
		check &= 0x1 << 16;
		if (check)
			break;
		count++;
		usleep_range(1, 1);
	}
	regmap_read(sf_pcie->ahbsys, AHB_SYSM_REG(0xE), &val);
	val &= 0xFFFF;
	return val;
}

void sf_pcie_phy1_cr_write(struct sf_pcie *sf_pcie, u32 data, u32 addr)
{
	u32 val, check = 0,count=0;
	val = ((0xffff & addr) << 16) | (0xffff & data);
	regmap_write(sf_pcie->ahbsys, AHB_SYSM_REG(0xF), val);
	usleep_range(1, 1);
	while(count<0x10){
		regmap_read(sf_pcie->ahbsys, AHB_SYSM_REG(0x10), &check);
		check &= 0x1;
		if(check)
			break;
		count++;
		usleep_range(1, 1);
	}
}

static u32 sf_pcie_phy1_cr_read(struct sf_pcie *sf_pcie, u32 addr)
{
	u32 val, check = 0, count = 0;
	regmap_write(sf_pcie->ahbsys, AHB_SYSM_REG(0x11), addr);
	usleep_range(1, 1);
	while (count < 0x10) {
		regmap_read(sf_pcie->ahbsys, AHB_SYSM_REG(0x12), &check);
		check &= 0x1 << 16;

		if (check)
			break;
		count++;
		usleep_range(1, 1);
	}
	regmap_read(sf_pcie->ahbsys, AHB_SYSM_REG(0x12), &val);
	val &= 0xFFFF;
	return val;
}

static int sf_pcie_write_serdes_reg(struct sf_pcie *sf_pcie, int phy_id,
				    u32 waddr, u32 wdata)
{
	u32 rdata, count;
	rdata = 0;
	count = 0;
	if (phy_id == 0) {
		sf_pcie_phy0_cr_write(sf_pcie, wdata, waddr);
		rdata = sf_pcie_phy0_cr_read(sf_pcie, waddr);

	} else {
		sf_pcie_phy1_cr_write(sf_pcie, wdata, waddr);
		rdata = sf_pcie_phy1_cr_read(sf_pcie, waddr);
	}
	while ((rdata != wdata) && (count < 10)) {
		if (phy_id == 0) {
			sf_pcie_phy0_cr_write(sf_pcie, wdata, waddr);
			rdata = sf_pcie_phy0_cr_read(sf_pcie, waddr);
		} else {
			sf_pcie_phy1_cr_write(sf_pcie, wdata, waddr);
			rdata = sf_pcie_phy1_cr_read(sf_pcie, waddr);
		}
		count++;
	}
	if (rdata != wdata) {
		struct device *dev = sf_pcie->pci->dev;
		dev_err(dev, "=====read as 0x%x try write as 0x%x phy%d write error======\n",
		       rdata, wdata, phy_id);
		return -EIO;
	}
	return 0;
}

static void sf_pcie_change_pipe1_clk_sel_to_0(struct sf_pcie *sf_pcie)
{
	u32 val;
	regmap_read(sf_pcie->ahbsys, AHB_SYSM_REG(0x5), &val);
	val = val & 0xfff3ffff;
	regmap_write(sf_pcie->ahbsys, AHB_SYSM_REG(0x5), val);
}

static void sf_pcie_enable_dbi_ro_wr_en(struct sf_pcie *sf_pcie)
{
	u32 val;
	val = readl(sf_pcie->pci->dbi_base + 0x8bc);
	val = val | 0x1 ;
	msleep(10);
	writel(val, sf_pcie->pci->dbi_base + 0x8bc);
	val = readl(sf_pcie->pci->dbi_base + 0x8bc);
	msleep(20);
}

static void sf_pcie_enable_part_lanes_rxei_exit(struct sf_pcie *sf_pcie)
{
	u32 val;
	val = readl(sf_pcie->pci->dbi_base + 0x708);
	val = val | 0x1 << 22;
	writel(val, sf_pcie->pci->dbi_base + 0x708);
	val = readl(sf_pcie->pci->dbi_base + 0x708);
	msleep(20);
}

static void sf_pcie_enable_speed_change(struct sf_pcie *sf_pcie)
{
	u32 val;
	val = readl(sf_pcie->pci->dbi_base + 0x80c);
	val = val | 0x1 << 17;
	writel(val, sf_pcie->pci->dbi_base + 0x80c);
	val = readl(sf_pcie->pci->dbi_base + 0x80c);
	msleep(20);
}

static void sf_pcie_assert_pipe_reset(struct sf_pcie *sf_pcie)
{
	u32 mask;
	switch (sf_pcie->lane_mode) {
	case PCIE0_LANE0_PCIE1_LANE1:
		mask = (sf_pcie->ctrl_id ? 0x2 : 0x1) << CFG_PIPE_RSTN;
		break;
	case PCIE1_LANE0_PCIE0_LANE1:
		mask = (sf_pcie->ctrl_id ? 0x1 : 0x2) << CFG_PIPE_RSTN;
		break;
	case PCIE0_LANE0_LANE1:
	case PCIE1_LANE0_LANE1:
	default:
		mask = 0x3 << CFG_PIPE_RSTN;
		break;
	}
	regmap_clear_bits(sf_pcie->ahbsys, AHB_SYSM_REG(1), mask);
}

static void sf_pcie_deassert_pipe_reset(struct sf_pcie *sf_pcie)
{
	u32 mask;
	switch (sf_pcie->lane_mode) {
	case PCIE0_LANE0_PCIE1_LANE1:
		mask = (sf_pcie->ctrl_id ? 0x2 : 0x1) << CFG_PIPE_RSTN;
		break;
	case PCIE1_LANE0_PCIE0_LANE1:
		mask = (sf_pcie->ctrl_id ? 0x1 : 0x2) << CFG_PIPE_RSTN;
		break;
	case PCIE0_LANE0_LANE1:
	case PCIE1_LANE0_LANE1:
	default:
		mask = 0x3 << CFG_PIPE_RSTN;
		break;
	}
	regmap_set_bits(sf_pcie->ahbsys, AHB_SYSM_REG(1), mask);
}

static void sf_pcie_assert_core_reset(struct sf_pcie *sf_pcie)
{
	u32 mask;
	mask = sf_pcie->ctrl_id ? GENMASK(8,6) : GENMASK(5,3);
	regmap_clear_bits(sf_pcie->ahbsys, AHB_SYSM_REG(1), mask);
}

static void sf_pcie_deassert_core_reset(struct sf_pcie *sf_pcie)
{
	u32 mask;

	mask = sf_pcie->ctrl_id ? GENMASK(8,6) : GENMASK(5,3);
	regmap_set_bits(sf_pcie->ahbsys, AHB_SYSM_REG(1), mask);
}

static void sf_pcie_configure_device_type(struct sf_pcie *sf_pcie)
{
	if (sf_pcie->ctrl_id) {
		regmap_update_bits(sf_pcie->ahbsys, AHB_SYSM_REG(0), GENMASK(7,4), FIELD_PREP(GENMASK(7,4), sf_pcie->device_type));
	} else {
		regmap_update_bits(sf_pcie->ahbsys, AHB_SYSM_REG(0), GENMASK(3,0), FIELD_PREP(GENMASK(3,0), sf_pcie->device_type));
	}
}

static void sf_pcie_configure_lane_mode(struct sf_pcie *sf_pcie)
{
	regmap_update_bits(sf_pcie->ahbsys, AHB_SYSM_REG(0), GENMASK(9, 8),
			   FIELD_PREP(GENMASK(9, 8), sf_pcie->lane_mode));
}

static void sf_pcie_init_legacy_interrupt(struct sf_pcie *sf_pcie)
{
	/* disable all interrupts by default. they will be enabled later */
	writel_relaxed(0, sf_pcie->elbi + ELBI_REG(38));
	writel_relaxed(0, sf_pcie->elbi + ELBI_REG(39));
	writel_relaxed(0, sf_pcie->elbi + ELBI_REG(40));
	writel_relaxed(0, sf_pcie->elbi + ELBI_REG(41));
}

static void sf_pcie_ltssm_enable(struct sf_pcie *sf_pcie)
{
	u32 val;
	val = readl(sf_pcie->elbi + ELBI_REG(0));
	val |= 0x1 << APP_LTSSM_ENABLE;
	writel(val, sf_pcie->elbi + ELBI_REG(0));
}

static int sf_pcie_establish_link(struct sf_pcie *sf_pcie)
{
	struct dw_pcie *pci = sf_pcie->pci;

	if (dw_pcie_link_up(pci))
		return 0;

	if (sf_pcie->pcie_ops->ltssm_enable)
		sf_pcie->pcie_ops->ltssm_enable(sf_pcie);

	return dw_pcie_wait_for_link(pci);
}

static int sf_pcie_clk_enable(struct sf_pcie *sf_pcie)
{
	int ret;
	ret = clk_prepare_enable(sf_pcie->csr_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sf_pcie->ref_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sf_pcie->phy_clk);
	if (ret)
		return ret;
	return 0;
}

static void sf_pcie_clk_disable(struct sf_pcie *sf_pcie)
{
	clk_disable_unprepare(sf_pcie->csr_clk);
	clk_disable_unprepare(sf_pcie->ref_clk);
	clk_disable_unprepare(sf_pcie->phy_clk);
}

static int sf_pcie_lvds_enable(struct sf_pcie *sf_pcie)
{
	int ret = 0;
	ret = regmap_set_bits(sf_pcie->topsys, TOPSYS_LVDS_CFG,
			      BIT(TXEN) | BIT(BIAS_EN));
	if (ret) {
		return ret;
	}

	ret = regmap_set_bits(sf_pcie->topsys, TOPSYS_LVDS_CFG1,
			      BIT(TXEN) | BIT(BIAS_EN));
	if (ret) {
		return ret;
	}
	return 0;
}

static int sf_pcie_lvds_disable(struct sf_pcie *sf_pcie)
{
	int ret = 0;
	ret = regmap_clear_bits(sf_pcie->topsys, TOPSYS_LVDS_CFG,
				BIT(TXEN) | BIT(BIAS_EN));
	if (ret) {
		return ret;
	}
	ret = regmap_clear_bits(sf_pcie->topsys, TOPSYS_LVDS_CFG1,
				BIT(TXEN) | BIT(BIAS_EN));
	if (ret) {
		return ret;
	}
	return 0;
}

static void sf_pcie_assert_phy_reset(struct sf_pcie *sf_pcie)
{
	regmap_clear_bits(sf_pcie->ahbsys, AHB_SYSM_REG(1), 0x1);
}

static void sf_pcie_deassert_phy_reset(struct sf_pcie *sf_pcie)
{
	regmap_set_bits(sf_pcie->ahbsys, AHB_SYSM_REG(1), 0x1);
}

static int sf_pcie_phy_init(struct sf_pcie *sf_pcie)
{
	if (phy_init_done) {
		dev_info(sf_pcie->pci->dev, "Phy already init. So return.\n");
		return 0;
	}
	/*
	 * Configure pcie lane mode
	 * */
	sf_pcie_configure_lane_mode(sf_pcie);

	regmap_write(sf_pcie->ahbsys, 0x10014, 0x40000);

	/*
	 * deassert phy reset
	 * */
	sf_pcie_deassert_phy_reset(sf_pcie);

	return 0;
}

/*
 * The bus interconnect subtracts address offset from the request
 * before sending it to PCIE slave port. Since DT puts config space
 * at the beginning, we can obtain the address offset from there and
 * subtract it.
 */
static u64 sf_pcie_cpu_addr_fixup(struct dw_pcie *pci, u64 cpu_addr)
{
	struct pcie_port *pp = &pci->pp;

	return cpu_addr - pp->cfg0_base;
}

static int sf_pcie_init(struct sf_pcie *sf_pcie)
{
	int ret;

	ret = sf_pcie_lvds_enable(sf_pcie);
	if (ret) {
		dev_err(sf_pcie->pci->dev, "lvds enable failed.\n");
		return ret;
	}

	ret = sf_pcie_clk_enable(sf_pcie);
	if (ret) {
		dev_err(sf_pcie->pci->dev, "clk enbale failed.\n");
		return ret;
	}

	/*
	 * config device type
	 * */
	sf_pcie_configure_device_type(sf_pcie);

	/*
	 * init phy
	 * */
	mutex_lock(&pcie_phy_lock);
	ret = sf_pcie_phy_init(sf_pcie);
	mutex_unlock(&pcie_phy_lock);
	if (ret)
		return ret;

	return 0;
}

static void sf_pcie_deinit(struct sf_pcie *sf_pcie)
{
	sf_pcie_assert_phy_reset(sf_pcie);
	sf_pcie_assert_pipe_reset(sf_pcie);
	sf_pcie_assert_core_reset(sf_pcie);
	sf_pcie_lvds_disable(sf_pcie);
	sf_pcie_clk_disable(sf_pcie);
}

static int sf_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct sf_pcie *sf_pcie = to_sf_pcie(pci);
	int ret;

	if (sf_pcie->pcie_ops->init) {
		ret = sf_pcie->pcie_ops->init(sf_pcie);
		if (ret)
			return ret;
	} else {
		dev_warn(pci->dev, "No init function.\n");
	}

	sf_pcie_deassert_pipe_reset(sf_pcie);
	sf_pcie_deassert_core_reset(sf_pcie);

	if (sf_pcie->ctrl_id) {
		/*
		 * Fixed vendor ID and device ID of
		 * siflower rc controller 1
		 * */
		u32 val;
		val = readl(sf_pcie->pci->dbi_base + 0x8bc);
		val |= 1;
		writel(val, sf_pcie->pci->dbi_base + 0x8bc);
		writel(0x78781a0a, sf_pcie->pci->dbi_base);
	}

	dw_pcie_setup_rc(pp);
	dw_pcie_msi_init(pp);

	sf_pcie_enable_dbi_ro_wr_en(sf_pcie);
	sf_pcie_enable_part_lanes_rxei_exit(sf_pcie);
	sf_pcie_change_pipe1_clk_sel_to_0(sf_pcie);

	/*
	 * reset ep device by reset_gpio
	 * Note: The direction speed change maybe occur when we
	 * hold the reset gpio. So we should set the field
	 * DIRECTION_SPEED_CHANGE of GEN2_CTRL_OFF register after
	 * release reset gpio.
	 * */
	if (sf_pcie->reset_gpio) {
		gpiod_set_value_cansleep(sf_pcie->reset_gpio, 1);
		msleep(sf_pcie->reset_ms);
		gpiod_set_value_cansleep(sf_pcie->reset_gpio, 0);
		msleep(10);
	}

	sf_pcie_init_legacy_interrupt(sf_pcie);

	/*
	 * before link up with GEN1, we should config the field
	 * DIRECTION_SPEED_CHANGE of GEN2_CTRL_OFF register to insure
	 * the LTSSM to initiate a speed change to Gen2 or Gen3 after
	 * the link is initialized at Gen1 speed.
	 * */
	sf_pcie_enable_speed_change(sf_pcie);

	ret = sf_pcie_establish_link(sf_pcie);
	if (ret)
		return ret;

	return 0;
}

static const struct dw_pcie_host_ops sf_pcie_host_ops = {
	.host_init = sf_pcie_host_init,
};

enum sf_pcie_legacy_int_0_12 {
	CFG_PME_INT = 0,
	CFG_AER_RC_ERR_MSI,
	CFG_AER_RC_ERR_INT,
	CFG_SYS_ERR_RC,
	RADM_UPDATE_FLAG_0 = 4,
	RADM_UPDATE_FLAG_1,
	RADM_UPDATE_FLAG_2,
	RADM_UPDATE_FLAG_3,
	RADM_UPDATE_FLAG_4,
	RADM_UPDATE_FLAG_5,
	RADM_UPDATE_FLAG_6,
	RADM_UPDATE_FLAG_7
};

enum sf_pcie_legacy_int_13_44 {
	DEASSET_INTD_GRT = 0,
	DEASSET_INTC_GRT,
	DEASSET_INTB_GRT,
	DEASSET_INTA_GRT,
	ASSET_INTD_GRT,
	ASSET_INTC_GRT,
	ASSET_INTB_GRT,
	ASSET_INTA_GRT = 7,
	EDMA_INT_0 = 8,
	EDMA_INT_1,
	EDMA_INT_2,
	EDMA_INT_3,
	EDMA_INT_4,
	EDMA_INT_5,
	EDMA_INT_6,
	EDMA_INT_7,
	CFG_BW_MGT_MSI = 16,
	CFG_BW_MGT_INT,
	CFG_LINK_AUTO_BW_MGT_MSI,
	CFG_LINK_AUTO_BW_MGT_INT,
	RADM_DEASSET_INTD_GRT,
	RADM_DEASSET_INTC_GRT,
	RADM_DEASSET_INTB_GRT,
	RADM_DEASSET_INTA_GRT,
	RADM_ASSET_INTD_GRT,
	RADM_ASSET_INTC_GRT,
	RADM_ASSET_INTB_GRT,
	RADM_ASSET_INTA_GRT,
	HP_MSI,
	HP_INT,
	HP_PME,
	CFG_PME_MSI


};
#define SF_PCIE_NR_IRQS		45

static void sf_pcie_int_handler(struct irq_desc *desc)
{
	struct sf_pcie *sf_pcie = irq_desc_get_handler_data(desc);
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
	struct irq_domain *irq_domain = sf_pcie->irq_domain;
	u32 int_sta1, int_sta2;
	u64 status;

	int_sta1 = readl_relaxed(sf_pcie->elbi + ELBI_REG(44));
	int_sta2 = readl_relaxed(sf_pcie->elbi + ELBI_REG(45));

	/* Get the interrupt status from both status registers.
	 * Note that a status bit may be set even if it's not enabled, and the
	 * second register does not only contain interrupt status, so mask them
	 * with irq_enable_bits.
	 */
	status = int_sta1;
	status |= (u64)int_sta2 << 32;
	status &= sf_pcie->irq_enable_bits;

	if (unlikely(!status))
		return;

	chained_irq_enter(irqchip, desc);

	do {
		irq_hw_number_t hwirq;
		unsigned int irq;

		/* Iterate through every set bit of status */
		hwirq = __ffs64(status);
		status &= ~BIT_ULL(hwirq);
		irq = irq_linear_revmap(irq_domain, hwirq);
		generic_handle_irq(irq);
	} while (status);

	chained_irq_exit(irqchip, desc);
}

static void sf_pcie_irq_enable(struct irq_data *data)
{
	struct sf_pcie *sf_pcie = irq_data_get_irq_chip_data(data);
	irq_hw_number_t hwirq = data->hwirq;
	u32 val;

	raw_spin_lock(&sf_pcie->irq_enable_lock);
	sf_pcie->irq_enable_bits |= BIT_ULL(hwirq);
	val = sf_pcie->irq_enable_bits >> ALIGN_DOWN(hwirq, 32);
	writel_relaxed(val, sf_pcie->elbi + ELBI_REG(38 + hwirq / 32));
	raw_spin_unlock(&sf_pcie->irq_enable_lock);
}

static void sf_pcie_irq_disable(struct irq_data *data)
{
	struct sf_pcie *sf_pcie = irq_data_get_irq_chip_data(data);
	irq_hw_number_t hwirq = data->hwirq;
	u32 val;

	raw_spin_lock(&sf_pcie->irq_enable_lock);
	sf_pcie->irq_enable_bits &= ~BIT_ULL(hwirq);
	val = sf_pcie->irq_enable_bits >> ALIGN_DOWN(hwirq, 32);
	writel_relaxed(val, sf_pcie->elbi + ELBI_REG(38 + hwirq / 32));
	raw_spin_unlock(&sf_pcie->irq_enable_lock);
}

static void sf_pcie_irq_mask(struct irq_data *data)
{
	struct sf_pcie *sf_pcie = irq_data_get_irq_chip_data(data);
	irq_hw_number_t hwirq = data->hwirq;
	u32 val;

	raw_spin_lock(&sf_pcie->irq_mask_lock);
	val = readl_relaxed(sf_pcie->elbi + ELBI_REG(40 + hwirq / 32));
	val |= BIT(hwirq % 32);
	writel_relaxed(val, sf_pcie->elbi + ELBI_REG(40 + hwirq / 32));
	raw_spin_unlock(&sf_pcie->irq_mask_lock);
}

static void sf_pcie_irq_unmask(struct irq_data *data)
{
	struct sf_pcie *sf_pcie = irq_data_get_irq_chip_data(data);
	irq_hw_number_t hwirq = data->hwirq;
	u32 val;

	raw_spin_lock(&sf_pcie->irq_mask_lock);
	val = readl_relaxed(sf_pcie->elbi + ELBI_REG(40 + hwirq / 32));
	val &= ~BIT(hwirq % 32);
	writel_relaxed(val, sf_pcie->elbi + ELBI_REG(40 + hwirq / 32));
	raw_spin_unlock(&sf_pcie->irq_mask_lock);
}

static void sf_pcie_irq_ack(struct irq_data *data)
{
	struct sf_pcie *sf_pcie = irq_data_get_irq_chip_data(data);
	irq_hw_number_t hwirq = data->hwirq;

	writel_relaxed(BIT(hwirq % 32), sf_pcie->elbi + ELBI_REG(42 + hwirq / 32));
	writel_relaxed(0, sf_pcie->elbi + ELBI_REG(42 + hwirq / 32));
}

static int sf_pcie_irq_set_affinity(struct irq_data *data, const struct cpumask *dest,
				    bool force)
{
	struct sf_pcie *sf_pcie = irq_data_get_irq_chip_data(data);

	return __irq_set_affinity(sf_pcie->pci->pp.irq, dest, force);
}

static struct irq_chip sf_pcie_irq_chip = {
	.name			= "Siflower PCI",
	.irq_enable		= sf_pcie_irq_enable,
	.irq_disable		= sf_pcie_irq_disable,
	.irq_ack		= sf_pcie_irq_ack,
	.irq_mask		= sf_pcie_irq_mask,
	.irq_unmask		= sf_pcie_irq_unmask,
	.irq_set_affinity	= sf_pcie_irq_set_affinity,
};

static int sf_pcie_irq_map(struct irq_domain *domain, unsigned int irq,
			    irq_hw_number_t hwirq)
{
	irq_set_chip_data(irq, domain->host_data);
	irq_set_chip_and_handler(irq, &sf_pcie_irq_chip, handle_edge_irq);
	return 0;
}

static const struct irq_domain_ops sf_pcie_domain_ops = {
	.map = sf_pcie_irq_map,
};

static int sf_add_pcie_port(struct sf_pcie *sf_pcie,
			    struct platform_device *pdev)
{
	struct dw_pcie *pci = sf_pcie->pci;
	struct pcie_port *pp = &pci->pp;
	struct device *dev = &pdev->dev;
	struct device_node *np;
	int ret;

	pp->irq = platform_get_irq_byname(pdev, "intr");
	if (pp->irq < 0)
		return pp->irq;

	for_each_child_of_node(dev->of_node, np) {
		if (of_property_read_bool(np, "interrupt-controller"))
			break;
	}
	if (np) {
		sf_pcie->irq_domain = irq_domain_add_linear(np, SF_PCIE_NR_IRQS,
							    &sf_pcie_domain_ops, sf_pcie);
		of_node_put(np);
		np = NULL;
		if (!sf_pcie->irq_domain)
			return -ENOMEM;

		irq_set_chained_handler_and_data(pp->irq, sf_pcie_int_handler,
						 sf_pcie);
	}

	pp->ops = &sf_pcie_host_ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "failed to initialize host\n");
		if (sf_pcie->irq_domain)
			irq_domain_remove(sf_pcie->irq_domain);
		return ret;
	}

	return 0;
}

static int sf_pcie_link_up(struct dw_pcie *pci)
{
	struct sf_pcie *pcie = to_sf_pcie(pci);
	u32 rdlh_link_up, smlh_link_up;

	rdlh_link_up = (readl(pcie->elbi + ELBI_REG(25)) >> 20) & 0x1;
	smlh_link_up = (readl(pcie->elbi + ELBI_REG(30)) >> 7) & 0x1;

	if (rdlh_link_up && smlh_link_up) {
		pcie->link_state = true;
		return 1;
	}

	/*
	 * nerver link up
	 * */
	pcie->link_state = false;
	return 0;
}

static int sf_pcie_retry_link(struct platform_device *pdev);

static const struct dw_pcie_ops dw_pcie_ops = {
	.cpu_addr_fixup = sf_pcie_cpu_addr_fixup,
	.link_up = sf_pcie_link_up,
};

static const struct sf_pcie_ops sf_pcie_ops = {
	.init = sf_pcie_init,
	.deinit = sf_pcie_deinit,
	.ltssm_enable = sf_pcie_ltssm_enable,
};


static int cal_tx_swing(struct sf_pcie *sf_pcie, int pcs, int tx)
{
	int addr = 22, tx_swing = 0;
	if (pcs == 1)
		addr = 23;
	regmap_read(sf_pcie->ahbsys, AHB_SYSM_REG(addr), &tx_swing);
	if (tx == 0) {
		tx_swing = tx_swing & 0x7F;
	} else if (tx == 1) {
		tx_swing = (tx_swing >> 7) & 0x7F;
	}
	tx_swing = 1800 * (tx_swing + 1) / 128;
	return tx_swing;
}

static ssize_t set_tx_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sf_pcie *sf_pcie = platform_get_drvdata(pdev);
	int tx = 0, rx = 0, tx_swing[2], gen = 0, db = 0;
	tx = (readl(sf_pcie->pci->dbi_base + 0x80C) >> 18) & 0x1;
	regmap_read(sf_pcie->ahbsys, AHB_SYSM_REG(33), &rx);
	tx_swing[0] = cal_tx_swing(sf_pcie, 0, tx);
	tx_swing[1] = cal_tx_swing(sf_pcie, 1, tx);
	gen = sf_pcie->pci->link_gen;
	//if (gen == 2)
	db = readl(sf_pcie->pci->dbi_base + 0xA0) & BIT(16);

	return scnprintf(
		buf, PAGE_SIZE,
		"PCIe%d tx setting is %s\nphy0 tx_swing = %dmV tx_deemphasis = %sdB rx_eq = %d\nphy1 tx_swing = %dmV tx_deemphasis = %sdB rx_eq = %d\n",
		sf_pcie->ctrl_id, tx ? "Low Swing" : "Full Swing", tx_swing[0],
		db ? "-3.5" : "-6", rx & 0x7, tx_swing[1], db ? "-3.5" : "-6",
		(rx >> 3) & 0x7);
}

static void setting_more(struct sf_pcie *sf_pcie, int pcs, int val, int width,
			 int offset)
{
	int mask, addr;
	width == 7 ? (mask = 0x7F) : (mask = 0x3F);
	pcs ? (addr = 23) : (addr = 22);
	val = (val & mask) << offset;
	regmap_clear_bits(sf_pcie->ahbsys, AHB_SYSM_REG(addr),
			  (mask << offset));
	regmap_set_bits(sf_pcie->ahbsys, AHB_SYSM_REG(addr), val);
	return;
}

static ssize_t set_tx_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sf_pcie *sf_pcie = platform_get_drvdata(pdev);
	int pcs, gen, db, tx, val2, tmp = 0;
	sscanf(buf, "%d.%d.%d.0x%x", &pcs, &db, &tx, &val2);
	gen = sf_pcie->pci->link_gen;

	if ((pcs != 0 && pcs != 1) || (tx != 1 && tx != 0) ||
	    (db != 0 && db != 1)) {
		return -EINVAL;
	} else {
		tmp = readl(sf_pcie->pci->dbi_base + 0x80C);
		tmp = (tmp & ~BIT(18)) | (tx << 18);
		writel(tmp, sf_pcie->pci->dbi_base + 0x80C);
		if (tx == 1)
			setting_more(sf_pcie, pcs, val2, 7, 7);
		else if (tx == 0)
			setting_more(sf_pcie, pcs, val2, 7, 0);

		tmp = readl(sf_pcie->pci->dbi_base + 0x80C);
		tmp = (tmp & ~BIT(22)) | BIT(22);
		writel(tmp, sf_pcie->pci->dbi_base + 0x80C);
		tmp = readl(sf_pcie->pci->dbi_base + 0xA0);
		tmp = (tmp & ~BIT(6)) | ((db & 0x1) << 6);
		writel(tmp, sf_pcie->pci->dbi_base + 0xA0);

		tmp = readl(sf_pcie->elbi + ELBI_REG(32)) | BIT(5);
		writel(tmp, sf_pcie->elbi + ELBI_REG(32));

		sf_pcie_retry_link(pdev);
	}

	return count;
}
static DEVICE_ATTR_RW(set_tx);

static ssize_t set_prbs_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sf_pcie *sf_pcie = platform_get_drvdata(pdev);
	int val1 = 0, val2 = 0;
	val1 = sf_pcie_phy0_cr_read(sf_pcie, 0x1015);
	val2 = sf_pcie_phy1_cr_read(sf_pcie, 0x1015);
	if (val1 == val2)
		return scnprintf(buf, PAGE_SIZE, "now prbs = %d\n", val1);
	else
		return scnprintf(buf, PAGE_SIZE, "prbs setting error!!!\n");
}

static ssize_t set_prbs_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sf_pcie *sf_pcie = platform_get_drvdata(pdev);
	int prbs, ret = 0;
	ret = kstrtoint(buf, 0, &prbs);
	if (ret)
		return ret;

	if (prbs >= 1 && prbs <= 8) {
		sf_pcie_deassert_phy_reset(sf_pcie);
		msleep(10);
		ret = sf_pcie_write_serdes_reg(sf_pcie, 0, 0x1001, 0x5);
		if (ret)
			return ret;
		ret = sf_pcie_write_serdes_reg(sf_pcie, 1, 0x1001, 0x5);
		if (ret)
			return ret;
		ret = sf_pcie_write_serdes_reg(sf_pcie, 0, 0x1015, prbs);
		if (ret)
			return ret;
		ret = sf_pcie_write_serdes_reg(sf_pcie, 1, 0x1015, prbs);
		if (ret)
			return ret;
		return count;
	}

	return -EINVAL;
}
static DEVICE_ATTR_RW(set_prbs);

static ssize_t relink_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	unsigned long val;

	if (kstrtoul(buf, 0, &val) < 0)
		return -EINVAL;

	if (val) {
		sf_pcie_retry_link(pdev);
	}
	return count;
}
static DEVICE_ATTR_WO(relink);

static int sf_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct dw_pcie *pci;
	struct sf_pcie *sf_pcie;
	struct resource *res;
	int ret;

	sf_pcie = devm_kzalloc(dev, sizeof(*sf_pcie), GFP_KERNEL);
	if (!sf_pcie)
		return -ENOMEM;

	raw_spin_lock_init(&sf_pcie->irq_enable_lock);
	raw_spin_lock_init(&sf_pcie->irq_mask_lock);
	pci = devm_kzalloc(dev, sizeof(*pci), GFP_KERNEL);
	if (!pci)
		return -ENOMEM;

	pci->dev = dev;
	pci->ops = &dw_pcie_ops;

	sf_pcie->pci = pci;
	sf_pcie->pcie_ops = &sf_pcie_ops;

	platform_set_drvdata(pdev, sf_pcie);

	sf_pcie->csr_clk = devm_clk_get(&pdev->dev, "csr");
	if (IS_ERR(sf_pcie->csr_clk))
		return PTR_ERR(sf_pcie->csr_clk);

	sf_pcie->ref_clk = devm_clk_get(&pdev->dev, "ref");
	if (IS_ERR(sf_pcie->ref_clk))
		return PTR_ERR(sf_pcie->ref_clk);

	sf_pcie->phy_clk = devm_clk_get(&pdev->dev, "phy");
	if (IS_ERR(sf_pcie->phy_clk))
		return PTR_ERR(sf_pcie->phy_clk);

	sf_pcie->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						      GPIOD_OUT_HIGH);
	if (IS_ERR(sf_pcie->reset_gpio)) {
		return dev_err_probe(dev, PTR_ERR(sf_pcie->reset_gpio),
				     "unable to get reset gpio\n");
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
	pci->dbi_base = devm_pci_remap_cfg_resource(dev, res);
	if (IS_ERR(pci->dbi_base)) {
		return PTR_ERR(pci->dbi_base);
	}

	sf_pcie->topsys = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						       "topsys");
	if (IS_ERR(sf_pcie->topsys))
		return PTR_ERR(sf_pcie->topsys);

	sf_pcie->ahbsys = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
						       "ahbsys");
	if (IS_ERR(sf_pcie->ahbsys))
		return PTR_ERR(sf_pcie->ahbsys);

	sf_pcie->elbi = devm_platform_ioremap_resource_byname(pdev, "elbi");
	if (IS_ERR(sf_pcie->elbi)) {
		return PTR_ERR(sf_pcie->elbi);
	}

	pci->atu_base = devm_platform_ioremap_resource_byname(pdev, "iatu");
	if (IS_ERR(pci->atu_base)) {
		return PTR_ERR(pci->atu_base);
	}

	sf_pcie->dma = devm_platform_ioremap_resource_byname(pdev, "dma");
	if (IS_ERR(sf_pcie->dma)) {
		return PTR_ERR(sf_pcie->dma);
	}

	ret = of_property_read_u32(node, "ctrl-id", &sf_pcie->ctrl_id);
	if (ret) {
		/* default use pcie0 */
		sf_pcie->ctrl_id = 0;
	}

	ret = of_property_read_u32(node, "reset-ms", &sf_pcie->reset_ms);
	if (ret) {
		/* default halt reset: 100ms */
		sf_pcie->reset_ms = 100;
	}

	ret = of_property_read_u32(node, "lane-mode", &sf_pcie->lane_mode);
	if (ret) {
		/* default use PCIE0_LANE0_PCIE1_LANE1 */
		dev_err(dev, "Use default PCIE0_LANE0_PCIE1_LANE1 lane mode.\n");
		sf_pcie->lane_mode = PCIE0_LANE0_PCIE1_LANE1;
	}
	sf_pcie->device_type = PCIE_RC;
	sf_pcie->link_state = false;

	ret = device_create_file(&pdev->dev, &dev_attr_relink);
	ret |= device_create_file(&pdev->dev, &dev_attr_set_tx);
	ret |= device_create_file(&pdev->dev, &dev_attr_set_prbs);
	if (ret)
		return ret;

	ret = sf_add_pcie_port(sf_pcie, pdev);
	if (ret)
		goto remove_file;

	return 0;

remove_file:
	device_remove_file(&pdev->dev, &dev_attr_relink);
	device_remove_file(&pdev->dev, &dev_attr_set_tx);
	device_remove_file(&pdev->dev, &dev_attr_set_prbs);
	return ret;

}

static int sf_pcie_remove(struct platform_device *pdev)
{
	struct sf_pcie *pcie = platform_get_drvdata(pdev);
	dev_err(&pdev->dev, "pcie controller driver was remove.");
	if (!pcie->link_state)
		return 0;

	device_remove_file(&pdev->dev, &dev_attr_relink);
	device_remove_file(&pdev->dev, &dev_attr_set_tx);
	device_remove_file(&pdev->dev, &dev_attr_set_prbs);

	dw_pcie_host_deinit(&pcie->pci->pp);
	if (pcie->pcie_ops->deinit)
		pcie->pcie_ops->deinit(pcie);
	if (pcie->irq_domain)
		irq_domain_remove(pcie->irq_domain);
	return 0;

}

static int sf_pcie_retry_link(struct platform_device *pdev)
{
	struct sf_pcie *sf_pcie = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	dev_info(dev, "Try To Relink PCIe....\n");
	if (!sf_pcie) {
		dev_err(dev, "Drivers probe failed\n");
		if (!sf_pcie_probe(pdev)){
			dev_info(dev, "Link up !!!\n");
		} else {
			device_links_no_driver(dev);
			devres_release_all(dev);
			kfree(dev->dma_range_map);
			dev->dma_range_map = NULL;
			dev->driver = NULL;
			dev_set_drvdata(dev, NULL);
			if (dev->pm_domain && dev->pm_domain->dismiss)
				dev->pm_domain->dismiss(dev);
			dev_pm_set_driver_flags(dev, 0);
			dev_err(dev, "probe still failed\n");
		}
	} else {
		struct pci_host_bridge *bridge = sf_pcie->pci->pp.bridge;

		dev_info(dev, "PCIe relinking...\n");
		sf_pcie->link_state = false;
		phy_init_done = false;
		pci_stop_root_bus(bridge->bus);
		pci_remove_root_bus(bridge->bus);
		sf_pcie_host_init(&sf_pcie->pci->pp);
		pci_host_probe(bridge);
		if (sf_pcie->link_state)
			dev_info(dev, "Link up !!!\n");
	}
	return 0;
}

static const struct of_device_id sf_pcie_of_match[] = {
	{ .compatible = "siflower,h8898-pcie", },
	{},
};

static struct platform_driver sf_pcie_driver = {
	.driver = {
		.name	= "sf-pcie",
		.of_match_table = sf_pcie_of_match,
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
	.probe    = sf_pcie_probe,
	.remove	  = sf_pcie_remove,
};

static int __init sf_pcie_driver_init(void)
{
	return platform_driver_register(&sf_pcie_driver);
}
device_initcall(sf_pcie_driver_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kaijun Wang <kaijun.wang@siflower.com.cn>");
MODULE_DESCRIPTION("PCIe Controller driver for SF21H8898 SoC");
