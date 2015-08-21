#ifndef __P1395_CAN_MASTER_H
#define __P1395_CAN_MASTER_H

#include "P1395_CAN_CORE.h"
#include "P1395_CAN_CORE_PRIVATE.h"


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



typedef struct {
  // ------------------- ETHERNET CONTROL BOARD --------------------- //
  // Standard Registers for all Boards
  ETMCanStatusRegister   status_data; 
  ETMCanSystemDebugData  debug_data;
  ETMCanCanStatus        can_status;
  ETMCanAgileConfig     configuration;

  unsigned int           status_received_register;  // When a status message is recieved, the corresponding bit is set in this register
  unsigned int           mirror_control_state;
  unsigned long          mirror_system_powered_seconds;
  unsigned long          mirror_system_hv_on_seconds;
  unsigned long          mirror_system_xray_on_seconds;
  unsigned long          mirror_time_seconds_now;
  unsigned int           mirror_average_output_power_watts;
  unsigned int           mirror_thyratron_warmup_counter_seconds;
  unsigned int           mirror_magnetron_heater_warmup_counter_seconds;
  unsigned int           mirror_gun_driver_heater_warmup_counter_seconds;

  unsigned int           mirror_sync_0_control_word; //pulse_inhibit_status_bits;
  unsigned int           unused_2; //software_pulse_enable;
  unsigned int           unused_3; //pulse_sync_disable_requested; // This is used by the CAN interrupt to signal that we need to send a pulse_sync_disable message
  unsigned int           unused_4; //status_connected_boards;   // This register indicates which boards are connected.
  unsigned int           unused_array[14];
} ETMCanRamMirrorEthernetBoard;

extern ETMCanRamMirrorEthernetBoard     etm_can_ethernet_board_data;


typedef struct {
  // ------------------- HV LAMBDA BOARD --------------------- //
  // Standard Registers for all Boards
  ETMCanStatusRegister  status_data; 
  ETMCanSystemDebugData debug_data;
  ETMCanCanStatus       can_status;
  ETMCanAgileConfig     configuration;
  
  // Values that the Ethernet control board sets on HV Lambda
  unsigned int ecb_high_set_point;
  unsigned int ecb_low_set_point;

  // "SLOW" Data that the Ethernet control board reads back from HV Lambda
  unsigned int eoc_not_reached_count;
  unsigned int readback_vmon;
  unsigned int readback_imon;
  unsigned int readback_base_plate_temp;

  // DPARKER still need to add this data to Ethernet Interface
  unsigned int readback_high_vprog;
  unsigned int readback_low_vprog;
  unsigned int readback_peak_lambda_voltage;

  //unsigned int high_vprog;
  //unsigned int low_vprog;
  unsigned int           unused_array[23];

} ETMCanRamMirrorHVLambda;

extern ETMCanRamMirrorHVLambda          etm_can_hv_lambda_mirror;

#define _HV_LAMBDA_NOT_CONFIGURED          etm_can_hv_lambda_mirror.status_data.status_bits.control_2_not_configured
#define _HV_LAMBDA_NOT_CONNECTED           etm_can_hv_lambda_mirror.status_data.status_bits.control_7_ecb_can_not_active
#define _HV_LAMBDA_NOT_READY               etm_can_hv_lambda_mirror.status_data.status_bits.control_0_not_ready


typedef struct {
  // ------------------- ION PUMP BOARD --------------------- //
  // Standard Registers for all Boards
  ETMCanStatusRegister  status_data; 
  ETMCanSystemDebugData debug_data;
  ETMCanCanStatus       can_status;
  ETMCanAgileConfig     configuration;
  
  // Values that the Ethernet control board sets 
  // NONE!!!!
  
  // "SLOW" Data that the Ethernet control board reads back
  unsigned int ionpump_readback_ion_pump_volage_monitor;
  unsigned int ionpump_readback_ion_pump_current_monitor;
  unsigned int ionpump_readback_filtered_high_energy_target_current;
  unsigned int ionpump_readback_filtered_low_energy_target_current;

  unsigned int           unused_array[28];

} ETMCanRamMirrorIonPump;

extern ETMCanRamMirrorIonPump           etm_can_ion_pump_mirror;

