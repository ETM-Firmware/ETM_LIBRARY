#ifndef __P1395_CAN_MASTER_H
#define __P1395_CAN_MASTER_H

#include "P1395_CAN_CORE.h"


typedef struct {
  unsigned high_energy_pulse:1;
  unsigned arc_this_pulse:1;
  unsigned tbd_2:1;
  unsigned tbd_3:1;

  unsigned tbd_4:1;
  unsigned tbd_5:1;
  unsigned tbd_6:1;
  unsigned tbd_7:1;

  unsigned tbd_8:1;
  unsigned tbd_9:1;
  unsigned tbd_A:1;
  unsigned tbd_B:1;

  unsigned tbd_C:1;
  unsigned tbd_D:1;
  unsigned tbd_E:1;
  unsigned tbd_F:1;
} HighSpeedLogStatusBits;

typedef struct {
  unsigned int pulse_count;
  HighSpeedLogStatusBits status_bits; //This will contain high_low_energy?, arc_this_pulse?, what else???
  
  unsigned int x_ray_on_seconds_lsw;  // This is the lsw of x_ray_on_seconds, when the ECB recieved the "next pulse level" command
  unsigned int x_ray_on_milliseconds; // This is a representation of the milliseconds, when the ECB recieved the "next pulse level" command

  unsigned int hvlambda_readback_high_energy_lambda_program_voltage;
  unsigned int hvlambda_readback_low_energy_lambda_program_voltage;
  unsigned int hvlambda_readback_peak_lambda_voltage;

  unsigned int afc_readback_current_position;
  unsigned int afc_readback_target_position;
  unsigned int afc_readback_a_input;
  unsigned int afc_readback_b_input;
  unsigned int afc_readback_filtered_error_reading;

  unsigned int ionpump_readback_high_energy_target_current_reading;
  unsigned int ionpump_readback_low_energy_target_current_reading;

  unsigned int magmon_readback_magnetron_high_energy_current;
  unsigned int magmon_readback_magnetron_low_energy_current;

  unsigned int psync_readback_trigger_width_and_filtered_trigger_width;
  unsigned int psync_readback_high_energy_grid_width_and_delay;
  unsigned int psync_readback_low_energy_grid_width_and_delay;
} ETMCanHighSpeedData;
// 19 words


extern ETMCanBoardData local_data_ecb;
extern ETMCanBoardData mirror_hv_lambda;
extern ETMCanBoardData mirror_ion_pump;
extern ETMCanBoardData mirror_afc;
extern ETMCanBoardData mirror_cooling;
extern ETMCanBoardData mirror_htr_mag;
extern ETMCanBoardData mirror_gun_drv;
extern ETMCanBoardData mirror_magnetron_mon;
extern ETMCanBoardData mirror_pulse_sync;




#define _CONTROL_NOT_READY            local_data_ecb.status.control_notice_bits.control_not_ready
#define _CONTROL_NOT_CONFIGURED       local_data_ecb.status.control_notice_bits.control_not_configured
#define _CONTROL_SELF_CHECK_ERROR     local_data_ecb.status.control_notice_bits.control_self_check_error


#define _NOTICE_0                     local_data_ecb.status.control_notice_bits.notice_0
#define _NOTICE_1                     local_data_ecb.status.control_notice_bits.notice_1
#define _NOTICE_2                     local_data_ecb.status.control_notice_bits.notice_2
#define _NOTICE_3                     local_data_ecb.status.control_notice_bits.notice_3
#define _NOTICE_4                     local_data_ecb.status.control_notice_bits.notice_4
#define _NOTICE_5                     local_data_ecb.status.control_notice_bits.notice_5
#define _NOTICE_6                     local_data_ecb.status.control_notice_bits.notice_6
#define _NOTICE_7                     local_data_ecb.status.control_notice_bits.notice_7

