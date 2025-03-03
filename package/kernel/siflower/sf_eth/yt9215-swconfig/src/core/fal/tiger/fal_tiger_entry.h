/*This file is auto generated by python script. 2022-08-09 09:54:37 .*/


 #ifndef _ALL_TIGER_ENTRY_H_
 #define _ALL_TIGER_ENTRY_H_
 #define STRUCT_ENTRY(struct_name, bytes) \
         typedef struct{ \
                 uint32 entry_data[bytes]; \
         }struct_name

STRUCT_ENTRY(global_ctrl0_t, 1);
STRUCT_ENTRY(global_ctrl1_t, 1);
STRUCT_ENTRY(ext_cpu_port_ctrl_t, 1);
STRUCT_ENTRY(cpu_tag_tpid_t, 1);
STRUCT_ENTRY(look_up_vlan_sel_t, 1);
STRUCT_ENTRY(intr_mask_t, 1);
STRUCT_ENTRY(intr_status_t, 1);
STRUCT_ENTRY(global_mac_addr0_t, 1);
STRUCT_ENTRY(global_mac_addr1_t, 1);
STRUCT_ENTRY(eee_ctrl_t, 1);
STRUCT_ENTRY(sg_phy_t, 1);
STRUCT_ENTRY(port_ctrl_t, 1);
STRUCT_ENTRY(port_status_t, 1);
STRUCT_ENTRY(loop_detect_top_ctrl_t, 1);
STRUCT_ENTRY(pon_strap_en_t, 1);
STRUCT_ENTRY(pon_strap_val_t, 1);
STRUCT_ENTRY(pon_strap_t, 1);
STRUCT_ENTRY(mdio_polling_t, 1);
STRUCT_ENTRY(oam_dying_gasp_t, 1);
STRUCT_ENTRY(acl2gpio_t, 1);
STRUCT_ENTRY(extif0_mode_t, 1);
STRUCT_ENTRY(extif0_mode2_t, 1);
STRUCT_ENTRY(extif1_mode_t, 1);
STRUCT_ENTRY(extif1_mode2_t, 1);
STRUCT_ENTRY(tpid_profile0_t, 1);
STRUCT_ENTRY(tpid_profile1_t, 1);
STRUCT_ENTRY(parser_port_ctrln_t, 1);
STRUCT_ENTRY(link_agg_hash_ctrl_t, 1);
STRUCT_ENTRY(udf_ctrln_t, 1);
STRUCT_ENTRY(parser_erp_oui_t, 1);
STRUCT_ENTRY(parser_erp_ethtype_t, 1);
STRUCT_ENTRY(parser_erp_udf_ctrl_t, 1);
STRUCT_ENTRY(parser_erp_udfn_t, 1);
STRUCT_ENTRY(dos_tcp_flags0_t, 1);
STRUCT_ENTRY(dos_tcp_flags1_t, 1);
STRUCT_ENTRY(dos_tcp_flags2_t, 1);
STRUCT_ENTRY(dos_tcp_flags3_t, 1);
STRUCT_ENTRY(dos_tcp_flags4_t, 1);
STRUCT_ENTRY(dos_tcp_flags5_t, 1);
STRUCT_ENTRY(dos_ctrl_t, 1);
STRUCT_ENTRY(dos_large_icmp_ctrl_t, 1);
STRUCT_ENTRY(oam_en_ctrl_t, 1);
STRUCT_ENTRY(wol_ctrl_t, 1);
STRUCT_ENTRY(dos_ctrl1_t, 1);
STRUCT_ENTRY(protocol_based_vlann_t, 1);
STRUCT_ENTRY(port_vlan_ctrln_t, 1);
STRUCT_ENTRY(port_vlan_ctrl1n_t, 1);
STRUCT_ENTRY(vlan_trans_untag_vid_mode_ctrl_t, 1);
STRUCT_ENTRY(vlan_range_profilen_t, 3);
STRUCT_ENTRY(protocol_based_vlan_data_tbl_t, 1);
STRUCT_ENTRY(vlan_xlate_action_tbl_t, 2);
STRUCT_ENTRY(vlan_xlate_tbl_t, 2);
STRUCT_ENTRY(acl_blk_keep_ctrl_t, 1);
STRUCT_ENTRY(acl_port_ctrl_t, 1);
STRUCT_ENTRY(acl_blk_cmd_t, 1);
STRUCT_ENTRY(rule_ext_ctrl_t, 1);
STRUCT_ENTRY(acl_rule_bin0_t, 2);
STRUCT_ENTRY(acl_rule_bin1_t, 2);
STRUCT_ENTRY(acl_rule_bin2_t, 2);
STRUCT_ENTRY(acl_rule_bin3_t, 2);
STRUCT_ENTRY(acl_rule_bin4_t, 2);
STRUCT_ENTRY(acl_rule_bin5_t, 2);
STRUCT_ENTRY(acl_rule_bin6_t, 2);
STRUCT_ENTRY(acl_rule_bin7_t, 2);
STRUCT_ENTRY(acl_rule_mask_bin0_t, 2);
STRUCT_ENTRY(acl_rule_mask_bin1_t, 2);
STRUCT_ENTRY(acl_rule_mask_bin2_t, 2);
STRUCT_ENTRY(acl_rule_mask_bin3_t, 2);
STRUCT_ENTRY(acl_rule_mask_bin4_t, 2);
STRUCT_ENTRY(acl_rule_mask_bin5_t, 2);
STRUCT_ENTRY(acl_rule_mask_bin6_t, 2);
STRUCT_ENTRY(acl_rule_mask_bin7_t, 2);
STRUCT_ENTRY(mac_da0_rule_t, 2);
STRUCT_ENTRY(mac_da0_rule_mask_t, 2);
STRUCT_ENTRY(mac_da1_sa1_rule_t, 2);
STRUCT_ENTRY(mac_da1_sa1_rule_mask_t, 2);
STRUCT_ENTRY(mac_sa0_rule_t, 2);
STRUCT_ENTRY(mac_sa0_rule_mask_t, 2);
STRUCT_ENTRY(ipv4_da_rule_t, 2);
STRUCT_ENTRY(ipv4_da_rule_mask_t, 2);
STRUCT_ENTRY(ipv4_sa_rule_t, 2);
STRUCT_ENTRY(ipv4_sa_rule_mask_t, 2);
STRUCT_ENTRY(l4_port_rule_t, 2);
STRUCT_ENTRY(l4_port_rule_mask_t, 2);
STRUCT_ENTRY(vlan_rule_t, 2);
STRUCT_ENTRY(vlan_rule_mask_t, 2);
STRUCT_ENTRY(misc_rule_t, 2);
STRUCT_ENTRY(misc_rule_mask_t, 2);
STRUCT_ENTRY(udf_rule_t, 2);
STRUCT_ENTRY(udf_rule_mask_t, 2);
STRUCT_ENTRY(ipv6_da0_rule_t, 2);
STRUCT_ENTRY(ipv6_da0_rule_mask_t, 2);
STRUCT_ENTRY(ipv6_da1_rule_t, 2);
STRUCT_ENTRY(ipv6_da1_rule_mask_t, 2);
STRUCT_ENTRY(ipv6_da2_rule_t, 2);
STRUCT_ENTRY(ipv6_da2_rule_mask_t, 2);
STRUCT_ENTRY(ipv6_da3_rule_t, 2);
STRUCT_ENTRY(ipv6_da3_rule_mask_t, 2);
STRUCT_ENTRY(ipv6_sa0_rule_t, 2);
STRUCT_ENTRY(ipv6_sa0_rule_mask_t, 2);
STRUCT_ENTRY(ipv6_sa1_rule_t, 2);
STRUCT_ENTRY(ipv6_sa1_rule_mask_t, 2);
STRUCT_ENTRY(ipv6_sa2_rule_t, 2);
STRUCT_ENTRY(ipv6_sa2_rule_mask_t, 2);
STRUCT_ENTRY(ipv6_sa3_rule_t, 2);
STRUCT_ENTRY(ipv6_sa3_rule_mask_t, 2);
STRUCT_ENTRY(vid_pri_dei_rule_t, 2);
STRUCT_ENTRY(vid_pri_dei_rule_mask_t, 2);
STRUCT_ENTRY(ether_type_value_rule_t, 2);
STRUCT_ENTRY(ether_type_value_rule_mask_t, 2);
STRUCT_ENTRY(dscp_to_int_prio_map_t, 1);
STRUCT_ENTRY(pri_to_int_prio_map_t, 1);
STRUCT_ENTRY(qos_port_ctrln_t, 1);
STRUCT_ENTRY(qos_merge_precedence_ctrln_t, 1);
STRUCT_ENTRY(hdr_err_action_t, 1);
STRUCT_ENTRY(l2_vlan_ingress_filter_en_t, 1);
STRUCT_ENTRY(l2_arp_bcast_per_port_ctrl_t, 1);
STRUCT_ENTRY(l2_nd_per_port_ctrl_t, 1);
STRUCT_ENTRY(l2_lldp_eee_per_port_ctrl_t, 1);
STRUCT_ENTRY(l2_lldp_per_port_ctrl_t, 1);
STRUCT_ENTRY(l2_port_isolation_ctrln_t, 1);
STRUCT_ENTRY(l2_erp_per_port_ctrl_t, 1);
STRUCT_ENTRY(l2_stp_staten_t, 1);
STRUCT_ENTRY(l2_src_match_ctrl_t, 1);
STRUCT_ENTRY(l2_learn_per_port_ctrln_t, 1);
STRUCT_ENTRY(l2_station_move_ctrl0_t, 1);
STRUCT_ENTRY(l2_station_move_ctrl1_t, 1);
STRUCT_ENTRY(l2_port_learn_mac_cntn_t, 1);
STRUCT_ENTRY(l2_learn_global_ctrl_t, 1);
STRUCT_ENTRY(l2_learn_mac_cnt_t, 1);
STRUCT_ENTRY(l2_aging_ctrl_t, 1);
STRUCT_ENTRY(l2_aging_per_port_ctrl_t, 1);
STRUCT_ENTRY(l2_fdb_tbl_op_data_0_t, 1);
STRUCT_ENTRY(l2_fdb_tbl_op_data_1_t, 2);
STRUCT_ENTRY(l2_fdb_tbl_op_data_2_t, 1);
STRUCT_ENTRY(l2_fdb_tbl_op_t, 1);
STRUCT_ENTRY(l2_fdb_tbl_op_result_t, 1);
STRUCT_ENTRY(l2_igmp_static_router_port_mask_t, 1);
STRUCT_ENTRY(l2_igmp_dynamic_router_port_ctrl_t, 1);
STRUCT_ENTRY(l2_igmp_dynamic_router_port_t, 1);
STRUCT_ENTRY(l2_igmp_dynamic_router_port0_timer_t, 1);
STRUCT_ENTRY(l2_igmp_dynamic_router_port1_timer_t, 1);
STRUCT_ENTRY(l2_igmp_global_ctrl_t, 1);
STRUCT_ENTRY(l2_igmp_per_port_ctrl_t, 1);
STRUCT_ENTRY(l2_fdb_tbl_op_data_0_dummy_t, 1);
STRUCT_ENTRY(l2_fdb_tbl_op_data_1_dummy_t, 1);
STRUCT_ENTRY(l2_fdb_tbl_op_data_2_dummy_t, 1);
STRUCT_ENTRY(l2_igmp_dynamic_router_port_dummy_t, 1);
STRUCT_ENTRY(l2_igmp_dynamic_router_port0_timer_dummy_t, 1);
STRUCT_ENTRY(l2_igmp_dynamic_router_port1_timer_dummy_t, 1);
STRUCT_ENTRY(l2_igmp_learn_ctrl_t, 1);
STRUCT_ENTRY(l2_igmp_learn_group_cnt_t, 1);
STRUCT_ENTRY(l2_unknown_ucast_filter_mask_t, 1);
STRUCT_ENTRY(l2_unknown_mcast_filter_mask_t, 1);
STRUCT_ENTRY(l2_mcast_filter_mask_t, 1);
STRUCT_ENTRY(l2_bcast_filter_mask_t, 1);
STRUCT_ENTRY(l2_port_vlan_transparent_ctrl_t, 1);
STRUCT_ENTRY(l2_egr_vlan_filter_en_t, 1);
STRUCT_ENTRY(l2_dot1x_ctrl1_t, 1);
STRUCT_ENTRY(l2_dot1x_ctrl2_t, 1);
STRUCT_ENTRY(ipmc_leaky_ctrl_t, 1);
STRUCT_ENTRY(link_agg_groupn_t, 1);
STRUCT_ENTRY(link_agg_membern_t, 1);
STRUCT_ENTRY(rma_ctrln_t, 1);
STRUCT_ENTRY(cpu_copy_dst_ctrl_t, 1);
STRUCT_ENTRY(acl_unmatch_permit_enable_ctrl_t, 1);
STRUCT_ENTRY(oam_mux_act_t, 1);
STRUCT_ENTRY(oam_par_act_t, 1);
STRUCT_ENTRY(cascade_ctrl_t, 1);
STRUCT_ENTRY(loop_detect_act_ctrl_t, 1);
STRUCT_ENTRY(igmp_router_port_aging_ctrl_t, 1);
STRUCT_ENTRY(multi_vlan_tbl_t, 1);
STRUCT_ENTRY(l2_learn_clear_op_t, 1);
STRUCT_ENTRY(l2_uc_unknown_act_ctrl_t, 1);
STRUCT_ENTRY(l2_mc_unknown_act_ctrl_t, 1);
STRUCT_ENTRY(l2_lag_learn_limit_ctrln_t, 1);
STRUCT_ENTRY(l2_loop_detect_flag_t, 1);
STRUCT_ENTRY(l2_loop_detect_flag_dummy_t, 1);
STRUCT_ENTRY(l2_loop_detect_timer_t, 1);
STRUCT_ENTRY(l2_arp_bcast_per_port_ctrl1_t, 1);
STRUCT_ENTRY(l2_nd_per_port_ctrl1_t, 1);
STRUCT_ENTRY(l2_lldp_eee_per_port_ctrl1_t, 1);
STRUCT_ENTRY(l2_lldp_per_port_ctrl1_t, 1);
STRUCT_ENTRY(l2_arp_bcast_nd_per_port_ctrl2_t, 1);
STRUCT_ENTRY(l2_lldp_eee_per_port_ctrl2_t, 1);
STRUCT_ENTRY(l2_fdb_hw_flush_ctrl_t, 1);
STRUCT_ENTRY(l2_uni_que_ctrl_t, 1);
STRUCT_ENTRY(l2_vlan_tbl_t, 2);
STRUCT_ENTRY(l2_fdb_tbl_bin0_t, 3);
STRUCT_ENTRY(l2_fdb_tbl_bin1_t, 3);
STRUCT_ENTRY(l2_fdb_tbl_bin2_t, 3);
STRUCT_ENTRY(l2_fdb_tbl_bin3_t, 3);
STRUCT_ENTRY(l2_fdb_tbl_bin4_t, 3);
STRUCT_ENTRY(l2_fdb_tbl_bin5_t, 3);
STRUCT_ENTRY(l2_fdb_tbl_bin6_t, 3);
STRUCT_ENTRY(l2_fdb_tbl_bin7_t, 3);
STRUCT_ENTRY(acl_action_tbl_t, 3);
STRUCT_ENTRY(port_rate_ctrln_t, 1);
STRUCT_ENTRY(storm_ctrl_timeslot_t, 1);
STRUCT_ENTRY(meter_timeslot_t, 1);
STRUCT_ENTRY(port_meter_ctrln_t, 1);
STRUCT_ENTRY(storm_ctrl_mc_type_ctrl_t, 1);
STRUCT_ENTRY(drop_event_log_t, 1);
STRUCT_ENTRY(stats_offset_t, 1);
STRUCT_ENTRY(storm_ctrl_config_tbl_t, 1);
STRUCT_ENTRY(storm_ctrl_cnt_tbl_t, 1);
STRUCT_ENTRY(meter_config_tbl_t, 3);
STRUCT_ENTRY(meter_token_tbl_t, 2);
STRUCT_ENTRY(flow_stats_tbl_t, 2);
STRUCT_ENTRY(flow_stats_cfg_tbl_t, 1);
STRUCT_ENTRY(cpu_code_to_cpu_prio_mapn_t, 1);
STRUCT_ENTRY(int_prio_to_ucast_qid_mapn_t, 1);
STRUCT_ENTRY(int_prio_to_mcast_qid_mapn_t, 1);
STRUCT_ENTRY(mirror_ctrl_t, 1);
STRUCT_ENTRY(mirror_qos_ctrl_t, 1);
STRUCT_ENTRY(flush_cfg_t, 1);
STRUCT_ENTRY(oq_enq_dis_tbl_t, 1);
STRUCT_ENTRY(egr_port_ctrln_t, 1);
STRUCT_ENTRY(egr_port_vlan_ctrln_t, 1);
STRUCT_ENTRY(egr_dscp_remarkn_t, 1);
STRUCT_ENTRY(egr_prio_remarkn_t, 1);
STRUCT_ENTRY(egr_tpid_profile_t, 1);
STRUCT_ENTRY(egr_vlan_trans_rule_ctrln_t, 1);
STRUCT_ENTRY(egr_vlan_trans_rule_ctrl1n_t, 1);
STRUCT_ENTRY(egr_vlan_trans_data_ctrln_t, 1);
STRUCT_ENTRY(egr_vlan_tag_transparent_ctrl_t, 1);
STRUCT_ENTRY(cpu_pkt_bypassedit_ctrl_t, 1);
STRUCT_ENTRY(ipg_pre_len_cfg_t, 1);
STRUCT_ENTRY(qsch_shp_slot_time_cfg_t, 1);
STRUCT_ENTRY(psch_shp_slot_time_cfg_t, 1);
STRUCT_ENTRY(qsch_flow_map_tbl_t, 1);
STRUCT_ENTRY(qsch_c_dwrr_cfg_tbl_t, 1);
STRUCT_ENTRY(qsch_e_dwrr_cfg_tbl_t, 1);
STRUCT_ENTRY(qsch_shp_cfg_tbl_t, 3);
STRUCT_ENTRY(qsch_meter_cfg_tbl_t, 1);
STRUCT_ENTRY(psch_shp_cfg_tbl_t, 2);
STRUCT_ENTRY(psch_meter_cfg_tbl_t, 1);
#endif
