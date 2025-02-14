#ifndef __YT9215S_API_H__
#define __YT9215S_API_H__

#include "yt_error.h"
#include "chipdef_tiger.h"
#include "yt_l2.h"
#include "yt_vlan.h"
#include "yt_port.h"
#include "yt_stat.h"
#include "yt_lag.h"
#include "yt_port_isolation.h"

#define SWITCH_SMI_CONTROLLER_ON_UNIT(unit)   (SWITCH_ACCESOR_CONTROLLER_ON_UNIT(unit).smi_controller)

/* SMI format */
#define REG_ADDR_BIT1_ADDR      0
#define REG_ADDR_BIT1_DATA      1
#define REG_ADDR_BIT0_WRITE     0
#define REG_ADDR_BIT0_READ      1

#define GLOBAL_CTRL1m			        0
#define LOOK_UP_VLAN_SELm		        1
#define SG_PHYm					2
#define PORT_CTRLm				3
#define PORT_STATUSm			        4
#define EXTIF0_MODEm			        5
#define EXTIF1_MODEm			        6
#define PORT_VLAN_CTRLNm		        7
#define L2_LEARN_PER_PORT_CTRLNm	        8
#define L2_LEARN_GLOBAL_CTRLm		        9
#define L2_LAG_LEARN_LIMIT_CTRLNm	        10
#define L2_VLAN_TBLm				11
#define L2_PORT_ISOLATION_CTRLNm		12
#define METER_TIMESLOTm				13
#define METER_CONFIG_TBLm			14
#define EGR_PORT_CTRLNm				15
#define EGR_PORT_VLAN_CTRLNm		        16
#define EGR_TPID_PROFILEm			17
#define QSCH_SHP_SLOT_TIME_CFGm		        18
#define PSCH_SHP_SLOT_TIME_CFGm		        19
#define QSCH_SHP_CFG_TBLm			20
#define PSCH_SHP_CFG_TBLm			21
#define L2_VLAN_INGRESS_FILTER_ENm	        22
#define L2_EGR_VLAN_FILTER_ENm		        23
#define LINK_AGG_GROUPNm                        24
#define LINK_AGG_MEMBERNm                       25
#define LINK_AGG_HASH_CTRLm                     26
#define L2_FDB_TBL_OP_DATA_0_DUMMYm             27
#define L2_FDB_TBL_OP_DATA_1_DUMMYm             28
#define L2_FDB_TBL_OP_DATA_2_DUMMYm             29
#define L2_FDB_TBL_OP_RESULTm                   30
#define L2_FDB_TBL_OPm                          31

#define NUM_MEMS				32

uint32_t hal_mem32_write(uint8_t unit, uint32_t reg_addr, uint32_t reg_value);
uint32_t hal_mem32_read(uint8_t unit, uint32_t reg_addr, uint32_t *reg_value);
yt_ret_t yt_vlan_port_egrTagMode_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port,  yt_egr_tag_mode_t tagMode);
yt_ret_t  yt_vlan_port_igrPvid_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t vid);
yt_ret_t  yt_vlan_port_vidTypeSel_set(yt_unit_t unit, yt_port_t port, yt_vlan_type_t mode);
yt_ret_t  yt_vlan_port_set(yt_unit_t unit,  yt_vlan_t vid,  yt_port_mask_t  member_portmask, yt_port_mask_t  untag_portmask);
yt_ret_t yt_stat_mib_clear(yt_unit_t unit, yt_port_t port);
yt_ret_t yt_stat_mib_clear_all(yt_unit_t unit);
yt_ret_t yt_stat_mib_port_get(yt_unit_t unit, yt_port_t port, yt_stat_mib_port_cnt_t *pcnt);
yt_ret_t yt_port_macAutoNeg_enable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable);
yt_ret_t yt_port_mac_force_set(yt_unit_t unit, yt_port_t port, yt_port_force_ctrl_t port_ctrl);
yt_ret_t yt_port_extif_mode_set(yt_unit_t unit, yt_port_t port, yt_extif_mode_t mode);
yt_ret_t yt_port_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable);
yt_ret_t yt_stat_mib_enable_set (yt_unit_t unit, yt_enable_t enable);
yt_ret_t yt_port_link_status_all_get(yt_unit_t unit, yt_port_t port, yt_port_linkStatus_all_t *pAllLinkStatus);
yt_ret_t  yt_vlan_port_igrPvid_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t *pVid);
yt_ret_t sf_yt_lag_hash_sel_set(yt_unit_t unit, uint8_t hash_mask);
yt_ret_t sf_yt_lag_hash_sel_get(yt_unit_t unit, uint8_t *p_hash_mask);
yt_ret_t yt_lag_group_port_set(yt_unit_t unit, uint8_t groupId, yt_port_mask_t member_portmask);
yt_ret_t yt_lag_group_info_get(yt_unit_t unit, uint8_t groupId, yt_link_agg_group_t *p_laginfo);
yt_ret_t fal_tiger_l2_fdb_uc_withindex_getnext(yt_unit_t unit, uint16_t index, uint16_t *pNext_index, l2_ucastMacAddr_info_t *pUcastMac);
yt_ret_t fal_tiger_l2_fdb_uc_withindex_get(yt_unit_t unit, uint16_t index, l2_ucastMacAddr_info_t *pUcastMac);
yt_ret_t  yt_init(void);
uint32_t yt_exit(uint8_t unit);
#endif