#define _FAULT_0                      local_data_ecb.status.fault_bits.fault_0
#define _FAULT_1                      local_data_ecb.status.fault_bits.fault_1
#define _FAULT_2                      local_data_ecb.status.fault_bits.fault_2
#define _FAULT_3                      local_data_ecb.status.fault_bits.fault_3
#define _FAULT_4                      local_data_ecb.status.fault_bits.fault_4
#define _FAULT_5                      local_data_ecb.status.fault_bits.fault_5
#define _FAULT_6                      local_data_ecb.status.fault_bits.fault_6
#define _FAULT_7                      local_data_ecb.status.fault_bits.fault_7
#define _FAULT_8                      local_data_ecb.status.fault_bits.fault_8
#define _FAULT_9                      local_data_ecb.status.fault_bits.fault_9
#define _FAULT_A                      local_data_ecb.status.fault_bits.fault_A
#define _FAULT_B                      local_data_ecb.status.fault_bits.fault_B
#define _FAULT_C                      local_data_ecb.status.fault_bits.fault_C
#define _FAULT_D                      local_data_ecb.status.fault_bits.fault_D
#define _FAULT_E                      local_data_ecb.status.fault_bits.fault_E
#define _FAULT_F                      local_data_ecb.status.fault_bits.fault_F

#define _WARNING_0                    local_data_ecb.status.warning_bits.warning_0
#define _WARNING_1                    local_data_ecb.status.warning_bits.warning_1
#define _WARNING_2                    local_data_ecb.status.warning_bits.warning_2
#define _WARNING_3                    local_data_ecb.status.warning_bits.warning_3
#define _WARNING_4                    local_data_ecb.status.warning_bits.warning_4
#define _WARNING_5                    local_data_ecb.status.warning_bits.warning_5
#define _WARNING_6                    local_data_ecb.status.warning_bits.warning_6
#define _WARNING_7                    local_data_ecb.status.warning_bits.warning_7
#define _WARNING_8                    local_data_ecb.status.warning_bits.warning_8
#define _WARNING_9                    local_data_ecb.status.warning_bits.warning_9
#define _WARNING_A                    local_data_ecb.status.warning_bits.warning_A
#define _WARNING_B                    local_data_ecb.status.warning_bits.warning_B
#define _WARNING_C                    local_data_ecb.status.warning_bits.warning_C
#define _WARNING_D                    local_data_ecb.status.warning_bits.warning_D
#define _WARNING_E                    local_data_ecb.status.warning_bits.warning_E
#define _WARNING_F                    local_data_ecb.status.warning_bits.warning_F


#define _CONTROL_REGISTER             *(unsigned int*)&local_data_ecb.status.control_notice_bits
#define _FAULT_REGISTER               *(unsigned int*)&local_data_ecb.status.fault_bits
#define _WARNING_REGISTER             *(unsigned int*)&local_data_ecb.status.warning_bits





// Board Configuration data - 0x06
#define config_agile_number_high_word      local_data_ecb.config_data[0]
#define config_agile_number_low_word       local_data_ecb.config_data[1]
#define config_agile_dash                  local_data_ecb.config_data[2]
#define config_agile_rev_ascii             local_data_ecb.config_data[3]

// Board Configuration data - 0x07
#define config_serial_number               local_data_ecb.config_data[4]
#define config_firmware_agile_rev          local_data_ecb.config_data[5]
#define config_firmware_branch             local_data_ecb.config_data[6]
#define config_firmware_branch_rev         local_data_ecb.config_data[7]






#define mirror_ecb_control_state                        local_data_ecb.board_data[0]
#define mirror_ecb_avg_output_power_watts               local_data_ecb.board_data[1]
#define mirror_ecb_thyratron_warmup_remaining           local_data_ecb.board_data[2]
#define mirror_ecb_magnetron_warmup_remaining           local_data_ecb.board_data[3]
#define mirror_ecb_gun_driver_warmup_remaining          local_data_ecb.board_data[4]
//#define mirror_board_com_fault                          local_data_ecb.board_data[5]
#define mirror_ecb_time_now_seconds // 6,7long          
#define mirror_ecb_system_power_seconds  // 8,9 long
#define mirror_ecb_system_hv_on_seconds  // 10,11 long
#define mirror_ecb_system_xray_on_seconds // 12,13 long