#define _ION_PUMP_NOT_READY                   etm_can_ion_pump_mirror.status_data.status_bits.control_0_not_ready
#define _ION_PUMP_NOT_CONFIGURED              etm_can_ion_pump_mirror.status_data.status_bits.control_2_not_configured
#define _ION_PUMP_NOT_CONNECTED               etm_can_ion_pump_mirror.status_data.status_bits.control_7_ecb_can_not_active

typedef struct {
  // -------------------- AFC CONTROL BOARD ---------------//
  // Standard Registers for all Boards
  ETMCanStatusRegister  status_data; 
  ETMCanSystemDebugData debug_data;
  ETMCanCanStatus       can_status;
  ETMCanAgileConfig     configuration;
  
  // Values that the Ethernet control board sets 
  unsigned int afc_home_position;
  int          afc_offset;
    
  // "SLOW" Data that the Ethernet control board reads back
  unsigned int afc_readback_home_position;
  unsigned int afc_readback_offset;
  unsigned int afc_readback_current_position;

  // DPARKER - Need to add this to ethernet interface
  unsigned int afc_readback_afc_a_input_reading;
  unsigned int afc_readback_afc_b_input_reading;
  unsigned int afc_readback_filtered_error_reading;
  unsigned int afc_readback_target_position;
  
  unsigned int aft_control_voltage;
  unsigned int readback_aft_control_voltage;
  unsigned int           unused_array[21];  

} ETMCanRamMirrorAFC;

extern ETMCanRamMirrorAFC               etm_can_afc_mirror;

#define _AFC_NOT_READY                   etm_can_afc_mirror.status_data.status_bits.control_0_not_ready
#define _AFC_NOT_CONFIGURED              etm_can_afc_mirror.status_data.status_bits.control_2_not_configured
#define _AFC_NOT_CONNECTED               etm_can_afc_mirror.status_data.status_bits.control_7_ecb_can_not_active

typedef struct {
  // -------------------- COOLING INTERFACE BOARD ---------------//
  // Standard Registers for all Boards
  ETMCanStatusRegister  status_data; 
  ETMCanSystemDebugData debug_data;
  ETMCanCanStatus       can_status;
  ETMCanAgileConfig     configuration;
  
  // Values that the Ethernet control board sets 
  // NONE!!!!
    
  // "SLOW" Data that the Ethernet control board reads back
  unsigned int cool_readback_hvps_coolant_flow;
  unsigned int cool_readback_magnetron_coolant_flow;
  unsigned int cool_readback_linac_coolant_flow;
  unsigned int cool_readback_circulator_coolant_flow;
  unsigned int cool_readback_hx_coolant_flow;
  unsigned int cool_readback_spare_coolant_flow;
  unsigned int cool_readback_coolant_temperature;
  unsigned int cool_readback_sf6_pressure;
  unsigned int cool_readback_cabinet_temperature;
  unsigned int cool_readback_linac_temperature;
  unsigned int cool_readback_spare_word_0;
  unsigned int cool_readback_spare_word_1;
  unsigned int           unused_array[20];  

} ETMCanRamMirrorCooling;

extern ETMCanRamMirrorCooling           etm_can_cooling_mirror;

#define _COOLING_NOT_READY                   etm_can_cooling_mirror.status_data.status_bits.control_0_not_ready
#define _COOLING_NOT_CONFIGURED              etm_can_cooling_mirror.status_data.status_bits.control_2_not_configured
#define _COOLING_NOT_CONNECTED               etm_can_cooling_mirror.status_data.status_bits.control_7_ecb_can_not_active



typedef struct {
  // -------------------- HEATER/MAGNET INTERFACE BOARD ---------------//
  // Standard Registers for all Boards
  ETMCanStatusRegister  status_data; 
  ETMCanSystemDebugData debug_data;
  ETMCanCanStatus       can_status;
  ETMCanAgileConfig     configuration;
  
  // Values that the Ethernet control board sets 
  unsigned int htrmag_magnet_current_set_point;
  unsigned int htrmag_heater_current_set_point;
      
  // "SLOW" Data that the Ethernet control board reads back
  unsigned int htrmag_readback_heater_current;
  unsigned int htrmag_readback_heater_voltage;
  unsigned int htrmag_readback_magnet_current;
  unsigned int htrmag_readback_magnet_voltage;
  unsigned int htrmag_readback_heater_current_set_point;
  unsigned int htrmag_readback_heater_voltage_set_point;
  unsigned int htrmag_readback_magnet_current_set_point;
  unsigned int htrmag_readback_magnet_voltage_set_point;

  // Additional Control/Interface data
  unsigned int htrmag_heater_current_set_point_scaled;//htrmag_heater_enable;
  unsigned int unused_0;  //htrmag_magnet_enable;
  unsigned int           unused_array[20];  

} ETMCanRamMirrorHeaterMagnet;

