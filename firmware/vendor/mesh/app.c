/********************************************************************************************************
 * @file	app.c
 *
 * @brief	for TLSR chips
 *
 * @author	telink
 * @date	Sep. 30, 2010
 *
 * @par     Copyright (c) 2017, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "proj_lib/pm.h"
#include "proj_lib/ble/ll/ll.h"
#include "proj_lib/ble/blt_config.h"
#include "proj_lib/ble/ll/ll_whitelist.h"
#include "proj_lib/ble/trace.h"
#include "proj_lib/ble/ble_common.h"
#include "proj_lib/ble/service/ble_ll_ota.h"
#include "proj_lib/ble/blt_config.h"
#include "proj_lib/ble/ble_smp.h"
#include "proj_lib/mesh_crypto/mesh_crypto.h"
#include "proj_lib/mesh_crypto/mesh_md5.h"
#include "proj_lib/mesh_crypto/sha256_telink.h"
#include "proj_lib/sig_mesh/app_mesh.h"
#include "../common/app_provison.h"
#include "../common/app_beacon.h"
#include "../common/app_proxy.h"
#include "../common/app_health.h"
#include "../common/vendor_model.h"
//#include "proj/drivers/keyboard.h"
#include "app.h"
#if __TLSR_RISCV_EN__
#include "stack/ble/host/gap/gap.h"
#else
#include "stack/ble/gap/gap.h"
#endif
#include "proj_lib/ble/l2cap.h"
#include "vendor/common/blt_soft_timer.h"
#include "stack/ble/ble.h"
#include "app_buffer.h"
//#include "proj/drivers/rf_pa.h"
#include "../common/remote_prov.h"
#if MI_API_ENABLE
#include "vendor/common/mi_api/telink_sdk_mible_api.h"
#include "vendor/common/mi_api/libs/mijia_profiles/mi_service_server.h"
#include "vendor/common/mi_api/libs/mesh_auth/mible_mesh_auth.h"
#include "vendor/common/mi_api/telink_sdk_mesh_api.h"
#include "gatt_dfu/mible_dfu_main.h"
#include "mible_log.h"
#endif 
#if (HCI_ACCESS==HCI_USE_UART)
#include "drivers.h"
#endif

#if DU_ENABLE
#include "vendor/common/user_du.h"
#include "vendor/common/mi_api/telink_sdk_mible_api.h"
#endif

#if 1
// refert to "app_buffer.c/app_buffer.h"
#else
#define BLT_RX_FIFO_SIZE        (MESH_DLE_MODE ? DLE_RX_FIFO_SIZE : 64)
#define BLT_TX_FIFO_SIZE        (MESH_DLE_MODE ? DLE_TX_FIFO_SIZE : 40)
#if GATT_LPN_EN
MYFIFO_INIT(blt_rxfifo, BLT_RX_FIFO_SIZE, 8); //adv_filter_proc() reserve (BLE_RCV_FIFO_MAX_CNT+2) buf while gatt connecting.
MYFIFO_INIT(blt_txfifo, BLT_TX_FIFO_SIZE, 16); // set to 16, because there is no blt_notify_fifo_
#else
    #if DEBUG_CFG_CMD_GROUP_AK_EN
MYFIFO_INIT(blt_rxfifo, BLT_RX_FIFO_SIZE, 128);  // some phones may not support DLE, so use the same count with no DLE.
    #else
MYFIFO_INIT(blt_rxfifo, BLT_RX_FIFO_SIZE, 16);  // some phones may not support DLE, so use the same count with no DLE.
    #endif
	#if DU_LPN_EN
#define BLT_TX_FIFO_CNT			32// low down the rentention cost part .
	#else
#define BLT_TX_FIFO_CNT         (MESH_BLE_NOTIFY_FIFO_EN ? 32 : 128) // set to 128 in extend mode, because there is no blt_notify_fifo_
	#endif
MYFIFO_INIT(blt_txfifo, BLT_TX_FIFO_SIZE, BLT_TX_FIFO_CNT);  // some phones may not support DLE, so use the same count with no DLE.
#endif
#endif


// u8		peer_type;
// u8		peer_mac[12];

//////////////////////////////////////////////////////////////////////////////
//	Initialization: MAC address, Adv Packet, Response Packet
//////////////////////////////////////////////////////////////////////////////

//----------------------- UI ---------------------------------------------
#if (BLT_SOFTWARE_TIMER_ENABLE)
/**
 * @brief   This function is soft timer callback function.
 * @return  <0:delete the timer; ==0:timer use the same interval as prior; >0:timer use the return value as new interval. 
 */
int soft_timer_test0(void)
{
	//gpio 0 toggle to see the effect
	DBG_CHN4_TOGGLE;
	static u32 soft_timer_cnt0 = 0;
	soft_timer_cnt0++;
	return 0;
}
#endif
#if MI_SWITCH_LPN_EN
void mi_mesh_switch_sys_mode(u32 clk)
{
	// ignore hard ware uart function ,and it need to reset all the param part 
	if(clk == 16000000){
		clock_init(SYS_CLK_16M_Crystal);

	}else if (clk == 48000000){
		clock_init(SYS_CLK_48M_Crystal);
	}else{}
}