// all the other mirrors
//

#define _HV_LAMBDA_NOT_CONFIGURED            mirror_hv_lambda.status.control_notice_bits.control_2_not_configured;



ETMCanBoardDebuggingData debug_data_ecb;
ETMCanBoardDebuggingData debug_data_slave_mirror;



// This "defines" data that may need to be displayed on the GUI
#define local_hv_lambda_high_en_set_point               mirror_hv_lambda.local_data[0]
#define local_hv_lambda_low_en_set_point                mirror_hv_lambda.local_data[1]

#define local_afc_home_position                         mirror_afc.local_data[0]
#define local_afc_aft_control_voltage                   mirror_afc.local_data[1]

#define local_heater_current_full_set_point             mirror_htr_mag.local_data[0]
#define local_heater_current_scaled_set_point           mirror_htr_mag.local_data[1]
#define local_magnet_current_set_point                  mirror_htr_mag.local_data[2]

#define local_gun_drv_high_en_pulse_top_v               mirror_gun_drv.local_data[0]
#define local_gun_drv_low_en_pulse_top_v                mirror_gun_drv.local_data[1]
#define local_gun_drv_heater_v_set_point                mirror_gun_drv.local_data[2]
#define local_gun_drv_cathode_set_point                 mirror_gun_drv.local_data[3]

#define local_pulse_sync_timing_reg_0_word_0            mirror_pulse_sync.local_data[0]
#define local_pulse_sync_timing_reg_0_word_1            mirror_pulse_sync.local_data[1]
#define local_pulse_sync_timing_reg_0_word_2            mirror_pulse_sync.local_data[2]
#define local_pulse_sync_timing_reg_1_word_0            mirror_pulse_sync.local_data[3]
#define local_pulse_sync_timing_reg_1_word_1            mirror_pulse_sync.local_data[4]
#define local_pulse_sync_timing_reg_1_word_2            mirror_pulse_sync.local_data[5]
#define local_pulse_sync_timing_reg_2_word_0            mirror_pulse_sync.local_data[6]
#define local_pulse_sync_timing_reg_2_word_1            mirror_pulse_sync.local_data[7]
#define local_pulse_sync_timing_reg_2_word_2            mirror_pulse_sync.local_data[8]
#define local_pulse_sync_timing_reg_3_word_0            mirror_pulse_sync.local_data[9]
#define local_pulse_sync_timing_reg_3_word_1            mirror_pulse_sync.local_data[10]
#define local_pulse_sync_timing_reg_3_word_2            mirror_pulse_sync.local_data[11]




//#define _HV_LAMBDA_NOT_CONNECTED           etm_can_hv_lambda_mirror.status_data.status_bits.control_7_ecb_can_not_active
#define _HV_LAMBDA_NOT_READY               etm_can_hv_lambda_mirror.status_data.status_bits.control_0_not_ready



#define _ION_PUMP_NOT_READY                   etm_can_ion_pump_mirror.status_data.status_bits.control_0_not_ready
#define _ION_PUMP_NOT_CONFIGURED              etm_can_ion_pump_mirror.status_data.status_bits.control_2_not_configured
//#define _ION_PUMP_NOT_CONNECTED               etm_can_ion_pump_mirror.status_data.status_bits.control_7_ecb_can_not_active

#define _AFC_NOT_READY                   etm_can_afc_mirror.status_data.status_bits.control_0_not_ready
#define _AFC_NOT_CONFIGURED              etm_can_afc_mirror.status_data.status_bits.control_2_not_configured
//#define _AFC_NOT_CONNECTED               etm_can_afc_mirror.status_data.status_bits.control_7_ecb_can_not_active

