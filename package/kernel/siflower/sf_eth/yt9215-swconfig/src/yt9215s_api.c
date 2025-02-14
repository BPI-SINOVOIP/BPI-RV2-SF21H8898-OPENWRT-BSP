#include "yt9215s_api.h"
#include "ctrlif.h"
#include "fal_dispatch.h"
#include "fal_tiger_entry.h"
#include "fal_tiger_vlan.h"
#include "fal_tiger_struct.h"
#include "fal_tiger_port.h"
#include "yt_vlan.h"
#include "cal_bprofile.h"
#include "hal_ctrl.h"
#include "fal_init.h"
#include "fal_tiger_l2.h"
#include "fal_tiger_rate.h"
#include "sw_yt9215.h"
#include "fal_tiger_lag.h"
#include "hal_mem.h"

osal_mux g_cfgmux;
uint8_t ghal_mem32_init      = FALSE;
uint32 DEBUG_MODE_K = 0;
uint8_t ghal_reg_table_init  = FALSE;
osal_mux yt_lock[YT_LOCK_ID_MAX];


#ifdef MEM_MODE_CMODEL
hal_reg_tbl_mode_t greg_tbl_mode = HAL_REG_TBL_MODE_CMODEL;
#else
hal_reg_tbl_mode_t greg_tbl_mode = HAL_REG_TBL_MODE_NORMAL;
#endif

const field_id_t global_ctrl1m_field[] = {
    {GLOBAL_CTRL1_METER_ENf,1,0,4},
    {GLOBAL_CTRL1_AC_ENf,1,0,3},
    {GLOBAL_CTRL1_ACL_ENf,1,0,2},
    {GLOBAL_CTRL1_MIB_ENf,1,0,1},
    {GLOBAL_CTRL1_INTERRUP_POLARITYf,1,0,0},
};

const field_id_t look_up_vlan_selm_field[] = {
    {LOOK_UP_VLAN_SEL_LOOK_UP_VLAN_SELf,11,0,0},
};

const field_id_t sg_phym_field[] = {
    {SG_PHY_APPLICATION_MODEf,3,0,7},
    {SG_PHY_PAUSEf,2,0,5},
    {SG_PHY_LINKf,1,0,4},
    {SG_PHY_DUPLEX_MODEf,1,0,3},
    {SG_PHY_SPEED_MODEf,3,0,0},
};

const field_id_t port_ctrlm_field[] = {
    {PORT_CTRL_FLOW_LINK_ANf,1,0,10},
    {PORT_CTRL_AN_LINK_ENf,1,0,9},
    {PORT_CTRL_HALF_FC_ENf,1,0,8},
    {PORT_CTRL_DUPLEX_MODEf,1,0,7},
    {PORT_CTRL_RX_FC_ENf,1,0,6},
    {PORT_CTRL_TX_FC_ENf,1,0,5},
    {PORT_CTRL_RXMAC_ENf,1,0,4},
    {PORT_CTRL_TXMAC_ENf,1,0,3},
    {PORT_CTRL_SPEED_MODEf,3,0,0},
};

const field_id_t extif0_modem_field[] = {
    {EXTIF0_MODE_XMII_MODEf,3,0,29},
    {EXTIF0_MODE_RGMII_DELAY_TYPE_SELf,1,0,28},
    {EXTIF0_MODE_EN_MACMODE_LNKDN_SWHf,1,0,26},
    {EXTIF0_MODE_RGMII_TXC_DLY100_10_ENf,1,0,20},
    {EXTIF0_MODE_XMII_LINK_UPf,1,0,19},
    {EXTIF0_MODE_XMII_PORT_ENf,1,0,18},
    {EXTIF0_MODE_XMII_SOFT_RSTf,1,0,17},
    {EXTIF0_MODE_RGMII_TXC_DELAY_SELf,4,0,13},
    {EXTIF0_MODE_REMII_RX_XMII_SELf,1,0,12},
    {EXTIF0_MODE_XMII_TXC_IN_SELf,1,0,11},
    {EXTIF0_MODE_XMII_RXC_IN_SELf,1,0,10},
    {EXTIF0_MODE_REVERT_TXDf,1,0,9},
    {EXTIF0_MODE_RGMII_TXC_DELAY_ENf,1,0,8},
    {EXTIF0_MODE_RGMII_TXC_OUT_SELf,1,0,7},
    {EXTIF0_MODE_RGMII_RXC_DELAY_SELf,4,0,3},
    {EXTIF0_MODE_RMII_PHY_TXC_OUT_SELf,1,0,2},
    {EXTIF0_MODE_REMII_TXC_OUT_SELf,1,0,1},
    {EXTIF0_MODE_REMII_RXC_OUT_SELf,1,0,0},
};

const field_id_t extif1_modem_field[] = {
    {EXTIF1_MODE_XMII_MODEf,3,0,29},
    {EXTIF1_MODE_RGMII_DELAY_TYPE_SELf,1,0,28},
    {EXTIF1_MODE_EN_MACMODE_LNKDN_SWHf,1,0,26},
    {EXTIF1_MODE_RGMII_TXC_DLY100_10_ENf,1,0,20},
    {EXTIF1_MODE_XMII_LINK_UPf,1,0,19},
    {EXTIF1_MODE_XMII_PORT_ENf,1,0,18},
    {EXTIF1_MODE_XMII_SOFT_RSTf,1,0,17},
    {EXTIF1_MODE_RGMII_TXC_DELAY_SELf,4,0,13},
    {EXTIF1_MODE_REMII_RX_XMII_SELf,1,0,12},
    {EXTIF1_MODE_XMII_TXC_IN_SELf,1,0,11},
    {EXTIF1_MODE_XMII_RXC_IN_SELf,1,0,10},
    {EXTIF1_MODE_REVERT_TXDf,1,0,9},
    {EXTIF1_MODE_RGMII_TXC_DELAY_ENf,1,0,8},
    {EXTIF1_MODE_RGMII_TXC_OUT_SELf,1,0,7},
    {EXTIF1_MODE_RGMII_RXC_DELAY_SELf,4,0,3},
    {EXTIF1_MODE_RMII_PHY_TXC_OUT_SELf,1,0,2},
    {EXTIF1_MODE_REMII_TXC_OUT_SELf,1,0,1},
    {EXTIF1_MODE_REMII_RXC_OUT_SELf,1,0,0},
};

const field_id_t port_vlan_ctrlnm_field[] = {
    {PORT_VLAN_CTRLN_PRIORITY_TAG_SVID_ENf,1,0,31},
    {PORT_VLAN_CTRLN_PRIORITY_TAG_CVID_ENf,1,0,30},
    {PORT_VLAN_CTRLN_DEFAULT_SVIDf,12,0,18},
    {PORT_VLAN_CTRLN_DEFAULT_CVIDf,12,0,6},
    {PORT_VLAN_CTRLN_DEFAULT_SPRIOf,3,0,3},
    {PORT_VLAN_CTRLN_DEFAULT_CPRIOf,3,0,0},
};

const field_id_t l2_vlan_tblm_field[] = {
    {L2_VLAN_TBL_UNTAG_MEMBER_BITMAPf,11,1,8},
    {L2_VLAN_TBL_STP_IDf,4,1,4},
    {L2_VLAN_TBL_SVL_ENf,1,1,3},
    {L2_VLAN_TBL_FID_1f,3,1,0},
    {L2_VLAN_TBL_FID_0f,9,0,23},
    {L2_VLAN_TBL_LEARN_DISABLEf,1,0,22},
    {L2_VLAN_TBL_INT_PRI_VALIDf,1,0,21},
    {L2_VLAN_TBL_INT_PRIf,3,0,18},
    {L2_VLAN_TBL_PORT_MEMBER_BITMAPf,11,0,7},
    {L2_VLAN_TBL_BYPASS_1X_ACCESS_CONTROLf,1,0,6},
    {L2_VLAN_TBL_METER_ENf,1,0,5},
    {L2_VLAN_TBL_METER_IDf,5,0,0},
};

const field_id_t l2_port_isolation_ctrlnm_field[] = {
	{L2_PORT_ISOLATION_CTRLN_ISOLATED_PORT_MASKf,11,0,0},
};

const field_id_t egr_port_vlan_ctrlnm_field[] = {
    {EGR_PORT_VLAN_CTRLN_STAG_KEEP_MODEf,1,0,31},
    {EGR_PORT_VLAN_CTRLN_CTAG_KEEP_MODEf,1,0,30},
    {EGR_PORT_VLAN_CTRLN_STAG_MODEf,3,0,27},
    {EGR_PORT_VLAN_CTRLN_DEFAULT_SVIDf,12,0,15},
    {EGR_PORT_VLAN_CTRLN_CTAG_MODEf,3,0,12},
    {EGR_PORT_VLAN_CTRLN_DEFAULT_CVIDf,12,0,0},
};

const field_id_t port_statusm_field[] = {
    {PORT_STATUS_LINKf,1,0,8},
    {PORT_STATUS_DUPLEX_MODEf,1,0,7},
    {PORT_STATUS_RX_FC_ENf,1,0,6},
    {PORT_STATUS_TX_FC_ENf,1,0,5},
    {PORT_STATUS_RXMAC_ENf,1,0,4},
    {PORT_STATUS_TXMAC_ENf,1,0,3},
    {PORT_STATUS_SPEED_MODEf,3,0,0},
};

const field_id_t l2_learn_per_port_ctrlnm_field[] = {
    {L2_LEARN_PER_PORT_CTRLN_VID_LEARN_MULTI_ENf,1,0,22},
    {L2_LEARN_PER_PORT_CTRLN_VID_LEARN_MODEf,1,0,21},
    {L2_LEARN_PER_PORT_CTRLN_VID_LEARN_ENf,1,0,20},
    {L2_LEARN_PER_PORT_CTRLN_SUSPEND_COPY_ENf,1,0,19},
    {L2_LEARN_PER_PORT_CTRLN_SUSPEND_DROP_ENf,1,0,18},
    {L2_LEARN_PER_PORT_CTRLN_LEARN_DISABLEf,1,0,17},
    {L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_ENf,1,0,16},
    {L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_NUMf,13,0,3},
    {L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_DROPf,1,0,2},
    {L2_LEARN_PER_PORT_CTRLN_LEARN_MODEf,2,0,0},
};

const field_id_t l2_learn_global_ctrlm_field[] = {
    {L2_LEARN_GLOBAL_CTRL_LEARN_MODEf,2,0,17},
    {L2_LEARN_GLOBAL_CTRL_GIP_LEARN_OVERWRITE_ALLOWf,1,0,16},
    {L2_LEARN_GLOBAL_CTRL_SA_LEARN_OVERWRITE_ALLOWf,1,0,15},
    {L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_ENf,1,0,14},
    {L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_NUMf,13,0,1},
    {L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_DROPf,1,0,0},
};

const field_id_t l2_lag_learn_limit_ctrlnm_field[] = {
    {L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_ENf,1,0,14},
    {L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_NUMf,13,0,1},
    {L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_DROPf,1,0,0},
};

const field_id_t meter_timeslotm_field[] = {
    {METER_TIMESLOT_TIMESLOTf,12,0,0},
};

const field_id_t meter_config_tblm_field[] = {
    {METER_CONFIG_TBL_METER_ENf,1,2,14},
    {METER_CONFIG_TBL_CFf,1,2,13},
    {METER_CONFIG_TBL_DROP_COLORf,2,2,11},
    {METER_CONFIG_TBL_COLOR_MODEf,1,2,10},
    {METER_CONFIG_TBL_TOKEN_UNITf,3,2,7},
    {METER_CONFIG_TBL_BYTE_RATE_MODEf,1,2,6},
    {METER_CONFIG_TBL_RATE_MODEf,1,2,5},
    {METER_CONFIG_TBL_METER_MODEf,1,2,4},
    {METER_CONFIG_TBL_CBS_1f,4,2,0},
    {METER_CONFIG_TBL_CBS_0f,12,1,20},
    {METER_CONFIG_TBL_CIRf,18,1,2},
    {METER_CONFIG_TBL_EBS_1f,2,1,0},
    {METER_CONFIG_TBL_EBS_0f,14,0,18},
    {METER_CONFIG_TBL_EIRf,18,0,0},
};

const field_id_t egr_port_ctrlnm_field[] = {
    {EGR_PORT_CTRLN_CFI_REMARK_ENf,1,0,10},
    {EGR_PORT_CTRLN_DEI_REMARK_ENf,1,0,9},
    {EGR_PORT_CTRLN_DSCP_REMARK_ENf,1,0,8},
    {EGR_PORT_CTRLN_CPRIO_REMARK_ENf,1,0,7},
    {EGR_PORT_CTRLN_SPRIO_REMARK_ENf,1,0,6},
    {EGR_PORT_CTRLN_CTAG_TPID_SELf,2,0,4},
    {EGR_PORT_CTRLN_STAG_TPID_SELf,2,0,2},
    {EGR_PORT_CTRLN_CPRIO_REMARK_SELf,1,0,1},
    {EGR_PORT_CTRLN_SPRIO_REMARK_SELf,1,0,0},
};

const field_id_t egr_tpid_profilem_field[] = {
    {EGR_TPID_PROFILE_TPIDf,16,0,0},
};

const field_id_t qsch_shp_slot_time_cfgm_field[] = {
    {QSCH_SHP_SLOT_TIME_CFG_QSCH_SHP_SLOT_TIMEf,12,0,0},
};

const field_id_t psch_shp_slot_time_cfgm_field[] = {
    {PSCH_SHP_SLOT_TIME_CFG_PSCH_SHP_SLOT_TIMEf,12,0,0},
};

const field_id_t qsch_shp_cfg_tblm_field[] = {
    {QSCH_SHP_CFG_TBL_CFf,1,2,6},
    {QSCH_SHP_CFG_TBL_E_SHAPER_ENf,1,2,5},
    {QSCH_SHP_CFG_TBL_C_SHAPER_ENf,1,2,4},
    {QSCH_SHP_CFG_TBL_SHAPER_MODEf,1,2,3},
    {QSCH_SHP_CFG_TBL_TOKEN_LEVELf,3,2,0},
    {QSCH_SHP_CFG_TBL_EBSf,14,1,18},
    {QSCH_SHP_CFG_TBL_EIRf,18,1,0},
    {QSCH_SHP_CFG_TBL_CBSf,14,0,18},
    {QSCH_SHP_CFG_TBL_CIRf,18,0,0},
};

const field_id_t psch_shp_cfg_tblm_field[] = {
    {PSCH_SHP_CFG_TBL_C_SHAPER_ENf,1,1,4},
    {PSCH_SHP_CFG_TBL_SHAPER_MODEf,1,1,3},
    {PSCH_SHP_CFG_TBL_TOKEN_LEVELf,3,1,0},
    {PSCH_SHP_CFG_TBL_CBSf,14,0,18},
    {PSCH_SHP_CFG_TBL_CIRf,18,0,0},
};

const field_id_t l2_vlan_ingress_filter_enm_field[] = {
    {L2_VLAN_INGRESS_FILTER_EN_IGMP_BYPASS_ENf,11,0,11},
    {L2_VLAN_INGRESS_FILTER_EN_FILTER_ENf,11,0,0},
};

const field_id_t l2_egr_vlan_filter_enm_field[] = {
    {L2_EGR_VLAN_FILTER_EN_FILTER_ENf,11,0,0},
};

const field_id_t link_agg_groupnm_field[] = {
    {LINK_AGG_GROUPN_PORT_MASKf,11,0,3},
    {LINK_AGG_GROUPN_MEMBER_NUMf,3,0,0},
};

const field_id_t link_agg_membernm_field[] = {
    {LINK_AGG_MEMBERN_PORTf,4,0,0},
};

const field_id_t link_agg_hash_ctrlm_field[] = {
    {LINK_AGG_HASH_CTRL_HASH_FIELD_SELf,8,0,0},
};

const field_id_t l2_fdb_tbl_op_data_0_dummym_field[] = {
    {L2_FDB_TBL_OP_DATA_0_DUMMY_MAC_DA_0f,32,0,0},
};

const field_id_t l2_fdb_tbl_op_data_1_dummym_field[] = {
    {L2_FDB_TBL_OP_DATA_1_DUMMY_DMAC_INT_PRI_ENf,1,0,31},
    {L2_FDB_TBL_OP_DATA_1_DUMMY_STATUSf,3,0,28},
    {L2_FDB_TBL_OP_DATA_1_DUMMY_FIDf,12,0,16},
    {L2_FDB_TBL_OP_DATA_1_DUMMY_MAC_DA_1f,16,0,0},
};

const field_id_t l2_fdb_tbl_op_data_2_dummym_field[] = {
    {L2_FDB_TBL_OP_DATA_2_DUMMY_MOVE_AGING_STATUSf,2,0,30},
    {L2_FDB_TBL_OP_DATA_2_DUMMY_SMAC_DROPf,1,0,29},
    {L2_FDB_TBL_OP_DATA_2_DUMMY_DST_PORT_MASKf,11,0,18},
    {L2_FDB_TBL_OP_DATA_2_DUMMY_DMAC_DROPf,1,0,17},
    {L2_FDB_TBL_OP_DATA_2_DUMMY_COPY_TO_CPUf,1,0,16},
    {L2_FDB_TBL_OP_DATA_2_DUMMY_SMAC_INT_PRI_ENf,1,0,15},
    {L2_FDB_TBL_OP_DATA_2_DUMMY_INT_PRIf,3,0,12},
    {L2_FDB_TBL_OP_DATA_2_DUMMY_NEW_VIDf,12,0,0},
};

const field_id_t l2_fdb_tbl_op_resultm_field[] = {
    {L2_FDB_TBL_OP_RESULT_OP_DONEf,1,0,15},
    {L2_FDB_TBL_OP_RESULT_LOOKUP_FAILf,1,0,14},
    {L2_FDB_TBL_OP_RESULT_OVERWRITEf,1,0,13},
    {L2_FDB_TBL_OP_RESULT_OP_RESULTf,1,0,12},
    {L2_FDB_TBL_OP_RESULT_ENTRY_INDEXf,12,0,0},
};

const field_id_t l2_fdb_tbl_opm_field[] = {
    {L2_FDB_TBL_OP_ENTRY_INDEXf,12,0,11},
    {L2_FDB_TBL_OP_OP_MODEf,1,0,10},
    {L2_FDB_TBL_OP_FLUSH_MODEf,3,0,7},
    {L2_FDB_TBL_OP_FLUSH_STATIC_ENf,1,0,6},
    {L2_FDB_TBL_OP_GET_NEXT_TYPEf,2,0,4},
    {L2_FDB_TBL_OP_OP_CMDf,3,0,1},
    {L2_FDB_TBL_OP_OP_STARTf,1,0,0},
};

