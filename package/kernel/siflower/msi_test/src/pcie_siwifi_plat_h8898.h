#ifndef _PCIE_SIWIFI_PLAT_8898_H_
#define _PCIE_SIWIFI_PLAT_8898_H_

#include <linux/pci.h>
#include "pcie_siwifi_platform.h"

// bar1 limited to 16M
#define BAR1_LIMITED_ADDR 0x01000000
// bar2 limited to 256M
#define BAR2_LIMITED_ADDR 0x10000000

#define IATU_OFFSET        0x200
#define DMA_UNROLL_OFFSET  0x1000
#define IATU_UNROLL_OFFSET 0x1800

#define IATU_REGION_CTRL_1_OFF_INBOUND   0x100
#define IATU_REGION_CTRL_2_OFF_INBOUND   0x104
#define IATU_LWR_BASE_ADDR_OFF_INBOUND   0x108
#define IATU_UPPER_BASE_ADDR_OFF_INBOUND 0x10c
#define IATU_LIMIT_ADDR_OFF_INBOUND      0x110
#define IATU_LWR_TARGET_ADDR_OFF_INBOUND 0x114

#define IATU_REGION_CTRL_1_OFF_OUTBOUND_i     0x00000000
#define IATU_REGION_CTRL_2_OFF_OUTBOUND_i     0x00000004
#define IATU_LWR_BASE_ADDR_OFF_OUTBOUND_i     0x00000008
#define IATU_UPPER_BASE_ADDR_OFF_OUTBOUND_i   0x0000000C
#define IATU_LIMIT_ADDR_OFF_OUTBOUND_i        0x00000010
#define IATU_LWR_TARGET_ADDR_OFF_OUTBOUND_i   0x00000014
#define IATU_UPPER_TARGET_ADDR_OFF_OUTBOUND_i 0x00000018

#define CTRL_1_OFFSET_FUNC_NUM   20
#define CTRL_2_OFFSET_REGION_EN  31
#define CTRL_2_OFFSET_MATCH_MODE 30
#define CTRL_2_OFFSET_BAR_NUM    8

struct siwifi_band_ep_cpu {
	unsigned int vender;
	unsigned int device;
	u8 band;
	u8 cpu;
	bool is_used;
};

struct siwifi_8898 {
	// To access atu config
	u8 *pci_bar0_vaddr;
	// To access sysm/cpu xbar
	u8 *pci_bar1_vaddr;
	// To access wifi/iram/mac xbar
	u8 *pci_bar2_vaddr;
};

int siwifi_8898_platform_init(struct pci_dev *pci_dev, struct siwifi_plat **siwifi_plat,
				const struct pci_device_id *pci_id);
int siwifi_pci_memory_access_check(struct siwifi_plat *siwifi_plat);
#endif /* _PCIE_SIWIFI_PLAT_8898_H_ */