#define _COOLING_NOT_READY                   etm_can_cooling_mirror.status_data.status_bits.control_0_not_ready
#define _COOLING_NOT_CONFIGURED              etm_can_cooling_mirror.status_data.status_bits.control_2_not_configured
//#define _COOLING_NOT_CONNECTED               etm_can_cooling_mirror.status_data.status_bits.control_7_ecb_can_not_active




#define _HEATER_MAGNET_NOT_READY                  etm_can_heater_magnet_mirror.status_data.status_bits.control_0_not_ready
#define _HEATER_MAGNET_NOT_CONFIGURED       etm_can_heater_magnet_mirror.status_data.status_bits.control_2_not_configured
//#define _HEATER_MAGNET_NOT_CONNECTED        etm_can_heater_magnet_mirror.status_data.status_bits.control_7_ecb_can_not_active


#define _GUN_HEATER_OFF                 etm_can_gun_driver_mirror.status_data.status_bits.status_1
#define _GUN_DRIVER_NOT_READY           etm_can_gun_driver_mirror.status_data.status_bits.control_0_not_ready
#define _GUN_DRIVER_NOT_CONFIGURED      etm_can_gun_driver_mirror.status_data.status_bits.control_2_not_configured
//#define _GUN_DRIVER_NOT_CONNECTED       etm_can_gun_driver_mirror.status_data.status_bits.control_7_ecb_can_not_active



#define _PULSE_CURRENT_NOT_READY               etm_can_magnetron_current_mirror.status_data.status_bits.control_0_not_ready
#define _PULSE_CURRENT_NOT_CONFIGURED          etm_can_magnetron_current_mirror.status_data.status_bits.control_2_not_configured
//#define _PULSE_CURRENT_NOT_CONNECTED           etm_can_magnetron_current_mirror.status_data.status_bits.control_7_ecb_can_not_active


#define _PULSE_SYNC_NOT_READY              etm_can_pulse_sync_mirror.status_data.status_bits.control_0_not_ready
#define _PULSE_SYNC_NOT_CONFIGURED         etm_can_pulse_sync_mirror.status_data.status_bits.control_2_not_configured
//#define _PULSE_SYNC_NOT_CONNECTED          etm_can_pulse_sync_mirror.status_data.status_bits.control_7_ecb_can_not_active
#define _PULSE_SYNC_CUSTOMER_HV_OFF        etm_can_pulse_sync_mirror.status_data.status_bits.status_0
#define _PULSE_SYNC_CUSTOMER_XRAY_OFF      etm_can_pulse_sync_mirror.status_data.status_bits.status_1

// PUBLIC Variables
#define HIGH_SPEED_DATA_BUFFER_SIZE   16
extern ETMCanHighSpeedData high_speed_data_buffer_a[HIGH_SPEED_DATA_BUFFER_SIZE];
extern ETMCanHighSpeedData high_speed_data_buffer_b[HIGH_SPEED_DATA_BUFFER_SIZE];


extern ETMCanHighSpeedData              etm_can_high_speed_data_test;


void ETMCanMasterDoCan(void);

// Public Functions
void ETMCanMasterInitialize(unsigned int requested_can_port, unsigned long fcy, unsigned int etm_can_address, unsigned long can_operation_led, unsigned int can_interrupt_priority);
/*
  This is called once when the processor starts up to initialize the can bus and all of the can variables
*/

void ETMCanMasterLoadConfiguration(unsigned long agile_id, unsigned int agile_dash, unsigned int agile_rev, unsigned int firmware_agile_rev, unsigned int firmware_branch, unsigned int firmware_branch_rev, unsigned int serial_number);
/*
  This is called once when the prcoessor starts up to load the board configuration into RAM so it can be sent over CAN to the ECB
*/

// DPARKER how do you set the agile rev and the serial number?? Should this be done over can?



void SendCalibrationSetPointToSlave(unsigned int index, unsigned int data_1, unsigned int data_0);

void ReadCalibrationSetPointFromSlave(unsigned int index);

void SendSlaveLoadDefaultEEpromData(unsigned int board_id);

void SendSlaveReset(unsigned int board_id);

void SendToEventLog(unsigned int log_id);