const tbl_reg_info_t tbl_reg_list[] = {
		{/* mem: global_ctrl1 */
        /* Asic Base Address */ 0x80004,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 5,
        /* fields_id */ global_ctrl1m_field,
        },
		{/* mem: look_up_vlan_sel */
        /* Asic Base Address */ 0x80014,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 1,
        /* fields_id */ look_up_vlan_selm_field,
        },
		{/* mem: sg_phy */
        /* Asic Base Address */ 0x8008c,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 2,
        /* Fields_num */ 5,
        /* fields_id */ sg_phym_field,
        },
		{/* mem: port_ctrl */
        /* Asic Base Address */ 0x80100,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 11,
        /* Fields_num */ 9,
        /* fields_id */ port_ctrlm_field,
        },
		{/* mem: port_status */
        /* Asic Base Address */ 0x80200,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 11,
        /* Fields_num */ 7,
        /* fields_id */ port_statusm_field,
        },
		{/* mem: extif0_mode */
        /* Asic Base Address */ 0x80400,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 18,
        /* fields_id */ extif0_modem_field,
        },
		{/* mem: extif1_mode */
        /* Asic Base Address */ 0x80408,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 18,
        /* fields_id */ extif1_modem_field,
        },
		{/* mem: port_vlan_ctrln */
        /* Asic Base Address */ 0x230010,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 11,
        /* Fields_num */ 6,
        /* fields_id */ port_vlan_ctrlnm_field,
        },
		{/* mem: l2_learn_per_port_ctrln */
        /* Asic Base Address */ 0x1803d0,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 11,
        /* Fields_num */ 10,
        /* fields_id */ l2_learn_per_port_ctrlnm_field,
        },
		{/* mem: l2_learn_global_ctrl */
        /* Asic Base Address */ 0x180438,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 6,
        /* fields_id */ l2_learn_global_ctrlm_field,
        },
		{/* mem: l2_lag_learn_limit_ctrln */
        /* Asic Base Address */ 0x180808,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 2,
        /* Fields_num */ 3,
        /* fields_id */ l2_lag_learn_limit_ctrlnm_field,
        },
		{/* mem: l2_vlan_tbl */
        /* Asic Base Address */ 0x188000,
        /* offset */ 0x8,
        /* bytes */ 0x8,
        /* Entry Number */ 4096,
        /* Fields_num */ 12,
        /* fields_id */ l2_vlan_tblm_field,
        },
	{/* mem: l2_port_isolation_ctrln */
	/* Asic Base Address */ 0x180294,
	/* offset */ 0x4,
	/* bytes */ 0x4,
	/* Entry Number */ 11,
	/* Fields_num */ 1,
	/* fields_id */ l2_port_isolation_ctrlnm_field,
	},
		{/* mem: meter_timeslot */
        /* Asic Base Address */ 0x220104,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 1,
        /* fields_id */ meter_timeslotm_field,
        },
		{/* mem: meter_config_tbl */
        /* Asic Base Address */ 0x220800,
        /* offset */ 0x10,
        /* bytes */ 0xc,
        /* Entry Number */ 75,
        /* Fields_num */ 14,
        /* fields_id */ meter_config_tblm_field,
        },
		{/* mem: egr_port_ctrln */
        /* Asic Base Address */ 0x100000,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 11,
        /* Fields_num */ 9,
        /* fields_id */ egr_port_ctrlnm_field,
        },
		{/* mem: egr_port_vlan_ctrln */
        /* Asic Base Address */ 0x100080,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 11,
        /* Fields_num */ 6,
        /* fields_id */ egr_port_vlan_ctrlnm_field,
        },
		{/* mem: egr_tpid_profile */
        /* Asic Base Address */ 0x100300,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 4,
        /* Fields_num */ 1,
        /* fields_id */ egr_tpid_profilem_field,
        },
		{/* mem: qsch_shp_slot_time_cfg */
        /* Asic Base Address */ 0x340008,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 1,
        /* fields_id */ qsch_shp_slot_time_cfgm_field,
        },
        {/* mem: psch_shp_slot_time_cfg */
        /* Asic Base Address */ 0x34000c,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 1,
        /* fields_id */ psch_shp_slot_time_cfgm_field,
        },
		{/* mem: qsch_shp_cfg_tbl */
        /* Asic Base Address */ 0x34c000,
        /* offset */ 0x10,
        /* bytes */ 0xc,
        /* Entry Number */ 132,
        /* Fields_num */ 9,
        /* fields_id */ qsch_shp_cfg_tblm_field,
        },
		{/* mem: psch_shp_cfg_tbl */
        /* Asic Base Address */ 0x354000,
        /* offset */ 0x8,
        /* bytes */ 0x8,
        /* Entry Number */ 11,
        /* Fields_num */ 5,
        /* fields_id */ psch_shp_cfg_tblm_field,
        },
		{/* mem: l2_vlan_ingress_filter_en */
		/* Asic Base Address */ 0x180280,
		/* offset */ 0x4,
		/* bytes */ 0x4,
		/* Entry Number */ 1,
		/* Fields_num */ 2,
		/* fields_id */ l2_vlan_ingress_filter_enm_field,
		},
		{/* mem: l2_egr_vlan_filter_en */
		/* Asic Base Address */ 0x180598,
		/* offset */ 0x4,
		/* bytes */ 0x4,
		/* Entry Number */ 1,
		/* Fields_num */ 1,
		/* fields_id */ l2_egr_vlan_filter_enm_field,
		},
        {/* mem: link_agg_groupn */
        /* Asic Base Address */ 0x1805a8,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 2,
        /* Fields_num */ 2,
        /* fields_id */ link_agg_groupnm_field,
        },
        {/* mem: link_agg_membern */
        /* Asic Base Address */ 0x1805b0,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 8,
        /* Fields_num */ 1,
        /* fields_id */ link_agg_membernm_field,
        },
        {/* mem: link_agg_hash_ctrl */
        /* Asic Base Address */ 0x210090,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 1,
        /* fields_id */ link_agg_hash_ctrlm_field,
        },
        {/* mem: l2_fdb_tbl_op_data_0_dummy */
        /* Asic Base Address */ 0x1804b0,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 1,
        /* fields_id */ l2_fdb_tbl_op_data_0_dummym_field,
        },
        {/* mem: l2_fdb_tbl_op_data_1_dummy */
        /* Asic Base Address */ 0x1804b4,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 4,
        /* fields_id */ l2_fdb_tbl_op_data_1_dummym_field,
        },
        {/* mem: l2_fdb_tbl_op_data_2_dummy */
        /* Asic Base Address */ 0x1804b8,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 8,
        /* fields_id */ l2_fdb_tbl_op_data_2_dummym_field,
        },
        {/* mem: l2_fdb_tbl_op_result */
        /* Asic Base Address */ 0x180464,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 5,
        /* fields_id */ l2_fdb_tbl_op_resultm_field,
        },
        {/* mem: l2_fdb_tbl_op */
        /* Asic Base Address */ 0x180460,
        /* offset */ 0x4,
        /* bytes */ 0x4,
        /* Entry Number */ 1,
        /* Fields_num */ 7,
        /* fields_id */ l2_fdb_tbl_opm_field,
        },
};

#define DRV_MEM_DS_VALID_CHK(mem_id, idx) \
do {\
    CMM_PARAM_CHK((NUM_MEMS <= mem_id), CMM_ERR_INPUT);\
    if((sizeof(tbl_reg_list) / sizeof(tbl_reg_list[0])) < NUM_MEMS) \
    {\
        osal_printf("Mem size %u is less than %u\r\n", (uint32_t)(sizeof(tbl_reg_list)/sizeof(tbl_reg_list[0])), NUM_MEMS);\
        return CMM_ERR_REG_TABLE_NUM;\
    }\
    if(tbl_reg_list[mem_id].entry_number<= idx )\
    {\
        osal_printf("Mem %u entry num %u is less than %u\r\n", mem_id, tbl_reg_list[mem_id].entry_number , idx);\
        return CMM_ERR_REG_TABLE_IDX;\
    }\
} while(0)

yt_serdes_mode_t cal_serdes_mode_get(yt_unit_t unit, yt_port_t port)
{
    uint8_t sds_id;

    if(!CMM_PORT_VALID(unit, port))
    {
        return INVALID_ID;
    }

    sds_id = UNITINFO(unit)->pPortDescp[port]->serdes_index;
    if(sds_id != INVALID_ID)
    {
        return UNITINFO(unit)->pSerdesDescp[sds_id]->mode;
    }

    return INVALID_ID;
}

yt_bool_t cal_is_combo_port(yt_unit_t unit, yt_port_t port)
{
    yt_port_medium_t media;

    if(!CMM_PORT_VALID(unit, port))
    {
        return INVALID_ID;
    }

    media = UNITINFO(unit)->pPortDescp[port]->medium;

    if(PORT_MEDI_COMBO_FIBER == media ||
        PORT_MEDI_COMBO_COPPER == media)
    {
        return TRUE;
    }

    return FALSE;
}

static yt_ret_t fal_tiger_patch_qos_init(yt_unit_t unit)
{
    uint32_t port;
    uint32_t qid;
    uint32_t regAddr;
    int32_t ret = 0;

    for (port = 0; port < FAL_MAX_PORT_NUM; port++)
    {
        for (qid = 0; qid < CAL_MAX_UCAST_QUEUE_NUM(unit); qid++)
        {
            regAddr = QOS_FORCEAC_UCASTQUE_REG(unit, port, qid);
            CMM_ERR_CHK(hal_mem32_write(unit, regAddr, 0x80c060c), ret);
            CMM_ERR_CHK(hal_mem32_write(unit, regAddr + 4, 0x9000000), ret);
        }

        for (qid = 0; qid < CAL_MAX_MCAST_QUEUE_NUM(unit); qid++)
        {
            regAddr = QOS_FORCEAC_MCASTQUE_REG(unit, port, qid);
            CMM_ERR_CHK(hal_mem32_write(unit, regAddr, 0x1400060c), ret);
        }
    }

    return CMM_ERR_OK;
}

static yt_ret_t fal_tiger_patch_init(yt_unit_t unit)
{
    uint8_t i;
    uint32_t regData;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    global_ctrl1_t gEntry;

    uint8_t intTuneVal[4] = {0x07, 0x00, 0x00, 0x00};

#ifdef MEM_MODE_CMODEL
    return CMM_ERR_OK;
#endif
    /* flow control */
    for(i = 0; i < FAL_MAX_PORT_NUM; i++)
    {
        hal_mem32_write(unit, 0x281000+i*8, 0x8040284B);
        hal_mem32_write(unit, 0x281004+i*8, 0x26F1B);
    }
    for(i = 0; i < 4; i++)
    {
        hal_mem32_write(unit, 0x2801D0+i*4, 0x14A);
        hal_mem32_write(unit, 0x180904+i*4, intTuneVal[i]);
    }

#if defined(BOARD_YT9215RB_YT8531_DEMO) || defined(BOARD_YT9215S_YT8531_FIB_DEMO)
    /*select mdio grp1 pin*/
    hal_mem32_read(unit, 0x80358, &regData);
    regData |= (1 << 2);
    hal_mem32_write(unit, 0x80358, regData);
#endif
    /*for xmii on low temp*/
    hal_mem32_read(unit, 0x80400, &regData);
    regData |= (1 << 11);
    hal_mem32_write(unit, 0x80400, regData);

    /*disable internal cpu port learn fdb*/
    hal_table_reg_read(unit, L2_LEARN_PER_PORT_CTRLNm, FAL_INTERNAL_CPU_MACID, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl);
    HAL_FIELD_SET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_DISABLEf, &l2_learn_per_port_ctrl, YT_ENABLE);
    hal_table_reg_write(unit, L2_LEARN_PER_PORT_CTRLNm, FAL_INTERNAL_CPU_MACID, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl);

    hal_table_reg_read(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &gEntry);
    HAL_FIELD_SET(GLOBAL_CTRL1m, GLOBAL_CTRL1_ACL_ENf, &gEntry, YT_ENABLE);
    hal_table_reg_write(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &gEntry);

    /*
    * disable sds intf,will be enabled when config sds mode
    * set to mdio access mode
    */
    hal_mem32_read(unit, CHIP_INTERFACE_CTRL_REG, &regData);
    regData &= ~(0x43<<0);
    hal_mem32_write(unit, CHIP_INTERFACE_CTRL_REG, regData);

    fal_tiger_patch_qos_init(unit);
#ifdef PORT_INCLUDED
    fal_tiger_port_init(unit);
#endif

    return CMM_ERR_OK;
}

yt_ret_t CMM_CALSdsMode_to_YTExtMode(yt_serdes_mode_t sdsMode, yt_extif_mode_t *pExtifMode)
{
    switch(sdsMode)
    {
        case SERDES_MODE_DISABLE:
            *pExtifMode = YT_EXTIF_MODE_SG_DISABLE;
            break;
        case SERDES_MODE_SGMII_MAC:
            *pExtifMode = YT_EXTIF_MODE_SG_MAC;
            break;
        case SERDES_MODE_SGMII_PHY:
            *pExtifMode = YT_EXTIF_MODE_SG_PHY;
            break;
        case SERDES_MODE_1000BX:
            *pExtifMode = YT_EXTIF_MODE_FIB_1000;
            break;
        case SERDES_MODE_100B_FX:
            *pExtifMode = YT_EXTIF_MODE_FIB_100;
            break;
        case SERDES_MODE_2500BX:
            *pExtifMode = YT_EXTIF_MODE_BX2500;
            break;
        case SERDES_MODE_SGMII_AS:
            *pExtifMode = YT_EXTIF_MODE_SGFIB_AS;
            break;
        default:
            return CMM_ERR_FAIL;
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_port_phyCombo_mode_set(yt_unit_t unit, yt_port_t port, yt_combo_mode_t mode)
{
    yt_port_t phy_addr;
    yt_macid_t mac_id;

    mac_id = CAL_YTP_TO_MAC(unit, port);
    phy_addr = CAL_YTP_TO_PHYADDR(unit, port);

    if(phy_addr != INVALID_ID)
    {
        if(HALPHYDRV(unit, mac_id) != NULL)
        {
            HALPHYDRV_FUNC(unit, mac_id)->phy_combo_mode_set(unit, phy_addr, mode);
            return CMM_ERR_OK;
        }
    }

    return CMM_ERR_FAIL;
}

/* config interfaces according to board profile */
static yt_ret_t fal_tiger_port_interface_init(yt_unit_t unit)
{
#ifdef PORT_INCLUDED
    yt_port_t port;
    yt_port_attri_t attr;
    yt_serdes_mode_t sdsMode;
    yt_extif_mode_t extifMode;
    uint8_t extif_id = 0;
#endif

#ifdef PORT_INCLUDED
    for(port = 0; port < CAL_PORT_NUM_ON_UNIT(unit); port++)
    {
        attr = CAL_PORT_ATTRIBUTE(unit, port);
        switch(attr)
        {
            case PORT_ATTR_EXT_RGMII:
                yt_port_extif_mode_set(unit, port, YT_EXTIF_MODE_RGMII);
                break;
            case PORT_ATTR_EXT_RMII_MAC:
                yt_port_extif_mode_set(unit, port, YT_EXTIF_MODE_RMII_MAC);
                break;
            case PORT_ATTR_EXT_RMII_PHY:
                yt_port_extif_mode_set(unit, port, YT_EXTIF_MODE_RMII_PHY);
                break;
            case PORT_ATTR_EXT_MII:
                yt_port_extif_mode_set(unit, port, YT_EXTIF_MODE_MII);
                break;
            case PORT_ATTR_ETH:
                /*ext port*/
                if(CAL_PORT_TYPE_EXT == CAL_YTP_PORT_TYPE(unit, port))
                {
                    /*config serdes*/
                    if(CAL_IS_SERDES(unit, port))
                    {
                        sdsMode = CAL_SERDES_MODE(unit, port);
                        if(CMM_ERR_OK == CMM_CALSdsMode_to_YTExtMode(sdsMode, &extifMode))
                        {
                            yt_port_extif_mode_set(unit, port, extifMode);
                        }
                        /*TODO:check if sg phy,config phy if necessary*/
                    }

                    if(CAL_IS_PHY_PORT(unit, port))
                    {
                        yt_port_extif_mode_set(unit, port, YT_EXTIF_MODE_RGMII);
                        if(CAL_IS_YTPHY(unit, port))
                        {
                            extif_id = CAL_YTP_TO_EXTPORT(unit, port);
                            hal_mem32_write(unit, 0x8035C+4*extif_id, TRUE);/*enable rgmii AN*/
                        }
                        else
                        {
                            /*TODO:set mac force and start polling*/
                        }
                    }
                }
                /*config combo port*/
                if(CAL_IS_COMBO_PORT(unit, port))
                {
                    /*phy combo mode*/
                    if(CAL_IS_PHY_PORT(unit, port))
                    {
                        if(PORT_MEDI_COMBO_FIBER == CAL_PORT_MEDIUM(unit, port))
                        {
                            fal_tiger_port_phyCombo_mode_set(unit, port, COMBO_MODE_FIBER_FIRST);
                        }
                        else if(PORT_MEDI_COMBO_COPPER == CAL_PORT_MEDIUM(unit, port))
                        {
                            fal_tiger_port_phyCombo_mode_set(unit, port, COMBO_MODE_COPPER_FIRST);
                        }
                    }
                    /*TODO:other*/
                }
                /*enable port*/
                if(CAL_IS_PHY_PORT(unit, port))
                {
                    yt_port_enable_set(unit, port, YT_ENABLE);
                }
                break;
            case PORT_ATTR_INT_CPU:
            case PORT_ATTR_CASCADE:
                break;
            default:
                break;
        }
    }
#else
    CMM_UNUSED_PARAM(unit);
#endif
    return CMM_ERR_OK;
}


/**
 * @internal      fal_tiger_led_init
 * @endinternal
 *
 * @brief         init LED
 * @param[in]     unit                -unit id
 * @retval        CMM_ERR_OK          -on success
 * @retval        CMM_ERR_FAIL        -on fail
 */
static yt_ret_t  fal_tiger_led_init(yt_unit_t unit)
{
#ifdef LED_INCLUDED
    uint32_t ret;
    uint32_t regAddr;
    uint32_t regVal;
    uint32_t port;
    uint8_t index;
    uint8_t portNum;
    uint8_t ledNum;
    yt_sled_dataNum_t dataNum;
    yt_led_remapping_t dstInfo;
    const yt_sled_remapInfo_t *sInfo = SLED_PARAM(unit)->pRemapInfo;

    if (NULL == LEDDSCP_ON_UNIT(unit))
    {
        return CMM_ERR_NULL_POINT;
    }

    fal_tiger_led_mode_set(unit, LED_MODE(unit));

    /* serial mode */
    if (LED_MODE_SERIAL == LED_MODE(unit))
    {
        if (NULL == SLED_PARAM(unit))
        {
            return CMM_ERR_NULL_POINT;
        }

        /* active mode */
        fal_tiger_led_serial_activeMode_set(unit, SLED_PARAM(unit)->activeMode);

        dataNum = SLED_PARAM(unit)->dataNum;
        switch(dataNum)
        {
            case SLED_DATANUM_5:
                portNum = 5;
                ledNum = 1;
                break;

            case SLED_DATANUM_7:
                portNum = 7;
                ledNum = 1;
                break;

           case SLED_DATANUM_10:
                portNum = 5;
                ledNum = 2;
                break;

            case SLED_DATANUM_14:
                portNum = 7;
                ledNum = 2;
                break;

            case SLED_DATANUM_15:
                portNum = 5;
                ledNum = 3;
                break;

            case SLED_DATANUM_21:
                portNum = 7;
                ledNum = 3;
                break;

            default:
                return CMM_ERR_NOT_SUPPORT;
        }

        /* serial port num-- start */
        ret = hal_mem32_read(unit, LED_GLB_CTRL, &regVal);
        if (CMM_ERR_OK != ret)
        {
            return CMM_ERR_FAIL;
        }
        regVal &= 0xfffe1fff;
        regVal |= ((portNum & 0xf) << 13);
        hal_mem32_write(unit, LED_GLB_CTRL, regVal);
        /* serial port num-- end */

        /* serial pin num-- start */
        ret = hal_mem32_read(unit, LED_SERIAL_CTRL, &regVal);
        if (CMM_ERR_OK != ret)
        {
            return CMM_ERR_FAIL;
        }
        regVal &= 0xfffffffc;
        regVal |= ((ledNum - 1) & 0x3);
        hal_mem32_write(unit, LED_SERIAL_CTRL, regVal);
        /* serial pin num-- end */

        /* remaping */
        for (index = 0; index < SLED_PARAM(unit)->remapInfoNum; index++)
        {
            sInfo = SLED_PARAM(unit)->pRemapInfo + index;
            dstInfo.port = sInfo->port;
            dstInfo.ledId = sInfo->ledId;
            fal_tiger_led_serial_remapping_set(unit, index, dstInfo);
        }
    }

    /* preset disable_link_try bit of LED0 action(exclude CPU_PORT) */
    for (port = 0; port < FAL_MAX_PORT_NUM - 1; port++)
    {
        regAddr = LED_CTRL_0_BASE + port * 4;
        ret = hal_mem32_read(unit, regAddr, &regVal);
        if (CMM_ERR_OK != ret)
        {
            return CMM_ERR_FAIL;
        }

        regVal |= 0x20000;
        hal_mem32_write(unit, regAddr, regVal);
    }
#else
    CMM_UNUSED_PARAM(unit);
#endif

    return CMM_ERR_OK;
}
yt_ret_t fal_tiger_init(yt_unit_t unit)
{
    fal_tiger_patch_init(unit);
    fal_tiger_led_init(unit);
    fal_tiger_port_interface_init(unit);
    return CMM_ERR_OK;
}

yt_ret_t profile_yt9215s_fib_init(yt_hwProfile_info_t *hwprofile_info);

yt_fal_init_info_t gFal_init_info[] =
{
    {YT_SW_ID_9215, fal_tiger_init},
};

yt_ret_t int_phy_ext_reg_read(yt_unit_t unit, uint8_t phy_addr, uint16_t regAddr, uint16_t *pData)
{
    if(HALSWDRV(unit) == NULL || pData == NULL)
    {
        return CMM_ERR_FAIL;
    }

    HALSWDRV_FUNC(unit)->switch_intif_write(unit, phy_addr, 0x1e, regAddr);
    HALSWDRV_FUNC(unit)->switch_intif_read(unit, phy_addr, 0x1f, pData);

    return CMM_ERR_OK;
}

yt_ret_t int_phy_ext_reg_write(yt_unit_t unit, uint8_t phy_addr, uint16_t regAddr, uint16_t data)
{
    if(HALSWDRV(unit) == NULL)
    {
        return CMM_ERR_FAIL;
    }

    HALSWDRV_FUNC(unit)->switch_intif_write(unit, phy_addr, 0x1e, regAddr);
    HALSWDRV_FUNC(unit)->switch_intif_write(unit, phy_addr, 0x1f, data);

    return CMM_ERR_OK;
}

yt_ret_t ext_phy_mmd_reg_read(yt_unit_t unit, uint8_t phy_addr, uint8_t mmd_id, uint16_t regAddr, uint16_t *pData)
{
    if(HALSWDRV(unit) == NULL || pData == NULL)
    {
        return CMM_ERR_FAIL;
    }

    HALSWDRV_FUNC(unit)->switch_extif_write(unit, phy_addr, 0xd, mmd_id);
    HALSWDRV_FUNC(unit)->switch_extif_write(unit, phy_addr, 0xe, regAddr);
    HALSWDRV_FUNC(unit)->switch_extif_write(unit, phy_addr, 0xd, 0x4000+mmd_id);
    HALSWDRV_FUNC(unit)->switch_extif_read(unit, phy_addr, 0xe, pData);

    return CMM_ERR_OK;
}

yt_ret_t int_serdes_init(yt_unit_t unit)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t phyAddr;
    static yt_bool_t initFlag = 0;
    uint16_t phyData;
    yt_port_t port;

    if(initFlag)
    {
        return CMM_ERR_OK;
    }

    for(port = 0; port < CAL_PORT_NUM_ON_UNIT(unit); port++)
    {
        if (!CAL_IS_SERDES(unit, port))
        {
            continue;
        }

        phyAddr = CAL_YTP_TO_PHYADDR(unit, port);
        if (phyAddr == INVALID_ID)
        {
            continue;
        }
        CMM_ERR_CHK(int_phy_ext_reg_read(unit, phyAddr, 0xa0, &phyData), ret);
        if (IS_BIT_SET(phyData, 14))
        {
            CLEAR_BIT(phyData, 14);
            CMM_ERR_CHK(int_phy_ext_reg_write(unit, phyAddr, 0xa0, phyData), ret);
        }
    }
    initFlag = 1;

    return CMM_ERR_OK;
}

yt_ret_t int_serdes_restart(yt_unit_t unit, uint8_t phyAddr)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_enable_set(yt_unit_t unit, uint8_t phyAddr, yt_enable_t enable)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint16_t phyData = 0;

    CMM_ERR_CHK(HALSWDRV_FUNC(unit)->switch_intif_read(unit, phyAddr, PHY_BASE_CTRL_REG_0, &phyData), ret);
    if(YT_ENABLE == enable)
    {
        CLEAR_BIT(phyData, 11);
        SET_BIT(phyData, 9);
    }
    else
    {
        SET_BIT(phyData, 11);
    }
    CMM_ERR_CHK(HALSWDRV_FUNC(unit)->switch_intif_write(unit, phyAddr, PHY_BASE_CTRL_REG_0, phyData), ret);

    return CMM_ERR_OK;
}