#endif

#define TEST_FORWARD_ADDR_FILTER_EN     0
#if TEST_FORWARD_ADDR_FILTER_EN
#define FILTER_TYPE_F2	1
#define FILTER_TYPE_F4	2
#define FILTER_TYPE_F6	3
#define FILTER_TYPE_F8	4
#define FILTER_TYPE_FA	5
#define FILTER_TYPE_FC	6

#define FILTER_NODE_MAC_F1 	{0xf1,0x22,0x33,0x44,0xff,0xff}
#define FILTER_NODE_MAC_F2 	{0xf2,0x22,0x33,0x44,0xff,0xff}
#define FILTER_NODE_MAC_F4	{0xf4,0x22,0x33,0x44,0xff,0xff}
#define FILTER_NODE_MAC_F6	{0xf6,0x22,0x33,0x44,0xff,0xff}
#define FILTER_NODE_MAC_F8	{0xf8,0x22,0x33,0x44,0xff,0xff}
#define FILTER_NODE_MAC_FA	{0xfa,0x22,0x33,0x44,0xff,0xff}
#define FILTER_NODE_MAC_FC	{0xfc,0x22,0x33,0x44,0xff,0xff}


typedef struct{
	u8 mac[6];
}filter_mac_str;

#define FORWARD_FILTER_TYPE		FILTER_TYPE_FA
	#if (FORWARD_FILTER_TYPE == FILTER_TYPE_F2)
	filter_mac_str forward_mac[1]={FILTER_NODE_MAC_F4};
	#elif(FORWARD_FILTER_TYPE == FILTER_TYPE_F4)
	filter_mac_str forward_mac[2]={FILTER_NODE_MAC_F2,FILTER_NODE_MAC_F6};
	#elif(FORWARD_FILTER_TYPE == FILTER_TYPE_F6)
	filter_mac_str forward_mac[2]={FILTER_NODE_MAC_F4,FILTER_NODE_MAC_F8};
	#elif(FORWARD_FILTER_TYPE == FILTER_TYPE_F8)
	filter_mac_str forward_mac[2]={FILTER_NODE_MAC_F6,FILTER_NODE_MAC_FA};
	#elif(FORWARD_FILTER_TYPE == FILTER_TYPE_FA)
	filter_mac_str forward_mac[2]={FILTER_NODE_MAC_F8,FILTER_NODE_MAC_FC};
	#elif(FORWARD_FILTER_TYPE == FILTER_TYPE_FC)
	filter_mac_str forward_mac[1]=FILTER_NODE_MAC_FA;
	#endif

u8 find_mac_in_filter_list(u8 *p_mac)
{
	foreach_arr(i,forward_mac){
		u8 *p_mac_filter = (u8 *)(forward_mac+i);
		if(!memcmp(p_mac_filter,p_mac,sizeof(filter_mac_str))){
			return 1;
		}
	}
	return 0;
}
#endif


