#include <linux/interrupt.h>
#include "pcie_main.h"
#include "pcie_top_reg.h"
#include "pcie_reg_access.h"
#include "pcie_siwifi_platform.h"
#include "pcie_siwifi_plat_h8898.h"

#define ALLOC_VECTOR  1
#define VECTOR_NUM    8

static unsigned int func_id_count;

irqreturn_t msi_test_wifi_handler(int irq, void *dev_id)
{
	struct pci_dev *pdev = (struct pci_dev *)dev_id;
	unsigned int func_num, event_id, offset;
	func_num = (pdev->device == 0xa2a2) ? 0 : 1;
	event_id = irq - pdev->irq;
	offset = ((func_num << 1) | event_id) * 8;
	func_id_count += (1 << offset);

	pr_info("%s <%x><%x> func%d %s irq: %3d id: %d %s func_id_count: 0x%08x\n",
		__func__, pdev->vendor, pdev->device,
		func_num, (func_num == 0) ? "OOOOO" : "|||||",
		irq, event_id, (event_id == 0) ? "-----" : "+++++",
		func_id_count);

	return IRQ_HANDLED;
}

irqreturn_t msi_test_all_handler(int irq, void *dev_id)
{
	struct pci_dev *pdev = (struct pci_dev *)dev_id;
	unsigned int func_num, event_id, offset;
	func_num = (pdev->device == 0xa2a2) ? 0 : 1;
	event_id = irq - pdev->irq;
	offset = (event_id * 4) + (func_num * 2);
	func_id_count += (1 << offset);

	pr_info("%s <%x><%x> func%d %s irq: %3d id: %d func_id_count: 0x%08x\n",
		__func__, pdev->vendor, pdev->device,
		func_num, (func_num == 0) ? "OOOOO" : "|||||",
		irq, event_id, func_id_count);

	return IRQ_HANDLED;
}

int msi_init(struct pci_dev *pdev)
{
	int ret, i, irq, vector_num;
	func_id_count = 0;
	vector_num = (bin_type == 1) ? 2 : VECTOR_NUM;

	ret = pci_alloc_irq_vectors(pdev, vector_num, vector_num, PCI_IRQ_MSI);
	if (ret < 0) {
		dev_err(&(pdev->dev), "pci_alloc_irq_vectors failed\n");
	}
	pr_info("pci_alloc_irq_vectors\t ret: %d\n", ret);

	for (i = 0; i < vector_num; i++) {
		pr_info("request >>>>>>>>>> int: %d\n", i);
		irq = pci_irq_vector(pdev, i);
		pr_info("pci_irq_vector     irq: %d\n", irq);
		if (bin_type == 1) {
			ret = devm_request_irq(&pdev->dev, irq, msi_test_wifi_handler, 0, "msi_test_wifi", pdev);
		} else if (bin_type == 2) {
			ret = devm_request_irq(&pdev->dev, irq, msi_test_all_handler, 0, "msi_test_all", pdev);
		}
		pr_info("devm_request_irq   ret: %d\n", ret);
		if (ret) {
			dev_err(&(pdev->dev), "Failed to request MSI irq: %d ret: %d\n", irq, ret);
			goto free_irq;
		}
	}
	return 0;

free_irq:
	while (i > 0) {
		i--;
		irq = pci_irq_vector(pdev, i);
		devm_free_irq(&pdev->dev, irq, pdev);
	}
	pci_free_irq_vectors(pdev);

	return -1;
}

int msi_deinit(struct pci_dev *pdev)
{
	int i, irq, vector_num;
	vector_num = (bin_type == 1) ? 2 : VECTOR_NUM;

	for (i = 0; i < vector_num; i++) {
		irq = pci_irq_vector(pdev, i);
		devm_free_irq(&pdev->dev, irq, pdev);
	}
	pci_free_irq_vectors(pdev);
	return 0;
}