yt_ret_t int_serdes_enable_get(yt_unit_t unit, uint8_t phyAddr, yt_enable_t *pEnable)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint16_t phyData = 0;

    CMM_ERR_CHK(HALSWDRV_FUNC(unit)->switch_intif_read(unit, phyAddr, PHY_BASE_CTRL_REG_0, &phyData), ret);
    *pEnable = IS_BIT_SET(phyData, 11) ? YT_DISABLE : YT_ENABLE;

    return CMM_ERR_OK;
}

yt_ret_t int_serdes_medium_set(yt_unit_t unit, uint8_t phyAddr, yt_port_medium_t medium)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(medium);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_medium_get(yt_unit_t unit, uint8_t phyAddr, yt_port_medium_t *pMedium)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pMedium);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_autoNeg_enable_set(yt_unit_t unit, uint8_t phyAddr, yt_enable_t enable)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(enable);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_autoNeg_enable_get(yt_unit_t unit, uint8_t phyAddr, yt_enable_t *pEnable)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pEnable);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_autoNeg_ability_set(yt_unit_t unit, uint8_t phyAddr, yt_port_an_ability_t ability)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(ability);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_autoNeg_ability_get(yt_unit_t unit, uint8_t phyAddr, yt_port_an_ability_t *pAbility)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pAbility);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_force_speed_duplex_set(yt_unit_t unit, uint8_t phyAddr, yt_port_speed_duplex_t speed_dup)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(speed_dup);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_force_speed_duplex_get(yt_unit_t unit, uint8_t phyAddr, yt_port_speed_duplex_t *pSpeedDup)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pSpeedDup);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_link_status_get(yt_unit_t unit, uint8_t phyAddr, yt_port_linkStatus_all_t *pLinkStatus)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pLinkStatus);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_eee_enable_set(yt_unit_t unit, uint8_t phyAddr, yt_enable_t enable)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(enable);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_eee_enable_get(yt_unit_t unit, uint8_t phyAddr, yt_enable_t *pEnable)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pEnable);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_combo_mode_set(yt_unit_t unit, uint8_t phyAddr, yt_combo_mode_t mode)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(mode);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_combo_mode_get(yt_unit_t unit, uint8_t phyAddr, yt_combo_mode_t *pMode)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pMode);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_cable_diag(yt_unit_t unit, uint8_t phyAddr, yt_port_cableDiag_t *pCableDiagStatus)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pCableDiagStatus);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_interrupt_status_get(yt_unit_t unit, uint8_t phyAddr, uint16_t *pStatusData)
{
    CMM_UNUSED_PARAM(unit);
    CMM_UNUSED_PARAM(phyAddr);
    CMM_UNUSED_PARAM(pStatusData);
    return CMM_ERR_OK;
}

yt_ret_t int_serdes_test_template(yt_unit_t unit, uint8_t phyAddr, yt_utp_template_testmode_t mode)
{
    cmm_err_t ret = CMM_ERR_OK;

    if(mode < YT_UTP_TEMPLATE_TMODE_SDS2500M || mode > YT_UTP_TEMPLATE_TMODE_SDS1000M)
    {
        return CMM_ERR_INPUT;
    }

    switch(mode)
    {
        case YT_UTP_TEMPLATE_TMODE_SDS2500M:
        case YT_UTP_TEMPLATE_TMODE_SDS1000M:
            /*CMM_ERR_CHK(int_phy_ext_reg_write(unit, phyAddr, 0x5, 0xc100), ret);*/
            CMM_ERR_CHK(int_phy_ext_reg_write(unit, phyAddr, 0xa0, 0x8c00), ret);
            break;
        default:
            return CMM_ERR_INPUT;
    }

    return CMM_ERR_OK;
}


yt_phy_drv_func_t int_serdes_drv_func =
{
    .phy_num = 2,
    .phy_init = int_serdes_init,
    .phy_restart = int_serdes_restart,
    .phy_enable_set = int_serdes_enable_set,
    .phy_enable_get = int_serdes_enable_get,
    .phy_medium_set = int_serdes_medium_set,
    .phy_medium_get = int_serdes_medium_get,
    .phy_autoNeg_enable_set = int_serdes_autoNeg_enable_set,
    .phy_autoNeg_enable_get = int_serdes_autoNeg_enable_get,
    .phy_autoNeg_ability_set = int_serdes_autoNeg_ability_set,
    .phy_autoNeg_ability_get = int_serdes_autoNeg_ability_get,
    .phy_force_speed_duplex_set = int_serdes_force_speed_duplex_set,
    .phy_force_speed_duplex_get = int_serdes_force_speed_duplex_get,
    .phy_link_status_get = int_serdes_link_status_get,
    .phy_eee_enable_set = int_serdes_eee_enable_set,
    .phy_eee_enable_get = int_serdes_eee_enable_get,
    .phy_combo_mode_set = int_serdes_combo_mode_set,
    .phy_combo_mode_get = int_serdes_combo_mode_get,
    .phy_cable_diag = int_serdes_cable_diag,
    .phy_interrupt_status_get = int_serdes_interrupt_status_get,
    .phy_test_template = int_serdes_test_template,
};

yt_phy_drv_t int_serdes_drv =
{
    .chip_id = YT_PHY_ID_INTSERDES,
    .chip_model = YT_PHY_MODEL_INTSERDES,
    .pDrvFunc = &int_serdes_drv_func
};

yt_phy_drv_t *gpPhyDrvList[] =
{
    //[YT_PHY_MODEL_INT861X] = &int_yt861x_drv,
#if defined(BOARD_YT9215SC_DEFAULT_DEMO) || defined(BOARD_YT9215S_FIB_DEMO) || defined(BOARD_YT9215S_YT8531_FIB_DEMO)
    [YT_PHY_MODEL_INTSERDES] = &int_serdes_drv,
#else
    [YT_PHY_MODEL_INTSERDES] = NULL,
#endif
};

yt_ret_t yt9215_init(yt_unit_t unit)
{
    CMM_UNUSED_PARAM(unit);
    return CMM_ERR_OK;
}

yt_ret_t yt9215_mac_config(yt_unit_t unit)
{
    CMM_UNUSED_PARAM(unit);
    return CMM_ERR_OK;
}

yt_ret_t yt9215_led_config(yt_unit_t unit)
{
    CMM_UNUSED_PARAM(unit);
    return CMM_ERR_OK;
}

yt_ret_t yt9215_intif_read(yt_unit_t unit, uint8_t intif_addr, uint32_t regAddr, uint16_t *pData)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t base_data;
    uint32_t op_data = 0;
    uint32_t wait_count = 0;

    CMM_ERR_CHK(hal_mem32_read(unit, INT_IF_ACCESS_ADDR_CTRL, &base_data), ret);
    base_data &= 0xFC00FFF1;
    base_data |= ((intif_addr&0x1f) << 21 | (regAddr&0x1f) << 16 | 2 << 2);
    CMM_ERR_CHK(hal_mem32_write(unit, INT_IF_ACCESS_ADDR_CTRL, base_data), ret);

    op_data = 1;
    CMM_ERR_CHK(hal_mem32_write(unit, INT_IF_ACCESS_FRAME_CTRL, op_data), ret);

    while(MAX_BUSYING_WAIT_TIME > wait_count)
    {
        CMM_ERR_CHK(hal_mem32_read(unit, INT_IF_ACCESS_FRAME_CTRL, &op_data), ret);

        if(!op_data)
        {
            CMM_ERR_CHK(hal_mem32_read(unit, INT_IF_ACCESS_DATA_1_ADDR, &base_data), ret);
            *pData = base_data;
            return CMM_ERR_OK;
        }
        wait_count ++;
    }

    return CMM_ERR_BUSYING_TIME;
}

yt_ret_t yt9215_intif_write(yt_unit_t unit, uint8_t intif_addr, uint32_t regAddr, uint16_t data)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t base_data;
    uint32_t op_data = 0;
    uint32_t wait_count = 0;

    CMM_ERR_CHK(hal_mem32_read(unit, INT_IF_ACCESS_ADDR_CTRL, &base_data), ret);
    base_data &= 0xFC00FFF1;
    base_data |= ((intif_addr&0x1f) << 21 | (regAddr&0x1f) << 16 | 1 << 2);
    CMM_ERR_CHK(hal_mem32_write(unit, INT_IF_ACCESS_ADDR_CTRL, base_data), ret);
    CMM_ERR_CHK(hal_mem32_write(unit, INT_IF_ACCESS_DATA_0_ADDR, data), ret);

    op_data = 1;
    CMM_ERR_CHK(hal_mem32_write(unit, INT_IF_ACCESS_FRAME_CTRL, op_data), ret);

    while(MAX_BUSYING_WAIT_TIME > wait_count)
    {
        CMM_ERR_CHK(hal_mem32_read(unit, INT_IF_ACCESS_FRAME_CTRL, &op_data), ret);

        if(!op_data)
        {
            return CMM_ERR_OK;
        }

        wait_count ++;
    }

    return CMM_ERR_BUSYING_TIME;
}

yt_ret_t yt9215_extif_read(yt_unit_t unit, uint8_t extif_addr, uint32_t regAddr, uint16_t *pData)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t base_data;
    uint32_t op_data = 0;
    uint32_t wait_count = 0;

    CMM_ERR_CHK(hal_mem32_read(unit, EXT_IF_ACCESS_ADDR_CTRL, &base_data), ret);
    base_data &= 0xFC00F0F1;/*clause 22*/
    base_data |= ((extif_addr&0x1f) << 21 | (regAddr&0x1f) << 16 | 4<<8 | 2 << 2);
    CMM_ERR_CHK(hal_mem32_write(unit, EXT_IF_ACCESS_ADDR_CTRL, base_data), ret);

    op_data = 1;
    CMM_ERR_CHK(hal_mem32_write(unit, EXT_IF_ACCESS_FRAME_CTRL, op_data), ret);

    while(MAX_BUSYING_WAIT_TIME > wait_count)
    {
        CMM_ERR_CHK(hal_mem32_read(unit, EXT_IF_ACCESS_FRAME_CTRL, &op_data), ret);

        if(!op_data)
        {
            CMM_ERR_CHK(hal_mem32_read(unit, EXT_IF_ACCESS_DATA_1_ADDR, &base_data), ret);
            *pData = base_data;
            return CMM_ERR_OK;
        }
        wait_count ++;
    }

    return CMM_ERR_BUSYING_TIME;
}

yt_ret_t yt9215_extif_write(yt_unit_t unit, uint8_t extif_addr, uint32_t regAddr, uint16_t data)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t base_data;
    uint32_t op_data = 0;
    uint32_t wait_count = 0;

    CMM_ERR_CHK(hal_mem32_read(unit, EXT_IF_ACCESS_ADDR_CTRL, &base_data), ret);
    base_data &= 0xFC00F0F1;/*clause 22*/
    base_data |= ((extif_addr&0x1f) << 21 | (regAddr&0x1f) << 16 | 4<<8 | 1 << 2);
    CMM_ERR_CHK(hal_mem32_write(unit, EXT_IF_ACCESS_ADDR_CTRL, base_data), ret);
    CMM_ERR_CHK(hal_mem32_write(unit, EXT_IF_ACCESS_DATA_0_ADDR, data), ret);

    op_data = 1;
    CMM_ERR_CHK(hal_mem32_write(unit, EXT_IF_ACCESS_FRAME_CTRL, op_data), ret);

    while(MAX_BUSYING_WAIT_TIME > wait_count)
    {
        CMM_ERR_CHK(hal_mem32_read(unit, EXT_IF_ACCESS_FRAME_CTRL, &op_data), ret);

        if(!op_data)
        {
            return CMM_ERR_OK;
        }

        wait_count ++;
    }

    return CMM_ERR_BUSYING_TIME;
}

yt_ret_t yt9215_extif_polling_config(yt_unit_t unit)
{
    CMM_UNUSED_PARAM(unit);
    return CMM_ERR_OK;
}

yt_switch_drv_func_t yt9215_drv_func =
{
    .switch_init = yt9215_init,
    .switch_mac_config = yt9215_mac_config,
    .switch_led_config = yt9215_led_config,
    .switch_intif_read = yt9215_intif_read,
    .switch_intif_write = yt9215_intif_write,
    .switch_extif_read = yt9215_extif_read,
    .switch_extif_write = yt9215_extif_write,
    .switch_extif_polling_config = yt9215_extif_polling_config,
};