#define LOG_ID_ENTERED_STATE_STARTUP                                          0x0010
#define LOG_ID_ENTERED_STATE_WAIT_FOR_PERSONALITY_FROM_PULSE_SYNC             0x0011
#define LOG_ID_PERSONALITY_RECEIVED                                           0x0012
#define LOG_ID_PERSONALITY_ERROR_6_4                                          0x0013
#define LOG_ID_PERSONALITY_ERROR_2_5                                          0x0014
#define LOG_ID_ENTERED_STATE_WAITING_FOR_INITIALIZATION                       0x0015
#define LOG_ID_ALL_MODULES_CONFIGURED                                         0x0016
#define LOG_ID_ENTERED_STATE_WARMUP                                           0x0017
#define LOG_ID_WARMUP_DONE                                                    0x0018
#define LOG_ID_ENTERED_STATE_STANDBY                                          0x0019
#define LOG_ID_CUSTOMER_HV_ON                                                 0x001A
#define LOG_ID_ENTERED_STATE_DRIVE_UP                                         0x001B
#define LOG_ID_DRIVEUP_COMPLETE                                               0x001C
#define LOG_ID_CUSTOMER_HV_OFF                                                0x001D
#define LOG_ID_DRIVE_UP_TIMEOUT                                               0x001E
#define LOG_ID_ENTERED_STATE_READY                                            0x001F
#define LOG_ID_CUSTOMER_XRAY_ON                                               0x0020

#define LOG_ID_ENTERED_STATE_XRAY_ON                                          0x0022
#define LOG_ID_CUSTOMER_XRAY_OFF                                              0x0023

#define LOG_ID_ENTERED_STATE_FAULT_HOLD                                       0x0025

#define LOG_ID_ENTERED_STATE_FAULT_RESET                                      0x0027
#define LOG_ID_HV_OFF_FAULTS_CLEAR                                            0x0028
#define LOG_ID_ENTERED_STATE_FAULT_SYSTEM                                     0x0029







#define LOG_ID_NOT_READY_ION_PUMP_BOARD                                       0x1110
#define LOG_ID_READY_ION_PUMP_BOARD                                           0x1111
#define LOG_ID_NOT_CONFIGURED_ION_PUMP_BOARD                                  0x1112
#define LOG_ID_CONFIGURED_ION_PUMP_BOARD                                      0x1113
#define LOG_ID_NOT_CONNECTED_ION_PUMP_BOARD                                   0x1114
#define LOG_ID_CONNECTED_ION_PUMP_BOARD                                       0x1115


#define LOG_ID_NOT_READY_PULSE_MONITOR_BOARD                                  0x1120
#define LOG_ID_READY_PULSE_MONITOR_BOARD                                      0x1121
#define LOG_ID_NOT_CONFIGURED_PULSE_MONITOR_BOARD                             0x1122
#define LOG_ID_CONFIGURED_PULSE_MONITOR_BOARD                                 0x1123
#define LOG_ID_NOT_CONNECTED_MAGNETRON_CURRENT_BOARD                          0x1124
#define LOG_ID_CONNECTED_MAGNETRON_CURRENT_BOARD                              0x1125


#define LOG_ID_NOT_READY_PULSE_SYNC_BOARD                                     0x1130
#define LOG_ID_READY_PULSE_SYNC_BOARD                                         0x1131
#define LOG_ID_NOT_CONFIGURED_PULSE_SYNC_BOARD                                0x1132
#define LOG_ID_CONFIGURED_PULSE_SYNC_BOARD                                    0x1133
#define LOG_ID_NOT_CONNECTED_PULSE_SYNC_BOARD                                 0x1134
#define LOG_ID_CONNECTED_PULSE_SYNC_BOARD                                     0x1135



