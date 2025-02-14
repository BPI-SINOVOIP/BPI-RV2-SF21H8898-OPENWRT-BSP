#include <linux/pci.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include "pcie_irqs.h"
#include "pcie_siwifi_platform.h"
#include "pcie_siwifi_plat_h8898.h"

int msi = 1;
module_param(msi, int, S_IRUGO);
MODULE_PARM_DESC(msi, "msi enable");

int band_mode = 2;
module_param(band_mode, int, S_IRUGO);
MODULE_PARM_DESC(band_mode, "band mode (default 2) 0:lb 1:hb 2:both");

int bin_type = 0;
module_param(bin_type, int, S_IRUGO);
MODULE_PARM_DESC(bin_type, "bin file to load to 2880 (default 0)"
			   "0:common_test 1:msi_test_wifi 2:msi_test_all");

static const struct pci_device_id sf_pci_id_table[] = {
	{PCI_DEVICE(PCI_VENDER_ID_X2880_EP0, PCI_DEVICE_ID_X2880_EP0)},
	{PCI_DEVICE(PCI_VENDER_ID_X2880_EP1, PCI_DEVICE_ID_X2880_EP1)},
	{0}
};

static int sf_pci_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
{
	struct siwifi_plat *sf_plat = NULL;
	int ret = 0;

	ret = pci_enable_device(pdev);
	if (ret) {
		dev_err(&(pdev->dev), "pci_enable_device failed\n");
		return ret;
	}

	pci_set_master(pdev);

	if (bin_type != 0) { // msi_test_wifi | msi_test_all
		ret = msi_init(pdev);
		if (ret)
			goto fail;
	}

	ret = siwifi_8898_platform_init(pdev, &sf_plat, pci_id);
	if (ret) {
		dev_err(&(pdev->dev), "siwifi_pci_probe fail ret=%d\n", ret);
		return 0;
	}

	if (((band_mode == BAND_24G_LB) && (sf_plat->params->band != BAND_24G_LB)) ||
	    ((band_mode == BAND_5G_HB) && (sf_plat->params->band != BAND_5G_HB))) {
		dev_warn(&(pdev->dev), "force bootup %s only, so we exit\n",
			band_mode == 0 ? "2.4G" : "5G");
		ret = 0;
	} else {
		pr_info("platform_on start\n");
		ret = siwifi_platform_on(sf_plat);
		pr_info("platform_on %s, ret: %d\n", (ret ? "faild" : "ok"), ret);
	}

	if (ret == 0) {
		sf_plat->enabled = true;
		dev_info(&(pdev->dev), "probe success\n");
		return 0;
	}
fail:
	pci_set_drvdata(pdev, NULL);
	return ret;
}

void sf_pci_remove(struct pci_dev *pdev)
{
	struct siwifi_plat *sf_plat = NULL;
	sf_plat = (struct siwifi_plat *)pci_get_drvdata(pdev);

	if (!sf_plat || !sf_plat->enabled) {
		pr_info("No need to remove\n");
		return;
	}

	if (bin_type != 0) { // msi_test_wifi | msi_test_all
		msi_deinit(pdev);
	}

	siwifi_platform_off(sf_plat);
	pci_set_drvdata(pdev, NULL);
	pr_info("remove success\n");
}
static struct pci_driver sf_pci_driver = {
	.name = "sf_pci",
	.id_table = sf_pci_id_table,
	.probe = sf_pci_probe,
	.remove = sf_pci_remove,
};

static int __init sf_pci_init(void)
{
	int ret;
	ret = pci_register_driver(&sf_pci_driver);
	if (ret)
		pr_err("Failed to register sf pci driver: %d\n", ret);
	return ret;
}

static void __exit sf_pci_exit(void)
{
	pci_unregister_driver(&sf_pci_driver);
}

module_init(sf_pci_init);
module_exit(sf_pci_exit);
MODULE_LICENSE("GPL");