yt_switch_drv_t yt9215_drv =
{
    .chip_id = YT_SW_ID_9215,
    .chip_model = YT_SW_MODEL_9215,
    .pDrvFunc = &yt9215_drv_func
};

yt_switch_drv_t *gpSwDrvList[] =
{
    [YT_SW_MODEL_9215] = &yt9215_drv,
    [YT_SW_MODEL_9218] = NULL,
};

const yt_swchip_if_info_t yt9218_intf_info =
{
	.allif_num = 11,
	.intif_num = 8,
	.intif_start_mac_id = 0,
	.extif_num = 2,
	.extif_start_mac_id = 8,
	.extif_start_id = 0,
	.intcpu_mac_id = 10
};

const yt_swchip_if_info_t yt9215_intf_info =
{
	.allif_num = 8,
	.intif_num = 5,
	.intif_start_mac_id = 0,
	.extif_num = 2,
	.extif_start_mac_id = 8,
	.extif_start_id = 0,
	.intcpu_mac_id = 10
};


const yt_swchip_if_info_t *gpChipIntfInfoList[] =
{
	[YT_SW_MODEL_9215] = &yt9215_intf_info,
	[YT_SW_MODEL_9218] = &yt9218_intf_info,
};

/* chip capability */
/* chip 9001 related to yt9218 */
const yt_swchip_cap_t yt9218_capacity =
{
    .max_ucast_queue_num = 8,
    .max_mcast_queue_num = 4,
    .max_meter_entry_num = 64,
    .max_value_of_int_pri = 7,
    .max_value_of_int_drop = 2,
    .max_value_of_dscp = 63,
    .max_vlan_xlate_entry_num = 64,
    .max_vlan_egr_xlate_tbl_num = 32,
    .max_protocol_vlan_tbl_num = 4,
    .max_value_of_msti = 15,
    .max_vlan_range_profile_num = 10,
    .max_vlan_meter_entry_num = 32
};

/* chip 9002 related to yt9215 */
const yt_swchip_cap_t yt9215_capacity =
{
    .max_ucast_queue_num = 8,
    .max_mcast_queue_num = 4,
    .max_meter_entry_num = 64,
    .max_value_of_int_pri = 7,
    .max_value_of_int_drop = 2,
    .max_value_of_dscp = 63,
    .max_vlan_xlate_entry_num = 64,
    .max_vlan_egr_xlate_tbl_num = 32,
    .max_protocol_vlan_tbl_num = 0,
    .max_value_of_msti = 15,
    .max_vlan_range_profile_num = 10,
    .max_vlan_meter_entry_num = 32
};


const yt_swchip_cap_t *gpChipCapList[] =
{
	[YT_SW_MODEL_9215] = &yt9215_capacity,
	[YT_SW_MODEL_9218] = &yt9218_capacity,
};

yt_phy_chip_model_t cal_phy_model_get(yt_unit_t unit, yt_port_t port)
{
    uint8_t phy_index;

    if(!CMM_PORT_VALID(unit, port))
    {
        return INVALID_ID;
    }

    phy_index = UNITINFO(unit)->pPortDescp[port]->phy_index;
    if(phy_index != INVALID_ID)
    {
        return UNITINFO(unit)->pPhyDescp[phy_index]->chip_model;
    }

    return INVALID_ID;
}