extern ETMCanRamMirrorHeaterMagnet      etm_can_heater_magnet_mirror;


#define _HEATER_MAGNET_OFF                  etm_can_heater_magnet_mirror.status_data.status_bits.control_0_not_ready
#define _HEATER_MAGNET_NOT_CONFIGURED       etm_can_heater_magnet_mirror.status_data.status_bits.control_2_not_configured
#define _HEATER_MAGNET_NOT_CONNECTED        etm_can_heater_magnet_mirror.status_data.status_bits.control_7_ecb_can_not_active


typedef struct {
  // -------------------- GUN DRIVER INTERFACE BOARD ---------------//
  // Standard Registers for all Boards
  ETMCanStatusRegister  status_data; 
  ETMCanSystemDebugData debug_data;
  ETMCanCanStatus       can_status;
  ETMCanAgileConfig     configuration;
  
  // Values that the Ethernet control board sets 
  unsigned int gun_high_energy_pulse_top_voltage_set_point;
  unsigned int gun_low_energy_pulse_top_voltage_set_point;
  unsigned int gun_heater_voltage_set_point;
  unsigned int gun_cathode_voltage_set_point;
  
  // "SLOW" Data that the Ethernet control board reads back
  unsigned int gun_readback_high_energy_pulse_top_voltage_monitor;
  unsigned int gun_readback_low_energy_pulse_top_voltage_monitor;
  unsigned int gun_readback_cathode_voltage_monitor;
  unsigned int gun_readback_peak_beam_current;
  unsigned int gun_readback_heater_voltage_monitor;
  unsigned int gun_readback_heater_current_monitor;
  unsigned int gun_readback_heater_time_delay_remaining;
  unsigned int gun_readback_driver_temperature;
  unsigned int gun_readback_high_energy_pulse_top_set_point;
  unsigned int gun_readback_low_energy_pulse_top_set_point;
  unsigned int gun_readback_heater_voltage_set_point;
  unsigned int gun_readback_cathode_voltage_set_point;

  // DPARKER NEED TO ADD TO INTERFACE
  unsigned int gun_readback_fpga_asdr_register;
  unsigned int gun_readback_analog_fault_status;
  unsigned int gun_readback_system_logic_state;
  unsigned int gun_readback_bias_voltage_mon;
  unsigned int           unused_array[12];  

} ETMCanRamMirrorGunDriver;

extern ETMCanRamMirrorGunDriver         etm_can_gun_driver_mirror;


#define _GUN_HEATER_ON                  etm_can_gun_driver_mirror.status_data.status_bits.status_7 // DPARKER Need to put the real status number
#define _GUN_DRIVER_NOT_READY           etm_can_gun_driver_mirror.status_data.status_bits.control_0_not_ready
#define _GUN_DRIVER_NOT_CONFIGURED      etm_can_gun_driver_mirror.status_data.status_bits.control_2_not_configured
#define _GUN_DRIVER_NOT_CONNECTED       etm_can_gun_driver_mirror.status_data.status_bits.control_7_ecb_can_not_active




typedef struct {
  // -------------------- MAGNETRON CURRENT MONITOR BOARD ---------------//
  // Standard Registers for all Boards
  ETMCanStatusRegister  status_data; 
  ETMCanSystemDebugData debug_data;
  ETMCanCanStatus       can_status;
  ETMCanAgileConfig     configuration;
  
  // Values that the Ethernet control board sets 
  // NONE!!!!
  
  // "SLOW" Data that the Ethernet control board reads back
  unsigned int magmon_readback_spare;
  unsigned int magmon_readback_arcs_this_hv_on;
  unsigned int magmon_filtered_high_energy_pulse_current;
  unsigned int magmon_filtered_low_energy_pulse_current;
  unsigned long magmon_arcs_lifetime;
  unsigned long magmon_pulses_this_hv_on;
  unsigned long long magmon_pulses_lifetime;
  unsigned int           unused_array[20];  

} ETMCanRamMirrorMagnetronCurrent;

