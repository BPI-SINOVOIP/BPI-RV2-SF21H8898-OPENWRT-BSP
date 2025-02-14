#include "pcie_main.h"
#include "pcie_top_reg.h"
#include "pcie_reg_access.h"
#include "pcie_siwifi_plat_h8898.h"

static struct siwifi_band_ep_cpu siwifi_band_ep_cpu[] = {
	{
		.vender = PCI_VENDER_ID_X2880_EP0,
		.device = PCI_DEVICE_ID_X2880_EP0,
		.band = BAND_5G_HB,
		.cpu = LMAC_BOOT_C906,
		.is_used = true,
	},

	{
		.vender = PCI_VENDER_ID_X2880_EP1,
		.device = PCI_DEVICE_ID_X2880_EP1,
		.band = BAND_24G_LB,
		.cpu = LMAC_BOOT_C906,
		.is_used = true,
	},
};

static u8 *siwifi_8898_get_address(struct siwifi_plat *siwifi_plat, int addr_name, unsigned int offset)
{
	struct siwifi_8898 *siwifi_8898 = (struct siwifi_8898 *)siwifi_plat->priv;

	if (WARN(addr_name >= SIWIFI_ADDR_MAX, "Invalid address %d", addr_name))
		return NULL;

	if (addr_name == SIWIFI_ADDR_WIFI) {
		return (u8 *)(siwifi_8898->pci_bar2_vaddr - BAR2_LIMITED_ADDR + offset);
	} else {
		return (u8 *)(siwifi_8898->pci_bar1_vaddr - BAR1_LIMITED_ADDR + offset);
	}
}

void siwifi_8898_config_ATU(struct siwifi_plat *siwifi_plat)
{
	struct siwifi_8898 *siwifi_8898 = (struct siwifi_8898 *)siwifi_plat->priv;
	unsigned int ctrl11;
	void *vaddr = siwifi_8898->pci_bar0_vaddr;
	void *iatu_addr = vaddr + IATU_UNROLL_OFFSET;

	// 1 config inbound addr for access iram1/iram2 which is in macxbar bar2
	writel(0xc0000200, iatu_addr + IATU_REGION_CTRL_2_OFF_INBOUND);
	// Base addr
	writel(0x0, iatu_addr + IATU_LWR_BASE_ADDR_OFF_INBOUND);
	// Target addr
	writel(0x0 + BAR2_LIMITED_ADDR, iatu_addr + IATU_LWR_TARGET_ADDR_OFF_INBOUND);
	// Limited address range to 256M
	writel(BAR2_LIMITED_ADDR, iatu_addr + IATU_LIMIT_ADDR_OFF_INBOUND);

	// 2 config second atu for access sysm which is in cpuxbar with bar1
	writel(0xc0000100, iatu_addr + IATU_REGION_CTRL_2_OFF_INBOUND + IATU_OFFSET * 1);
	// Base addr
	writel(0x00000000, iatu_addr + IATU_LWR_BASE_ADDR_OFF_INBOUND + IATU_OFFSET * 1);
	// Target addr, map to cpu xbar, add offset
	writel(0x60000000 + BAR1_LIMITED_ADDR,
		iatu_addr + IATU_LWR_TARGET_ADDR_OFF_INBOUND + IATU_OFFSET * 1);
	// Limited address range to 16M
	writel(BAR1_LIMITED_ADDR, iatu_addr + IATU_LIMIT_ADDR_OFF_INBOUND + IATU_OFFSET * 1);

	ctrl11 = readl(iatu_addr + IATU_REGION_CTRL_2_OFF_INBOUND);
	pr_info("++++++IATU_REGION_CTRL_2_OFF_INBOUND222: addr %x val=%#x\n",
		IATU_UNROLL_OFFSET + IATU_REGION_CTRL_2_OFF_INBOUND, ctrl11);

	// Config outband atu
	ctrl11 = readl(iatu_addr + IATU_REGION_CTRL_2_OFF_OUTBOUND_i);
	writel(ctrl11 | (0x1 << 31), iatu_addr + IATU_REGION_CTRL_2_OFF_OUTBOUND_i);
	// Base addr
	writel(0x0, iatu_addr + IATU_LWR_BASE_ADDR_OFF_OUTBOUND_i);
	// Target addr
	writel(DDR_PHY_BASE_ADDR, iatu_addr + IATU_LWR_TARGET_ADDR_OFF_OUTBOUND_i);
	// Limited to 256M
	writel(0x10000000, iatu_addr + IATU_LIMIT_ADDR_OFF_OUTBOUND_i);
	pr_info("siwifi_8898_config_ATU+++<<<\n");
}

int siwifi_pci_memory_access_check(struct siwifi_plat *siwifi_plat)
{
	volatile u32 *iram1 = (u32 *)SIWIFI_ADDR((siwifi_plat), SIWIFI_ADDR_WIFI,
						 RAM_LMAC_FW_ADDR(siwifi_plat->params->band));
	volatile u32 *iram2 = (u32 *)SIWIFI_ADDR((siwifi_plat), SIWIFI_ADDR_WIFI,
						 RAM_LMAC_FW_ADDR(siwifi_plat->params->band) + 4);
	*iram1 = 0x5a5a5a5a;
	*iram2 = 0xa5a5a5a5;
	if ((*iram1 != 0x5a5a5a5a) && (*iram2 != 0xa5a5a5a5)) {
		pr_info("iram check failed!!!\n");
		return -EFAULT;
	} else {
		pr_info("iram check passed!\n");
		return 0;
	}
}