/* port descp */
const yt_portDescp_t YT9215sSGFibPortDescp[] =
{
    /*macid	attribute		phy_index	phy_addr	serdes_index	ethtype			medium				smi */
    {0,		PORT_ATTR_ETH,	0,			0,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {1,		PORT_ATTR_ETH,	0,			1,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {2,		PORT_ATTR_ETH,	0,			2,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {3,		PORT_ATTR_ETH,	0,			3,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {4,		PORT_ATTR_ETH,	0,			4,			INVALID_ID,		ETH_TYPE_GE,	PORT_MEDI_COPPER,	YT_SMI_INT},
    {8,		PORT_ATTR_ETH,	INVALID_ID,	8,	        0,              ETH_TYPE_GE25,	PORT_MEDI_FIBER,	YT_SMI_EXT},
#ifdef INTER_MCU
    /* internel cpu port */
    {10,	PORT_ATTR_INT_CPU,				INVALID_ID, INVALID_ID, PORT_ATTR_ETH,	ETH_TYPE_GE,	PORT_MEDI_COPPER,	INVALID_ID},
#endif

    {INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID,YT_SMI_NONE},
};

/* serdes.descp */
const yt_serdesDescp_t YT9215sSdsDescp[] =
{
	/*serdes_id mode*/
	{0,              SERDES_MODE_2500BX},
	{INVALID_ID,INVALID_ID},
};

/* phy.descp */
const yt_phyDescp_t YT9215sPhyDescp[] =
{
	/*chip_id chip_model				start_mac_id	phy_max*/
	{0,		  YT_PHY_MODEL_INTSERDES,	0,				5},
	{INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID},
};

/* LED description */
const yt_sled_remapInfo_t YT9215sRemapInfo[] =
{
    {6, 1}, {4, 0}, {5, 1}, {3, 0}, {2, 0}, {1, 0}, {0, 0},
    {6, 0}, {5, 0}, {4, 1}, {3, 1}, {2, 1}, {1, 1}, {0, 1},
    {6, 2}, {5, 2}, {4, 2}, {3, 2}, {2, 2}, {1, 2}, {0, 2}
};

yt_sled_param_t YT9215sSLEDParam = {
    LED_SERIAL_ACTIVE_MODE_LOW,
    SLED_DATANUM_21,
    0,
    YT9215sRemapInfo
};

const yt_ledDescp_t YT9215sLEDDescp = {LED_MODE_SERIAL, &YT9215sSLEDParam};

yt_swDescp_t yt9215s_fib_swDescp[BOARD_MAX_UNIT_NUM];

const board_profile_identify_t YT9215sSGFibProfileIdentifier = {BOARD_ID_YT9215S, "YT9215S SGFIB Demo"};


/* hardware profile */
const yt_hwProfile_t yt9215s_fib_demo =
{
    .pIdentifier = &YT9215sSGFibProfileIdentifier,
    .profile_init = profile_yt9215s_fib_init,
};

yt_ret_t profile_yt9215s_fib_init(yt_hwProfile_info_t *hwprofile_info)
{
    uint8_t	i, unit;

    hwprofile_info->pIdentifier = &YT9215sSGFibProfileIdentifier;

    /* switch info */
    hwprofile_info->switch_count = BOARD_MAX_UNIT_NUM;

    for (unit = 0; unit < BOARD_MAX_UNIT_NUM ; unit++) {
        yt9215s_fib_swDescp[unit].chip_id = YT_SW_ID_9215;
        yt9215s_fib_swDescp[unit].chip_model = YT_SW_MODEL_9215;

        i = 0;
        while(YT9215sSGFibPortDescp[i].mac_id != INVALID_ID)
        {
            yt9215s_fib_swDescp[unit].pPortDescp[i] = &YT9215sSGFibPortDescp[i];
            i++;
        }
        yt9215s_fib_swDescp[unit].port_num = i;

        i = 0;
        while(YT9215sSdsDescp[i].serdes_id!= INVALID_ID)
        {
            yt9215s_fib_swDescp[unit].pSerdesDescp[i] = &YT9215sSdsDescp[i];
            i++;
        }

        i = 0;
        while(YT9215sPhyDescp[i].phy_index!= INVALID_ID)
        {
            yt9215s_fib_swDescp[unit].pPhyDescp[i] = &YT9215sPhyDescp[i];
            i++;
        }

        YT9215sSLEDParam.remapInfoNum = sizeof(YT9215sRemapInfo)/sizeof(yt_sled_remapInfo_t);
        yt9215s_fib_swDescp[unit].pLEDDescp = &YT9215sLEDDescp;

        hwprofile_info->pSwDescp[unit] = &yt9215s_fib_swDescp[unit];
    }
    return CMM_ERR_OK;
}

uint32_t hal_mem32_write(uint8_t unit, uint32_t reg_addr, uint32_t reg_value)
{
    uint32_t ret = CMM_ERR_OK;
    uint8_t phyAddr;
    uint8_t regAddr;
    uint16_t regVal;

    CMM_PARAM_CHK(NULL == SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_write, CMM_ERR_NOT_INIT);

    phyAddr           = SWITCH_SMI_CONTROLLER_ON_UNIT(unit).phyAddr;
    regAddr           = ((SWITCH_SMI_CONTROLLER_ON_UNIT(unit).switchId<<2)|(REG_ADDR_BIT1_ADDR<<1)|(REG_ADDR_BIT0_WRITE));
    /* Set reg_addr[31:16] */
    regVal = (reg_addr >> 16)&0xffff;
    CMM_ERR_CHK(SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_write(phyAddr, regAddr, regVal), ret);

    /* Set reg_addr[15:0] */
    regVal = reg_addr&0xffff;
    CMM_ERR_CHK(SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_write(phyAddr, regAddr, regVal), ret);

    /* Write Data [31:16] out */
    regAddr           = ((SWITCH_SMI_CONTROLLER_ON_UNIT(unit).switchId<<2)|(REG_ADDR_BIT1_DATA<<1)|(REG_ADDR_BIT0_WRITE));
    regVal = (reg_value >> 16)&0xffff;
    CMM_ERR_CHK(SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_write(phyAddr, regAddr, regVal), ret);

    /* Write Data [15:0] out */
    regVal = reg_value&0xffff;
    CMM_ERR_CHK(SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_write(phyAddr, regAddr, regVal), ret);

    return CMM_ERR_OK;
}

uint32_t hal_mem32_read(uint8_t unit, uint32_t reg_addr, uint32_t *reg_value)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t rData;
    uint8_t phyAddr;
    uint8_t regAddr;
    uint16_t regVal;

    CMM_PARAM_CHK(NULL == SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_write, CMM_ERR_NOT_INIT);
    CMM_PARAM_CHK(NULL == SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_read, CMM_ERR_NOT_INIT);

    phyAddr           = SWITCH_SMI_CONTROLLER_ON_UNIT(unit).phyAddr;
    regAddr           = ((SWITCH_SMI_CONTROLLER_ON_UNIT(unit).switchId<<2)|(REG_ADDR_BIT1_ADDR<<1)|(REG_ADDR_BIT0_READ));
    /* Set reg_addr[31:16] */
    regVal = (reg_addr >> 16)&0xffff;
    CMM_ERR_CHK(SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_write(phyAddr, regAddr, regVal), ret);

    /* Set reg_addr[15:0] */
    regVal = reg_addr&0xffff;
    CMM_ERR_CHK(SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_write(phyAddr, regAddr, regVal), ret);

    regAddr           = ((SWITCH_SMI_CONTROLLER_ON_UNIT(unit).switchId<<2)|(REG_ADDR_BIT1_DATA<<1)|((REG_ADDR_BIT0_READ)));
    /* Read Data [31:16] */
    regVal = 0x0;
    CMM_ERR_CHK(SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_read(phyAddr, regAddr, &regVal), ret);
    rData = (uint32_t)(regVal<<16);

    /* Read Data [15:0] */
    regVal = 0x0;
    CMM_ERR_CHK(SWITCH_SMI_CONTROLLER_ON_UNIT(unit).smi_read(phyAddr, regAddr, &regVal), ret);
    if (CMM_ERR_OK != ret)
    {
        return -1;
    }
    rData |= regVal;

    *reg_value = rData;

    return CMM_ERR_OK;
}

uint32  hal_tbl_reg_field_get(uint32_t mem_id, uint32_t field_id,  void *entry, void *data)
{
    uint32 word, low, width;
    uint32 value_hi, value_lo, value;
    uint32 *ptbl = (uint32_t *)entry;
    uint32 *pdata = (uint32_t *)data;

    if (mem_id >= NUM_MEMS) return CMM_ERR_FAIL; //tbl_reg not found
    if (field_id >= tbl_reg_list[mem_id].fields_num) return CMM_ERR_FAIL; //field not found

    word = tbl_reg_list[mem_id].fields[field_id].word_offset;
    low = tbl_reg_list[mem_id].fields[field_id].bit_offset;
    width = tbl_reg_list[mem_id].fields[field_id].width;

    if (low + width < 32)
    {
        value = ptbl[word];
        *pdata = (uint32)GET_FIELD(value, low, width);
    }
    else
    {
        if (low == 0 && width == 32)
        {
            *pdata = ptbl[word];
        }
        else
        {
            value_hi = ptbl[word + 1];
            value_lo = ptbl[word];
            value_hi = (uint32)GET_FIELD(value_hi, 0, (width - (32 - low)));
            value_lo = (uint32)GET_FIELD(value_lo, low, (32 - low));
            *pdata = (value_hi << (32 - low)) + value_lo;
        }
    }

    return CMM_ERR_OK;
}

uint32  hal_tbl_reg_field_set(uint32_t mem_id, uint32_t field_id,  void *entry, uint32_t data)
{
    uint32 word, low, width, value;
    uint32 *ptbl = (uint32_t *)entry;

    if (mem_id >= NUM_MEMS) return CMM_ERR_FAIL; //tbl_reg not found
    if (field_id >= tbl_reg_list[mem_id].fields_num) return CMM_ERR_FAIL; //field not found

    word = tbl_reg_list[mem_id].fields[field_id].word_offset;
    low = tbl_reg_list[mem_id].fields[field_id].bit_offset;
    width = tbl_reg_list[mem_id].fields[field_id].width;

    value = ptbl[word];
    value = (uint32)CLR_FIELD(value, low, width);
    if(width == 32)
    {
        value = 0;
    }
    value |= (data << low);

    ptbl[word] = value;

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_port_enable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable)
{
    cmm_err_t ret = CMM_ERR_OK;
    port_ctrl_t port_ctrl;
    yt_port_t phy_addr;
    yt_macid_t mac_id;
    uint32_t an_en;
    uint32_t tx_en;
    uint32_t rx_en;
    yt_enable_t phy_en = YT_ENABLE;

    mac_id = CAL_YTP_TO_MAC(unit, port);
    phy_addr = CAL_YTP_TO_PHYADDR(unit, port);

    CMM_PARAM_CHK(hal_table_reg_read(unit, PORT_CTRLm, mac_id, sizeof(port_ctrl_t), &port_ctrl), ret);
    HAL_FIELD_GET(PORT_CTRLm, PORT_CTRL_AN_LINK_ENf, &port_ctrl, &an_en);
    HAL_FIELD_GET(PORT_CTRLm, PORT_CTRL_TXMAC_ENf, &port_ctrl, &tx_en);
    HAL_FIELD_GET(PORT_CTRLm, PORT_CTRL_RXMAC_ENf, &port_ctrl, &rx_en);

    if(phy_addr != INVALID_ID)
    {
        if(HALPHYDRV(unit, mac_id) != NULL)
        {
            HALPHYDRV_FUNC(unit, mac_id)->phy_enable_get(unit, phy_addr, &phy_en);
        }
    }

    if((an_en || tx_en || rx_en) && phy_en)
    {
        *pEnable = YT_ENABLE;
    }
    else
    {
        *pEnable = YT_DISABLE;
    }

    return CMM_ERR_OK;
}

/* extif id = extif start id+extif_start_mac-mac */
uint8_t chipdef_get_extif_by_macid(yt_macid_t mac_id, const yt_swchip_if_info_t *pChipIfInfo)
{
	uint8_t extif_id = INVALID_ID;

	if(mac_id < pChipIfInfo->extif_start_mac_id ||
		mac_id > pChipIfInfo->extif_start_mac_id + pChipIfInfo->extif_num)
	{
		return extif_id;
	}

	extif_id = pChipIfInfo->extif_start_id + mac_id - pChipIfInfo->extif_start_mac_id;

	return extif_id;
}

static uint32_t hal_table_reg_op(uint8_t unit, uint8_t is_write, uint32 mem_id, uint32_t idx, uint32_t len, void *pvalue)
{
    uint32 i;
    cmm_err_t ret;

    /*Use 32bits addr now*/
    uint32 addr;
    uint32 *pvalue_tmp = (uint32*)pvalue;

    CMM_UNUSED_PARAM(len);

    CMM_PARAM_CHK((NULL == pvalue), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((FALSE == ghal_mem32_init), CMM_ERR_NOT_INIT);
    CMM_PARAM_CHK((FALSE == ghal_reg_table_init), CMM_ERR_NOT_INIT);


    DRV_MEM_DS_VALID_CHK(mem_id, idx);
    osal_mux_lock(&g_cfgmux);

    addr = tbl_reg_list[mem_id].asic_offset + tbl_reg_list[mem_id].max_entry_size*idx ;

    for(i = 0; (tbl_reg_list[mem_id].entry_real_size/(sizeof(uint32))) > i; i++)
    {
        if(NULL == (pvalue_tmp + i))
        {
            osal_mux_unlock(&g_cfgmux);
            return CMM_ERR_NULL_POINT;
        }
        if(is_write)
        {
            ret = hal_mem32_write(unit, addr, *(pvalue_tmp + i));
            if(DEBUG_MODE_K == 1)
            {
                  osal_printf("Write addr[0x%x] = 0x%x\n",addr,*(uint32 *)(pvalue_tmp + i));
            }
        }
        else
        {
            ret = hal_mem32_read(unit, addr, pvalue_tmp + i);
        }

        if(CMM_ERR_OK != ret)
        {
            osal_mux_unlock(&g_cfgmux);
            return ret;
        }

        if(is_write == 0)
       {
          if(DEBUG_MODE_K == 1)
          {
               osal_printf("Read addr[0x%x] = 0x%x\n",addr,*(uint32 *)(pvalue_tmp + i));
          }
       }

        addr += (sizeof(uint32));
    }


    osal_mux_unlock(&g_cfgmux);

	return CMM_ERR_OK;
}

static uint32_t hal_table_reg_normal_write(uint8_t unit, uint32 mem_id, uint32_t idx, uint16_t len, void *pvalue)
{
    cmm_err_t ret;

    CMM_UNUSED_PARAM(unit);

    CMM_PARAM_CHK((NULL == pvalue), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((FALSE == ghal_mem32_init), CMM_ERR_NOT_INIT);
    CMM_PARAM_CHK((FALSE == ghal_reg_table_init), CMM_ERR_NOT_INIT);

    CMM_ERR_CHK(hal_table_reg_op(unit, TRUE, mem_id, idx, len, pvalue), ret);

    return CMM_ERR_OK;
}

static uint32_t hal_table_reg_normal_read(uint8_t unit, uint32 mem_id, uint32_t idx, uint16_t len, void *pvalue)
{
    cmm_err_t ret;

    CMM_PARAM_CHK((NULL == pvalue), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((FALSE == ghal_mem32_init), CMM_ERR_NOT_INIT);
    CMM_PARAM_CHK((FALSE == ghal_reg_table_init), CMM_ERR_NOT_INIT);

#ifdef INTERNAL_MSG_DEBUG
    if (MEM_FLAG_MSG == tbl_reg_list[mem_id].flag)
    {
        CMM_ERR_CHK(hal_dbg_msg_read(unit, mem_id, pvalue), ret);
    }
    else
#endif
    {
        CMM_ERR_CHK(hal_table_reg_op(unit, FALSE, mem_id, idx, len, pvalue), ret);
    }

    return CMM_ERR_OK;
}


uint32 hal_table_reg_write(uint8_t unit, uint32 mem_id, uint32_t idx, uint16_t len, void *pvalue)
{
    cmm_err_t ret;

#ifdef MEM_MODE_CMODEL
    if ((HAL_REG_TBL_MODE_CMODEL == greg_tbl_mode) || (HAL_REG_TBL_MODE_BOTH == greg_tbl_mode))
    {
        CMM_ERR_CHK(hal_table_reg_cmodel_write(unit, mem_id, idx, len, pvalue), ret);
    }
#endif

    if ((HAL_REG_TBL_MODE_NORMAL == greg_tbl_mode) || (HAL_REG_TBL_MODE_BOTH == greg_tbl_mode))
    {
        CMM_ERR_CHK(hal_table_reg_normal_write(unit, mem_id, idx, len, pvalue), ret);
    }
    return CMM_ERR_OK;
}


uint32 hal_table_reg_read(uint8_t unit, uint32 mem_id, uint32_t idx,uint16_t len, void *pvalue)
{
    cmm_err_t ret;

#ifdef MEM_MODE_CMODEL
    if ((HAL_REG_TBL_MODE_CMODEL == greg_tbl_mode) || (HAL_REG_TBL_MODE_BOTH == greg_tbl_mode))
    {
        CMM_ERR_CHK(hal_table_reg_cmodel_read(unit, mem_id, idx, len, pvalue), ret);
    }
#endif

    if ((HAL_REG_TBL_MODE_NORMAL == greg_tbl_mode) || (HAL_REG_TBL_MODE_BOTH == greg_tbl_mode))
    {
        CMM_ERR_CHK(hal_table_reg_normal_read(unit, mem_id, idx, len, pvalue), ret);
    }

    return CMM_ERR_OK;
}

static uint32_t fal_tiger_vlan_port_egrTagKeep_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t port, yt_enable_t enable)
{
    egr_port_vlan_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(hal_table_reg_read(unit, EGR_PORT_VLAN_CTRLNm, macid ,sizeof(entry), &entry), ret);
    if(VLAN_TYPE_CVLAN == type)
    {
        HAL_FIELD_SET(EGR_PORT_VLAN_CTRLNm, EGR_PORT_VLAN_CTRLN_CTAG_KEEP_MODEf, &entry, enable);
    }
    else  if(VLAN_TYPE_SVLAN == type)
    {
        HAL_FIELD_SET(EGR_PORT_VLAN_CTRLNm, EGR_PORT_VLAN_CTRLN_STAG_KEEP_MODEf, &entry, enable);
    }
    CMM_ERR_CHK(hal_table_reg_write(unit, EGR_PORT_VLAN_CTRLNm, macid, sizeof(entry), &entry), ret);

    return CMM_ERR_OK;
}

/*
 * VLAN_TAG_MODE_KEEP_ALL and VLAN_TAG_MODE_KEEP_TAGGED_MODE map to internal 4
 * VLAN_TAG_MODE_ENTRY_BASED maps to internal mode 5
 */
yt_ret_t yt_vlan_port_egrTagMode_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port,  yt_egr_tag_mode_t tagMode)
{
    egr_port_vlan_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
	uint8_t tagVal = tagMode;
	yt_enable_t tagKeepTagOnly = YT_DISABLE;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

	if(tagMode == VLAN_TAG_MODE_KEEP_TAGGED_MODE || tagMode == VLAN_TAG_MODE_ENTRY_BASED)
	{
		tagVal = tagMode - 1;
	}

	/* keep all or keep tag only*/
	if(tagMode == VLAN_TAG_MODE_KEEP_TAGGED_MODE || tagMode == VLAN_TAG_MODE_KEEP_ALL)
	{
		tagKeepTagOnly = tagMode==VLAN_TAG_MODE_KEEP_ALL ? YT_DISABLE : YT_ENABLE;
		CMM_ERR_CHK(fal_tiger_vlan_port_egrTagKeep_set(unit, type, port, tagKeepTagOnly), ret);
	}

    CMM_ERR_CHK(hal_table_reg_read(unit, EGR_PORT_VLAN_CTRLNm, macid ,sizeof(entry), &entry), ret);
    if(VLAN_TYPE_CVLAN == type)
    {
        HAL_FIELD_SET(EGR_PORT_VLAN_CTRLNm, EGR_PORT_VLAN_CTRLN_CTAG_MODEf, &entry, tagVal);
    }
    else if(VLAN_TYPE_SVLAN == type)
    {
        HAL_FIELD_SET(EGR_PORT_VLAN_CTRLNm, EGR_PORT_VLAN_CTRLN_STAG_MODEf, &entry, tagVal);
    }
    CMM_ERR_CHK(hal_table_reg_write(unit, EGR_PORT_VLAN_CTRLNm, macid, sizeof(entry), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t  yt_vlan_port_igrPvid_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t vid)
{
    port_vlan_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(hal_table_reg_read(unit, PORT_VLAN_CTRLNm, macid, sizeof(port_vlan_ctrln_t), &entry), ret);
    if(VLAN_TYPE_SVLAN == type)
    {
        HAL_FIELD_SET(PORT_VLAN_CTRLNm, PORT_VLAN_CTRLN_DEFAULT_SVIDf, &entry, vid);
    }
    else if (VLAN_TYPE_CVLAN == type)
    {
        HAL_FIELD_SET(PORT_VLAN_CTRLNm, PORT_VLAN_CTRLN_DEFAULT_CVIDf, &entry, vid);
    }
    CMM_ERR_CHK(hal_table_reg_write(unit, PORT_VLAN_CTRLNm, macid, sizeof(port_vlan_ctrln_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t  yt_vlan_port_vidTypeSel_set(yt_unit_t unit, yt_port_t port, yt_vlan_type_t mode)
{
    look_up_vlan_sel_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint16_t macmask = 0;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    osal_memset(&entry, 0, sizeof(entry));
    CMM_ERR_CHK(hal_table_reg_read(unit, LOOK_UP_VLAN_SELm, 0, sizeof(look_up_vlan_sel_t), &entry), ret);
    HAL_FIELD_GET(LOOK_UP_VLAN_SELm, LOOK_UP_VLAN_SEL_LOOK_UP_VLAN_SELf, &entry, &macmask);
    if(VLAN_TYPE_SVLAN == mode )
    {
        SET_BIT(macmask, macid);
    }
    else
    {
        CLEAR_BIT(macmask, macid);
    }
    HAL_FIELD_SET(LOOK_UP_VLAN_SELm, LOOK_UP_VLAN_SEL_LOOK_UP_VLAN_SELf, &entry, macmask);
    CMM_ERR_CHK(hal_table_reg_write(unit, LOOK_UP_VLAN_SELm, 0, sizeof(look_up_vlan_sel_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t  yt_vlan_port_set(yt_unit_t unit,  yt_vlan_t vid,  yt_port_mask_t  member_portmask, yt_port_mask_t  untag_portmask)
{
    l2_vlan_tbl_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_port_mask_t  macmask;
    yt_port_mask_t  utagmacmask;

    CAL_YTPLIST_TO_MLIST(unit, member_portmask, macmask);
    CAL_YTPLIST_TO_MLIST(unit, untag_portmask, utagmacmask);
    CMM_ERR_CHK(hal_table_reg_read(unit, L2_VLAN_TBLm, vid,sizeof(l2_vlan_tbl_t), &entry), ret);
    HAL_FIELD_SET(L2_VLAN_TBLm, L2_VLAN_TBL_PORT_MEMBER_BITMAPf, &entry, macmask.portbits[0]);
    HAL_FIELD_SET(L2_VLAN_TBLm, L2_VLAN_TBL_UNTAG_MEMBER_BITMAPf, &entry, utagmacmask.portbits[0]);
    CMM_ERR_CHK(hal_table_reg_write(unit, L2_VLAN_TBLm, vid, sizeof(l2_vlan_tbl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t yt_stat_mib_clear(yt_unit_t unit, yt_port_t port)
{
    uint32_t ctrl_data = 0;
    yt_port_t port_mac_id;
    cmm_err_t ret = CMM_ERR_OK;

    port_mac_id = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(hal_mem32_read(unit, MIB_OP_ADDR, &ctrl_data), ret);
    ctrl_data = port_mac_id << 3 | 0x2 << 0;
    CMM_ERR_CHK(hal_mem32_write(unit, MIB_OP_ADDR, ctrl_data), ret);
    ctrl_data |= 1 << 30;
    CMM_ERR_CHK(hal_mem32_write(unit, MIB_OP_ADDR, ctrl_data), ret);

    return CMM_ERR_OK;
}

yt_ret_t yt_stat_mib_clear_all(yt_unit_t unit)
{
    uint32_t ctrl_data = 0;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_ERR_CHK(hal_mem32_read(unit, MIB_OP_ADDR, &ctrl_data), ret);
    ctrl_data = 0;
    CMM_ERR_CHK(hal_mem32_write(unit, MIB_OP_ADDR, ctrl_data), ret);
    ctrl_data |= 1 << 30;
    CMM_ERR_CHK(hal_mem32_write(unit, MIB_OP_ADDR, ctrl_data), ret);

    return CMM_ERR_OK;
}

yt_ret_t yt_stat_mib_port_get(yt_unit_t unit, yt_port_t port, yt_stat_mib_port_cnt_t *pcnt)
{
    cmm_err_t ret = CMM_ERR_OK;
    uint32 *port_cnt;
    uint32_t mib_index = 0;
    uint32 mib_data = 0;
    uint32 tmp_data = 0;
    uint64 result_data = 0;
    yt_port_t port_mac_id;

    if(NULL == pcnt)
        return CMM_ERR_NULL_POINT;

    port_mac_id = CAL_YTP_TO_MAC(unit,port);
    port_cnt = (uint32 *)pcnt;
    for(mib_index = 0; mib_index < YT_STAT_MAX; mib_index ++ )
    {
        CMM_ERR_CHK(hal_mem32_read(unit, MIB_TBL_BASE_ADDR0 + 0x100 * port_mac_id + 0x4 * mib_index, &mib_data), ret);
        if(mib_index == RX_OKBYTE || mib_index == RX_NOT_OKBYTE || mib_index == TX_OKBYTE)
        {
            CMM_ERR_CHK(hal_mem32_read(unit, MIB_TBL_BASE_ADDR0 + 0x100 * port_mac_id + 0x4 *(mib_index + 1), &tmp_data), ret);
            result_data = tmp_data;

            *(uint64 *)port_cnt = (result_data << 32) |mib_data;
            mib_index += 1;
            result_data = 0;
            port_cnt += 2;
        }
        else if(mib_index == RX_UNICAST)
        {
            *port_cnt = pcnt->RX_64B + pcnt->RX_65_127B + pcnt->RX_128_255B + pcnt->RX_256_511B + pcnt->RX_512_1023B
                            + pcnt->RX_1024_1518B + pcnt->RX_JUMBO - pcnt->RX_MULTICAST - pcnt->RX_BROADCAST - pcnt->RX_PAUSE - pcnt->RX_OAM_COUNTER;
            port_cnt += 1;
        }
        else if(mib_index == TX_UNICAST)
        {
            *port_cnt = pcnt->TX_OK_PKT - pcnt->TX_MULTICAST - pcnt->TX_BROADCAST - pcnt->TX_PAUSE - pcnt->TX_OAM_COUNTER;
            port_cnt += 1;
        }
        else if(mib_index == RX_JUMBO)
        {
            *port_cnt = mib_data;
            port_cnt += 1;
            mib_index += 1;
        }
        else if(mib_index == TX_JUMBO)
        {
            *port_cnt = mib_data;
            port_cnt += 1;
            mib_index += 1;
        }
        else
        {
            *port_cnt = mib_data;
            port_cnt += 1;
        }
    }

    return CMM_ERR_OK;
}

yt_ret_t yt_port_macAutoNeg_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable)
{
    cmm_err_t ret = CMM_ERR_OK;
    yt_enable_t orgEnable = YT_DISABLE;
    port_ctrl_t port_ctrl;
    yt_macid_t mac_id;
    yt_port_t phy_addr;

    mac_id = CAL_YTP_TO_MAC(unit, port);
    phy_addr = CAL_YTP_TO_PHYADDR(unit, port);

    osal_memset(&port_ctrl, 0, sizeof(port_ctrl_t));
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, PORT_CTRLm, mac_id, sizeof(port_ctrl_t), &port_ctrl), ret);
    HAL_FIELD_GET(PORT_CTRLm, PORT_CTRL_AN_LINK_ENf, &port_ctrl, &orgEnable);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_FLOW_LINK_ANf, &port_ctrl, enable);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_AN_LINK_ENf, &port_ctrl, enable);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, PORT_CTRLm, mac_id, sizeof(port_ctrl_t), &port_ctrl), ret);

    /*mac force to auto,need phy tiger*/
    if(orgEnable == YT_DISABLE && enable == YT_ENABLE)
    {
        if(phy_addr != INVALID_ID)
        {
            if(HALPHYDRV(unit, mac_id) != NULL)
            {
                HALPHYDRV_FUNC(unit, mac_id)->phy_restart(unit, phy_addr);
            }
        }
    }

    return CMM_ERR_OK;
}

yt_ret_t yt_port_macAutoNeg_enable_get(yt_unit_t unit, yt_port_t port, yt_enable_t *pEnable)
{
    cmm_err_t ret = CMM_ERR_OK;
    port_ctrl_t port_ctrl;
    yt_macid_t mac_id;

    mac_id = CAL_YTP_TO_MAC(unit, port);
    osal_memset(&port_ctrl, 0, sizeof(port_ctrl_t));

    CMM_ERR_CHK(hal_table_reg_read(unit, PORT_CTRLm, mac_id, sizeof(port_ctrl_t), &port_ctrl), ret);
    HAL_FIELD_GET(PORT_CTRLm, PORT_CTRL_AN_LINK_ENf, &port_ctrl, pEnable);

    return CMM_ERR_OK;
}

static yt_ret_t fal_port_speedDup_split(yt_port_speed_duplex_t speed_dup, yt_port_speed_t *pSpeed, yt_port_duplex_t *pDuplex)
{
    switch(speed_dup)
    {
        case PORT_SPEED_DUP_10HALF:
            *pSpeed  = PORT_SPEED_10M;
            *pDuplex = PORT_DUPLEX_HALF;
            break;
        case PORT_SPEED_DUP_10FULL:
            *pSpeed  = PORT_SPEED_10M;
            *pDuplex = PORT_DUPLEX_FULL;
            break;
        case PORT_SPEED_DUP_100HALF:
            *pSpeed  = PORT_SPEED_100M;
            *pDuplex = PORT_DUPLEX_HALF;
            break;
        case PORT_SPEED_DUP_100FULL:
            *pSpeed  = PORT_SPEED_100M;
            *pDuplex = PORT_DUPLEX_FULL;
            break;
        case PORT_SPEED_DUP_1000FULL:
            *pSpeed  = PORT_SPEED_1000M;
            *pDuplex = PORT_DUPLEX_FULL;
            break;
        case PORT_SPEED_DUP_2500FULL:
            *pSpeed = PORT_SPEED_2500M;
            *pDuplex = PORT_DUPLEX_FULL;
            break;
        default:
            return CMM_ERR_INPUT;
    }

    return CMM_ERR_OK;
}

yt_ret_t yt_port_mac_force_set(yt_unit_t unit, yt_port_t port, yt_port_force_ctrl_t port_ctrl)
{
    port_ctrl_t port_ctrl_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t mac_id;
    yt_port_speed_t speed = PORT_SPEED_1000M;
    yt_port_speed_mode_t speed_mode = PORT_SPEED_MODE_1000M;
    yt_port_duplex_t duplex_mode = PORT_DUPLEX_FULL;

    mac_id = CAL_YTP_TO_MAC(unit, port);
    osal_memset(&port_ctrl_entry, 0, sizeof(port_ctrl_t));

    ret = fal_port_speedDup_split(port_ctrl.speed_dup, &speed, &duplex_mode);
    if(ret != CMM_ERR_OK)
    {
        return ret;
    }

    if(speed == PORT_SPEED_2500M)
    {
        /*b100 for mac and sds 2.5g*/
        speed_mode = PORT_SPEED_MODE_2500M;
    }
    else
    {
        speed_mode = (yt_port_speed_mode_t)speed;
    }

    CMM_ERR_CHK(hal_table_reg_read(unit, PORT_CTRLm, mac_id, sizeof(port_ctrl_t), &port_ctrl_entry), ret);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_FLOW_LINK_ANf, &port_ctrl_entry, YT_DISABLE);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_AN_LINK_ENf, &port_ctrl_entry, YT_DISABLE);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_SPEED_MODEf, &port_ctrl_entry, speed_mode);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_DUPLEX_MODEf, &port_ctrl_entry, duplex_mode);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_RX_FC_ENf, &port_ctrl_entry, port_ctrl.rx_fc_en);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_TX_FC_ENf, &port_ctrl_entry, port_ctrl.tx_fc_en);
    CMM_ERR_CHK(hal_table_reg_write(unit, PORT_CTRLm, mac_id, sizeof(port_ctrl_t), &port_ctrl_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t yt_port_extif_mode_set(yt_unit_t unit, yt_port_t port, yt_extif_mode_t mode)
{
    cmm_err_t ret = CMM_ERR_OK;
    sg_phy_t sg_data;
    extif0_mode_t extif_data;
    yt_port_t extif_id;
    uint32_t reg_data = 0;
    uint32_t extif_mode_reg;
    yt_port_t extif_bit;

    if (CAL_YTP_TO_MAC(unit, port) == 4)
    {
        if (mode >= YT_EXTIF_MODE_SG_MAC)
        {
            return CMM_ERR_NOT_SUPPORT;
        }
        extif_id = 1;
        extif_bit = 2;
    }
    else
    {
        extif_id = CAL_YTP_TO_EXTPORT(unit, port);
        if(extif_id == INVALID_ID)
        {
            return CMM_ERR_INPUT;
        }
        extif_bit = (extif_id == 0) ? 1 : 0;
    }
    switch(mode)
    {
        case YT_EXTIF_MODE_MII:
        case YT_EXTIF_MODE_REMII:
        case YT_EXTIF_MODE_RMII_MAC:
        case YT_EXTIF_MODE_RMII_PHY:
        case YT_EXTIF_MODE_RGMII:
        case YT_EXTIF_MODE_XMII_DISABLE:
        {
            extif_mode_reg = (extif_id == 0) ? EXTIF0_MODEm : EXTIF1_MODEm;
            /*select xmii mode*/
            CMM_ERR_CHK(hal_mem32_read(unit, CHIP_INTERFACE_SELECT_REG, &reg_data), ret);
            if (mode == YT_EXTIF_MODE_XMII_DISABLE)
            {
                CLEAR_BIT(reg_data, extif_bit);
            }
            else
            {
                SET_BIT(reg_data, extif_bit);
            }
            CMM_ERR_CHK(hal_mem32_write(unit, CHIP_INTERFACE_SELECT_REG, reg_data), ret);
            /*config rgmii or xmii mode*/
            CMM_ERR_CHK(hal_table_reg_read(unit, extif_mode_reg, 0, sizeof(extif0_mode_t), &extif_data), ret);
            HAL_FIELD_SET(extif_mode_reg, EXTIF0_MODE_XMII_MODEf, &extif_data, mode);
            if (mode == YT_EXTIF_MODE_XMII_DISABLE)
            {
                HAL_FIELD_SET(extif_mode_reg, EXTIF0_MODE_XMII_PORT_ENf, &extif_data, FALSE);
            }
            else
            {
                HAL_FIELD_SET(extif_mode_reg, EXTIF0_MODE_XMII_PORT_ENf, &extif_data, TRUE);
            }
            CMM_ERR_CHK(hal_table_reg_write(unit, extif_mode_reg, 0, sizeof(extif0_mode_t), &extif_data), ret);
        }
            break;
        case YT_EXTIF_MODE_SG_MAC:
        case YT_EXTIF_MODE_SG_PHY:
        case YT_EXTIF_MODE_FIB_1000:
        case YT_EXTIF_MODE_FIB_100:
        case YT_EXTIF_MODE_BX2500:
        case YT_EXTIF_MODE_SGFIB_AS:
        case YT_EXTIF_MODE_SG_DISABLE:
        {
            /*enable serder interface*/
            CMM_ERR_CHK(hal_mem32_read(unit, CHIP_INTERFACE_CTRL_REG, &reg_data), ret);
            reg_data |= 1 << extif_id;
            CMM_ERR_CHK(hal_mem32_write(unit, CHIP_INTERFACE_CTRL_REG, reg_data), ret);
            /*select sg mode*/
            CMM_ERR_CHK(hal_mem32_read(unit, CHIP_INTERFACE_SELECT_REG, &reg_data), ret);
            reg_data &= ~(1 << extif_bit);
            CMM_ERR_CHK(hal_mem32_write(unit, CHIP_INTERFACE_SELECT_REG, reg_data), ret);
            /*config fiber or sg mode*/
            CMM_ERR_CHK(hal_table_reg_read(unit, SG_PHYm, extif_id, sizeof(sg_phy_t), &sg_data), ret);
            HAL_FIELD_SET(SG_PHYm, SG_PHY_APPLICATION_MODEf, &sg_data, mode - YT_EXTIF_MODE_SG_MAC);
            CMM_ERR_CHK(hal_table_reg_write(unit, SG_PHYm, extif_id, sizeof(sg_phy_t), &sg_data), ret);
        }
            break;
        default:
            return CMM_ERR_INPUT;
    }

    return CMM_ERR_OK;
}

yt_ret_t yt_port_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enable)
{
    cmm_err_t ret = CMM_ERR_OK;
    port_ctrl_t port_ctrl;
    yt_port_t phy_addr;
    yt_macid_t mac_id;
    yt_enable_t orgEnable;
    uint32_t regData;

    mac_id = CAL_YTP_TO_MAC(unit, port);
    phy_addr = CAL_YTP_TO_PHYADDR(unit, port);

    if(YT_ENABLE == enable  &&
        CAL_PORT_TYPE_INTPHY == CAL_YTP_PORT_TYPE(unit, port))
    {
        /*enable phy*/
        hal_mem32_read(unit, 0x8002c, &regData);
        if(!(regData & (1<<(mac_id+8))))
        {
            regData = regData | (1<<(mac_id+8)) | 0xff;
            hal_mem32_write(unit, 0x8002c, regData);
        }
    }

    /* if no change, do nothing */
    fal_tiger_port_enable_get(unit, port, &orgEnable);
    if(orgEnable == enable)
    {
        return CMM_ERR_OK;
    }

    CMM_PARAM_CHK(hal_table_reg_read(unit, PORT_CTRLm, mac_id, sizeof(port_ctrl_t), &port_ctrl), ret);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_AN_LINK_ENf, &port_ctrl, enable);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_TXMAC_ENf, &port_ctrl, enable);
    HAL_FIELD_SET(PORT_CTRLm, PORT_CTRL_RXMAC_ENf, &port_ctrl, enable);
    CMM_PARAM_CHK(hal_table_reg_write(unit, PORT_CTRLm, mac_id, sizeof(port_ctrl_t), &port_ctrl), ret);

    if(phy_addr != INVALID_ID)
    {
        if(HALPHYDRV(unit, mac_id) != NULL)
        {
            HALPHYDRV_FUNC(unit, mac_id)->phy_enable_set(unit, phy_addr, enable);
        }
    }

    return CMM_ERR_OK;
}

yt_ret_t yt_stat_mib_enable_set(yt_unit_t unit, yt_enable_t enable)
{
    global_ctrl1_t entry;
    cmm_err_t ret = CMM_ERR_OK;

    osal_memset(&entry, 0, sizeof(global_ctrl1_t));

    CMM_ERR_CHK(hal_table_reg_read(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &entry), ret);
    HAL_FIELD_SET(GLOBAL_CTRL1m, GLOBAL_CTRL1_MIB_ENf, &entry, enable);
    CMM_ERR_CHK(hal_table_reg_write(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t yt_port_link_status_all_get(yt_unit_t unit, yt_port_t port, yt_port_linkStatus_all_t *pAllLinkStatus)
{
    cmm_err_t ret = CMM_ERR_OK;
    port_status_t   port_status;
    yt_macid_t mac_id;

    mac_id = CAL_YTP_TO_MAC(unit,port);
    osal_memset(&port_status, 0, sizeof(port_status_t));

    CMM_ERR_CHK(hal_table_reg_read(unit, PORT_STATUSm, mac_id, sizeof(port_status_t), &port_status), ret);

    HAL_FIELD_GET(PORT_STATUSm, PORT_STATUS_LINKf, &port_status, &pAllLinkStatus->link_status);
    HAL_FIELD_GET(PORT_STATUSm, PORT_STATUS_SPEED_MODEf, &port_status, &pAllLinkStatus->link_speed);
    HAL_FIELD_GET(PORT_STATUSm, PORT_STATUS_DUPLEX_MODEf, &port_status, &pAllLinkStatus->link_duplex);
    HAL_FIELD_GET(PORT_STATUSm, PORT_STATUS_RX_FC_ENf, &port_status, &pAllLinkStatus->rx_fc_en);
    HAL_FIELD_GET(PORT_STATUSm, PORT_STATUS_TX_FC_ENf, &port_status, &pAllLinkStatus->tx_fc_en);

    /*b100 for mac and sds 2.5g*/
    if((yt_port_speed_mode_t)pAllLinkStatus->link_speed == PORT_SPEED_MODE_2500M)
    {
        pAllLinkStatus->link_speed = PORT_SPEED_2500M;
    }

    return CMM_ERR_OK;
}

yt_ret_t sf_yt_lag_hash_sel_set(yt_unit_t unit, uint8_t hash_mask)
{
    link_agg_hash_ctrl_t  entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, LINK_AGG_HASH_CTRLm, 0, sizeof(link_agg_hash_ctrl_t), &entry), ret);
    HAL_FIELD_SET (LINK_AGG_HASH_CTRLm, LINK_AGG_HASH_CTRL_HASH_FIELD_SELf, &entry, hash_mask);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, LINK_AGG_HASH_CTRLm, 0, sizeof(link_agg_hash_ctrl_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t sf_yt_lag_hash_sel_get(yt_unit_t unit, uint8_t *p_hash_mask)
{
    link_agg_hash_ctrl_t  entry;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, LINK_AGG_HASH_CTRLm, 0, sizeof(link_agg_hash_ctrl_t), &entry), ret);
    HAL_FIELD_GET(LINK_AGG_HASH_CTRLm, LINK_AGG_HASH_CTRL_HASH_FIELD_SELf, &entry, p_hash_mask);

    return CMM_ERR_OK;
}

yt_ret_t yt_lag_group_port_set(yt_unit_t unit, uint8_t groupId, yt_port_mask_t member_portmask)
{
    link_agg_groupn_t group_entry;
    link_agg_membern_t member_entry[LAG_MEM_NUM_PERGRP];
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t index = 0;
    uint8_t macid = 0;
    uint32_t  mask;
    yt_port_mask_t macmask;
    uint8_t idx_offset = 0;
    yt_link_agg_group_t laginfo;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupId), CMM_ERR_INPUT);

    /*check if port mask conflic with another lag*/
    yt_lag_group_info_get(unit, (groupId == 1 ? 0:1), &laginfo);
    if(member_portmask.portbits[0] & laginfo.member_portmask)
    {
        return CMM_ERR_SAMEENTRY_EXIST;
    }

    osal_memset(member_entry, 0, sizeof(member_entry));
    CAL_YTPLIST_TO_MLIST(unit, member_portmask, macmask);

    mask = macmask.portbits[0];

    while(mask)
    {
        if((mask & 0x1) != 0)
        {
            if(index >= LAG_MEM_NUM_PERGRP)
            {
                return CMM_ERR_TABLE_FULL;
            }
            HAL_FIELD_SET(LINK_AGG_MEMBERNm, LINK_AGG_MEMBERN_PORTf, &member_entry[index], macid);
            index++;
        }
        macid++;
        mask >>= 1;
    }

    HAL_FIELD_SET(LINK_AGG_GROUPNm, LINK_AGG_GROUPN_MEMBER_NUMf, &group_entry, index);

    idx_offset = (groupId == 1) ? LAG_MEM_NUM_PERGRP : 0;

    for(index=0; index<LAG_MEM_NUM_PERGRP; index++)
    {
        CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, LINK_AGG_MEMBERNm, index+idx_offset, sizeof(link_agg_membern_t), &member_entry[index]), ret);
    }

    HAL_FIELD_SET(LINK_AGG_GROUPNm, LINK_AGG_GROUPN_PORT_MASKf, &group_entry, macmask.portbits[0]);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, LINK_AGG_GROUPNm, groupId, sizeof(link_agg_groupn_t), &group_entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t yt_port_isolation_set(yt_unit_t unit, yt_port_t port, yt_port_mask_t iso_portmask)
{
	l2_port_isolation_ctrln_t  entry;
	cmm_err_t ret = CMM_ERR_OK;
	yt_macid_t macid;
	yt_port_mask_t macmask;

	macid = CAL_YTP_TO_MAC(unit,port);
	CAL_YTPLIST_TO_MLIST(unit,iso_portmask, macmask);
	CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_PORT_ISOLATION_CTRLNm, macid, sizeof(l2_port_isolation_ctrln_t), &entry), ret);
	HAL_FIELD_SET(L2_PORT_ISOLATION_CTRLNm, L2_PORT_ISOLATION_CTRLN_ISOLATED_PORT_MASKf, &entry, macmask.portbits[0]);
	CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_PORT_ISOLATION_CTRLNm, macid, sizeof(l2_port_isolation_ctrln_t), &entry), ret);
	return CMM_ERR_OK;
}

