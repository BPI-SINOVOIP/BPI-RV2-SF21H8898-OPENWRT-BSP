#ifndef _PCIE_REG_ACCESS_H_
#define _PCIE_REG_ACCESS_H_

#include "pcie_siwifi_platform.h"
/*****************************************************************************
 * Addresses within SIWIFI_ADDR_CPU
 *****************************************************************************/
#define RAM_LMAC_FW_ADDR(band)            (((band) == BAND_5G_HB) ? 0x13E00000 : 0x13C00000)
#define WIFI_BASE_ADDR(band)              (((band) == BAND_5G_HB) ? 0x16000000 : 0x14000000)
#define RAM_LMAC_CALIBRATION_ADDR         0x13E00000
#define RAM_LMAC_CALIBRATION_CFG_ADDR_HB  0x13C00000

/*****************************************************************************
 * Addresses within SIWIFI_ADDR_SYSTEM
 *****************************************************************************/
/* Shard RAM */
#define SHARED_RAM_START_ADDR(band)      (((band) == BAND_5G_HB) ? 0x16000000 : 0x14000000)
#define SHARED_RAM_OTHER_BAND_ADDR(band) (((band) == BAND_5G_HB) ? 0x14000000 : 0x16000000)
#define IPC_RAM_START_ADDR(band)         (((band) == BAND_5G_HB) ? 0x13eef000 : 0x13cda000)

/* pcie base address */
#define DDR_PHY_BASE_ADDR  0x20000000
#define PCIE_MAP_BASE_ADDR 0x20000000
#define PCIE_HOST_MEM_ADDR (PCIE_MAP_BASE_ADDR - DDR_PHY_BASE_ADDR)

/* IPC registers */
#define IPC_REG_BASE_ADDR  0x00800000

/* MAC platform */
#define NXMAC_VERSION_1_ADDR         0x00B00004
#define NXMAC_MU_MIMO_TX_BIT         BIT(19)
#define NXMAC_BFMER_BIT              BIT(18)
#define NXMAC_BFMEE_BIT              BIT(17)
#define NXMAC_MAC_80211MH_FORMAT_BIT BIT(16)
#define NXMAC_HE_BIT                 BIT(15)
#define NXMAC_COEX_BIT               BIT(14)
#define NXMAC_WAPI_BIT               BIT(13)
#define NXMAC_TPC_BIT                BIT(12)
#define NXMAC_VHT_BIT                BIT(11)
#define NXMAC_HT_BIT                 BIT(10)
#define NXMAC_GCMP_BIT               BIT(9)
#define NXMAC_RCE_BIT                BIT(8)
#define NXMAC_CCMP_BIT               BIT(7)
#define NXMAC_TKIP_BIT               BIT(6)
#define NXMAC_WEP_BIT                BIT(5)
#define NXMAC_SECURITY_BIT           BIT(4)
#define NXMAC_SME_BIT                BIT(3)
#define NXMAC_HCCA_BIT               BIT(2)
#define NXMAC_EDCA_BIT               BIT(1)
#define NXMAC_QOS_BIT                BIT(0)

#define NXMAC_RX_CNTRL_ADDR                   0x00B00060
#define NXMAC_EN_DUPLICATE_DETECTION_BIT      BIT(31)
#define NXMAC_ACCEPT_UNKNOWN_BIT              BIT(30)
#define NXMAC_ACCEPT_OTHER_DATA_FRAMES_BIT    BIT(29)
#define NXMAC_ACCEPT_QO_S_NULL_BIT            BIT(28)
#define NXMAC_ACCEPT_QCFWO_DATA_BIT           BIT(27)
#define NXMAC_ACCEPT_Q_DATA_BIT               BIT(26)
#define NXMAC_ACCEPT_CFWO_DATA_BIT            BIT(25)
#define NXMAC_ACCEPT_DATA_BIT                 BIT(24)
#define NXMAC_ACCEPT_OTHER_CNTRL_FRAMES_BIT   BIT(23)
#define NXMAC_ACCEPT_CF_END_BIT               BIT(22)
#define NXMAC_ACCEPT_ACK_BIT                  BIT(21)
#define NXMAC_ACCEPT_CTS_BIT                  BIT(20)
#define NXMAC_ACCEPT_RTS_BIT                  BIT(19)
#define NXMAC_ACCEPT_PS_POLL_BIT              BIT(18)
#define NXMAC_ACCEPT_BA_BIT                   BIT(17)
#define NXMAC_ACCEPT_BAR_BIT                  BIT(16)
#define NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT    BIT(15)
#define NXMAC_ACCEPT_BFMEE_FRAMES_BIT         BIT(14)
#define NXMAC_ACCEPT_ALL_BEACON_BIT           BIT(13)
#define NXMAC_ACCEPT_NOT_EXPECTED_BA_BIT      BIT(12)
#define NXMAC_ACCEPT_DECRYPT_ERROR_FRAMES_BIT BIT(11)
#define NXMAC_ACCEPT_BEACON_BIT               BIT(10)
#define NXMAC_ACCEPT_PROBE_RESP_BIT           BIT(9)
#define NXMAC_ACCEPT_PROBE_REQ_BIT            BIT(8)
#define NXMAC_ACCEPT_MY_UNICAST_BIT           BIT(7)
#define NXMAC_ACCEPT_UNICAST_BIT              BIT(6)
#define NXMAC_ACCEPT_ERROR_FRAMES_BIT         BIT(5)
#define NXMAC_ACCEPT_OTHER_BSSID_BIT          BIT(4)
#define NXMAC_ACCEPT_BROADCAST_BIT            BIT(3)
#define NXMAC_ACCEPT_MULTICAST_BIT            BIT(2)
#define NXMAC_DONT_DECRYPT_BIT                BIT(1)
#define NXMAC_EXC_UNENCRYPTED_BIT             BIT(0)

#define NXMAC_MAC_CNTRL_2_ADDR                0x00B08050

#endif /* _PCIE_REG_ACCESS_H_ */