//----------------------- handle BLE event ---------------------------------------------
int app_event_handler (u32 h, u8 *p, int n)
{
	static u32 event_cb_num;
	event_cb_num++;
	int send_to_hci = 1;

	if (h == (HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META))		//LE event
	{
		u8 subcode = p[0];
		#if MI_API_ENABLE
		telink_ble_mi_app_event(subcode,p,n);
		#endif 
	//------------ ADV packet --------------------------------------------
		if (subcode == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
		{
			event_adv_report_t *pa = (event_adv_report_t *)p;
			#if MD_REMOTE_PROV
				#if REMOTE_PROV_SCAN_GATT_EN
			mesh_cmd_conn_prov_adv_cb(pa);// only for the remote gatt-provision proc part.
				#endif
			mesh_cmd_extend_loop_cb(pa);
			#endif
			if(LL_TYPE_ADV_NONCONN_IND != (pa->event_type & 0x0F)){
				#if DU_ULTRA_PROV_EN
				app_event_handler_ultra_prov(pa->data);
				#endif
				
				return 0;
			}
			#if TEST_FORWARD_ADDR_FILTER_EN
			if(!find_mac_in_filter_list(pa->mac)){
				return 0;
			}
			#endif
			#if 0 // TESTCASE_FLAG_ENABLE
			u8 mac_pts[] = {0xDA,0xE2,0x08,0xDC,0x1B,0x00};	// 0x001BDC08E2DA
			u8 mac_pts2[] = {0xDB,0xE2,0x08,0xDC,0x1B,0x00};	// 0x001BDC08E2DA
			if(memcmp(pa->mac, mac_pts,6) && memcmp(pa->mac, mac_pts2,6)){
				return 0;
			}
			#endif
			
			#if DEBUG_MESH_DONGLE_IN_VC_EN
			send_to_hci = (0 == mesh_dongle_adv_report2vc(pa->data, MESH_ADV_PAYLOAD));
			#else
			send_to_hci = (0 == app_event_handler_adv(pa->data, MESH_BEAR_ADV, 1));
			#endif
		}

	//------------ connection complete -------------------------------------
		else if (subcode == HCI_SUB_EVT_LE_CONNECTION_COMPLETE)	// connection complete
		{
			#if MI_SWITCH_LPN_EN
			mi_mesh_switch_sys_mode(48000000);
			bls_ll_setAdvParam( ADV_INTERVAL_MIN, ADV_INTERVAL_MAX, \
			 	 	 	 	 	     ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
			 	 	 	 	 	     0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
			#endif
			#if MI_API_ENABLE
			mible_status_t status = MI_SUCCESS;
            if (NULL == mible_conn_timer){
                status = mible_timer_create(&mible_conn_timer, mible_conn_timeout_cb,
                                                               MIBLE_TIMER_SINGLE_SHOT);
            }
			if (MI_SUCCESS != status){
                MI_LOG_ERROR("mible_conn_timer: fail, timer is not created");
            }else{
        		mible_conn_handle = 1;
                mible_timer_start(mible_conn_timer, 20*1000, NULL);
                MI_LOG_DEBUG("mible_conn_timer: succ, timer is created");
            }
			#endif
			event_connection_complete_t *pc = (event_connection_complete_t *)p;
			if (!pc->status)							// status OK
			{
				//app_led_en (pc->handle, 1);

				//peer_type = pc->peer_adr_type;
				//memcpy (peer_mac, pc->mac, 6);
			}
			#if DU_LPN_EN
	        LOG_MSG_INFO(TL_LOG_NODE_SDK,0,0,"connect suc",0);	
				#if !LPN_CONTROL_EN
			blc_ll_setScanEnable (0, 0);
			mi_mesh_state_set(0);
				#endif
			#endif
			#if DEBUG_BLE_EVENT_ENABLE
			rf_link_light_event_callback(LGT_CMD_BLE_CONN);
			#endif

			#if DEBUG_MESH_DONGLE_IN_VC_EN
			debug_mesh_report_BLE_st2usb(1);
			#endif
			proxy_cfg_list_init_upon_connection();
			#if 0 // FEATURE_FRIEND_EN
			fn_update_RecWin(get_RecWin_connected());
			#endif
			#if !DU_ENABLE
			mesh_service_change_report();
			#endif
			#if LPN_CONTROL_EN
			bls_l2cap_requestConnParamUpdate (48, 56, 10, 500);
			#endif
			
		}

	//------------ connection update complete -------------------------------
		else if (subcode == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)	// connection update
		{
			#if 0 // FEATURE_FRIEND_EN
			fn_update_RecWin(get_RecWin_connected());
			#endif
		}
	}

	//------------ disconnect -------------------------------------
	else if (h == (HCI_FLAG_EVENT_BT_STD | HCI_EVT_DISCONNECTION_COMPLETE))		//disconnect
	{
		#if MI_SWITCH_LPN_EN
		mi_mesh_switch_sys_mode(16000000);
		#endif
		#if DU_ENABLE
		clock_init(SYS_CLK_16M_Crystal);
		blc_ll_setScanEnable (BLS_FLAG_SCAN_ENABLE | BLS_FLAG_ADV_IN_SLAVE_MODE, 0);
		if(p_ota->ota_suc){
			//LOG_MSG_INFO(TL_LOG_NODE_SDK,0,0,"ota reboot ,when ble is disconnct!",0);
			du_ota_suc_reboot();			
		}
		#endif
		event_disconnection_t	*pd = (event_disconnection_t *)p;
		//app_led_en (pd->handle, 0);
		#if MI_API_ENABLE
		telink_ble_mi_app_event(HCI_EVT_DISCONNECTION_COMPLETE,p,n);
		mible_conn_handle = 0xffff;
		mible_timer_stop(mible_conn_timer);
		#endif 
		//terminate reason
		if(pd->reason == HCI_ERR_CONN_TIMEOUT){

		}
		else if(pd->reason == HCI_ERR_REMOTE_USER_TERM_CONN){  //0x13

		}
		#if 0 // TODO
		else if(pd->reason == SLAVE_TERMINATE_CONN_ACKED || pd->reason == SLAVE_TERMINATE_CONN_TIMEOUT){

		}
		#endif
		LOG_MSG_INFO(TL_LOG_NODE_SDK,0,0,"disconnect reason is %x",pd->reason);
		#if DEBUG_BLE_EVENT_ENABLE
		rf_link_light_event_callback(LGT_CMD_BLE_ADV);
		#endif 

		#if DEBUG_MESH_DONGLE_IN_VC_EN
		debug_mesh_report_BLE_st2usb(0);
		#endif

		mesh_ble_disconnect_cb(pd->reason);
		#if 0 // FEATURE_FRIEND_EN
        fn_update_RecWin(FRI_REC_WIN_MS);   // restore
        #endif
	}

	if (send_to_hci)
	{
		//blc_hci_send_data (h, p, n);
	}

	return 0;
}

#define SCAN_IO_INTERVA_DEFL_US		(40000)

void proc_ui()
{
	static u32 tick, scan_io_interval_us = SCAN_IO_INTERVA_DEFL_US;
	if (!clock_time_exceed (tick, scan_io_interval_us))
	{
		return;
	}
	tick = clock_time();
	scan_io_interval_us = SCAN_IO_INTERVA_DEFL_US;

	#if EXTENDED_ADV_TX_TEST_EN
	static u32 tick2, scan_io_interval_us2 = 1000*1000;
	if (!clock_time_exceed (tick2, scan_io_interval_us2)){
		return;
	}
	tick2 = clock_time();
	transition_par_t tran = {0};
	static u32 B_0cnt;
	access_cmd_onoff(0xffff, 0, B_0cnt++&1, CMD_NO_ACK, &tran);
	return ;
	#endif

	#if AUDIO_MESH_EN
	proc_ui_audio();
	return ;
	#endif
	
	#if 0
	static u8 st_sw1_last,st_sw2_last;	
	u8 st_sw1 = !gpio_read(SW1_GPIO);
	u8 st_sw2 = !gpio_read(SW2_GPIO);
	
	if(!(st_sw1_last)&&st_sw1){
	    scan_io_interval_us = 100*1000; // fix dithering
	    access_cmd_onoff(0xffff, 0, G_ON, CMD_NO_ACK, 0);
		foreach(i,NET_KEY_MAX){
					mesh_key.net_key[i][0].node_identity =1;
		}
	}
	st_sw1_last = st_sw1;
	
	if(!(st_sw2_last)&&st_sw2){
	    scan_io_interval_us = 100*1000; // fix dithering
	    access_cmd_onoff(0xffff, 0, G_OFF, CMD_NO_ACK, 0);
	}
	st_sw2_last = st_sw2;

	
	#endif

	#if 0
	static u8 st_sw2_last;	
	u8 st_sw2 = !gpio_read(SW2_GPIO);
	
	if(!(st_sw2_last)&&st_sw2){ // dispatch just when you press the button 
		//trigger the unprivison data packet 
		static u8 beacon_data_num;
		beacon_data_num =1;
		mesh_provision_para_reset();
		while(beacon_data_num--){
			unprov_beacon_send(MESH_UNPROVISION_BEACON_WITH_URI,0);
		}
		prov_para.initial_pro_roles = MESH_INI_ROLE_NODE;
	    scan_io_interval_us = 100*1000; // fix dithering
	}
	st_sw2_last = st_sw2;
	#endif
	
	#if IV_UPDATE_TEST_EN
	mesh_iv_update_test_initiate();
	#endif
}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
#if 0
u8 notify_test_flag =0;
u8 notify_test_buf[19];
void test_sig_mesh_cmd_fun()
{
	if(notify_test_flag){
		static u32 tick_notify_test_tick =0;
		static u16 A_debug_sts_level =0;
		int ret_tmp =-1;
		if(!clock_time_exceed(tick_notify_test_tick,10*1000)){
			return;
		}	
		tick_notify_test_tick = clock_time();
		ret_tmp = mesh_tx_cmd_rsp(G_LEVEL_STATUS, (u8 *)&A_debug_sts_level, sizeof(A_debug_sts_level), ele_adr_primary, 
						ele_adr_primary, 0, 0);
		if(A_debug_notify_pkt_sts == BLE_SUCCESS && ret_tmp == 0){
			A_debug_notify_pkt_sts = HCI_ERR_MAC_CONN_FAILED;
			A_debug_sts_level++;
		}
	}
}
#endif
// simu uart io printf demo 
#if 0
void test_simu_io_user_define_proc()
{
    static u32 A_debug_print_tick =0;
    u8 data_test[4]={1,2,3,4};
    u8 data_val = 0xff;
    if(clock_time_exceed(A_debug_print_tick,100*1000)){
        A_debug_print_tick = clock_time();
        LOG_USER_MSG_INFO(data_test,sizeof(data_test),"user data val is %d",data_val);
    }
}
#endif

#if (GATT_LPN_EN)
int soft_timer_send_mesh_adv()
{
	int ret = -1;	
	mesh_send_adv2scan_mode(1);
	if(my_fifo_get(&mesh_adv_cmd_fifo)){		
		ret = get_mesh_adv_interval();
	}
	return ret;
}

void soft_timer_mesh_adv_proc()
{
	if(my_fifo_data_cnt_get(&mesh_adv_cmd_fifo)){
		if(!is_soft_timer_exist(&soft_timer_send_mesh_adv)){
			blt_soft_timer_update(&soft_timer_send_mesh_adv, get_mesh_adv_interval());
		}
	}
}
#endif

_attribute_no_inline_ void main_loop () // must add no inline, or it will be inline to main_() which is ramcode.
{
	static u32 tick_loop;

	tick_loop ++;
#if (BLT_SOFTWARE_TIMER_ENABLE)
	#if GATT_LPN_EN
	soft_timer_mesh_adv_proc();
	#endif
	blt_soft_timer_process(MAINLOOP_ENTRY);
#endif
	mesh_loop_proc_prior(); // priority loop, especially for 8269
//	test_simu_io_user_define_proc();
	#if DUAL_MESH_ZB_BL_EN
	if(RF_MODE_BLE != dual_mode_proc()){    // should be before is mesh latency window()
        proc_ui();
        proc_led();
        factory_reset_cnt_check();
		return ;
	}
	#endif
	#if DUAL_MESH_SIG_PVT_EN
	dual_mode_proc();
	#endif
	#if SIG_MESH_LOOP_PROC_10MS_EN
	if(is_mesh_latency_window()){
	    return ;
	}
	#endif
	
	#if SPIRIT_VENDOR_EN
	mesh_tx_indication_proc();
	#if 0 // for indication test
	static u8 A_send_indication=0;
	if(A_send_indication){
		A_send_indication = 0;
		u16 val= 0x3344;
		access_cmd_attr_indication(VD_MSG_ATTR_INDICA,0xffff,ATTR_CURRENT_TEMP, (u8 *)&val, sizeof(val));
	}
	#endif	
	#endif
	////////////////////////////////////// BLE entry /////////////////////////////////
	blt_sdk_main_loop ();


	////////////////////////////////////// UI entry /////////////////////////////////
	//  add spp UI task:
#if (BATT_CHECK_ENABLE)
    app_battery_power_check_and_sleep_handle(1);
#endif
	// du proc
	#if DU_ENABLE
	du_loop_proc();
	#endif
	#if !DU_LPN_EN
	proc_ui();
	proc_led();
	factory_reset_cnt_check();
	#endif
	#if DU_LPN_EN
		#if LPN_CONTROL_EN
		extern u8 save_power_mode ;
	if(is_provision_success()||save_power_mode == 0)
		#else
	if(is_provision_success()||mi_mesh_get_state())
		#endif
	{
		mesh_loop_process();
	}else{
		#if RTC_USE_32K_RC_ENABLE
		system_time_run();
		#endif
	} 
	#else
	mesh_loop_process();
	#endif
	#if MI_API_ENABLE
	ev_main();
	#if XIAOMI_MODULE_ENABLE
	mi_api_loop_run();
	#endif
	mi_schd_process();
	#endif 
	
	#if ADC_ENABLE
	static u32 adc_check_time;
    if(clock_time_exceed(adc_check_time, 1000*1000)){
        adc_check_time = clock_time();
		volatile static u16 T_adc_val;
		
		#if (BATT_CHECK_ENABLE)
		app_battery_check_and_re_init_user_adc();
		#endif
		#if(MCU_CORE_TYPE == MCU_CORE_8269)     
        T_adc_val = adc_BatteryValueGet();
		#else
		T_adc_val = adc_sample_and_get_result();
		#endif
    }  
	#endif
	//sim_tx_cmd_node2node();

	#if DEBUG_IV_UPDATE_TEST_EN
	iv_index_test_button_firmware();
	#endif
	#if (MI_SWITCH_LPN_EN||DU_LPN_EN)
	mi_mesh_lowpower_loop();
	#endif	
	#if MESH_MONITOR_EN
    if(is_provision_success() && node_binding_tick && clock_time_exceed(node_binding_tick, 3*1000*1000)){
		monitor_mode_en = 1;
    }
	#endif

#if __TLSR_RISCV_EN__
	main_loop_risv_sdk();	// at last should be better.
#endif
}

#if IRQ_TIMER0_ENABLE   // T_NOTE Timer 0
	#if __TLSR_RISCV_EN__
void light_hw_timer0_config(void)
{
	plic_interrupt_enable(IRQ4_TIMER0);
	timer_set_init_tick(TIMER0, 0);
	timer_set_cap_tick(TIMER0, IRQ_TIME0_INTERVAL * sys_clk.pclk);
	timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
    timer_start(TIMER0);
}
	#endif
#endif

#if IRQ_TIMER1_ENABLE
	#if __TLSR_RISCV_EN__
void light_hw_timer1_config(void)
{
	plic_interrupt_enable(IRQ3_TIMER1);
	timer_set_init_tick(TIMER1, 0);
	timer_set_cap_tick(TIMER1, IRQ_TIME1_INTERVAL * sys_clk.pclk);
	timer_set_mode(TIMER1, TIMER_MODE_SYSCLK);
    timer_start(TIMER1);
}
	#else
void light_hw_timer1_config(void){
    reg_irq_mask |= FLD_IRQ_TMR1_EN;
    reg_tmr1_tick = 0;
    reg_tmr1_capt = CLOCK_MCU_RUN_CODE_1US * IRQ_TIME1_INTERVAL ;
    reg_tmr_ctrl |= FLD_TMR1_EN;
}
	#endif
#endif
#if 0 // ecc verify 
void test_ecdsa_sig_verify2()
{
	// test for the part of the 
	micro_ecc_init(NULL);
	u8 ecdsa_sk[32];
	u8 ecdsa_pk[64];
	micro_ecc_keypair_gen(NULL, ecdsa_sk, ecdsa_pk);
	static u32 A_debug_sk_calc = 0;
	unsigned char hash_dat[32]={0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7, 
								0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7};
	unsigned char sign_dat[64];
	micro_ecc_sign(NULL, ecdsa_sk, hash_dat, sign_dat);
	if(micro_ecc_verify(NULL, ecdsa_pk, hash_dat, sign_dat)==0){
		A_debug_sk_calc =2;
	}else{
		A_debug_sk_calc =0x55;
	}

}
#endif


_attribute_no_inline_ void user_init() // must add no inline, or it will be inline to main_() which is ramcode.
{
	#if (PCBA_B91_SEL == PCBA_C1T216A20_V1_2) // without SWS for this PCBA.
	bootloader_unlock_flash(); // use DP instead of SWS to burn firmware.
	#endif
    #if (BATT_CHECK_ENABLE)
    app_battery_power_check_and_sleep_handle(0); //battery check must do before OTA relative operation
    #endif
    
	#if DEBUG_EVB_EN
	    set_sha256_init_para_mode(1);	// must 1
	#else
		user_sha256_data_proc();
	#endif
	mesh_global_var_init();
    #if (DUAL_MODE_WITH_TLK_MESH_EN)
	dual_mode_en_init();    // must before proc_telink_mesh_to_sig_mesh_, because "dual_mode_state" is used in it.
    #endif
	#if (0 == __TLSR_RISCV_EN__) // TODO
	proc_telink_mesh_to_sig_mesh();		// must at first
	set_blc_hci_flag_fun(0);// disable the hci part of for the lib .
	#endif
	#if (DUAL_MODE_ADAPT_EN)
	dual_mode_en_init();    // must before factory_reset_handle, because "dual_mode_state" is used in it.
	#endif

	#if (0 == __TLSR_RISCV_EN__) // B91 called in main_()
	blc_app_loadCustomizedParameters();  //load customized freq_offset cap value and tp value
	#endif

	#if (0 == __TLSR_RISCV_EN__) // TODO
	usb_id_init();
	usb_log_init();
	#endif
	#if TESTCASE_FLAG_ENABLE
		// need to have a simulate insert
	usb_dp_pullup_en (0);  //open USB enum
	gpio_set_func(GPIO_DP,AS_GPIO);
	gpio_set_output_en(GPIO_DP,1);
	gpio_write(GPIO_DP,0);
	sleep_us(20000);
	gpio_set_func(GPIO_DP,AS_USB);
	usb_dp_pullup_en (1);  //open USB enum
	#endif
	#if (__TLSR_RISCV_EN__ && DUMP_STR_EN)
	myudb_usb_init (0x120,&usb_log_txfifo);		//0x120: usb sub-id
	usb_set_pin_en ();
	#endif
	usb_dp_pullup_en (1);  //open USB enum
	#if 0
	// use to create the certify part .
	static u32 A_debug_simu =0x55;
	irq_disable();
	A_debug_simu = mi_mesh_otp_program_simulation();
	while(1);
	#else

	#endif
	////////////////// BLE stack initialization ////////////////////////////////////
#if (DUAL_VENDOR_EN)
	mesh_common_retrieve(FLASH_ADR_PROVISION_CFG_S);
	if(DUAL_VENDOR_ST_ALI != provision_mag.dual_vendor_st)
#endif
	{ble_mac_init();
	}

	//link layer initialization
	//bls_ll_init (tbl_mac);
#if(MCU_CORE_TYPE == MCU_CORE_8269)
	blc_ll_initBasicMCU(tbl_mac);   //mandatory
#elif((MCU_CORE_TYPE == MCU_CORE_8258) || (MCU_CORE_TYPE == MCU_CORE_8278) || (MCU_CORE_TYPE == MCU_CORE_9518))
	blc_ll_initBasicMCU();                      //mandatory
	blc_ll_initStandby_module(tbl_mac);				//mandatory
#endif
#if (EXTENDED_ADV_ENABLE)
    mesh_blc_ll_initExtendedAdv();
#endif
#if __TLSR_RISCV_EN__
	blc_ll_initAdvertising_module(); 	//adv module: 		 mandatory for BLE slave,
#else
	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
#endif
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,


#if (__TLSR_RISCV_EN__) 
	blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CONN_MAX_TX_OCTETS);

	blc_ll_initAclConnTxFifo(app_acl_txfifo, ACL_TX_FIFO_SIZE, ACL_TX_FIFO_NUM);
	blc_ll_initAclConnRxFifo(app_acl_rxfifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);

	u8 check_status = blc_controller_check_appBufferInitialization();
	if(check_status != BLE_SUCCESS){
		/* here user should set some log to know which application buffer incorrect */
		write_log32(0x88880000 | check_status);
		while(1);
	}
#endif
	//////////// Controller Initialization  End /////////////////////////

#if (BLE_REMOTE_PM_ENABLE)
	blc_ll_initPowerManagement_module();        //pm module:      	 optional
	#if (MI_SWITCH_LPN_EN||DU_LPN_EN)
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
	#else
	bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
	#endif
	blc_pm_setDeepsleepRetentionThreshold(50, 30);
	blc_pm_setDeepsleepRetentionEarlyWakeupTiming(400);
	bls_pm_registerFuncBeforeSuspend(app_func_before_suspend);
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);//(SUSPEND_ADV | SUSPEND_CONN)
#endif
	
	//l2cap initialization
	//blc_l2cap_register_handler (blc_l2cap_packet_receive);
	blc_l2cap_register_handler (app_l2cap_packet_receive); // define the l2cap part 
	blc_smp_setSecurityLevel(No_Security);
	///////////////////// USER application initialization ///////////////////

#if EXTENDED_ADV_ENABLE
	/*u8 status = */mesh_blc_ll_setExtAdvParamAndEnable();
#endif
	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_MIN, ADV_INTERVAL_MAX, \
			 	 	 	 	 	     ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
			 	 	 	 	 	     0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
#if ACTIVE_SCAN_ENABLE
	memcpy(pkt_scan_req.scanA,tbl_mac,6);
#endif
#if (DUAL_VENDOR_EN)
    status = status;    // it will be optimized
#else
	if(status != BLE_SUCCESS){  //adv setting err
		#if (0 == __TLSR_RISCV_EN__)
		write_reg8(0x8000, 0x11);  //debug
		#endif
		while(1);
	}
#endif
	
	// normally use this settings 
	blc_ll_setAdvCustomedChannel (37, 38, 39);
	bls_ll_setAdvEnable(1);  //adv enable

	rf_set_power_level_index (my_rf_power_index);	
	
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT|
								HCI_LE_EVT_MASK_CONNECTION_COMPLETE|
								HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE);

	////////////////// SPP initialization ///////////////////////////////////
