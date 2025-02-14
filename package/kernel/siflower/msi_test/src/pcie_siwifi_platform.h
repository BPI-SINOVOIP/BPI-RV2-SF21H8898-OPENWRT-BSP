#ifndef _PCIE_SIWIFI_PLAT_H_
#define _PCIE_SIWIFI_PLAT_H_

#include <linux/pci.h>

#define PCIE_MODE 0

#define PCI_VENDER_ID_X2880_EP0 0x1688
#define PCI_DEVICE_ID_X2880_EP0 0xa2a2
#if   PCIE_MODE == 0
#define PCI_VENDER_ID_X2880_EP1 0x1682
#define PCI_DEVICE_ID_X2880_EP1 0xa1a1
#define COMMON_TEST_BIN_HB   "common_test_0_hb.bin"
#define COMMON_TEST_BIN_LB   "common_test_0_lb.bin"
#define MSI_TEST_WIFI_BIN_HB "msi_test_wifi_0_hb.bin"
#define MSI_TEST_WIFI_BIN_LB "msi_test_wifi_0_lb.bin"
#define MSI_TEST_ALL_BIN_HB  "msi_test_all_0_hb.bin"
#define MSI_TEST_ALL_BIN_LB  "msi_test_all_0_lb.bin"
#elif PCIE_MODE == 1
#define PCI_VENDER_ID_X2880_EP1 0x1683
#define PCI_DEVICE_ID_X2880_EP1 0xa3a3
#define COMMON_TEST_BIN_HB   "common_test_1_hb.bin"
#define COMMON_TEST_BIN_LB   "common_test_1_lb.bin"
#define MSI_TEST_WIFI_BIN_HB "msi_test_wifi_1_hb.bin"
#define MSI_TEST_WIFI_BIN_LB "msi_test_wifi_1_lb.bin"
#define MSI_TEST_ALL_BIN_HB  "msi_test_all_1_hb.bin"
#define MSI_TEST_ALL_BIN_LB  "msi_test_all_1_lb.bin"
#endif
#define PCI_VENDOR_ID_DINIGROUP             0x17df
#define PCI_DEVICE_ID_DINIGROUP_DNV6_F2PCIE 0x1907
#define PCI_DEVICE_ID_XILINX_CEVA_VIRTEX7   0x7011
#define PCI_VENDER_ID_FPGA_PCIE_XLINUX 0x10ee
#define PCI_DEVICE_ID_FPGA_PCIE_XLINUX 0x8022

#define SIWIFI_ADDR(plat, base, offset) plat->get_address(plat, base, offset)

enum siwifi_lmac_boot_cpu {
	LMAC_BOOT_C906 = 0,
	LMAC_BOOT_A9 = 1,
};

enum siwifi_band_type {
	BAND_24G_LB = 0,
	BAND_5G_HB = 1,
	/* The max num of band type */
	BAND_TYPE_MAX,
};

struct siwifi_comm_params {
	u8 band;
	u8 cpu;
};

enum siwifi_platform_addr {
	// To access mac/iram/agc
	SIWIFI_ADDR_WIFI,
	// To access sysm, cpu xbar
	SIWIFI_ADDR_SYSTEM,
	SIWIFI_ADDR_MAX,
};

struct siwifi_comm_params;

/**
 * struct siwifi_plat - Operation pointers for SIWIFI PCI platform
 * @pci_dev: pointer to pci dev
 * @enabled: Set if embedded platform has been enabled (i.e. fw loaded and ipc started)
 * @get_address: Return the virtual address to access the requested address on the platform
 * @priv: Private data for the link driver
 */
struct siwifi_plat {
	struct pci_dev *pci_dev;
	struct siwifi_comm_params *params;

	bool enabled;
	u8 *(*get_address)(struct siwifi_plat *siwifi_plat, int addr_name, unsigned int offset);

	u8 priv[0] __aligned(sizeof(void *));
};

void siwifi_platform_reset(struct siwifi_plat *siwifi_plat);

int siwifi_platform_on(struct siwifi_plat *sf_plat);
void siwifi_platform_off(struct siwifi_plat *sf_plat);
void siwifi_plat_lmac_start_c906(struct siwifi_plat *siwifi_plat);
void siwifi_plat_lmac_stop_c906(struct siwifi_plat *siwifi_plat);

int siwifi_platform_register_drv(void);
void siwifi_platform_unregister_drv(void);

#endif /* _PCIE_SIWIFI_PLAT_H_ */