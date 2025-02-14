#include <linux/firmware.h>
#include "pcie_main.h"
#include "pcie_top_reg.h"
#include "pcie_reg_access.h"
#include "pcie_siwifi_platform.h"
#include "pcie_siwifi_plat_h8898.h"

static int siwifi_plat_bin_fw_upload(struct siwifi_plat *siwifi_plat, u8 *fw_addr, char *filename)
{
	const struct firmware *fw;
	struct device *dev = &(siwifi_plat->pci_dev->dev);
	int err = 0;
	unsigned int i, size;
	u32 *src, *dst;

	err = request_firmware(&fw, filename, dev);
	if (err) {
		return err;
	}

	/* Copy the file on the Embedded side */
	pr_info("### Now copy %s firmware, @ = %px\n", filename, fw_addr);

	src = (u32 *)fw->data;
	dst = (u32 *)fw_addr;
	size = (unsigned int)fw->size;

	/* Check potential platform bug on multiple stores vs memcpy */
	for (i = 0; i < size; src++, dst++, i += 4) {
		writel(*src, (void *)dst);
		if (*dst != *src) {
			pr_info("should not come here!! src %px dst %px src value 0x%x dst value 0x%x\n",
				src, dst, *src, *dst);
			writel(*src, (void *)dst);
		}
	}

	release_firmware(fw);

	return err;
}

int siwifi_plat_load_bin(struct siwifi_plat *sf_plat)
{
	char *filename;
	u8 band = sf_plat->params->band;
	switch (bin_type) {
	case 0:
		filename = (band == BAND_5G_HB) ? COMMON_TEST_BIN_HB   : COMMON_TEST_BIN_LB;
		break;
	case 1:
		filename = (band == BAND_5G_HB) ? MSI_TEST_WIFI_BIN_HB : MSI_TEST_WIFI_BIN_LB;
		break;
	case 2:
		filename = (band == BAND_5G_HB) ? MSI_TEST_ALL_BIN_HB  : MSI_TEST_ALL_BIN_LB;
		break;
	default:
		filename = (band == BAND_5G_HB) ? COMMON_TEST_BIN_HB   : COMMON_TEST_BIN_LB;
		break;
	}
	return siwifi_plat_bin_fw_upload(
		sf_plat,
		SIWIFI_ADDR(sf_plat, SIWIFI_ADDR_WIFI, RAM_LMAC_FW_ADDR(band)),
		filename);
}


int siwifi_platform_on(struct siwifi_plat *sf_plat)
{
	int ret;
	siwifi_platform_reset(sf_plat);

	siwifi_plat_lmac_stop_c906(sf_plat);

	ret = siwifi_plat_load_bin(sf_plat);
	if (ret)
		return ret;
	siwifi_plat_lmac_start_c906(sf_plat);
	return 0;
}

void siwifi_platform_off(struct siwifi_plat *sf_plat)
{
	struct siwifi_8898 *sf_8898 = (struct siwifi_8898 *)sf_plat->priv;
	siwifi_platform_reset(sf_plat);
	sf_plat->enabled = false;

	pci_disable_device(sf_plat->pci_dev);

	iounmap(sf_8898->pci_bar0_vaddr);
	iounmap(sf_8898->pci_bar1_vaddr);
	iounmap(sf_8898->pci_bar2_vaddr);

	pci_release_regions(sf_plat->pci_dev);
	pci_clear_master(sf_plat->pci_dev);
	kfree(sf_plat);
}