#if (HCI_ACCESS != HCI_USE_NONE)
	#if (HCI_ACCESS==HCI_USE_USB)
	//blt_set_bluetooth_version (BLUETOOTH_VER_4_2);
	//bls_ll_setAdvChannelMap (BLT_ENABLE_ADV_ALL);
	usb_bulk_drv_init (0);
	blc_register_hci_handler (app_hci_cmd_from_usb, blc_hci_tx_to_usb);
	#elif (HCI_ACCESS == HCI_USE_UART)	//uart
	uart_drv_init();
	blc_register_hci_handler (blc_rx_from_uart, blc_hci_tx_to_uart);		//default handler
	//blc_register_hci_handler(rx_from_uart_cb,tx_to_uart_cb);				//customized uart handler
	#endif
#endif



	#if ADC_ENABLE
	adc_drv_init();	// still init even though BATT_CHECK_ENABLE is enable, beause battery check may not be called in user init.
	#endif
	rf_pa_init();
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, (blt_event_callback_t)&mesh_ble_connect_cb);
	blc_hci_registerControllerEventHandler(app_event_handler);		//register event callback
	//bls_hci_mod_setEventMask_cmd(0xffff);			//enable all 15 events,event list see ble_ll.h
	bls_set_advertise_prepare (app_advertise_prepare_handler);
	//bls_set_update_chn_cb(chn_conn_update_dispatch);
	