extern ETMCanRamMirrorMagnetronCurrent  etm_can_magnetron_current_mirror;

#define _PULSE_CURRENT_NOT_READY               etm_can_magnetron_current_mirror.status_data.status_bits.control_0_not_ready
#define _PULSE_CURRENT_NOT_CONFIGURED          etm_can_magnetron_current_mirror.status_data.status_bits.control_2_not_configured
#define _PULSE_CURRENT_NOT_CONNECTED           etm_can_magnetron_current_mirror.status_data.status_bits.control_7_ecb_can_not_active


typedef struct {
  // -------------------- PULSE SYNC BOARD ---------------//
  // Standard Registers for all Boards
  ETMCanStatusRegister  status_data; 
  ETMCanSystemDebugData debug_data;
  ETMCanCanStatus       can_status;
  ETMCanAgileConfig     configuration;
  
  // Values that the Ethernet control board sets 
  unsigned char psync_grid_delay_high_intensity_3;
  unsigned char psync_grid_delay_high_intensity_2;
  unsigned char psync_grid_delay_high_intensity_1;
  unsigned char psync_grid_delay_high_intensity_0;
  unsigned char psync_pfn_delay_high;
  unsigned char psync_dose_sample_delay_high;

  unsigned char psync_grid_width_high_intensity_3;
  unsigned char psync_grid_width_high_intensity_2;
  unsigned char psync_grid_width_high_intensity_1;
  unsigned char psync_grid_width_high_intensity_0;
  unsigned char psync_afc_delay_high;
  unsigned char psync_magnetron_current_sample_delay_high;

  unsigned char psync_grid_delay_low_intensity_3;
  unsigned char psync_grid_delay_low_intensity_2;
  unsigned char psync_grid_delay_low_intensity_1;
  unsigned char psync_grid_delay_low_intensity_0;
  unsigned char psync_pfn_delay_low;
  unsigned char psync_dose_sample_delay_low;
 
  unsigned char psync_grid_width_low_intensity_3;
  unsigned char psync_grid_width_low_intensity_2;
  unsigned char psync_grid_width_low_intensity_1;
  unsigned char psync_grid_width_low_intensity_0;
  unsigned char psync_afc_delay_low;
  unsigned char psync_magnetron_current_sample_delay_low;

  unsigned int  psync_customer_led;

  // "SLOW" Data that the Ethernet control board reads back
  // NONE!!!!!!
  unsigned int           unused_array[19];  

} ETMCanRamMirrorPulseSync;

extern ETMCanRamMirrorPulseSync         etm_can_pulse_sync_mirror;
#define _PULSE_SYNC_NOT_READY              etm_can_pulse_sync_mirror.status_data.status_bits.control_0_not_ready
#define _PULSE_SYNC_NOT_CONFIGURED         etm_can_pulse_sync_mirror.status_data.status_bits.control_2_not_configured
#define _PULSE_SYNC_NOT_CONNECTED          etm_can_pulse_sync_mirror.status_data.status_bits.control_7_ecb_can_not_active
#define _PULSE_SYNC_CUSTOMER_HV_OFF        etm_can_pulse_sync_mirror.status_data.status_bits.status_0
#define _PULSE_SYNC_CUSTOMER_XRAY_OFF      etm_can_pulse_sync_mirror.status_data.status_bits.status_1

// PUBLIC Variables
#define HIGH_SPEED_DATA_BUFFER_SIZE   16
extern ETMCanHighSpeedData high_speed_data_buffer_a[HIGH_SPEED_DATA_BUFFER_SIZE];
extern ETMCanHighSpeedData high_speed_data_buffer_b[HIGH_SPEED_DATA_BUFFER_SIZE];


extern ETMCanHighSpeedData              etm_can_high_speed_data_test;


void ETMCanMasterDoCan(void);
void ETMCanMasterInitialize(void);



// Can Module Buffers
extern ETMCanMessageBuffer etm_can_rx_data_log_buffer;

// Can Module Funtions

//void ETMCanMasterPulseSyncDisable(void);
//unsigned int ETMCanMasterReadyToPulse(void);



void SendCalibrationSetPointToSlave(unsigned int index, unsigned int data_1, unsigned int data_0);

void ReadCalibrationSetPointFromSlave(unsigned int index);

void SendSlaveLoadDefaultEEpromData(unsigned int board_id);

void SendSlaveReset(unsigned int board_id);



#endif