void siwifi_platform_reset(struct siwifi_plat *siwifi_plat)
{
	u32 brom_reset = 0, clken_cfg = 0, hb_clken_cfg = 0, rstn_cfg = 0, hb_rstn_cfg;
	void *sysm_clken_cfg = (void *)SIWIFI_ADDR((siwifi_plat), SIWIFI_ADDR_SYSTEM, TOP_CRM_CLKEN_CFG_ADDR);
	void *sysm_hb_clken_cfg = (void *)SIWIFI_ADDR((siwifi_plat), SIWIFI_ADDR_SYSTEM, TOP_CRM_HB_CLKEN_CFG_ADDR);
	void *sysm_rstn_cfg = (void *)SIWIFI_ADDR((siwifi_plat), SIWIFI_ADDR_SYSTEM, TOP_CRM_RSTN_CFG_ADDR);
	void *sysm_hb_rstn_cfg = (void *)SIWIFI_ADDR((siwifi_plat), SIWIFI_ADDR_SYSTEM, TOP_CRM_HB_RSTN_CFG_ADDR);
	void *sysm_brom_rstn = (void *)SIWIFI_ADDR((siwifi_plat), SIWIFI_ADDR_SYSTEM, BROM_SYSM_RESET_ADDR);

	brom_reset = readl(sysm_brom_rstn);
	clken_cfg = readl(sysm_clken_cfg);
	hb_clken_cfg = readl(sysm_hb_clken_cfg);
	rstn_cfg = readl(sysm_rstn_cfg);
	hb_rstn_cfg = readl(sysm_hb_rstn_cfg);
	pr_info("Default >> brom_reset:%#8.x clken_cfg:%#.8x hb_clken_cfg:%#.8x rstn_cfg:%#.8x hb_rstn_cfg:%#.8x\n",
		brom_reset, clken_cfg, hb_clken_cfg, rstn_cfg, hb_rstn_cfg);

	/* APB clock is not only used for external devices, but also for accessing most of the registers.
	 * The system controller register uses APB clock, so APB clock must be available,
	 * otherwise the system cannot start normally.
	 */
	if (siwifi_plat->params->band == BAND_5G_HB) {
		writel(brom_reset   | TOP_BROM_RESET_WIFI_C906_5G, sysm_brom_rstn);
		writel(clken_cfg    | TOP_CRM_CLKEN_WIFI_C906_5G, sysm_clken_cfg);
		writel(hb_clken_cfg | TOP_CRM_HB_CLKEN_WIFI_C906_5G, sysm_hb_clken_cfg);
		writel(rstn_cfg     | TOP_CRM_RSTN_WIFI_C906_5G, sysm_rstn_cfg);
		writel(hb_rstn_cfg  | TOP_CRM_HB_RSTN_WIFI_C906_5G, sysm_hb_rstn_cfg);
	} else {
		writel(brom_reset   | TOP_BROM_RESET_WIFI_C906_24G, sysm_brom_rstn);
		writel(clken_cfg    | TOP_CRM_CLKEN_WIFI_C906_24G, sysm_clken_cfg);
		writel(rstn_cfg     | TOP_CRM_RSTN_WIFI_C906_24G, sysm_rstn_cfg);
	}

	brom_reset = readl(sysm_brom_rstn);
	clken_cfg = readl(sysm_clken_cfg);
	hb_clken_cfg = readl(sysm_hb_clken_cfg);
	rstn_cfg = readl(sysm_rstn_cfg);
	hb_rstn_cfg = readl(sysm_hb_rstn_cfg);
	pr_info("After reset >> brom_reset:%#.8x clken_cfg:%#.8x hb_clken_cfg:%#.8x rstn_cfg:%#.8x hb_rstn_cfg:%#.8x\n",
		brom_reset, clken_cfg, hb_clken_cfg, rstn_cfg, hb_rstn_cfg);
}

void siwifi_plat_lmac_stop_c906(struct siwifi_plat *siwifi_plat)
{
	void *cpu_addr = (void *)SIWIFI_ADDR(siwifi_plat, SIWIFI_ADDR_SYSTEM, CPU_SYSM_BASE_ADDR);
	void *wifi_addr = (void *)SIWIFI_ADDR(siwifi_plat, SIWIFI_ADDR_WIFI, WIFI_BASE_ADDR(BAND_24G_LB));

	if (siwifi_plat->params->band == BAND_5G_HB) {
		writel(0x1, wifi_addr + NXMAC_MAC_CNTRL_2_ADDR);
		writel(0x0, cpu_addr + CPU_C906_5G_SYSM_CLK_EN_ADDR);
		writel(0x0, cpu_addr + CPU_C906_5G_SYSM_RST_ADDR);
	} else {
		writel(0x1, wifi_addr + NXMAC_MAC_CNTRL_2_ADDR);
		writel(0x0, cpu_addr + CPU_C906_24G_SYSM_CLK_EN_ADDR);
		writel(0x0, cpu_addr + CPU_C906_24G_SYSM_RST_ADDR);
	}
}

static void set(void *addr, u32 val)
{
	u32 tmp = readl(addr);
	tmp |= val;
	writel(tmp, addr);
}