#if __TLSR_RISCV_EN__
	#if (BLE_OTA_ENABLE)
	////////////////// OTA relative ////////////////////////
	/* OTA module initialization must be called after "blc_ota_setNewFirmwwareStorageAddress"(if used), and before any other OTA API.*/
	blc_ota_initOtaServer_module();

	blc_ota_setOtaProcessTimeout(OTA_CMD_INTER_TIMEOUT_S);	//OTA process timeout:	30 seconds
	blc_ota_setOtaDataPacketTimeout(4); //OTA data packet timeout:	4 seconds
	blc_ota_registerOtaStartCmdCb(entry_ota_mode);
	blc_ota_registerOtaResultIndicationCb(show_ota_result);
	#endif
#else
	bls_ota_registerStartCmdCb(entry_ota_mode);
	bls_ota_registerResultIndicateCb(show_ota_result);
#endif
	#if !GATT_LPN_EN
	app_enable_scan_all_device ();	// enable scan adv packet 
	#endif
	// mesh_mode and layer init
	mesh_init_all();

	// OTA init
	#if (DUAL_MODE_ADAPT_EN && (0 == FW_START_BY_BOOTLOADER_EN) || DUAL_MODE_WITH_TLK_MESH_EN)
	if(DUAL_MODE_NOT_SUPPORT == dual_mode_state)
	#endif	
	{
		#if MI_API_ENABLE
			//fix the mi ota ,when uncomplete ,it need to goon after power on 
			u32 adr_record;
			if(!find_record_adr(RECORD_DFU_INFO,&adr_record)){
				bls_ota_clearNewFwDataArea(0);
				telink_record_clean_cpy();// trigger clean recycle ,and it will not need to clean in the conn state
			}
		#else
		bls_ota_clearNewFwDataArea(0);	 //must
		#endif
	}
	#if (MCU_CORE_TYPE == MCU_CORE_9518)
	// use original scan flow.
	blc_ll_initScanning_module_mesh();//blc_ll_initScanning_module(tbl_mac);
	#endif
	//gatt initialization
	#if((MCU_CORE_TYPE == MCU_CORE_8258) || (MCU_CORE_TYPE == MCU_CORE_8278) || (MCU_CORE_TYPE == MCU_CORE_9518))
	blc_gap_peripheral_init();    //gap initialization
	#endif	
	
	mesh_scan_rsp_init();
	my_att_init (provision_mag.gatt_mode);
	blc_att_setServerDataPendingTime_upon_ClientCmd(10);