yt_ret_t yt_lag_group_info_get(yt_unit_t unit, uint8_t groupId, yt_link_agg_group_t *p_laginfo)
{
    link_agg_groupn_t group_entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_port_mask_t macmask;
    yt_port_mask_t portmask;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupId), CMM_ERR_INPUT);

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, LINK_AGG_GROUPNm, groupId, sizeof(link_agg_groupn_t), &group_entry), ret);
    HAL_FIELD_GET(LINK_AGG_GROUPNm, LINK_AGG_GROUPN_MEMBER_NUMf, &group_entry, &(p_laginfo->member_num));
    HAL_FIELD_GET(LINK_AGG_GROUPNm, LINK_AGG_GROUPN_PORT_MASKf, &group_entry, macmask.portbits);
    CAL_MLIST_TO_YTPLIST(unit, macmask, portmask);
    p_laginfo->member_portmask = portmask.portbits[0];

    return CMM_ERR_OK;
}

yt_ret_t  yt_vlan_port_igrPvid_get(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t  port, yt_vlan_t *pVid)
{
    port_vlan_ctrln_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(hal_table_reg_read(unit, PORT_VLAN_CTRLNm, macid, sizeof(port_vlan_ctrln_t), &entry), ret);
    if(VLAN_TYPE_SVLAN == type)
    {
        HAL_FIELD_GET(PORT_VLAN_CTRLNm, PORT_VLAN_CTRLN_DEFAULT_SVIDf, &entry, pVid);
    }
    else if (VLAN_TYPE_CVLAN == type)
    {
        HAL_FIELD_GET(PORT_VLAN_CTRLNm, PORT_VLAN_CTRLN_DEFAULT_CVIDf, &entry, pVid);
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_stat_mib_init(yt_unit_t unit)
{
    CMM_UNUSED_PARAM(unit);
    STAT_LOCK_INIT();

    return CMM_ERR_OK;
}


yt_hwProfile_info_t	gBoardInfo;
const yt_swDescp_t	*gpSwitchUnit[YT_MAX_UNIT];
uint8_t g_cycle_time[YT_UNIT_NUM];

const yt_hwProfile_t *gSupport_hwProfile_list[] =
{
#if defined(BOARD_YT9215S_FIB_DEMO) || defined(BOARD_AUTO_DETECT)
    &yt9215s_fib_demo,
#endif
    NULL
};

uint32_t cal_board_profile_init(void)
{
    const yt_hwProfile_t     **pbProfile = gSupport_hwProfile_list;
    uint8_t	i;
    uint8_t j = 0;

    while (*pbProfile != NULL)
    {
        /*if(cal_board_identifier_cmp(gUSERDEFINE, *pbProfile->pIdentifier))*/
        {
            if ((*pbProfile)->profile_init)
            {
                (*pbProfile)->profile_init(&gBoardInfo);
            }

            for (i=0; i<gBoardInfo.switch_count; i++)
            {
                gpSwitchUnit[i] = gBoardInfo.pSwDescp[i];

                for(j = 0; j < gBoardInfo.pSwDescp[i]->port_num; j++)
                {
                    if(gBoardInfo.pSwDescp[i]->pPortDescp[j] &&
                        gBoardInfo.pSwDescp[i]->pPortDescp[j]->mac_id != INVALID_ID)
                    {
                        CMM_SET_MEMBER_PORT(gBoardInfo.portmask[i], j);
                        CMM_SET_MEMBER_PORT(gBoardInfo.allportmask[i], j);
                        CMM_SET_MEMBER_PORT(gBoardInfo.macmask[i],
                            gBoardInfo.pSwDescp[i]->pPortDescp[j]->mac_id);
                    }
                }

                gBoardInfo.pSwDescp[i]->pChipCap = gpChipCapList[gBoardInfo.pSwDescp[i]->chip_model];
                gBoardInfo.pSwDescp[i]->pChipIfInfo = gpChipIntfInfoList[gBoardInfo.pSwDescp[i]->chip_model];
            }

            return CMM_ERR_OK;
        }
        pbProfile++;
    }

    return CMM_ERR_FAIL;
}

uint32_t cal_mgm_init(void)
{
    CMM_PARAM_CHK((TRUE == gcal_inited), CMM_ERR_OK);

    cal_board_profile_init();

    gcal_inited = TRUE;

    return CMM_ERR_OK;
}

uint32_t hal_mem32_init(void)
{
    ghal_mem32_init = TRUE;
    return CMM_ERR_OK;
}


uint32_t hal_table_reg_init(void)
{
    osal_mux_init(&g_cfgmux, NULL);

#ifdef MEM_MODE_CMODEL
    hal_table_reg_cmodel_init();
#endif

    ghal_reg_table_init = TRUE;

    return CMM_ERR_OK;
}

uint32_t hal_ctrl_init(void)
{
    yt_unit_t unit;
    yt_port_t port_id;
    yt_macid_t mac_id;
    yt_switch_chip_model_t swModel;
    yt_phy_chip_model_t phyModel;

    for(unit = 0; unit < YT_UNIT_NUM; unit++)
    {
        swModel = CAL_SWCHIP_MODEL(unit);
        if(swModel >= YT_SW_MODEL_END)
        {
            return CMM_ERR_FAIL;
        }
        gHalCtrl[unit].pHalSwDrv = gpSwDrvList[swModel];

        for(port_id = 0; port_id < CAL_PORT_NUM_ON_UNIT(unit); port_id++)
        {
            if(CAL_IS_SERDES(unit, port_id))
            {
                mac_id = CAL_YTP_TO_MAC(unit, port_id);
                gHalCtrl[unit].pHalPhyDrv[mac_id] = gpPhyDrvList[YT_PHY_MODEL_INTSERDES];
            }
            else if(CAL_IS_PHY_PORT(unit, port_id))
            {
                phyModel = CAL_PHYCHIP_MODEL(unit, port_id);
                if(phyModel >= YT_PHY_MODEL_END)
                {
                    return CMM_ERR_FAIL;
                }
                mac_id = CAL_YTP_TO_MAC(unit, port_id);
                gHalCtrl[unit].pHalPhyDrv[mac_id] = gpPhyDrvList[phyModel];
            }
        }
    }

    return CMM_ERR_OK;
}
uint32_t hal_init(void)
{
    yt_ret_t ret = CMM_ERR_OK;

    hal_mem32_init();
    hal_table_reg_init();

    CMM_ERR_CHK(hal_ctrl_init(), ret);

    return CMM_ERR_OK;
}

fal_dispatch_t  fal_tiger_dispatch =
{
#ifdef VLAN_INCLUDED
    .vlan_init = fal_tiger_vlan_init,
    .vlan_port_set = yt_vlan_port_set,
    .vlan_port_igrDefVlan_set = yt_vlan_port_igrPvid_set,
    .vlan_port_igrDefVlan_get = yt_vlan_port_igrPvid_get,
    .vlan_port_egrTagMode_set = yt_vlan_port_egrTagMode_set,
    .vlan_egrTpid_set = fal_tiger_vlan_egrTpid_set,
    .vlan_port_egrTpidSel_set = fal_tiger_vlan_port_egrTpidIdx_set,
    .vlan_port_vidTypeSel_set = yt_vlan_port_vidTypeSel_set,
    .vlan_port_egrFilter_enable_set = yt_vlan_port_egrFilter_enable_set,
    .vlan_port_igrFilter_enable_set = yt_vlan_port_igrFilter_enable_set,
#endif

#ifdef RATE_INCLUDED
    .rate_init = fal_tiger_rate_init,
#endif

#ifdef LAG_INCLUDED
    .lag_hash_sel_set = sf_yt_lag_hash_sel_set,
    .lag_hash_sel_get = sf_yt_lag_hash_sel_get,
    .lag_group_port_set = yt_lag_group_port_set,
    .lag_group_info_get = yt_lag_group_info_get,
#endif

#ifdef STAT_INCLUDED
    .stat_mib_init = fal_tiger_stat_mib_init,
    .stat_mib_enable_set = yt_stat_mib_enable_set,
    .stat_mib_clear = yt_stat_mib_clear,
    .stat_mib_clear_all = yt_stat_mib_clear_all,
    .stat_mib_port_get = yt_stat_mib_port_get,
#endif

#ifdef L2_INCLUDED
    .l2_init = fal_tiger_l2_init,
    .l2_port_learnlimit_cnt_set = fal_tiger_l2_port_learnlimit_cnt_set,
    .l2_system_learnlimit_cnt_set = fal_tiger_l2_system_learnlimit_cnt_set,
    .l2_lag_learnlimit_cnt_set = fal_tiger_l2_lag_learnlimit_cnt_set,
    .l2_fdb_uc_withindex_getnext = fal_tiger_l2_fdb_uc_withindex_getnext,
#endif

#ifdef PORT_INCLUDED
    .port_init = fal_tiger_port_init,
    .port_enable_set = yt_port_enable_set,
    .port_enable_get = fal_tiger_port_enable_get,
    .port_link_status_all_get = yt_port_link_status_all_get,
    .port_macAutoNeg_enable_set = yt_port_macAutoNeg_enable_set,
    .port_macAutoNeg_enable_get = yt_port_macAutoNeg_enable_get,
    .port_mac_force_set = yt_port_mac_force_set,
    .port_extif_mode_set = yt_port_extif_mode_set,
#endif
};

fal_dispatch_info_t gfal_dispatch_info[] =
{
    {YT_SW_ID_9215,  YT_SW_REV_A, &fal_tiger_dispatch},
};

fal_dispatch_t *gpfal_dispatch[YT_MAX_UNIT];


uint32_t fal_dispatch_get(fal_dispatch_t **pdispatch, uint8_t unit)
{
    uint32_t num = sizeof(gfal_dispatch_info) / sizeof(fal_dispatch_info_t);
    uint32_t i;

    CMM_PARAM_CHK((YT_UNIT_NUM <= unit), CMM_ERR_INPUT);

    CMM_PARAM_CHK((NULL == pdispatch), CMM_ERR_NULL_POINT);

    for(i = 0; num > i; i++)
    {
        if(gfal_dispatch_info[i].chip_id == UNITINFO(unit)->chip_id)
        {
            *pdispatch = gfal_dispatch_info[i].pdispatch;
            return CMM_ERR_OK;
        }
    }

    return CMM_ERR_FAIL;
}

uint32_t fal_dispatch_init(void)
{
    uint32_t i;
    cmm_err_t ret = CMM_ERR_OK;

    static uint8_t gdispatch_inited = FALSE;

    CMM_PARAM_CHK((TRUE == gdispatch_inited), CMM_ERR_OK);

    for(i = 0; YT_UNIT_NUM > i; i++)
    {
        CMM_ERR_CHK(fal_dispatch_get(&(gpfal_dispatch[i]), i), ret);
        gpfal_dispatch[i]->is_inited = TRUE;
    }

    gdispatch_inited = TRUE;

    return CMM_ERR_OK;
}

uint32_t drv_init(uint8_t *dev)
{
    cmm_err_t ret = CMM_ERR_OK;

    ACCESS_LOCK_INIT();

#ifdef ACC_UART
    ret = uart_init(dev);
#else
    CMM_UNUSED_PARAM(dev);
#endif

    return ret;
}

yt_ret_t  yt_device_init(yt_unit_t unit, uint8_t *dev)
{
    cmm_err_t ret = CMM_ERR_OK;
    CMM_UNUSED_PARAM(unit);
    CMM_ERR_CHK(drv_init(dev),ret);
    return CMM_ERR_OK;
}

yt_ret_t  yt_drv_init(void)
{
    yt_unit_t unit;

    for(unit = 0; unit < YT_UNIT_NUM; unit++)
    {
#ifdef ACC_UART
        yt_device_init(unit, "ttyS2");
#else
        yt_device_init(unit, NULL);
#endif
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_init(void)
{
    yt_unit_t unit;
    uint8_t num = sizeof(gFal_init_info) /sizeof(yt_fal_init_info_t);
    uint8_t i;

    for(unit = 0; unit < YT_UNIT_NUM; unit++)
    {
        for(i = 0; num > i; i++)
        {
            if(gFal_init_info[i].chip_id == UNITINFO(unit)->chip_id)
            {
                gFal_init_info[i].chipFalInitFunc(unit);
            }
        }
    }

    return CMM_ERR_OK;
}
yt_ret_t  yt_basic_init(void)
{
    cmm_err_t ret = CMM_ERR_OK;
    CMM_ERR_CHK(cal_mgm_init(), ret);
    CMM_ERR_CHK(hal_init(), ret);
    CMM_ERR_CHK(fal_dispatch_init(), ret);
    CMM_ERR_CHK(yt_drv_init(), ret);
    CMM_ERR_CHK(fal_init(), ret);

    return CMM_ERR_OK;
}

uint32 hal_table_reg_mode_get(uint8_t unit, hal_reg_tbl_mode_t *pmode)
{
    CMM_UNUSED_PARAM(unit);
    CMM_PARAM_CHK((NULL == pmode), CMM_ERR_NULL_POINT);

    *pmode = greg_tbl_mode;

    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_op_info_get(yt_unit_t unit,  yt_mac_addr_t *pmac_addr, uint16_t *pfid, yt_l2_fdb_info_t *pfdb_info)
{

    l2_fdb_tbl_op_data_0_t l2_fdb_tbl_op_data_0;
    l2_fdb_tbl_op_data_1_t l2_fdb_tbl_op_data_1;
    l2_fdb_tbl_op_data_2_t l2_fdb_tbl_op_data_2;
    cmm_err_t ret           = CMM_ERR_OK;
    uint32_t macAddr0;
    uint32_t macAddr1;
    uint32_t status;
    uint32_t dmac_int_pri_en;
    uint32_t fid;
    uint32_t new_vid;
    uint32_t int_pri;
    uint32_t smac_int_pri_en;
    uint32_t copy_to_cpu;
    uint32_t dmac_drop;
    uint32_t dst_port_mask;
    uint32_t smac_drop;

    CMM_PARAM_CHK((NULL == pmac_addr), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfid), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb_tbl_op_data_0, 0, sizeof(l2_fdb_tbl_op_data_0));
    osal_memset(&l2_fdb_tbl_op_data_1, 0, sizeof(l2_fdb_tbl_op_data_1));
    osal_memset(&l2_fdb_tbl_op_data_2, 0, sizeof(l2_fdb_tbl_op_data_2));

    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_TBL_OP_DATA_0_DUMMYm, 0, sizeof(l2_fdb_tbl_op_data_0), &l2_fdb_tbl_op_data_0), ret);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_TBL_OP_DATA_1_DUMMYm, 0, sizeof(l2_fdb_tbl_op_data_1), &l2_fdb_tbl_op_data_1), ret);
    CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_TBL_OP_DATA_2_DUMMYm, 0, sizeof(l2_fdb_tbl_op_data_2), &l2_fdb_tbl_op_data_2), ret);

    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_1_DUMMYm, L2_FDB_TBL_OP_DATA_1_DUMMY_STATUSf, &l2_fdb_tbl_op_data_1, &status);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_1_DUMMYm, L2_FDB_TBL_OP_DATA_1_DUMMY_DMAC_INT_PRI_ENf, &l2_fdb_tbl_op_data_1, &dmac_int_pri_en);
    pfdb_info->STATUS           = status;
    pfdb_info->DMAC_INT_PRI_EN  = dmac_int_pri_en;

    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_NEW_VIDf, &l2_fdb_tbl_op_data_2, &new_vid);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_INT_PRIf, &l2_fdb_tbl_op_data_2, &int_pri);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_SMAC_INT_PRI_ENf, &l2_fdb_tbl_op_data_2, &smac_int_pri_en);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_COPY_TO_CPUf, &l2_fdb_tbl_op_data_2, &copy_to_cpu);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_DMAC_DROPf, &l2_fdb_tbl_op_data_2, &dmac_drop);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_DST_PORT_MASKf, &l2_fdb_tbl_op_data_2, &dst_port_mask);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_2_DUMMYm, L2_FDB_TBL_OP_DATA_2_DUMMY_SMAC_DROPf, &l2_fdb_tbl_op_data_2, &smac_drop);
    pfdb_info->NEW_VID         = new_vid;
    pfdb_info->INT_PRI          = int_pri;
    pfdb_info->SMAC_INT_PRI_EN  = smac_int_pri_en;
    pfdb_info->COPY_TO_CPU      = copy_to_cpu;
    pfdb_info->DMAC_DROP        = dmac_drop;
    pfdb_info->DST_PORT_MASK    = dst_port_mask;
    pfdb_info->SMAC_DROP        = smac_drop;

    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_1_DUMMYm, L2_FDB_TBL_OP_DATA_1_DUMMY_FIDf, &l2_fdb_tbl_op_data_1, &fid);
    *pfid = fid;

    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_0_DUMMYm, L2_FDB_TBL_OP_DATA_0_DUMMY_MAC_DA_0f, &l2_fdb_tbl_op_data_0, &macAddr0);
    HAL_FIELD_GET(L2_FDB_TBL_OP_DATA_1_DUMMYm, L2_FDB_TBL_OP_DATA_1_DUMMY_MAC_DA_1f, &l2_fdb_tbl_op_data_1, &macAddr1);
    pmac_addr->addr[0] = ((macAddr0 & 0xFF000000) >> 24);
    pmac_addr->addr[1] = ((macAddr0 & 0xFF0000) >> 16) ;
    pmac_addr->addr[2] = ((macAddr0 & 0xFF00) >> 8);
    pmac_addr->addr[3] = (macAddr0 & 0xFF);
    pmac_addr->addr[4] = ((macAddr1 & 0xFF00) >> 8);
    pmac_addr->addr[5] = (macAddr1 & 0xFF);

    return CMM_ERR_OK;
}