void siwifi_plat_lmac_start_c906(struct siwifi_plat *siwifi_plat)
{
	void *vaddr = (void *)SIWIFI_ADDR(siwifi_plat, SIWIFI_ADDR_SYSTEM, CPU_SYSM_BASE_ADDR);

	pr_info("C906 Boot Up Start >>>>>>>>>>>>>>>>>>>>> %s\n",
		siwifi_plat->params->band == BAND_5G_HB ? "5G" : "24G");

	if (siwifi_plat->params->band == BAND_5G_HB) {
		/* release the RSTN of TOP_BROM_RESET_C906_5G_SYSM_RESET_N */
		set(vaddr + BROM_SYSM_RESET_CPU_OFFSET, TOP_BROM_RESET_C906_5G_SYSM_RESET_N);
		/* relese  CPU_C906_24G_SYSM_SYNC_RESET */
		set(vaddr + CPU_C906_5G_SYSM_SYNC_RESET_ADDR, BIT(0));
		/* enable top crm clk */
		set(vaddr + TOP_CRM_CLKEN_CFG_OFFSET, TOP_CRM_CLKEN_C906_5G_CLK_EN |
			TOP_CRM_CLKEN_C906_5G_CLK_EN | TOP_CRM_CLKEN_AXI_MCU_CLK_EN | TOP_CRM_CLKEN_AXI_MAC_CLK_EN);
		set(vaddr + TOP_CRM_HB_CLKEN_CFG_OFFSET, TOP_CRM_HB_CLKEN_WLAN_5G_LP_ROOT_CLK_EN |
			TOP_CRM_HB_CLKEN_WLAN_HB_PLF_CLK_EN | TOP_CRM_HB_CLKEN_WLAN_HB_REF_CLK_EN);
		/* enable cpu clk */
		set(vaddr + CPU_C906_5G_SYSM_CLK_EN_ADDR, BIT(0));
		/* release the top crm resetn */
		set(vaddr + TOP_CRM_RSTN_CFG_OFFSET, TOP_CRM_RSTN_AXI_MCU_SW_RST_N | TOP_CRM_RSTN_AXI_MAC_SW_RST_N);
		set(vaddr + TOP_CRM_HB_RSTN_CFG_OFFSET, TOP_CRM_HB_RSTN_WLAN_HB_PLF_SW_RST_N);
		/* set the  boot up address */
		writel(0x53e00000, vaddr + CPU_C906_5G_SYSM_RVBA0_ADDR);
		/* release the resetn axi/timer/debug of cpu */
		set(vaddr + CPU_C906_5G_SYSM_RST_ADDR, BIT(12) | BIT(8) | BIT(0));
		/* release the resetn of cpu */
		set(vaddr + CPU_C906_5G_SYSM_RST_ADDR, BIT(4));
	} else {
		/* release the RSTN of TOP_BROM_RESET_C906_24G_SYSM_RESET_N */
		set(vaddr + BROM_SYSM_RESET_CPU_OFFSET, TOP_BROM_RESET_C906_24G_SYSM_RESET_N);
		/* relese  CPU_C906_24G_SYSM_SYNC_RESET */
		set(vaddr + CPU_C906_24G_SYSM_SYNC_RESET_ADDR, BIT(0));
		/* enable top crm clk */
		set(vaddr + TOP_CRM_CLKEN_CFG_OFFSET, TOP_CRM_CLKEN_C906_24G_CLK_EN |
			TOP_CRM_CLKEN_C906_24G_CLK_EN | TOP_CRM_CLKEN_AXI_MCU_CLK_EN | TOP_CRM_CLKEN_AXI_MAC_CLK_EN);
		/* enable cpu clk */
		set(vaddr + CPU_C906_24G_SYSM_CLK_EN_ADDR, BIT(0));
		/* release the top crm resetn */
		set(vaddr + TOP_CRM_RSTN_CFG_OFFSET, TOP_CRM_RSTN_AXI_MCU_SW_RST_N | TOP_CRM_RSTN_AXI_MAC_SW_RST_N);
		/* set the  boot up address */
		writel(0x53c00000, vaddr + CPU_C906_24G_SYSM_RVBA0_ADDR);
		/* release the resetn axi/timer/debug of cpu */
		set(vaddr + CPU_C906_24G_SYSM_RST_ADDR, BIT(12) | BIT(8) | BIT(0));
		/* release the resetn of cpu */
		set(vaddr + CPU_C906_24G_SYSM_RST_ADDR, BIT(4));
	}

	pr_info("C906 Boot Up End <<<<<<<<<<<<<<<<<<<<<<< %s\n",
		siwifi_plat->params->band == BAND_5G_HB ? "5G" : "24G");
}