#if MI_API_ENABLE
	mem_pool_init();

    #if DUAL_VENDOR_EN
    if(DUAL_VENDOR_ST_ALI != provision_mag.dual_vendor_st) // mac have been set to ali mode before.
    #endif
    {
	//// used for the telink callback part 
	//extern int mi_mesh_otp_program_simulation();
	//mi_mesh_otp_program_simulation();
	blc_att_setServerDataPendingTime_upon_ClientCmd(1);
	telink_record_part_init();
	#if (MI_SWITCH_LPN_EN||DU_LPN_EN) // use 16M will save power
	mi_mesh_switch_sys_mode(16000000);
	mi_mesh_sleep_init();
	#endif
	
	#if !DU_ENABLE
	blc_l2cap_register_pre_handler(telink_ble_mi_event_cb_att);// for telink event callback
	#endif
	mi_service_init();
	telink_mi_vendor_init();
	}
#endif 
	system_time_init();
#if TESTCASE_FLAG_ENABLE
	memset(&model_sig_cfg_s.hb_sub, 0x00, sizeof(mesh_heartbeat_sub_str)); // init para for test
#endif
#if IRQ_TIMER1_ENABLE
    light_hw_timer1_config();
#endif

#if IRQ_TIMER0_ENABLE  // T_NOTE: Config timer0
    light_hw_timer0_config();