static  uint32_t  fal_tiger_l2_op_result_get(yt_unit_t unit,  yt_l2_fdb_op_result_t *pop_result)
{
    l2_fdb_tbl_op_result_t l2_fdb_tbl_op_result;
    uint32_t op_node;
    uint32_t lookup_fail;
    uint16_t l2_fdb_tbl_op_busy_cnt = FDB_BUSY_CHECK_NUMBER;
    cmm_err_t ret = CMM_ERR_OK;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);

    while (l2_fdb_tbl_op_busy_cnt)
    {
        osal_memset(&l2_fdb_tbl_op_result, 0, sizeof(l2_fdb_tbl_op_result));
        CMM_ERR_CHK(HAL_TBL_REG_READ(unit, L2_FDB_TBL_OP_RESULTm, 0, sizeof(l2_fdb_tbl_op_result), &l2_fdb_tbl_op_result), ret);
        HAL_FIELD_GET(L2_FDB_TBL_OP_RESULTm, L2_FDB_TBL_OP_RESULT_OP_DONEf, &l2_fdb_tbl_op_result, &op_node);
        if(TRUE == op_node)
        {
            HAL_FIELD_GET(L2_FDB_TBL_OP_RESULTm, L2_FDB_TBL_OP_RESULT_LOOKUP_FAILf, &l2_fdb_tbl_op_result, &lookup_fail);
            HAL_FIELD_GET(L2_FDB_TBL_OP_RESULTm, L2_FDB_TBL_OP_RESULT_ENTRY_INDEXf, &l2_fdb_tbl_op_result, &pop_result->entry_idx);
            pop_result->lookup_fail = lookup_fail;
            pop_result->op_done     = TRUE;
            break;
        }

        l2_fdb_tbl_op_busy_cnt--;

        if(0 == l2_fdb_tbl_op_busy_cnt)
        {
            pop_result->lookup_fail = TRUE;
            pop_result->op_done     = FALSE;

            return CMM_ERR_FDB_OP_BUSY;
        }

    }
    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_op_get_next_withidx(yt_unit_t unit, uint16_t idx,
                                                                    yt_mac_addr_t *pmac_addr, uint16_t *pfid, uint16_t *pnext_index,
                                                                    yt_l2_fdb_info_t *pfdb_info, yt_l2_fdb_op_result_t *pop_result)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_tbl_op_t l2_fdb_tbl_op;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfdb_info), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfid), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pmac_addr), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pnext_index), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb_tbl_op, 0, sizeof(l2_fdb_tbl_op_t));
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_CMDf, &l2_fdb_tbl_op, FAL_TIGER_FDB_OP_CMD_GET_NEXT);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_GET_NEXT_TYPEf, &l2_fdb_tbl_op, FAL_TIGER_FDB_GET_NEXT_UCAST_ONE);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_MODEf, &l2_fdb_tbl_op, L2_FDB_OP_MODE_INDEX);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_ENTRY_INDEXf, &l2_fdb_tbl_op, idx);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_STARTf, &l2_fdb_tbl_op, TRUE);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OPm, 0, sizeof(l2_fdb_tbl_op), &l2_fdb_tbl_op), ret);

    CMM_ERR_CHK(fal_tiger_l2_op_result_get(unit, pop_result), ret);

    if (pop_result->lookup_fail == TRUE)
        return CMM_ERR_ENTRY_NOT_FOUND;

    *pnext_index = pop_result->entry_idx;

    CMM_ERR_CHK(fal_tiger_l2_op_info_get(unit, pmac_addr, pfid,  pfdb_info), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_l2_fdb_uc_withindex_getnext(yt_unit_t unit, uint16_t index, uint16_t *pNext_index, l2_ucastMacAddr_info_t *pUcastMac)
{
    cmm_err_t ret = CMM_ERR_OK;

    hal_reg_tbl_mode_t table_reg_mode;
    yt_l2_fdb_op_result_t op_result;
    yt_l2_fdb_info_t fdb_info;
    yt_mac_addr_t mac_addr;
    yt_port_t port;
    yt_macid_t macid;
    uint16_t fid;

    if(CAL_L2_FDB_NUM_MAX - 1 <= index)
    {
        return CMM_ERR_EXCEED_RANGE;
    }

    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);

    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_get_next_withidx(unit, index, &mac_addr, &fid, pNext_index, &fdb_info, &op_result),ret);

        if(FDB_STATUS_INVALID == fdb_info.STATUS ||
            IS_MCAST_ADDR(mac_addr.addr))
        {
            return CMM_ERR_ENTRY_NOT_FOUND;
        }

        for(port =0; port < CAL_MAX_PORT_NUM; port++)
        {
            macid = CAL_YTP_TO_MAC(unit,port);
            if(IS_BIT_SET(fdb_info.DST_PORT_MASK, macid))
                break;
        }
        pUcastMac->port = port;

        pUcastMac->vid = fid;
        osal_memcpy(pUcastMac->macaddr.addr, mac_addr.addr, MAC_ADDR_LEN);
        switch(fdb_info.STATUS)
        {
            case FDB_STATUS_PENDING:
                pUcastMac->type = FDB_TYPE_PENDING;
                break;
            case FDB_STATUS_STATIC:
                pUcastMac->type = FDB_TYPE_STATIC;
                break;
            default:
                pUcastMac->type = FDB_TYPE_DYNAMIC;
                break;
        }
    }

    return CMM_ERR_OK;
}