#define LOG_ID_NOT_READY_HV_LAMBDA_BOARD                                      0x1140
#define LOG_ID_READY_HV_LAMBDA_BOARD                                          0x1141
#define LOG_ID_NOT_CONFIGURED_HV_LAMBDA_BOARD                                 0x1142
#define LOG_ID_CONFIGURED_HV_LAMBDA_BOARD                                     0x1143
#define LOG_ID_NOT_CONNECTED_HV_LAMBDA_BOARD                                  0x1144
#define LOG_ID_CONNECTED_HV_LAMBDA_BOARD                                      0x1145


#define LOG_ID_NOT_READY_AFC_BOARD                                            0x1150
#define LOG_ID_READY_AFC_BOARD                                                0x1151
#define LOG_ID_NOT_CONFIGURED_AFC_BOARD                                       0x1152
#define LOG_ID_CONFIGURED_AFC_BOARD                                           0x1153
#define LOG_ID_NOT_CONNECTED_AFC_BOARD                                        0x1154
#define LOG_ID_CONNECTED_AFC_BOARD                                            0x1155


#define LOG_ID_NOT_READY_COOLING_INTERFACE_BOARD                              0x1160
#define LOG_ID_READY_COOLING_INTERFACE_BOARD                                  0x1161
#define LOG_ID_NOT_CONFIGURED_COOLING_INTERFACE_BOARD                         0x1162
#define LOG_ID_CONFIGURED_COOLING_INTERFACE_BOARD                             0x1163
#define LOG_ID_NOT_CONNECTED_COOLING_BOARD                                    0x1164
#define LOG_ID_CONNECTED_COOLING_BOARD                                        0x1165


#define LOG_ID_NOT_READY_HEATER_MAGNET                                        0x1170
#define LOG_ID_READY_HEATER_MAGNET                                            0x1171
#define LOG_ID_NOT_CONFIGURED_HEATER_MAGNET                                   0x1172
#define LOG_ID_CONFIGURED_HEATER_MAGNET                                       0x1173
#define LOG_ID_NOT_CONNECTED_HEATER_MAGNET_BOARD                              0x1174
#define LOG_ID_CONNECTED_HEATER_MAGNET_BOARD                                  0x1175

#define LOG_ID_FAULT_HTR_MAG_HEATER_OVER_CURRENT_ABSOLUTE                     0x1070
#define LOG_ID_FAULT_HTR_MAG_HEATER_UNDER_CURRENT_ABSOLUTE                    0x1071
#define LOG_ID_FAULT_HTR_MAG_HEATER_OVER_CURRENT_RELATIVE                     0x1072
#define LOG_ID_FAULT_HTR_MAG_HEATER_UNDER_CURRENT_RELATIVE                    0x1073
#define LOG_ID_FAULT_HTR_MAG_HEATER_OVER_VOLTAGE_ABSOLUTE                     0x1074
#define LOG_ID_FAULT_HTR_MAG_HEATER_UNDER_VOTLAGE_RELATIVE                    0x1075
#define LOG_ID_FAULT_HTR_MAG_MAGNET_OVER_CURRENT_ABSOLUTE                     0x1076
#define LOG_ID_FAULT_HTR_MAG_MAGNET_UNDER_CURRENT_ABSOLUTE                    0x1077
#define LOG_ID_FAULT_HTR_MAG_MAGNET_OVER_CURRENT_RELATIVE                     0x1078
#define LOG_ID_FAULT_HTR_MAG_MAGNET_UNDER_CURRENT_RELATIVE                    0x1079
#define LOG_ID_FAULT_HTR_MAG_MAGNET_OVER_VOLTAGE_ABSOLUTE                     0x107A
#define LOG_ID_FAULT_HTR_MAG_MAGNET_UNDER_VOTLAGE_RELATIVE                    0x107B
#define LOG_ID_FAULT_HTR_MAG_HW_HEATER_OVER_VOLTAGE                           0x107C
#define LOG_ID_FAULT_HTR_MAG_HW_TEMPERATURE_SWITCH                            0x107D
#define LOG_ID_FAULT_HTR_MAG_COOLANT_FAULT                                    0x107E
#define LOG_ID_FAULT_HTR_CAN_FAULT_LATCHED                                    0x107F