#endif
#if IRQ_GPIO_ENABLE
	gpio_set_interrupt_init(IRQ_GPIO_SELECT, PM_PIN_PULLUP_10K, INTR_FALLING_EDGE, IRQ25_GPIO); // An interrupt is triggered when PD2 is low
#endif
#if (BLT_SOFTWARE_TIMER_ENABLE)
	blt_soft_timer_init();
	//blt_soft_timer_add(&soft_timer_test0, 200*1000);
#endif

#if DU_ENABLE
	du_ui_proc_init();
#endif

#if DEBUG_CFG_CMD_GROUP_AK_EN
	if(blt_rxfifo.num >64){
		blt_rxfifo.num = 64;
	}
#endif
#if MESH_MONITOR_EN
	monitor_mode_en = is_provision_success();
#endif

#if __TLSR_RISCV_EN__
	user_init_risv_sdk();	// at last should be better.
#endif

#if MESH_FLASH_PROTECTION_EN
	mesh_flash_lock(); // must after bls_ota_clearNewFwDataArea();
#endif

    CB_USER_INIT();
}

#if (PM_DEEPSLEEP_RETENTION_ENABLE)
_attribute_ram_code_ void user_init_deepRetn(void)
{
	#if (PCBA_B91_SEL == PCBA_C1T216A20_V1_2) // without SWS for this PCBA.
	bootloader_unlock_flash(); // use DP instead of SWS to burn firmware.
	#endif
	#if (0 == __TLSR_RISCV_EN__) // B91 called in main_()
    blc_app_loadCustomizedParameters();
    #endif
	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (my_rf_power_index);
	blc_ll_recoverDeepRetention();

	DBG_CHN0_HIGH;    //debug
    // should enable IRQ here, because it may use irq here, for example BLE connect which need start IRQ quickly.
    irq_enable();
	#if (MI_SWITCH_LPN_EN||DU_LPN_EN)
	LOG_MSG_INFO(TL_LOG_NODE_SDK,0,0,"deep init %x",clock_time()/16000);
		if(bltPm.appWakeup_flg){ // it may have the rate not response by the event tick part ,so we should use the distance
			if(mi_mesh_sleep_time_exceed_adv_iner()){
				mi_mesh_sleep_init();
				bls_pm_setSuspendMask (SUSPEND_DISABLE);
			}
		}else{// if it is wakeup by the adv event tick part ,we need to refresh the adv tick part
			#if DU_LPN_EN //because the mesh disable the random part ,so it should set every time 
			u32 adv_inter =ADV_INTERVAL_MIN +(rand())%(ADV_INTERVAL_MAX-ADV_INTERVAL_MIN); 	
			bls_ll_setAdvInterval(adv_inter,adv_inter);
			#endif
			mi_mesh_sleep_init();
			bls_pm_setSuspendMask (SUSPEND_DISABLE);
		}
	#endif
	#if DU_ENABLE
	du_ui_proc_init_deep();
	#endif
//    light_pwm_init();   // cost about 1.5ms

#if (HCI_ACCESS == HCI_USE_UART)	//uart
	uart_drv_init();
#endif
#if ADC_ENABLE
	adc_drv_init();
#endif
}
#endif