static uint32_t  fal_tiger_l2_fdb_op_get_one_withidx(yt_unit_t unit, uint16_t idx,
                                                                    yt_mac_addr_t *pmac_addr, uint16_t *pfid, yt_l2_fdb_info_t *pfdb_info,
                                                                    yt_l2_fdb_op_result_t *pop_result)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_fdb_tbl_op_t l2_fdb_tbl_op;

    CMM_PARAM_CHK((NULL == pop_result), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfdb_info), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pfid), CMM_ERR_NULL_POINT);
    CMM_PARAM_CHK((NULL == pmac_addr), CMM_ERR_NULL_POINT);

    osal_memset(&l2_fdb_tbl_op, 0, sizeof(l2_fdb_tbl_op_t));
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_CMDf, &l2_fdb_tbl_op, FAL_TIGER_FDB_OP_CMD_GET_ONE);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_MODEf, &l2_fdb_tbl_op, L2_FDB_OP_MODE_INDEX);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_ENTRY_INDEXf, &l2_fdb_tbl_op, idx);
    HAL_FIELD_SET(L2_FDB_TBL_OPm, L2_FDB_TBL_OP_OP_STARTf, &l2_fdb_tbl_op, TRUE);

    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_FDB_TBL_OPm, 0, sizeof(l2_fdb_tbl_op), &l2_fdb_tbl_op), ret);

    CMM_ERR_CHK(fal_tiger_l2_op_result_get(unit, pop_result), ret);

    CMM_ERR_CHK(fal_tiger_l2_op_info_get(unit, pmac_addr, pfid,  pfdb_info), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_l2_fdb_uc_withindex_get(yt_unit_t unit, uint16_t index, l2_ucastMacAddr_info_t *pUcastMac)
{
    cmm_err_t ret = CMM_ERR_OK;

    hal_reg_tbl_mode_t table_reg_mode;
    yt_l2_fdb_op_result_t op_result;
    yt_l2_fdb_info_t fdb_info;
    yt_mac_addr_t mac_addr;
    yt_port_t port;
    yt_macid_t macid;
    uint16_t fid;

    if(CAL_L2_FDB_NUM_MAX <= index)
    {
        return CMM_ERR_EXCEED_RANGE;
    }

    CMM_ERR_CHK(hal_table_reg_mode_get(unit, &table_reg_mode), ret);

    if(HAL_REG_TBL_MODE_NORMAL == table_reg_mode)
    {
        CMM_ERR_CHK(fal_tiger_l2_fdb_op_get_one_withidx(unit, index, &mac_addr, &fid, &fdb_info, &op_result),ret);

        if(FDB_STATUS_INVALID == fdb_info.STATUS ||
            IS_MCAST_ADDR(mac_addr.addr))
        {
            return CMM_ERR_ENTRY_NOT_FOUND;
        }

        for(port =0; port < CAL_MAX_PORT_NUM; port++)
        {
            macid = CAL_YTP_TO_MAC(unit,port);
            if(IS_BIT_SET(fdb_info.DST_PORT_MASK, macid))
                break;
        }
        pUcastMac->port = port;

        pUcastMac->vid = fid;
        osal_memcpy(pUcastMac->macaddr.addr, mac_addr.addr, MAC_ADDR_LEN);
        switch(fdb_info.STATUS)
        {
            case FDB_STATUS_PENDING:
                pUcastMac->type = FDB_TYPE_PENDING;
                break;
            case FDB_STATUS_STATIC:
                pUcastMac->type = FDB_TYPE_STATIC;
                break;
            default:
                pUcastMac->type = FDB_TYPE_DYNAMIC;
                break;
        }
    }

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_system_learnlimit_cnt_set(yt_unit_t unit, uint32_t maxcnt)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_global_ctrl_t l2_learn_global_ctrl;

    if(maxcnt > L2_FDB_ENTRY_NUM)
        return CMM_ERR_INPUT;

    CMM_ERR_CHK(hal_table_reg_read(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);
    HAL_FIELD_SET(L2_LEARN_GLOBAL_CTRLm, L2_LEARN_GLOBAL_CTRL_LEARN_LIMIT_NUMf, &l2_learn_global_ctrl, maxcnt);
    CMM_ERR_CHK(hal_table_reg_write(unit, L2_LEARN_GLOBAL_CTRLm, 0, sizeof(l2_learn_global_ctrl_t), &l2_learn_global_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_l2_port_learnlimit_cnt_set(yt_unit_t unit, yt_port_t port, uint32_t maxcnt)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_learn_per_port_ctrln_t l2_learn_per_port_ctrl;
    yt_macid_t  macid;

    if(maxcnt > L2_FDB_ENTRY_NUM)
        return CMM_ERR_INPUT;

    macid = CAL_YTP_TO_MAC(unit,port);

    CMM_ERR_CHK(hal_table_reg_read(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);
    HAL_FIELD_SET(L2_LEARN_PER_PORT_CTRLNm, L2_LEARN_PER_PORT_CTRLN_LEARN_LIMIT_NUMf, &l2_learn_per_port_ctrl, maxcnt);
    CMM_ERR_CHK(hal_table_reg_write(unit, L2_LEARN_PER_PORT_CTRLNm, macid, sizeof(l2_learn_per_port_ctrl), &l2_learn_per_port_ctrl), ret);

    return CMM_ERR_OK;
}
yt_ret_t  fal_tiger_l2_lag_learnlimit_cnt_set(yt_unit_t unit, uint8_t groupid,  uint32_t maxcnt)
{
    cmm_err_t ret           = CMM_ERR_OK;
    l2_lag_learn_limit_ctrln_t l2_lag_learn_limit_ctrl;

    if(maxcnt > L2_FDB_ENTRY_NUM)
        return CMM_ERR_INPUT;

    CMM_PARAM_CHK((FAL_MAX_LAG_NUM - 1< groupid), CMM_ERR_INPUT);
    CMM_ERR_CHK(hal_table_reg_read(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    HAL_FIELD_SET(L2_LAG_LEARN_LIMIT_CTRLNm, L2_LAG_LEARN_LIMIT_CTRLN_LEARN_LIMIT_NUMf, &l2_lag_learn_limit_ctrl, maxcnt);

    CMM_ERR_CHK(hal_table_reg_write(unit, L2_LAG_LEARN_LIMIT_CTRLNm, groupid, sizeof(l2_lag_learn_limit_ctrl), &l2_lag_learn_limit_ctrl), ret);

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_l2_init(yt_unit_t unit)
{
    uint8_t id;

    fal_tiger_l2_system_learnlimit_cnt_set(unit, L2_FDB_ENTRY_NUM);
    for(id = 0; id < CAL_PORT_NUM_ON_UNIT(unit); id++)
    {
        fal_tiger_l2_port_learnlimit_cnt_set(unit, id, L2_FDB_ENTRY_NUM);
    }
    for(id = 0; id < FAL_MAX_LAG_NUM; id++)
    {
        fal_tiger_l2_lag_learnlimit_cnt_set(unit, id, L2_FDB_ENTRY_NUM);
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_port_init(yt_unit_t unit)
{
    global_ctrl1_t global_ctrl_tbl;
    yt_port_t port;
    yt_macid_t mac_id;
    cmm_err_t ret = CMM_ERR_OK;
    /* set ac drop global state enable */
    CMM_ERR_CHK(hal_table_reg_read(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &global_ctrl_tbl), ret);
    HAL_FIELD_SET(GLOBAL_CTRL1m, GLOBAL_CTRL1_AC_ENf, &global_ctrl_tbl, YT_ENABLE);
    CMM_ERR_CHK(hal_table_reg_write(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &global_ctrl_tbl), ret);

    for(port = 0; port < CAL_PORT_NUM_ON_UNIT(unit); port++)
    {
        mac_id = CAL_YTP_TO_MAC(unit, port);
        if(HALPHYDRV(unit, mac_id) != NULL)
        {
            HALPHYDRV_FUNC(unit, mac_id)->phy_init(unit);
        }
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_vlan_port_egrTpidIdx_set(yt_unit_t unit, yt_vlan_type_t  type, yt_port_t port, uint8_t tpidIdx)
{
    egr_port_ctrln_t port_ntry;
    cmm_err_t ret = CMM_ERR_OK;

    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    if(type > VLAN_TYPE_SVLAN || tpidIdx >= CAL_VLAN_TPID_PROFILE_NUM)
    {
            return CMM_ERR_INPUT;
    }

    CMM_ERR_CHK(hal_table_reg_read(unit, EGR_PORT_CTRLNm, macid, sizeof(port_ntry), &port_ntry), ret);
    if(VLAN_TYPE_CVLAN == type)
    {
        HAL_FIELD_SET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_CTAG_TPID_SELf, &port_ntry, tpidIdx);
    }
    else  if(VLAN_TYPE_SVLAN == type)
    {
        HAL_FIELD_SET(EGR_PORT_CTRLNm, EGR_PORT_CTRLN_STAG_TPID_SELf, &port_ntry, tpidIdx);
    }
    CMM_ERR_CHK(hal_table_reg_write(unit, EGR_PORT_CTRLNm, macid, sizeof(port_ntry), &port_ntry), ret);

     return CMM_ERR_OK;
}


yt_ret_t fal_tiger_vlan_egrTpid_set(yt_unit_t unit,  yt_tpid_profiles_t tpids)
{
    egr_tpid_profile_t tpid_entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint8_t i = 0;

    for(i=0; i< CAL_VLAN_TPID_PROFILE_NUM; i++)
    {
        HAL_FIELD_SET(EGR_TPID_PROFILEm, EGR_TPID_PROFILE_TPIDf, &tpid_entry, tpids.tpid[i]);
        CMM_ERR_CHK(hal_table_reg_write(unit, EGR_TPID_PROFILEm, i, sizeof(tpid_entry), &tpid_entry), ret);
    }

     return CMM_ERR_OK;
}

yt_ret_t  yt_vlan_port_igrFilter_enable_set(yt_unit_t unit, yt_port_t  port, yt_enable_t enabled)
{
    l2_vlan_ingress_filter_en_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint16_t macmask;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(hal_table_reg_read(unit, L2_VLAN_INGRESS_FILTER_ENm, 0,sizeof(l2_vlan_ingress_filter_en_t), &entry), ret);
    HAL_FIELD_GET(L2_VLAN_INGRESS_FILTER_ENm, L2_VLAN_INGRESS_FILTER_EN_FILTER_ENf, &entry, &macmask);
    if(enabled)
    {
		SET_BIT(macmask, macid);
    }
    else
    {
		CLEAR_BIT(macmask, macid);
    }
    HAL_FIELD_SET(L2_VLAN_INGRESS_FILTER_ENm, L2_VLAN_INGRESS_FILTER_EN_FILTER_ENf, &entry, macmask);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_VLAN_INGRESS_FILTER_ENm, 0, sizeof(l2_vlan_ingress_filter_en_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t  yt_vlan_port_egrFilter_enable_set(yt_unit_t unit, yt_port_t port, yt_enable_t enabled)
{
    l2_egr_vlan_filter_en_t entry;
    cmm_err_t ret = CMM_ERR_OK;
    uint16_t macmask = 0;
    yt_macid_t macid = CAL_YTP_TO_MAC(unit, port);

    CMM_ERR_CHK(hal_table_reg_read(unit, L2_EGR_VLAN_FILTER_ENm, 0,sizeof(l2_egr_vlan_filter_en_t), &entry), ret);
    HAL_FIELD_GET(L2_EGR_VLAN_FILTER_ENm, L2_EGR_VLAN_FILTER_EN_FILTER_ENf, &entry, &macmask);
    if(enabled)
    {
		SET_BIT(macmask, macid);
    }
    else
    {
		CLEAR_BIT(macmask, macid);
    }
    HAL_FIELD_SET(L2_EGR_VLAN_FILTER_ENm, L2_EGR_VLAN_FILTER_EN_FILTER_ENf, &entry, macmask);
    CMM_ERR_CHK(HAL_TBL_REG_WRITE(unit, L2_EGR_VLAN_FILTER_ENm, 0, sizeof(l2_egr_vlan_filter_en_t), &entry), ret);

    return CMM_ERR_OK;
}

yt_ret_t  fal_tiger_vlan_init(yt_unit_t unit)
{
    yt_tpid_profiles_t tpids;
    yt_port_t port;

    yt_vlan_port_set(unit, 1, YT_MAC_ALL_PORT_MASK(unit), YT_MAC_ALL_PORT_MASK(unit));

    tpids.tpid[0] = 0x8100;
    tpids.tpid[1] = 0x88a8;
    fal_tiger_vlan_egrTpid_set(unit, tpids);
    for(port = 0; port < YT_PORT_NUM; port++)
    {
        fal_tiger_vlan_port_egrTpidIdx_set(unit, VLAN_TYPE_CVLAN, port, 0);
        fal_tiger_vlan_port_egrTpidIdx_set(unit, VLAN_TYPE_SVLAN, port, 1);
    }

    return CMM_ERR_OK;
}

yt_ret_t fal_tiger_rate_init(yt_unit_t unit)
{
    global_ctrl1_t global_ctrl_tbl;
    meter_timeslot_t meter_timeslot;
    psch_shp_slot_time_cfg_t psch_time_slot;
    qsch_shp_slot_time_cfg_t qsch_time_slot;
    yt_macid_t macid;
    meter_config_tbl_t meter_config_tbl;
    psch_shp_cfg_tbl_t psch_config_tbl;
    qsch_shp_cfg_tbl_t qsch_config_tbl;
    cmm_err_t ret = CMM_ERR_OK;
    uint32_t i, ebs, cbs;
    uint32_t clksel;
    uint32_t devId;

    /* get cycle time*/
    /* TODO:for 9215,should be different according to different chip id(e.g. 9218) */
    CMM_ERR_CHK(hal_mem32_read(unit, FAL_SYS_CLK_REG, &clksel), ret);
    switch(clksel)
    {
        case 0:/*125M for 9215*/
            CMM_ERR_CHK(hal_mem32_read(unit, FAL_CHIP_DEVICE_REG, &devId), ret);
            if (YT_TIGER_DEVICE_FPGA == (devId>>16))
            {
                FAL_CHIP_CYCLE_TIME(unit) = 7;
            }
            else
            {
                FAL_CHIP_CYCLE_TIME(unit) = 8;
            }
            break;
        case 1:/*143M for 9218*/
            FAL_CHIP_CYCLE_TIME(unit) = 7;
            break;
        default:
            FAL_CHIP_CYCLE_TIME(unit) = 8;
            break;
    }

    /* set meter global state enable */
    CMM_ERR_CHK(hal_table_reg_read(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &global_ctrl_tbl), ret);
    HAL_FIELD_SET(GLOBAL_CTRL1m, GLOBAL_CTRL1_METER_ENf, &global_ctrl_tbl, YT_ENABLE);
    CMM_ERR_CHK(hal_table_reg_write(unit, GLOBAL_CTRL1m, 0, sizeof(global_ctrl1_t), &global_ctrl_tbl), ret);

    /* meter time_slot */
    CMM_ERR_CHK(hal_table_reg_read(unit, METER_TIMESLOTm, 0, sizeof(meter_timeslot_t), &meter_timeslot), ret);
    HAL_FIELD_SET(METER_TIMESLOTm, METER_TIMESLOT_TIMESLOTf, &meter_timeslot, FAL_TIGER_DEFAULT_METER_TIME_SLOT);
    CMM_ERR_CHK(hal_table_reg_write(unit, METER_TIMESLOTm, 0, sizeof(meter_timeslot_t), &meter_timeslot), ret);

    /* port shaping time_slot */
    CMM_ERR_CHK(hal_table_reg_read(unit, PSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(psch_shp_slot_time_cfg_t), &psch_time_slot), ret);
    HAL_FIELD_SET(PSCH_SHP_SLOT_TIME_CFGm, PSCH_SHP_SLOT_TIME_CFG_PSCH_SHP_SLOT_TIMEf, &psch_time_slot, FAL_TIGER_DEFAULT_PSHAP_TIME_SLOT);
    CMM_ERR_CHK(hal_table_reg_write(unit, PSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(psch_shp_slot_time_cfg_t), &psch_time_slot), ret);


    /* queue shaping time_slot */
    CMM_ERR_CHK(hal_table_reg_write(unit, QSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(qsch_shp_slot_time_cfg_t), &qsch_time_slot), ret);
    HAL_FIELD_SET(QSCH_SHP_SLOT_TIME_CFGm, QSCH_SHP_SLOT_TIME_CFG_QSCH_SHP_SLOT_TIMEf, &qsch_time_slot, FAL_TIGER_DEFAULT_QSHAP_TIME_SLOT);
    CMM_ERR_CHK(hal_table_reg_write(unit, QSCH_SHP_SLOT_TIME_CFGm, 0, sizeof(qsch_shp_slot_time_cfg_t), &qsch_time_slot), ret);

    /* vlan policy and acl policy*/
    for (i = 0; i < CAL_MAX_METER_ENTRY_NUM(unit); ++i) {
        CMM_ERR_CHK(hal_table_reg_read(unit, METER_CONFIG_TBLm, i, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_TOKEN_UNITf, &meter_config_tbl, FAL_TIGER_DEFAULT_TOKEN_UNIT);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, RATE_MODE_BYTE);
        cbs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 15)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CBS_0f, &meter_config_tbl, (cbs & 0xfff));
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CBS_1f, &meter_config_tbl, (cbs >> 12));
        ebs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 15)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EBS_0f, &meter_config_tbl, (ebs & 0x3fff));
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EBS_1f, &meter_config_tbl, (ebs >> 14));
        CMM_ERR_CHK(hal_table_reg_write(unit, METER_CONFIG_TBLm, i, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    }

    /* port policy*/
    for (i = 0; i < YT_PORT_NUM; ++i) {
        macid = CAL_YTP_TO_MAC(unit,i);
        CMM_ERR_CHK(hal_table_reg_read(unit, METER_CONFIG_TBLm, CAL_MAX_METER_ENTRY_NUM(unit) + macid, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_TOKEN_UNITf, &meter_config_tbl, FAL_TIGER_DEFAULT_TOKEN_UNIT);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_RATE_MODEf, &meter_config_tbl, RATE_MODE_BYTE);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_METER_ENf, &meter_config_tbl, YT_ENABLE);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_METER_MODEf, &meter_config_tbl, METER_MODE_RFC4115);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_COLOR_MODEf, &meter_config_tbl, COLOR_BLIND);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_DROP_COLORf, &meter_config_tbl, DROP_COLOR_YR);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CFf, &meter_config_tbl, CF_MODE_NONE);
        cbs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 15)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CBS_0f, &meter_config_tbl, (cbs & 0xfff));
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_CBS_1f, &meter_config_tbl, (cbs >> 12));
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EIRf, &meter_config_tbl, 0);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EBS_0f, &meter_config_tbl, 0);
        HAL_FIELD_SET(METER_CONFIG_TBLm, METER_CONFIG_TBL_EBS_1f, &meter_config_tbl, 0);
        CMM_ERR_CHK(hal_table_reg_write(unit, METER_CONFIG_TBLm, CAL_MAX_METER_ENTRY_NUM(unit) + macid, sizeof(meter_config_tbl_t), &meter_config_tbl), ret);
    }

    /* port shaping*/
    for (i = 0; i < YT_PORT_NUM; ++i) {
        macid = CAL_YTP_TO_MAC(unit,i);
        CMM_ERR_CHK(hal_table_reg_read(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &psch_config_tbl), ret);
        HAL_FIELD_SET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_TOKEN_LEVELf, &psch_config_tbl, FAL_TIGER_DEFAULT_TOKEN_UNIT);
        HAL_FIELD_SET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_SHAPER_MODEf, &psch_config_tbl, RATE_MODE_BYTE);
        cbs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 16)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(PSCH_SHP_CFG_TBLm, PSCH_SHP_CFG_TBL_CBSf, &psch_config_tbl, cbs);
        CMM_ERR_CHK(hal_table_reg_write(unit, PSCH_SHP_CFG_TBLm, macid, sizeof(psch_shp_cfg_tbl_t), &psch_config_tbl), ret);
    }

    /**queue shaping*/
    for (i = 0; i < FAL_MAX_PORT_NUM * (CAL_MAX_UCAST_QUEUE_NUM(unit) + CAL_MAX_MCAST_QUEUE_NUM(unit)); ++i) {
        CMM_ERR_CHK(hal_table_reg_read(unit, QSCH_SHP_CFG_TBLm, i, sizeof(qsch_shp_cfg_tbl_t), &qsch_config_tbl), ret);
        HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_TOKEN_LEVELf, &qsch_config_tbl, FAL_TIGER_DEFAULT_TOKEN_UNIT);
        HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_SHAPER_MODEf, &qsch_config_tbl, RATE_MODE_BYTE);
        //cbs = 0;
        cbs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 16)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_CBSf, &qsch_config_tbl, cbs);
        ebs = (uint32_t)((FAL_TIGER_DEFAULT_BURST_SIZE << (14 - 2 * FAL_TIGER_DEFAULT_TOKEN_UNIT) >> 16)); /// 1*unit for 2^15 tokens
        HAL_FIELD_SET(QSCH_SHP_CFG_TBLm, QSCH_SHP_CFG_TBL_EBSf, &qsch_config_tbl, ebs);
        CMM_ERR_CHK(hal_table_reg_write(unit, QSCH_SHP_CFG_TBLm, i, sizeof(qsch_shp_cfg_tbl_t), &qsch_config_tbl), ret);
    }

    return CMM_ERR_OK;
}

yt_ret_t yt_modules_init(void)
{
    yt_unit_t unit;

    for(unit = 0; unit < YT_UNIT_NUM; unit++)
    {
		fal_tiger_l2_init(unit);
        fal_tiger_port_init(unit);
        fal_tiger_vlan_init(unit);
        fal_tiger_rate_init(unit);
        fal_tiger_stat_mib_init(unit);
    }

    return CMM_ERR_OK;
}

yt_ret_t  yt_init(void)
{
    cmm_err_t ret = CMM_ERR_OK;

	CMM_ERR_CHK(yt_basic_init(),ret);

    CMM_ERR_CHK(yt_modules_init(),ret);

    return CMM_ERR_OK;
}

uint32_t yt_exit(uint8_t unit)
{
    if(!ghal_reg_table_init)
    {
        return CMM_ERR_OK;
    }

#ifdef MEM_MODE_CMODEL
    hal_table_reg_cmodel_exit();
#endif

#ifndef __KERNEL__
    osal_mux_destroy(&g_cfgmux);
#endif
    ghal_reg_table_init = FALSE;

    return CMM_ERR_OK;
}