int siwifi_8898_platform_init(struct pci_dev *pdev, struct siwifi_plat **sf_plat,
							const struct pci_device_id *pci_id)
{
	struct siwifi_8898 *siwifi_8898;
	struct siwifi_comm_params *params;
	int i, ret = 0;
	u16 pci_cmd;
	u8 band = 0;

	for (i = 0; i < ARRAY_SIZE(siwifi_band_ep_cpu); i++) {
		if ((pci_id->vendor == siwifi_band_ep_cpu[i].vender) &&
		    (pci_id->device == siwifi_band_ep_cpu[i].device)) {
			band = siwifi_band_ep_cpu[i].band;
			break;
		}
	}

	*sf_plat = kzalloc(sizeof(struct siwifi_plat) + sizeof(struct siwifi_8898), GFP_KERNEL);
	if (!*sf_plat) {
		dev_err(&(pdev->dev), "alloc plat failed\n");
		return -ENOMEM;
	}

	params = kzalloc(sizeof(struct siwifi_comm_params), GFP_KERNEL);
	if (!params) {
		dev_err(&(pdev->dev), "alloc params failed\n");
		ret = -ENOMEM;
		goto out_enable;
	}

	(*sf_plat)->params = params;

	for (i = 0; i < ARRAY_SIZE(siwifi_band_ep_cpu); i++) {
		if ((pci_id->vendor == siwifi_band_ep_cpu[i].vender) &&
		    (pci_id->device == siwifi_band_ep_cpu[i].device)) {
			if (siwifi_band_ep_cpu[i].is_used) {
				(*sf_plat)->params->cpu = siwifi_band_ep_cpu[i].cpu;
				(*sf_plat)->params->band = siwifi_band_ep_cpu[i].band;
				break;
			} else {
				dev_warn(&(pdev->dev), "Match vendorID: %x and deviceID: %x, But not used.\n",
					 pci_id->vendor, pci_id->device);
				ret = -ENODEV;
				goto out_free;
			}
		}
	}

	if (i == ARRAY_SIZE(siwifi_band_ep_cpu)) {
		dev_warn(&(pdev->dev), "Can't match vendorID: %x and deviceID: %x\n",
							pci_id->vendor, pci_id->device);
		ret = -ENODEV;
		goto out_free;
	}

	siwifi_8898 = (struct siwifi_8898 *)(*sf_plat)->priv;

	/* Hotplug fixups */
	pci_read_config_word(pdev, PCI_COMMAND, &pci_cmd);
	pci_cmd |= PCI_COMMAND_PARITY | PCI_COMMAND_SERR;
	pci_write_config_word(pdev, PCI_COMMAND, pci_cmd);
	pci_write_config_byte(pdev, PCI_CACHE_LINE_SIZE, L1_CACHE_BYTES >> 2);

	if ((ret = pci_request_regions(pdev, KBUILD_MODNAME))) {
		dev_err(&(pdev->dev), "pci_request_regions failed\n");
		goto out_request;
	}

	pr_info("bar0 info [%#llx - %#llx] flags[%#lx] len[%#llx]\n",
			pci_resource_start(pdev, 0), pci_resource_end(pdev, 0),
			pci_resource_flags(pdev, 0), pci_resource_len(pdev, 0));
	pr_info("bar1 info [%#llx - %#llx] flags[%#lx] len[%#llx]\n",
			pci_resource_start(pdev, 1), pci_resource_end(pdev, 1),
			pci_resource_flags(pdev, 1), pci_resource_len(pdev, 1));
	pr_info("bar2 info [%#llx - %#llx] flags[%#lx] len[%#llx]\n",
			pci_resource_start(pdev, 2), pci_resource_end(pdev, 2),
			pci_resource_flags(pdev, 2), pci_resource_len(pdev, 2));

	if (!(siwifi_8898->pci_bar0_vaddr = (u8 *)pci_ioremap_bar(pdev, 0))) {
		dev_err(&(pdev->dev), "pci_ioremap_bar(%d) failed\n", 0);
		ret = -ENOMEM;
		goto out_bar0;
	}
	if (!(siwifi_8898->pci_bar1_vaddr = (u8 *)pci_ioremap_bar(pdev, 1))) {
		dev_err(&(pdev->dev), "pci_ioremap_bar(%d) failed\n", 1);
		ret = -ENOMEM;
		goto out_bar1;
	}

	if (!(siwifi_8898->pci_bar2_vaddr = (u8 *)pci_ioremap_bar(pdev, 2))) {
		dev_err(&(pdev->dev), "pci_ioremap_bar(%d) failed\n", 2);
		ret = -ENOMEM;
		goto out_bar2;
	}

	(*sf_plat)->get_address = siwifi_8898_get_address;

	// Config ATU region for address map
	if (pci_id->vendor == PCI_VENDER_ID_X2880_EP0)
		siwifi_8898_config_ATU(*sf_plat);

	if (siwifi_pci_memory_access_check(*sf_plat)) {
		ret = -EFAULT;
		goto out;
	}

	(*sf_plat)->pci_dev = pdev;
	(*sf_plat)->enabled = false;
	pci_set_drvdata(pdev, *sf_plat);

	return 0;
out:
	iounmap(siwifi_8898->pci_bar2_vaddr);
out_bar2:
	iounmap(siwifi_8898->pci_bar1_vaddr);
out_bar1:
	iounmap(siwifi_8898->pci_bar0_vaddr);
out_bar0:
	pci_release_regions(pdev);
out_request:
	pci_clear_master(pdev);
	pci_disable_device(pdev);
out_free:
	kfree(params);
	params = NULL;
out_enable:
	kfree(*sf_plat);
	return ret;
}