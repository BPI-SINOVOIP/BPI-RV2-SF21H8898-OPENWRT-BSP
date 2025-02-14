#ifndef _PCIE_IRQS_H_
#define _PCIE_IRQS_H_

#include <linux/pci.h>
#include <linux/interrupt.h>
#include "pcie_siwifi_platform.h"

irqreturn_t msi_test_wifi_handler(int irq, void *dev_id);
irqreturn_t msi_test_all_handler(int irq, void *dev_id);
int msi_init(struct pci_dev *pdev);
int msi_deinit(struct pci_dev *pdev);
void msi_ipc_host_enable_irq(struct siwifi_plat *sf_plat);
void msi_ipc_host_disable_irq(struct siwifi_plat *sf_plat);

#endif /* _PCIE_IRQS_H_ */