#define LOG_ID_NOT_READY_GUN_DRIVER_BOARD                                     0x1180
#define LOG_ID_READY_GUN_DRIVER_BOARD                                         0x1181
#define LOG_ID_NOT_CONFIGURED_GUN_DRIVER_BOARD                                0x1182
#define LOG_ID_CONFIGURED_GUN_DRIVER_BOARD                                    0x1183
#define LOG_ID_NOT_CONNECTED_GUN_DRIVER_BOARD                                 0x1184
#define LOG_ID_CONNECTED_GUN_DRIVER_BOARD                                     0x1185
#define LOG_ID_GUN_DRIVER_BOARD_HEATER_OFF                                    0x1186
#define LOG_ID_GUN_DRIVER_BOARD_HEATER_ON                                     0x1187



//#define LOG_ID_FAULT_HTR_MAG_CAN_COMMUNICATION_LATCHED                        0x003A
//#define LOG_ID_FAULT_GUN_DRIVER_BOARD_HV_OFF                                  0x003B
//#define LOG_ID_FAULT_MODULE_NOT_CONFIGURED                                    0x004A







typedef struct {
  unsigned int  event_number; // this resets to zero at power up
  unsigned long event_time;   // this is the custom time format
  unsigned int  event_id;     // This tells what the event was

  // In the future we may add more data to the event;
} TYPE_EVENT;


typedef struct {
  TYPE_EVENT event_data[128];
  unsigned int write_index;
  unsigned int gui_index;
  unsigned int eeprom_index;
} TYPE_EVENT_LOG;
 

extern TYPE_EVENT_LOG event_log;

/* 
   The ethernet control board keeps a record of standard data from all the slave boards
   This includes status, low level errors, configuration, and debug information
   This is a hack to allow the data on the master to be accessed the same way as it is on the slave boards
*/



extern ETMCanSyncMessage    etm_can_master_sync_message;

#define _SYNC_CONTROL_RESET_ENABLE            etm_can_master_sync_message.sync_0_control_word.sync_0_reset_enable
#define _SYNC_CONTROL_HIGH_SPEED_LOGGING      etm_can_master_sync_message.sync_0_control_word.sync_1_high_speed_logging_enabled
#define _SYNC_CONTROL_PULSE_SYNC_DISABLE_HV   etm_can_master_sync_message.sync_0_control_word.sync_2_pulse_sync_disable_hv
#define _SYNC_CONTROL_PULSE_SYNC_DISABLE_XRAY etm_can_master_sync_message.sync_0_control_word.sync_3_pulse_sync_disable_xray
#define _SYNC_CONTROL_COOLING_FAULT           etm_can_master_sync_message.sync_0_control_word.sync_4_cooling_fault
#define _SYNC_CONTROL_CLEAR_DEBUG_DATA        etm_can_master_sync_message.sync_0_control_word.sync_F_clear_debug_data

#define _SYNC_CONTROL_PULSE_SYNC_WARMUP_LED   etm_can_master_sync_message.sync_0_control_word.sync_A_pulse_sync_warmup_led_on
#define _SYNC_CONTROL_PULSE_SYNC_STANDBY_LED  etm_can_master_sync_message.sync_0_control_word.sync_B_pulse_sync_standby_led_on
#define _SYNC_CONTROL_PULSE_SYNC_READY_LED    etm_can_master_sync_message.sync_0_control_word.sync_C_pulse_sync_ready_led_on
#define _SYNC_CONTROL_PULSE_SYNC_FAULT_LED    etm_can_master_sync_message.sync_0_control_word.sync_D_pulse_sync_fault_led_on

#define _SYNC_CONTROL_WORD                    *(unsigned int*)&etm_can_master_sync_message.sync_0_control_word



//extern unsigned int etm_can_master_next_pulse_level;  // This value will get updated when a next pulse level command is received
//extern unsigned int etm_can_master_next_pulse_count;  // This value will get updated when a next pulse level command is received



#endif
