#include <xc.h>
#include <timer.h>
#include "P1395_CAN_MASTER.h"
#include "A36507.h"

// ----------- Can Timers T4 & T5 Configuration ----------- //
#define T4_FREQUENCY_HZ          40  // This is 25mS rate
#define T5_FREQUENCY_HZ          4   // This is 250ms rate

// DPARKER remove the need for timers.h here
#define T4CON_VALUE              (T4_OFF & T4_IDLE_CON & T4_GATE_OFF & T4_PS_1_256 & T4_32BIT_MODE_OFF & T4_SOURCE_INT)

#define T5CON_VALUE              (T5_OFF & T5_IDLE_CON & T5_GATE_OFF & T5_PS_1_256 & T5_SOURCE_INT)


typedef struct {
  unsigned int  address;
  unsigned long led;
} TYPE_CAN_PARAMETERS;




// --------- Global Buffers --------------- //
TYPE_EVENT_LOG              event_log;
ETMCanHighSpeedData         high_speed_data_buffer_a[HIGH_SPEED_DATA_BUFFER_SIZE];
ETMCanHighSpeedData         high_speed_data_buffer_b[HIGH_SPEED_DATA_BUFFER_SIZE];


// --------- Local Buffers ---------------- // 
ETMCanMessageBuffer         etm_can_rx_data_log_buffer;
ETMCanMessageBuffer         etm_can_rx_message_buffer;
ETMCanMessageBuffer         etm_can_tx_message_buffer;


// ------------- Global Variables ------------ //
unsigned int etm_can_next_pulse_level;
unsigned int etm_can_next_pulse_count;


// --------------------- Local Variables -------------------------- //
unsigned int master_high_speed_update_index;
unsigned int master_low_speed_update_index;
P1395BoardBits board_status_received;
P1395BoardBits board_com_fault;
TYPE_CAN_PARAMETERS can_params;



// --------- Ram Structures that store the module status ---------- //
ETMCanRamMirrorEthernetBoard     etm_can_ethernet_board_data;
ETMCanRamMirrorHVLambda          etm_can_hv_lambda_mirror;
ETMCanRamMirrorIonPump           etm_can_ion_pump_mirror;
ETMCanRamMirrorAFC               etm_can_afc_mirror;
ETMCanRamMirrorCooling           etm_can_cooling_mirror;
ETMCanRamMirrorHeaterMagnet      etm_can_heater_magnet_mirror;
ETMCanRamMirrorGunDriver         etm_can_gun_driver_mirror;
ETMCanRamMirrorMagnetronCurrent  etm_can_magnetron_current_mirror;
ETMCanRamMirrorPulseSync         etm_can_pulse_sync_mirror;


ETMCanSyncMessage                etm_can_sync_message;                // This is the sync message that the ECB sends out, only word zero is used at this time




typedef struct {
  unsigned int reset_count;
  unsigned int can_timeout_count;
} PersistentData;

volatile PersistentData etm_can_persistent_data __attribute__ ((persistent));


void ETMCanMasterProcessMessage(void);

void ETMCanMasterCheckForTimeOut(void);








void ETMCanMasterTimedTransmit(void);
/*
  This schedules all of the Master commands (listed below)
  
*/




// These are the regularly scheduled commands that the ECB sends to sub boards
void ETMCanMasterSendSync();                         // This gets sent out 1 time every 50ms
void ETMCanMasterHVLambdaUpdateOutput(void);          // This gets sent out 1 time every 200ms
void ETMCanMasterHtrMagnetUpdateOutput(void);         // This gets sent out 1 time every 200ms 
void ETMCanMasterGunDriverUpdatePulseTop(void);       // This gets sent out 1 time every 200ms
void ETMCanMasterAFCUpdateHomeOffset(void);           // This gets sent out at 200ms / 6 = 1.2 Seconds
void ETMCanMasterGunDriverUpdateHeaterCathode(void);  // This gets sent out at 200ms / 6 = 1.2 Seconds
void ETMCanMasterPulseSyncUpdateHighRegZero(void);    // This gets sent out at 200ms / 6 = 1.2 Seconds
void ETMCanMasterPulseSyncUpdateHighRegOne(void);     // This gets sent out at 200ms / 6 = 1.2 Seconds
void ETMCanMasterPulseSyncUpdateLowRegZero(void);     // This gets sent out at 200ms / 6 = 1.2 Seconds
void ETMCanMasterPulseSyncUpdateLowRegOne(void);      // This gets sent out at 200ms / 6 = 1.2 Seconds

// DPARKER how are the LEDs set on Pulse Sync Board?? Missing that command

void ETMCanMasterSet2FromSlave(ETMCanMessage* message_ptr);
/*
  This processes Set2 Commands (From slave board).
  This is only used by the Ion Pump Board to transmit the target current reading each pulse
  This is not yet implimented
*/


void ETMCanMasterUpdateSlaveStatus(ETMCanMessage* message_ptr);
/*
  This moves the data from the status message into the RAM copy on the master
  It also keeps track of which boards have sent a status message and clears TMR5 once all boards have reported a status message
*/

void ETMCanMasterProcessLogData(void);
/*
  This moves data from a log message into the RAM copy on the master
*/


void ETMCanMasterClearDebug(void);
/*
  This sets all the debug data to zero.
  This is usefull when debugging
  It is also required on power cycle because persistent variables will be set to random value
*/


void ETMCanMasterInitialize(unsigned long fcy, unsigned int etm_can_address, unsigned long can_operation_led, unsigned int can_interrupt_priority) {
  unsigned long timer_period_value;

  if (can_interrupt_priority > 7) {
    can_interrupt_priority = 7;
  }

  can_params.address = etm_can_address;
  can_params.led = can_operation_led;

  etm_can_persistent_data.reset_count++;
  
  _SYNC_CONTROL_WORD = 0;
  etm_can_sync_message.sync_1 = 0;
  etm_can_sync_message.sync_2 = 0;
  etm_can_sync_message.sync_3 = 0;
  
  local_debug_data.reset_count = etm_can_persistent_data.reset_count;
  local_can_errors.timeout = etm_can_persistent_data.can_timeout_count;

  _CXIE = 0;
  _CXIF = 0;
  _CXIP = can_interrupt_priority;
  
  CXINTF = 0;
  
  CXINTEbits.RX0IE = 1; // Enable RXB0 interrupt
  CXINTEbits.RX1IE = 1; // Enable RXB1 interrupt
  CXINTEbits.TX0IE = 1; // Enable TXB0 interrupt
  CXINTEbits.ERRIE = 1; // Enable Error interrupt

  ETMCanBufferInitialize(&etm_can_rx_message_buffer);
  ETMCanBufferInitialize(&etm_can_tx_message_buffer);
  ETMCanBufferInitialize(&etm_can_rx_data_log_buffer);

  
  // ---------------- Set up CAN Control Registers ---------------- //
  
  // Set Baud Rate
  CXCTRL = CXCTRL_CONFIG_MODE_VALUE;
  while(CXCTRLbits.OPMODE != 4);
  
  if (fcy == 25000000) {
    CXCFG1 = CXCFG1_25MHZ_FCY_VALUE;    
  } else if (fcy == 20000000) {
    CXCFG1 = CXCFG1_20MHZ_FCY_VALUE;    
  } else if (fcy == 10000000) {
    CXCFG1 = CXCFG1_10MHZ_FCY_VALUE;    
  } else {
    // If you got here we can't configure the can module
    // Try to set to operate in 10MHZ mode.
    // Can probably won't talk
    CXCFG1 = CXCFG1_10MHZ_FCY_VALUE;    
  }
  
  CXCFG2 = CXCFG2_VALUE;
    
  // Load Mask registers for RX0 and RX1
  CXRXM0SID = ETM_CAN_MASTER_RX0_MASK;
  CXRXM1SID = ETM_CAN_MASTER_RX1_MASK;

  // Load Filter registers
  CXRXF0SID = ETM_CAN_MSG_LVL_FILTER;
  CXRXF1SID = ETM_CAN_MSG_DATA_LOG_FILTER;
  CXRXF2SID = ETM_CAN_MSG_MASTER_FILTER;
  CXRXF3SID = ETM_CAN_MSG_FILTER_OFF;
  CXRXF4SID = ETM_CAN_MSG_FILTER_OFF;
  CXRXF5SID = ETM_CAN_MSG_FILTER_OFF;

  // Set Transmitter Mode
  CXTX0CON = CXTXXCON_VALUE_LOW_PRIORITY;
  CXTX1CON = CXTXXCON_VALUE_MEDIUM_PRIORITY;
  CXTX2CON = CXTXXCON_VALUE_HIGH_PRIORITY;

  CXTX0DLC = CXTXXDLC_VALUE;
  CXTX1DLC = CXTXXDLC_VALUE;
  CXTX2DLC = CXTXXDLC_VALUE;

  // Set Receiver Mode
  CXRX0CON = CXRXXCON_VALUE;
  CXRX1CON = CXRXXCON_VALUE;
  
  // Switch to normal operation
  CXCTRL = CXCTRL_OPERATE_MODE_VALUE;
  while(CXCTRLbits.OPMODE != 0);
  
  //etm_can_ethernet_board_data.status_received_register = 0x0000;

  // Enable Can interrupt
  _CXIE = 1;

  // Configure T4
  timer_period_value = fcy;
  timer_period_value >>= 8;
  timer_period_value /= T4_FREQUENCY_HZ;
  if (timer_period_value > 0xFFFF) {
    timer_period_value = 0xFFFF;
  }
  T4CON = T4CON_VALUE;
  PR4 = timer_period_value;  
  TMR4 = 0;
  _T4IF = 0;
  _T4IE = 0;
  T4CONbits.TON = 1;

  // Configure T5
  timer_period_value = fcy;
  timer_period_value >>= 8;
  timer_period_value /= T5_FREQUENCY_HZ;
  if (timer_period_value > 0xFFFF) {
    timer_period_value = 0xFFFF;
  }
  T5CON = T5CON_VALUE;
  PR5 = timer_period_value;
  TMR5 = 0;
  _T5IF = 0;
  _T5IE = 0;
  T5CONbits.TON = 1;
}


void ETMCanMasterLoadConfiguration(unsigned long agile_id, unsigned int agile_dash, unsigned int firmware_agile_rev, unsigned int firmware_branch, unsigned int firmware_minor_rev) {

  etm_can_my_configuration.agile_number_low_word = (agile_id & 0xFFFF);
  agile_id >>= 16;
  etm_can_my_configuration.agile_number_high_word = agile_id;
  etm_can_my_configuration.agile_dash = agile_dash;
  etm_can_my_configuration.firmware_branch = firmware_branch;
  etm_can_my_configuration.firmware_major_rev = firmware_agile_rev;
  etm_can_my_configuration.firmware_minor_rev = firmware_minor_rev;

  // Load default values for agile rev and serial number at this time
  // Need some way to update these with the real numbers
  etm_can_my_configuration.agile_rev_ascii = 'A';
  etm_can_my_configuration.serial_number = 0;
}


void ETMCanMasterDoCan(void) {
  // Record the max TX counter
  if ((CXEC & 0xFF00) > (local_can_errors.CXEC_reg & 0xFF00)) {
    local_can_errors.CXEC_reg &= 0x00FF;
    local_can_errors.CXEC_reg += (CXEC & 0xFF00);
  }

  // Record the max RX counter
  if ((CXEC & 0x00FF) > (local_can_errors.CXEC_reg & 0x00FF)) {
    local_can_errors.CXEC_reg &= 0xFF00;
    local_can_errors.CXEC_reg += (CXEC & 0x00FF);
  }
  
  // Record the RCON state
  local_debug_data.reserved_1 = 0xC345;
  local_debug_data.reserved_0 = 0xF123;
  local_debug_data.i2c_bus_error_count = RCON;


  ETMCanMasterProcessMessage();
  ETMCanMasterTimedTransmit();
  ETMCanMasterProcessLogData();
  ETMCanMasterCheckForTimeOut();
  if (_SYNC_CONTROL_CLEAR_DEBUG_DATA) {
    ETMCanMasterClearDebug();
  }

  local_debug_data.can_bus_error_count = local_can_errors.timeout;
  local_debug_data.can_bus_error_count += local_can_errors.message_tx_buffer_overflow;
  local_debug_data.can_bus_error_count += local_can_errors.message_rx_buffer_overflow;
  local_debug_data.can_bus_error_count += local_can_errors.data_log_rx_buffer_overflow;
  local_debug_data.can_bus_error_count += local_can_errors.address_error;
  local_debug_data.can_bus_error_count += local_can_errors.invalid_index;
  local_debug_data.can_bus_error_count += local_can_errors.unknown_message_identifier;
}


void ETMCanMasterProcessMessage(void) {
  ETMCanMessage next_message;
  while (ETMCanBufferNotEmpty(&etm_can_rx_message_buffer)) {
    ETMCanReadMessageFromBuffer(&etm_can_rx_message_buffer, &next_message);
    if ((next_message.identifier & ETM_CAN_MSG_MASTER_ADDR_MASK) == ETM_CAN_MSG_SET_2_RX) {
      ETMCanMasterSet2FromSlave(&next_message);
    } else if ((next_message.identifier & ETM_CAN_MSG_MASTER_ADDR_MASK) == ETM_CAN_MSG_STATUS_RX) {
      ETMCanMasterUpdateSlaveStatus(&next_message);
    } else {
      local_can_errors.unknown_message_identifier++;
    } 
  }
  
  local_can_errors.message_tx_buffer_overflow = etm_can_tx_message_buffer.message_overwrite_count;
  local_can_errors.message_rx_buffer_overflow = etm_can_rx_message_buffer.message_overwrite_count;
  local_can_errors.data_log_rx_buffer_overflow = etm_can_rx_data_log_buffer.message_overwrite_count;
}


void ETMCanMasterTimedTransmit(void) {
  /*
    One command is schedule to be sent every 25ms
    This loops through 8 times so each command is sent once every 200mS (5Hz)
    The sync command and Pulse Sync enable command are each sent twice for an effecive rate of 100ms (10Hz)
  */

  if ((_STATUS_X_RAY_DISABLED == 0) && (_SYNC_CONTROL_PULSE_SYNC_DISABLE_XRAY)) {
    // We need to immediately send out a sync message
    ETMCanMasterSendSync();
  }


  if (_T4IF) {
    // should be true once every 25mS
    // each of the 8 cases will be true once every 200mS
    _T4IF = 0;

    if (!_STATUS_PERSONALITY_LOADED) {
      // Just send out a sync message
      ETMCanMasterSendSync();
    } else {
      master_high_speed_update_index++;
      master_high_speed_update_index &= 0x7;
      
      
      switch (master_high_speed_update_index) 
	{
	case 0x0:
	  // Send Sync Command (this is on TX1) - This also includes Pulse Sync Enable/Disable
	  ETMCanMasterSendSync();
	  break;
	  
	case 0x1:
	  // Send High/Low Energy Program voltage to Lambda Board
	  ETMCanMasterHVLambdaUpdateOutput();
	  break;
	  
	case 0x2:
	  // Send Sync Command (this is on TX1) - This also includes Pulse Sync Enable/Disable
	  ETMCanMasterSendSync();
	  break;
	  
	case 0x3:
	  // Send Heater/Magnet Current to Heater Magnet Board
	  ETMCanMasterHtrMagnetUpdateOutput();
	  break;
	  
	case 0x4:
	  // Send Sync Command (this is on TX1) - This also includes Pulse Sync Enable/Disable
	  ETMCanMasterSendSync();
	  break;
	  
	case 0x5:
	  // Send High/Low Energy Pulse top voltage to Gun Driver
	  ETMCanMasterGunDriverUpdatePulseTop();
	  break;
	  
	case 0x6:
	  // Send Sync Command (this is on TX1) - This also includes Pulse Sync Enable/Disable
	  ETMCanMasterSendSync();
	  break;
	  
	case 0x7:
	  // LOOP THROUGH SLOWER SPEED TRANSMITS
	  /*
	    THis is the following messages
	    Gun Driver Heater/Cathode
	    AFC Home/Offset
	    Pulse Sync Registers 1,2,3,4
	  */

	  master_low_speed_update_index++;
	  if (master_low_speed_update_index >= 6) {
	    master_low_speed_update_index = 0;
	  } 
	  
	  if (master_low_speed_update_index == 0) {
	    ETMCanMasterGunDriverUpdateHeaterCathode();  
	  }

	  if (master_low_speed_update_index == 1) {
	    ETMCanMasterAFCUpdateHomeOffset();	   
	  }

	  if (master_low_speed_update_index == 2) {
	    ETMCanMasterPulseSyncUpdateHighRegZero();	   
	  }

	  if (master_low_speed_update_index == 3) {
	    ETMCanMasterPulseSyncUpdateHighRegOne();	   
	  }

	  if (master_low_speed_update_index == 4) {
	    ETMCanMasterPulseSyncUpdateLowRegZero();	   
	  }

	  if (master_low_speed_update_index == 5) {
	    ETMCanMasterPulseSyncUpdateLowRegOne();	   
	  }
	  break;
	}
    }
  }
}




void ETMCanMasterSendSync(void) {
  ETMCanMessage sync_message;
  sync_message.identifier = ETM_CAN_MSG_SYNC_TX;
  sync_message.word0 = _SYNC_CONTROL_WORD;
  sync_message.word1 = etm_can_sync_message.sync_1;
  sync_message.word2 = etm_can_sync_message.sync_2;
  sync_message.word3 = etm_can_sync_message.sync_3;
  
  ETMCanTXMessage(&sync_message, &CXTX1CON);
  local_can_errors.tx_1++;

  _STATUS_X_RAY_DISABLED = _SYNC_CONTROL_PULSE_SYNC_DISABLE_XRAY;
}

void ETMCanMasterHVLambdaUpdateOutput(void) {
  ETMCanMessage can_message;
  
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_HV_LAMBDA_BOARD << 3));
  can_message.word3 = ETM_CAN_REGISTER_HV_LAMBDA_SET_1_LAMBDA_SET_POINT;
  can_message.word2 = etm_can_hv_lambda_mirror.ecb_low_set_point;
  can_message.word1 = etm_can_hv_lambda_mirror.ecb_high_set_point;
  can_message.word0 = 0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterAFCUpdateHomeOffset(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_AFC_CONTROL_BOARD << 3));
  can_message.word3 = ETM_CAN_REGISTER_AFC_SET_1_HOME_POSITION_AND_OFFSET;
  can_message.word2 = etm_can_afc_mirror.aft_control_voltage;
  can_message.word1 = (unsigned int)etm_can_afc_mirror.afc_offset;
  can_message.word0 = etm_can_afc_mirror.afc_home_position;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterHtrMagnetUpdateOutput(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_HEATER_MAGNET_BOARD << 3));
  can_message.word3 = ETM_CAN_REGISTER_HEATER_MAGNET_SET_1_CURRENT_SET_POINT;
  can_message.word2 = 0;
  can_message.word1 = etm_can_heater_magnet_mirror.htrmag_heater_current_set_point_scaled;
  can_message.word0 = etm_can_heater_magnet_mirror.htrmag_magnet_current_set_point;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterGunDriverUpdatePulseTop(void) {
  ETMCanMessage can_message;
  
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_GUN_DRIVER_BOARD << 3));
  can_message.word3 = ETM_CAN_REGISTER_GUN_DRIVER_SET_1_GRID_TOP_SET_POINT;
  can_message.word2 = 0;
  can_message.word1 = etm_can_gun_driver_mirror.gun_high_energy_pulse_top_voltage_set_point;
  can_message.word0 = etm_can_gun_driver_mirror.gun_low_energy_pulse_top_voltage_set_point;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterGunDriverUpdateHeaterCathode(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_GUN_DRIVER_BOARD << 3));
  can_message.word3 = ETM_CAN_REGISTER_GUN_DRIVER_SET_1_HEATER_CATHODE_SET_POINT;
  can_message.word2 = 0;
  can_message.word1 = etm_can_gun_driver_mirror.gun_cathode_voltage_set_point;
  can_message.word0 = etm_can_gun_driver_mirror.gun_heater_voltage_set_point;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterPulseSyncUpdateHighRegZero(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_PULSE_SYNC_BOARD << 3));
  can_message.word3 = ETM_CAN_REGISTER_PULSE_SYNC_SET_1_HIGH_ENERGY_TIMING_REG_0;
  can_message.word2 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_grid_delay_high_intensity_2;
  can_message.word1 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_grid_delay_high_intensity_0;
  can_message.word0 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_pfn_delay_high;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterPulseSyncUpdateHighRegOne(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_PULSE_SYNC_BOARD << 3));
  can_message.word3 = ETM_CAN_REGISTER_PULSE_SYNC_SET_1_HIGH_ENERGY_TIMING_REG_1;
  can_message.word2 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_grid_width_high_intensity_2;
  can_message.word1 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_grid_width_high_intensity_0;
  can_message.word0 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_afc_delay_high;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterPulseSyncUpdateLowRegZero(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_PULSE_SYNC_BOARD << 3));
  can_message.word3 = ETM_CAN_REGISTER_PULSE_SYNC_SET_1_LOW_ENERGY_TIMING_REG_0;
  can_message.word2 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_grid_delay_low_intensity_2;
  can_message.word1 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_grid_delay_low_intensity_0;
  can_message.word0 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_pfn_delay_low;

  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterPulseSyncUpdateLowRegOne(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_PULSE_SYNC_BOARD << 3));
  can_message.word3 = ETM_CAN_REGISTER_PULSE_SYNC_SET_1_LOW_ENERGY_TIMING_REG_1;
  can_message.word2 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_grid_width_low_intensity_2;
  can_message.word1 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_grid_width_low_intensity_0;
  can_message.word0 = *(unsigned int*)&etm_can_pulse_sync_mirror.psync_afc_delay_low;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}








void ETMCanMasterSet2FromSlave(ETMCanMessage* message_ptr) {
  unsigned int index_word;
  index_word = message_ptr->word3 & 0x0FFF;
  
  if ((index_word >= 0x0100) & (index_word < 0x0200)) {
    // This is Calibration data that was read from the slave EEPROM
    SendCalibrationData(message_ptr->word3, message_ptr->word1, message_ptr->word0);
  } else {
    // It was not a set value index 
    local_can_errors.invalid_index++;
  }
}


void ETMCanMasterUpdateSlaveStatus(ETMCanMessage* message_ptr) {
  ETMCanStatusRegister status_message;
  unsigned int source_board;
  unsigned int all_boards_connected;
  unsigned int message_bit;
  source_board = (message_ptr->identifier >> 3);
  source_board &= 0x000F;
  message_bit = 1 << source_board;
  
  status_message.status_bits = *(ETMCanStatusRegisterStatusBits*)&message_ptr->word0;
  status_message.fault_bits = *(ETMCanStatusRegisterFaultBits*)&message_ptr->word1;
  status_message.data_word_A   = message_ptr->word2;
  status_message.data_word_B   = message_ptr->word3;
  ClrWdt();

  status_message.status_bits.control_7_ecb_can_not_active = 0;

  switch (source_board) {
    /*
      Place all board specific status updates here
    */

  case ETM_CAN_ADDR_ION_PUMP_BOARD:
    etm_can_ion_pump_mirror.status_data = status_message;
    board_status_received.ion_pump_board = 1;
    //ETMCanSetBit(&etm_can_ethernet_board_data.status_received_register, ETM_CAN_BIT_ION_PUMP_BOARD);
    break;

  case ETM_CAN_ADDR_MAGNETRON_CURRENT_BOARD:
    etm_can_magnetron_current_mirror.status_data = status_message;
    board_status_received.magnetron_current_board = 1;
    //ETMCanSetBit(&etm_can_ethernet_board_data.status_received_register, ETM_CAN_BIT_MAGNETRON_CURRENT_BOARD);
    break;

  case ETM_CAN_ADDR_PULSE_SYNC_BOARD:
    etm_can_pulse_sync_mirror.status_data = status_message;
    board_status_received.pulse_sync_board = 1;
    //ETMCanSetBit(&etm_can_ethernet_board_data.status_received_register, ETM_CAN_BIT_PULSE_SYNC_BOARD);
    break;

  case ETM_CAN_ADDR_HV_LAMBDA_BOARD:
    etm_can_hv_lambda_mirror.status_data = status_message;
    board_status_received.hv_lambda_board = 1;
    //ETMCanSetBit(&etm_can_ethernet_board_data.status_received_register, ETM_CAN_BIT_HV_LAMBDA_BOARD);
    break;

  case ETM_CAN_ADDR_AFC_CONTROL_BOARD:
    etm_can_afc_mirror.status_data = status_message;
    board_status_received.afc_board = 1;
    //ETMCanSetBit(&etm_can_ethernet_board_data.status_received_register, ETM_CAN_BIT_AFC_CONTROL_BOARD);
    break;
    
  case ETM_CAN_ADDR_COOLING_INTERFACE_BOARD:
    etm_can_cooling_mirror.status_data = status_message;
    board_status_received.cooling_interface_board = 1;
    //ETMCanSetBit(&etm_can_ethernet_board_data.status_received_register, ETM_CAN_BIT_COOLING_INTERFACE_BOARD);
    break;

  case ETM_CAN_ADDR_HEATER_MAGNET_BOARD:
    etm_can_heater_magnet_mirror.status_data = status_message;
    board_status_received.heater_magnet_board = 1;
    //ETMCanSetBit(&etm_can_ethernet_board_data.status_received_register, ETM_CAN_BIT_HEATER_MAGNET_BOARD);
    break;

  case ETM_CAN_ADDR_GUN_DRIVER_BOARD:
    etm_can_gun_driver_mirror.status_data = status_message;
    board_status_received.gun_driver_board = 1;
    //ETMCanSetBit(&etm_can_ethernet_board_data.status_received_register, ETM_CAN_BIT_GUN_DRIVER_BOARD);
    break;


  default:
    local_can_errors.address_error++;
    break;
  }

  // Figure out if all the boards are connected
  all_boards_connected = 1;

#ifndef __IGNORE_ION_PUMP_MODULE
  if (!board_status_received.ion_pump_board) {
    all_boards_connected = 0;
  }
#endif


#ifndef __IGNORE_PULSE_CURRENT_MODULE
  if (!board_status_received.magnetron_current_board) {
    all_boards_connected = 0;
  }
#endif

#ifndef __IGNORE_PULSE_SYNC_MODULE
  if (!board_status_received.pulse_sync_board) {
    all_boards_connected = 0;
  }
#endif

#ifndef __IGNORE_HV_LAMBDA_MODULE
  if (!board_status_received.hv_lambda_board) {
    all_boards_connected = 0;
  }
#endif

#ifndef __IGNORE_AFC_MODULE
  if (!board_status_received.afc_board) {
    all_boards_connected = 0;
  }
#endif

#ifndef __IGNORE_COOLING_INTERFACE_MODULE
  if (!board_status_received.cooling_interface_board) {
    all_boards_connected = 0;
  }
#endif

#ifndef __IGNORE_HEATER_MAGNET_MODULE
  if (!board_status_received.heater_magnet_board) {
    all_boards_connected = 0;
  }
#endif

#ifndef __IGNORE_GUN_DRIVER_MODULE
  if (!board_status_received.gun_driver_board) {
    all_boards_connected = 0;
  }
#endif

  if (all_boards_connected) {
    // Clear the status received register
    *(unsigned int*)&board_status_received = 0x0000; 
    
    // Reset T5 to start the next timer cycle
    TMR5 = 0;
  }
}




void ETMCanMasterProcessLogData(void) {
  ETMCanMessage          next_message;
  unsigned int           data_log_index;
  ETMCanSystemDebugData* debug_data_ptr;
  ETMCanAgileConfig*     config_data_ptr;
  ETMCanCanStatus*       can_status_ptr;

  unsigned int           fast_log_buffer_index;
  ETMCanHighSpeedData*   ptr_high_speed_data;



  while (ETMCanBufferNotEmpty(&etm_can_rx_data_log_buffer)) {
    ETMCanReadMessageFromBuffer(&etm_can_rx_data_log_buffer, &next_message);
    data_log_index = next_message.identifier;
    data_log_index >>= 3;
    data_log_index &= 0x00FF;
    
    if ((data_log_index & 0x000F) <= 0x000B) {
      // It is a common data logging buffer, load into the appropraite ram location
      

      // First figure out which board the data is from
      switch (data_log_index >> 4) 
	{
	case ETM_CAN_ADDR_ION_PUMP_BOARD:
	  debug_data_ptr  = &etm_can_ion_pump_mirror.debug_data;
	  config_data_ptr = &etm_can_ion_pump_mirror.configuration;
	  can_status_ptr  = &etm_can_ion_pump_mirror.can_status;
	  break;
	  
	case ETM_CAN_ADDR_MAGNETRON_CURRENT_BOARD:
	  debug_data_ptr  = &etm_can_magnetron_current_mirror.debug_data;
	  config_data_ptr = &etm_can_magnetron_current_mirror.configuration;
	  can_status_ptr  = &etm_can_magnetron_current_mirror.can_status;
	  break;

	case ETM_CAN_ADDR_PULSE_SYNC_BOARD:
	  debug_data_ptr  = &etm_can_pulse_sync_mirror.debug_data;
	  config_data_ptr = &etm_can_pulse_sync_mirror.configuration;
	  can_status_ptr  = &etm_can_pulse_sync_mirror.can_status;
	  break;
	  
	case ETM_CAN_ADDR_HV_LAMBDA_BOARD:
	  debug_data_ptr  = &etm_can_hv_lambda_mirror.debug_data;
	  config_data_ptr = &etm_can_hv_lambda_mirror.configuration;
	  can_status_ptr  = &etm_can_hv_lambda_mirror.can_status;
	  break;
	  
	case ETM_CAN_ADDR_AFC_CONTROL_BOARD:
	  debug_data_ptr  = &etm_can_afc_mirror.debug_data;
	  config_data_ptr = &etm_can_afc_mirror.configuration;
	  can_status_ptr  = &etm_can_afc_mirror.can_status;
	  break;

	case ETM_CAN_ADDR_COOLING_INTERFACE_BOARD:
	  debug_data_ptr  = &etm_can_cooling_mirror.debug_data;
	  config_data_ptr = &etm_can_cooling_mirror.configuration;
	  can_status_ptr  = &etm_can_cooling_mirror.can_status;
	  break;
	  
	case ETM_CAN_ADDR_HEATER_MAGNET_BOARD:
	  debug_data_ptr  = &etm_can_heater_magnet_mirror.debug_data;
	  config_data_ptr = &etm_can_heater_magnet_mirror.configuration;
	  can_status_ptr  = &etm_can_heater_magnet_mirror.can_status;
	  break;

	case ETM_CAN_ADDR_GUN_DRIVER_BOARD:
	  debug_data_ptr  = &etm_can_gun_driver_mirror.debug_data;
	  config_data_ptr = &etm_can_gun_driver_mirror.configuration;
	  can_status_ptr  = &etm_can_gun_driver_mirror.can_status;
	  break;

	default:
	  local_can_errors.address_error++;
	  break;
	  
	}

      // Now figure out which data log it is
      switch (data_log_index & 0x000F)
	{
	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_ERROR_0:
	  debug_data_ptr->i2c_bus_error_count        = next_message.word3;
	  debug_data_ptr->spi_bus_error_count        = next_message.word2;
	  debug_data_ptr->can_bus_error_count        = next_message.word1;
	  debug_data_ptr->scale_error_count          = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_ERROR_1:
	  debug_data_ptr->reset_count                = next_message.word3;
	  debug_data_ptr->self_test_results          = *(ETMCanSelfTestRegister*)&next_message.word2;
	  debug_data_ptr->reserved_0                 = next_message.word1;  // This is RCON as of today
	  debug_data_ptr->reserved_1                 = next_message.word0;  // unused at this time
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_0:
	  debug_data_ptr->debug_0                    = next_message.word3;
	  debug_data_ptr->debug_1                    = next_message.word2;
	  debug_data_ptr->debug_2                    = next_message.word1;
	  debug_data_ptr->debug_3                    = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_1:
	  debug_data_ptr->debug_4                    = next_message.word3;
	  debug_data_ptr->debug_5                    = next_message.word2;
	  debug_data_ptr->debug_6                    = next_message.word1;
	  debug_data_ptr->debug_7                    = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_2:
	  debug_data_ptr->debug_8                    = next_message.word3;
	  debug_data_ptr->debug_9                    = next_message.word2;
	  debug_data_ptr->debug_A                    = next_message.word1;
	  debug_data_ptr->debug_B                    = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_3:
	  debug_data_ptr->debug_C                    = next_message.word3;
	  debug_data_ptr->debug_D                    = next_message.word2;
	  debug_data_ptr->debug_E                    = next_message.word1;
	  debug_data_ptr->debug_F                    = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_0:
	  config_data_ptr->agile_number_high_word     = next_message.word3;
	  config_data_ptr->agile_number_low_word      = next_message.word2;
	  config_data_ptr->agile_dash                 = next_message.word1;
	  config_data_ptr->agile_rev_ascii            = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_1:
	  config_data_ptr->serial_number              = next_message.word3;
	  config_data_ptr->firmware_branch            = next_message.word2;
	  config_data_ptr->firmware_major_rev         = next_message.word1;
	  config_data_ptr->firmware_minor_rev         = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_0:
	  can_status_ptr->CXEC_reg        = next_message.word3;
	  can_status_ptr->error_flag      = next_message.word2;
	  can_status_ptr->tx_1            = next_message.word1;
	  can_status_ptr->tx_2            = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_1:
	  can_status_ptr->rx_0_filt_0     = next_message.word3;
	  can_status_ptr->rx_0_filt_1     = next_message.word2;
	  can_status_ptr->rx_1_filt_2     = next_message.word1;
	  can_status_ptr->isr_entered     = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_2:
	  can_status_ptr->unknown_message_identifier     = next_message.word3;
	  can_status_ptr->invalid_index                  = next_message.word2;
	  can_status_ptr->address_error                  = next_message.word1;
	  can_status_ptr->tx_0                           = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_3:
	  can_status_ptr->message_tx_buffer_overflow     = next_message.word3;
	  can_status_ptr->message_rx_buffer_overflow     = next_message.word2;
	  can_status_ptr->data_log_rx_buffer_overflow    = next_message.word1;
	  can_status_ptr->timeout                        = next_message.word0;
	  break;
	  
	} 

    } else {
      // It is board specific logging data

      // Figure out where to store high speed logging data (this will only be used if it IS high speed data logging)
      // But I'm going to go ahead and calculate where to store it for all messages
      fast_log_buffer_index = next_message.word3 & 0x000F;
      if (next_message.word3 & 0x0010) {
	ptr_high_speed_data = &high_speed_data_buffer_a[fast_log_buffer_index];
      } else {
	ptr_high_speed_data = &high_speed_data_buffer_b[fast_log_buffer_index];
      }
      
      switch (data_log_index) 
	{
	case ETM_CAN_DATA_LOG_REGISTER_HV_LAMBDA_FAST_PROGRAM_VOLTAGE:
	  // Update the high speed data table
	  ptr_high_speed_data->hvlambda_readback_high_energy_lambda_program_voltage = next_message.word2;
	  ptr_high_speed_data->hvlambda_readback_low_energy_lambda_program_voltage = next_message.word1;
	  ptr_high_speed_data->hvlambda_readback_peak_lambda_voltage = next_message.word0;
	  
	  etm_can_hv_lambda_mirror.readback_high_vprog = next_message.word2;
	  etm_can_hv_lambda_mirror.readback_low_vprog = next_message.word1;
	  etm_can_hv_lambda_mirror.readback_peak_lambda_voltage = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_HV_LAMBDA_SLOW_SET_POINT:
	  etm_can_hv_lambda_mirror.eoc_not_reached_count = next_message.word3;
	  etm_can_hv_lambda_mirror.readback_vmon = next_message.word2;
	  etm_can_hv_lambda_mirror.readback_imon = next_message.word1;
	  etm_can_hv_lambda_mirror.readback_base_plate_temp = next_message.word0;
	  break;

	  
	case ETM_CAN_DATA_LOG_REGISTER_ION_PUMP_SLOW_MONITORS:
	  etm_can_ion_pump_mirror.ionpump_readback_ion_pump_volage_monitor = next_message.word3;
	  etm_can_ion_pump_mirror.ionpump_readback_ion_pump_current_monitor = next_message.word2;
	  etm_can_ion_pump_mirror.ionpump_readback_filtered_high_energy_target_current = next_message.word1;
	  etm_can_ion_pump_mirror.ionpump_readback_filtered_low_energy_target_current = next_message.word0;
	  break;


	case ETM_CAN_DATA_LOG_REGISTER_AFC_FAST_POSITION:
	  ptr_high_speed_data->afc_readback_current_position = next_message.word2;
	  ptr_high_speed_data->afc_readback_target_position = next_message.word1;
	  // unused word 0

	  etm_can_afc_mirror.afc_readback_current_position = next_message.word2;
	  etm_can_afc_mirror.afc_readback_target_position = next_message.word1;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_AFC_FAST_READINGS:
	  ptr_high_speed_data->afc_readback_a_input = next_message.word2;
	  ptr_high_speed_data->afc_readback_b_input = next_message.word1;
	  ptr_high_speed_data->afc_readback_filtered_error_reading = next_message.word0;

	  etm_can_afc_mirror.afc_readback_afc_a_input_reading = next_message.word2;
	  etm_can_afc_mirror.afc_readback_afc_b_input_reading = next_message.word1;
	  etm_can_afc_mirror.afc_readback_filtered_error_reading = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_AFC_SLOW_SETTINGS:
	  etm_can_afc_mirror.afc_readback_home_position = next_message.word3;
	  // unused word2
	  // unused word1
	  etm_can_afc_mirror.readback_aft_control_voltage = next_message.word0;
	  break;


	case ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_FAST_PREVIOUS_PULSE:
	  // Update the high speed data table
	  ptr_high_speed_data->magmon_readback_magnetron_low_energy_current = next_message.word2;  // This is the previous internal DAC Reading
	  ptr_high_speed_data->magmon_readback_magnetron_high_energy_current = next_message.word1; // This is the previous external DAC Reading
	  if (next_message.word0) {
	    ptr_high_speed_data->status_bits.arc_this_pulse = 1;
	  }
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_SLOW_FILTERED_PULSE:
	  etm_can_magnetron_current_mirror.magmon_readback_spare = next_message.word3;
	  etm_can_magnetron_current_mirror.magmon_readback_arcs_this_hv_on = next_message.word2;
	  etm_can_magnetron_current_mirror.magmon_filtered_low_energy_pulse_current = next_message.word1;
	  etm_can_magnetron_current_mirror.magmon_filtered_high_energy_pulse_current = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_SLOW_ARCS:
	  etm_can_magnetron_current_mirror.magmon_arcs_lifetime = next_message.word3;
	  etm_can_magnetron_current_mirror.magmon_arcs_lifetime <<= 16;
	  etm_can_magnetron_current_mirror.magmon_arcs_lifetime += next_message.word2;
	  etm_can_magnetron_current_mirror.magmon_pulses_this_hv_on = next_message.word1;
	  etm_can_magnetron_current_mirror.magmon_pulses_this_hv_on <<= 16;
	  etm_can_magnetron_current_mirror.magmon_pulses_this_hv_on += next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_SLOW_PULSE_COUNT:
	  etm_can_magnetron_current_mirror.magmon_pulses_lifetime = next_message.word3;
	  etm_can_magnetron_current_mirror.magmon_pulses_lifetime <<= 16;
	  etm_can_magnetron_current_mirror.magmon_pulses_lifetime += next_message.word2;
	  etm_can_magnetron_current_mirror.magmon_pulses_lifetime <<= 16;
	  etm_can_magnetron_current_mirror.magmon_pulses_lifetime += next_message.word1;
	  etm_can_magnetron_current_mirror.magmon_pulses_lifetime <<= 16;
	  etm_can_magnetron_current_mirror.magmon_pulses_lifetime += next_message.word0;
	  break;


	case ETM_CAN_DATA_LOG_REGISTER_COOLING_SLOW_FLOW_0:
	  etm_can_cooling_mirror.cool_readback_hvps_coolant_flow = next_message.word3;
	  etm_can_cooling_mirror.cool_readback_magnetron_coolant_flow = next_message.word2;
	  etm_can_cooling_mirror.cool_readback_linac_coolant_flow = next_message.word1;
	  etm_can_cooling_mirror.cool_readback_circulator_coolant_flow = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_COOLING_SLOW_FLOW_1:
	  etm_can_cooling_mirror.cool_readback_bottle_count = next_message.word3;
	  etm_can_cooling_mirror.cool_readback_pulses_available = ((next_message.word2 & 0xFF00) >> 8);
	  etm_can_cooling_mirror.cool_readback_low_pressure_override_available = (next_message.word2 & 0x00FF);
	  etm_can_cooling_mirror.cool_readback_hx_coolant_flow = next_message.word1;
	  etm_can_cooling_mirror.cool_readback_spare_coolant_flow = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_COOLING_SLOW_ANALOG_READINGS:
	  etm_can_cooling_mirror.cool_readback_coolant_temperature = next_message.word3;
	  etm_can_cooling_mirror.cool_readback_sf6_pressure = next_message.word2;
	  etm_can_cooling_mirror.cool_readback_cabinet_temperature = next_message.word1;
	  etm_can_cooling_mirror.cool_readback_linac_temperature = next_message.word0;
	  break;


	case ETM_CAN_DATA_LOG_REGISTER_HEATER_MAGNET_SLOW_READINGS:
	  etm_can_heater_magnet_mirror.htrmag_readback_heater_current = next_message.word3;
	  etm_can_heater_magnet_mirror.htrmag_readback_heater_voltage = next_message.word2;
	  etm_can_heater_magnet_mirror.htrmag_readback_magnet_current = next_message.word1;
	  etm_can_heater_magnet_mirror.htrmag_readback_magnet_voltage = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_HEATER_MAGNET_SLOW_SET_POINTS:
	  etm_can_heater_magnet_mirror.htrmag_readback_heater_current_set_point = next_message.word3;
	  etm_can_heater_magnet_mirror.htrmag_readback_heater_voltage_set_point = next_message.word2;
	  etm_can_heater_magnet_mirror.htrmag_readback_magnet_current_set_point = next_message.word1;
	  etm_can_heater_magnet_mirror.htrmag_readback_magnet_voltage_set_point = next_message.word0;
	  break;


	case ETM_CAN_DATA_LOG_REGISTER_GUN_DRIVER_SLOW_PULSE_TOP_MON:
	  etm_can_gun_driver_mirror.gun_readback_high_energy_pulse_top_voltage_monitor = next_message.word3;
	  etm_can_gun_driver_mirror.gun_readback_low_energy_pulse_top_voltage_monitor = next_message.word2;
	  etm_can_gun_driver_mirror.gun_readback_cathode_voltage_monitor = next_message.word1;
	  etm_can_gun_driver_mirror.gun_readback_peak_beam_current = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_GUN_DRIVER_SLOW_HEATER_MON:
	  etm_can_gun_driver_mirror.gun_readback_heater_voltage_monitor = next_message.word3;
	  etm_can_gun_driver_mirror.gun_readback_heater_current_monitor = next_message.word2;
	  etm_can_gun_driver_mirror.gun_readback_heater_time_delay_remaining = next_message.word1;
	  etm_can_gun_driver_mirror.gun_readback_driver_temperature = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_GUN_DRIVER_SLOW_SET_POINTS:
	  etm_can_gun_driver_mirror.gun_readback_high_energy_pulse_top_set_point = next_message.word3;
	  etm_can_gun_driver_mirror.gun_readback_low_energy_pulse_top_set_point = next_message.word2;
	  etm_can_gun_driver_mirror.gun_readback_heater_voltage_set_point = next_message.word1;
	  etm_can_gun_driver_mirror.gun_readback_cathode_voltage_set_point = next_message.word0;
	  break;


	case ETM_CAN_DATA_LOG_REGISTER_GUN_DRIVER_FPGA_DATA:
	  etm_can_gun_driver_mirror.gun_readback_fpga_asdr_register = next_message.word3;
	  etm_can_gun_driver_mirror.gun_readback_analog_fault_status = next_message.word2;
	  etm_can_gun_driver_mirror.gun_readback_system_logic_state = next_message.word1;
	  etm_can_gun_driver_mirror.gun_readback_bias_voltage_mon = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_PULSE_SYNC_FAST_TRIGGER_DATA:
	  ptr_high_speed_data->psync_readback_trigger_width_and_filtered_trigger_width = next_message.word2;
	  ptr_high_speed_data->psync_readback_high_energy_grid_width_and_delay = next_message.word1;
	  ptr_high_speed_data->psync_readback_low_energy_grid_width_and_delay = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_PULSE_SYNC_SLOW_TIMING_DATA_0:
	  // DPARKER - WE have not defined anywhere to store this data
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_PULSE_SYNC_SLOW_TIMING_DATA_1:
	  // DPARKER - WE have not defined anywhere to store this data
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_PULSE_SYNC_SLOW_TIMING_DATA_2:
	  // DPARKER - WE have not defined anywhere to store this data
	  break;

	default:
	  local_can_errors.unknown_message_identifier++;
	  break;
	}
    }
  }
}


/*
  This is the CAN Interrupt for the Master module
*/

void __attribute__((interrupt(__save__(CORCON,SR)), no_auto_psv)) _CXInterrupt(void) {
  ETMCanMessage can_message;
  unsigned int fast_log_buffer_index;
  unsigned int pulse_time;

  _CXIF = 0;
  //local_can_errors.isr_entered++;
  local_can_errors.isr_entered |= CXINTF;
  
  pulse_time = global_data_A36507.millisecond_counter;
  pulse_time += ETMScaleFactor2((TMR5>>11),MACRO_DEC_TO_CAL_FACTOR_2(.8192),0);
  if (_T2IF) {
    pulse_time += 10;
  }



  if(CXRX0CONbits.RXFUL) {
    /*
      A message has been received in Buffer Zero
    */
    if (!CXRX0CONbits.FILHIT0) {
      // The command was received by Filter 0
      local_can_errors.rx_0_filt_0++;
      // It is a Next Pulse Level Command
      ETMCanRXMessage(&can_message, &CXRX0CON);
      etm_can_next_pulse_level = can_message.word1;
      etm_can_next_pulse_count = can_message.word0;


      if (_SYNC_CONTROL_HIGH_SPEED_LOGGING) {
	// Prepare the buffer to store the data
	fast_log_buffer_index = etm_can_next_pulse_count & 0x000F;
	if (etm_can_next_pulse_count & 0x0010) {
	  // We are putting data into buffer A
	  global_data_A36507.buffer_a_ready_to_send = 0;
	  global_data_A36507.buffer_a_sent = 0;
	  if (fast_log_buffer_index >= 3) {
	    global_data_A36507.buffer_b_ready_to_send = 1;
	  }
	  
	  *(unsigned int*)&high_speed_data_buffer_a[fast_log_buffer_index].status_bits = 0; // clear the status bits register
	  high_speed_data_buffer_a[fast_log_buffer_index].pulse_count = etm_can_next_pulse_count;
	  if (etm_can_next_pulse_level) {
	    high_speed_data_buffer_a[fast_log_buffer_index].status_bits.high_energy_pulse = 1;
	  }

	  high_speed_data_buffer_a[fast_log_buffer_index].x_ray_on_seconds_lsw = global_data_A36507.time_seconds_now;
	  high_speed_data_buffer_a[fast_log_buffer_index].x_ray_on_milliseconds = pulse_time;
	  
	  high_speed_data_buffer_a[fast_log_buffer_index].hvlambda_readback_high_energy_lambda_program_voltage = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].hvlambda_readback_low_energy_lambda_program_voltage = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].hvlambda_readback_peak_lambda_voltage = 0;
	  
	  high_speed_data_buffer_a[fast_log_buffer_index].afc_readback_current_position = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].afc_readback_target_position = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].afc_readback_a_input = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].afc_readback_b_input = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].afc_readback_filtered_error_reading = 0;
	  
	  high_speed_data_buffer_a[fast_log_buffer_index].ionpump_readback_high_energy_target_current_reading = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].ionpump_readback_low_energy_target_current_reading = 0;
	  
	  high_speed_data_buffer_a[fast_log_buffer_index].magmon_readback_magnetron_high_energy_current = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].magmon_readback_magnetron_low_energy_current = 0;
	  
	  high_speed_data_buffer_a[fast_log_buffer_index].psync_readback_trigger_width_and_filtered_trigger_width = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].psync_readback_high_energy_grid_width_and_delay = 0;
	  high_speed_data_buffer_a[fast_log_buffer_index].psync_readback_low_energy_grid_width_and_delay = 0;
	  


	  high_speed_data_buffer_a[fast_log_buffer_index].ionpump_readback_high_energy_target_current_reading = etm_can_next_pulse_level;

	} else {
	  // We are putting data into buffer B
	  global_data_A36507.buffer_b_ready_to_send = 0;
	  global_data_A36507.buffer_b_sent = 0;
	  if (fast_log_buffer_index >= 3) {
	    global_data_A36507.buffer_a_ready_to_send = 1;
	  }
	  
	  *(unsigned int*)&high_speed_data_buffer_b[fast_log_buffer_index].status_bits = 0; // Clear the status bits register
	  high_speed_data_buffer_b[fast_log_buffer_index].pulse_count = etm_can_next_pulse_count;
	  if (etm_can_next_pulse_level) {
	    high_speed_data_buffer_b[fast_log_buffer_index].status_bits.high_energy_pulse = 1;
	  }
	  
	  high_speed_data_buffer_b[fast_log_buffer_index].x_ray_on_seconds_lsw = global_data_A36507.time_seconds_now;
	  high_speed_data_buffer_b[fast_log_buffer_index].x_ray_on_milliseconds = pulse_time;
	  
	  high_speed_data_buffer_b[fast_log_buffer_index].hvlambda_readback_high_energy_lambda_program_voltage = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].hvlambda_readback_low_energy_lambda_program_voltage = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].hvlambda_readback_peak_lambda_voltage = 0;
	  
	  high_speed_data_buffer_b[fast_log_buffer_index].afc_readback_current_position = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].afc_readback_target_position = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].afc_readback_a_input = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].afc_readback_b_input = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].afc_readback_filtered_error_reading = 0;
	  
	  high_speed_data_buffer_b[fast_log_buffer_index].ionpump_readback_high_energy_target_current_reading = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].ionpump_readback_low_energy_target_current_reading = 0;
	  
	  high_speed_data_buffer_b[fast_log_buffer_index].magmon_readback_magnetron_high_energy_current = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].magmon_readback_magnetron_low_energy_current = 0;
	  
	  high_speed_data_buffer_b[fast_log_buffer_index].psync_readback_trigger_width_and_filtered_trigger_width = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].psync_readback_high_energy_grid_width_and_delay = 0;
	  high_speed_data_buffer_b[fast_log_buffer_index].psync_readback_low_energy_grid_width_and_delay = 0;


	  high_speed_data_buffer_b[fast_log_buffer_index].ionpump_readback_high_energy_target_current_reading = etm_can_next_pulse_level;
	}
      }
    } else {
      // The commmand was received by Filter 1
      local_can_errors.rx_0_filt_1++;
      // The command is a data log.  Add it to the data log buffer
      ETMCanRXMessageBuffer(&etm_can_rx_data_log_buffer, &CXRX0CON);
    }
    CXINTFbits.RX0IF = 0; // Clear the Interuppt Status bit
  }
  
  if(CXRX1CONbits.RXFUL) {
    /* 
       A message has been recieved in Buffer 1
       This command gets pushed onto the command message buffer
    */
    local_can_errors.rx_1_filt_2++;


    /*
    if ((CXRX1SID & ETM_CAN_MSG_MASTER_ADDR_MASK) == ETM_CAN_MSG_STATUS_RX)  {
      // The master is receiving a status update
      // We need to immediately update the fault and pulse inhibit information
      msg_address = CXRX1SID;
      msg_address >>= 3;
      msg_address &= 0x000F;
      msg_address = 1 << msg_address;
      if (CXRX1B1 & 0x0001)  {
	// This board is faulted set the fault bit for this board address
	if ((etm_can_ethernet_board_data.not_operate_bits & msg_address) == 0) {
	  // This is a NEW not operate bit
	  // There could already be an "enable" command in process that would overwrite this disable when the interrupt exits (possibly even before the disable message is sent)
	  etm_can_ethernet_board_data.not_operate_bits |= msg_address;
	  etm_can_ethernet_board_data.pulse_sync_disable_requested = 1;
	}
      } else {
	// Clear the fault bit for this board address
	etm_can_ethernet_board_data.not_operate_bits &= ~msg_address;
      }
    }
    */
    ETMCanRXMessageBuffer(&etm_can_rx_message_buffer, &CXRX1CON);
    CXINTFbits.RX1IF = 0; // Clear the Interuppt Status bit
  }
  
  if ((!CXTX0CONbits.TXREQ) && (ETMCanBufferNotEmpty(&etm_can_tx_message_buffer))) {
    /*
      TX0 is empty and there is a message waiting in the transmit message buffer
      Load the next message into TX0
    */
    ETMCanTXMessageBuffer(&etm_can_tx_message_buffer, &CXTX0CON);
    CXINTFbits.TX0IF = 0;
    local_can_errors.tx_0++;
  }
  
  if (CXINTFbits.ERRIF) {
    // There was some sort of CAN Error
    local_can_errors.error_flag++;
    CXINTFbits.ERRIF = 0;
  } else {
    // FLASH THE CAN LED
    if (ETMReadPinLatch(can_params.led)) {
      ETMClearPin(can_params.led);
    } else {
      ETMSetPin(can_params.led);
    }
  }

  // Record the max TX counter
  if ((CXEC & 0xFF00) > (local_can_errors.CXEC_reg & 0xFF00)) {
    local_can_errors.CXEC_reg &= 0x00FF;
    local_can_errors.CXEC_reg += (CXEC & 0xFF00);
  }

  // Record the max RX counter
  if ((CXEC & 0x00FF) > (local_can_errors.CXEC_reg & 0x00FF)) {
    local_can_errors.CXEC_reg &= 0xFF00;
    local_can_errors.CXEC_reg += (CXEC & 0x00FF);
  }

}



void ETMCanMasterCheckForTimeOut(void) {
  
  // Ion Pump Board
  if (board_status_received.ion_pump_board) {                           // The slave board is connected
    if (board_com_fault.ion_pump_board) {                                      // The slave board has regained communication
      SendToEventLog(LOG_ID_CONNECTED_ION_PUMP_BOARD);
    }
    board_com_fault.ion_pump_board = 0;
    //_ION_PUMP_NOT_CONNECTED = 0;
  }
  
  // Pulse Current Monitor Board
  if (board_status_received.magnetron_current_board) {                  // The slave board is connected
    if (board_com_fault.magnetron_current_board) {                                 // The slave board has regained communication
      SendToEventLog(LOG_ID_CONNECTED_MAGNETRON_CURRENT_BOARD);
    }
    board_com_fault.magnetron_current_board = 0;
    //_PULSE_CURRENT_NOT_CONNECTED = 0;
  }
  
  // Pulse Sync Board
  if (board_status_received.pulse_sync_board) {                         // The slave board is connected
    if (board_com_fault.pulse_sync_board) {                                    // The slave board has regained communication
      SendToEventLog(LOG_ID_CONNECTED_PULSE_SYNC_BOARD);
    }
    board_com_fault.pulse_sync_board = 0;
    //_PULSE_SYNC_NOT_CONNECTED = 0;
  }
  
  // HV Lambda Board
  if (board_status_received.hv_lambda_board) {                          // The slave board is connected
    if (board_com_fault.hv_lambda_board) {                                     // The slave board has regained communication
      SendToEventLog(LOG_ID_CONNECTED_HV_LAMBDA_BOARD);
    }
    board_com_fault.hv_lambda_board = 0;
    //_HV_LAMBDA_NOT_CONNECTED = 0;
  }
  
  // AFC Board
  if (board_status_received.afc_board) {                                // The slave board is connected
    if (board_com_fault.afc_board) {                                           // The slave board has regained communication
      SendToEventLog(LOG_ID_CONNECTED_AFC_BOARD);
    }
    board_com_fault.afc_board = 0;
    //_AFC_NOT_CONNECTED = 0;
  }
  
  // Cooling Interface
  if (board_status_received.cooling_interface_board) {                  // The slave board is connected
    if (board_com_fault.cooling_interface_board) {                                       // The slave board has regained communication
      SendToEventLog(LOG_ID_CONNECTED_COOLING_BOARD);
    }
    board_com_fault.cooling_interface_board = 0;
    //_COOLING_NOT_CONNECTED = 0;
  }
  
  // Heater Magnet Supply
  if (board_status_received.heater_magnet_board) {                      // The slave board is connected
    if (board_com_fault.heater_magnet_board) {                                 // The slave board has regained communication
      SendToEventLog(LOG_ID_CONNECTED_HEATER_MAGNET_BOARD);
    }
    board_com_fault.heater_magnet_board = 0;
    //_HEATER_MAGNET_NOT_CONNECTED = 0;
  }
  
  // Gun Driver
  if (board_status_received.gun_driver_board) {                         // The slave board is not connected
    if (board_com_fault.gun_driver_board) {                                    // The slave board has regained communication
      SendToEventLog(LOG_ID_CONNECTED_GUN_DRIVER_BOARD);
    }
    board_com_fault.gun_driver_board = 0;
    //_GUN_DRIVER_NOT_CONNECTED = 0;
  }
  
  if (_T5IF) {
    // At least one board failed to communicate durring the T5 period
    _T5IF = 0;
    local_can_errors.timeout++;
    etm_can_persistent_data.can_timeout_count = local_can_errors.timeout;
    _CONTROL_CAN_COM_LOSS = 1;


    // ---------------- Set the state _*_NOT_CONNECTED bit and log when the board disconnects and reconnects -------------- //

    // Ion Pump Board
    if (!board_status_received.ion_pump_board) {                          // The slave board is not connected
      global_data_A36507.no_connect_count_ion_pump_board++;
      if (!board_com_fault.ion_pump_board) {                              // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_ION_PUMP_BOARD);
      }
      board_com_fault.ion_pump_board = 1;
      //_ION_PUMP_NOT_CONNECTED = 1;
    }
    
    // Pulse Current Monitor Board
    if (!board_status_received.magnetron_current_board) {                 // The slave board is not connected
      global_data_A36507.no_connect_count_magnetron_current_board++;
      if (!board_com_fault.magnetron_current_board) {                                // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_MAGNETRON_CURRENT_BOARD);
      }
      board_com_fault.magnetron_current_board = 1;
      //_PULSE_CURRENT_NOT_CONNECTED = 1;
    }

    // Pulse Sync Board
    if (!board_status_received.pulse_sync_board) {                        // The slave board is not connected
      global_data_A36507.no_connect_count_pulse_sync_board++;
      if (!board_com_fault.pulse_sync_board) {                                   // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_PULSE_SYNC_BOARD);
      }
      board_com_fault.pulse_sync_board = 1;
      //_PULSE_SYNC_NOT_CONNECTED = 1;
    }

    // HV Lambda Board
    if (!board_status_received.hv_lambda_board) {                         // The slave board is not connected
      global_data_A36507.no_connect_count_hv_lambda_board++;
      if (!board_com_fault.hv_lambda_board) {                                    // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_HV_LAMBDA_BOARD);
      }
      board_com_fault.hv_lambda_board = 1;
      //_HV_LAMBDA_NOT_CONNECTED = 1;
    }

    // AFC Board
    if (!board_status_received.afc_board) {                               // The slave board is not connected
      global_data_A36507.no_connect_count_afc_board++;
      if (!board_com_fault.afc_board) {                                   // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_AFC_BOARD);
      }
      board_com_fault.afc_board = 1;
      //_AFC_NOT_CONNECTED = 1;
    }    

    // Cooling Interface
    if (!board_status_received.cooling_interface_board) {                 // The slave board is not connected
      global_data_A36507.no_connect_count_cooling_interface_board++;
      if (!board_com_fault.cooling_interface_board) {                                      // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_COOLING_BOARD);
      }
      board_com_fault.cooling_interface_board = 1;
      //_COOLING_NOT_CONNECTED = 1;
    }

    // Heater Magnet Supply
    if (!board_status_received.heater_magnet_board) {                     // The slave board is not connected
      global_data_A36507.no_connect_count_heater_magnet_board++;
      if (!board_com_fault.heater_magnet_board) {                                // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_HEATER_MAGNET_BOARD);
      }
      board_com_fault.heater_magnet_board = 1;
      //_HEATER_MAGNET_NOT_CONNECTED = 1;
    }

    // Gun Driver
    if (!board_status_received.gun_driver_board) {                        // The slave board is not connected
      global_data_A36507.no_connect_count_gun_driver_board++;
      if (!board_com_fault.gun_driver_board) {                                   // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_GUN_DRIVER_BOARD);
      }
      board_com_fault.gun_driver_board = 1;
      //_GUN_DRIVER_NOT_CONNECTED = 1;
    }

    // Clear the status received register
    *(unsigned int*)&board_status_received = 0x0000;

  }
}



void SendCalibrationSetPointToSlave(unsigned int index, unsigned int data_1, unsigned int data_0) {
  ETMCanMessage can_message;
  unsigned int board_id;
  board_id = index & 0xF000;
  board_id >>= 12;
  
  
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (board_id << 3));
  can_message.word3 = index;
  can_message.word2 = 0;
  can_message.word1 = data_1;
  can_message.word0 = data_0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()  
}

void ReadCalibrationSetPointFromSlave(unsigned int index) {
  ETMCanMessage can_message;
  unsigned int board_id;
  board_id = index & 0xF000;
  board_id >>= 12;
  
  can_message.identifier = (ETM_CAN_MSG_REQUEST_TX | (board_id << 3));
  can_message.word3 = index;
  can_message.word2 = 0;
  can_message.word1 = 0;
  can_message.word0 = 0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()  
}

void SendSlaveLoadDefaultEEpromData(unsigned int board_id) {
  ETMCanMessage can_message;
  board_id &= 0x000F;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (board_id << 3));
  can_message.word3 = (board_id << 12) + ETM_CAN_REGISTER_DEFAULT_CMD_RESET_ANALOG_CALIBRATION;
  can_message.word2 = 0;
  can_message.word1 = 0;
  can_message.word0 = 0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()  
}

void SendSlaveReset(unsigned int board_id) {
  ETMCanMessage can_message;
  board_id &= 0x000F;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (board_id << 3));
  can_message.word3 = (board_id << 12) + ETM_CAN_REGISTER_DEFAULT_CMD_RESET_MCU;
  can_message.word2 = 0;
  can_message.word1 = 0;
  can_message.word0 = 0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()  
}


void ETMCanMasterClearDebug(void) {
  etm_can_tx_message_buffer.message_overwrite_count = 0;
  etm_can_rx_message_buffer.message_overwrite_count = 0;
  etm_can_rx_data_log_buffer.message_overwrite_count = 0;
  etm_can_persistent_data.reset_count = 0;
  etm_can_persistent_data.can_timeout_count = 0;

  global_data_A36507.no_connect_count_ion_pump_board = 0;
  global_data_A36507.no_connect_count_magnetron_current_board = 0;
  global_data_A36507.no_connect_count_pulse_sync_board = 0;
  global_data_A36507.no_connect_count_hv_lambda_board = 0;
  global_data_A36507.no_connect_count_afc_board = 0;
  global_data_A36507.no_connect_count_cooling_interface_board = 0;
  global_data_A36507.no_connect_count_heater_magnet_board = 0;
  global_data_A36507.no_connect_count_gun_driver_board = 0;
 
  local_debug_data.i2c_bus_error_count = 0;
  local_debug_data.spi_bus_error_count = 0;
  local_debug_data.can_bus_error_count = 0;
  local_debug_data.scale_error_count   = 0;
  
  local_debug_data.reset_count         = 0;
  //
  local_debug_data.reserved_1          = 0;
  local_debug_data.reserved_0          = 0;

  local_debug_data.debug_0             = 0;
  local_debug_data.debug_1             = 0;
  local_debug_data.debug_2             = 0;
  local_debug_data.debug_3             = 0;

  local_debug_data.debug_4             = 0;
  local_debug_data.debug_5             = 0;
  local_debug_data.debug_6             = 0;
  local_debug_data.debug_7             = 0;

  local_debug_data.debug_8             = 0;
  local_debug_data.debug_9             = 0;
  local_debug_data.debug_A             = 0;
  local_debug_data.debug_B             = 0;

  local_debug_data.debug_C             = 0;
  local_debug_data.debug_D             = 0;
  local_debug_data.debug_E             = 0;
  local_debug_data.debug_F             = 0;

  local_can_errors.CXEC_reg            = 0;
  local_can_errors.error_flag          = 0;
  local_can_errors.tx_1                = 0;
  local_can_errors.tx_2                = 0;
  
  local_can_errors.rx_0_filt_0         = 0;
  local_can_errors.rx_0_filt_1         = 0;
  local_can_errors.rx_1_filt_2         = 0;
  local_can_errors.isr_entered         = 0;

  local_can_errors.unknown_message_identifier  = 0;
  local_can_errors.invalid_index       = 0;
  local_can_errors.address_error       = 0;
  local_can_errors.tx_0                = 0;

  local_can_errors.message_tx_buffer_overflow  = 0;
  local_can_errors.message_rx_buffer_overflow  = 0;
  local_can_errors.data_log_rx_buffer_overflow = 0;
  local_can_errors.timeout             = 0;


  global_data_A36507.no_connect_count_ion_pump_board = 0;
  global_data_A36507.no_connect_count_magnetron_current_board = 0;
  global_data_A36507.no_connect_count_pulse_sync_board = 0;
  global_data_A36507.no_connect_count_hv_lambda_board = 0;
  global_data_A36507.no_connect_count_afc_board = 0;
  global_data_A36507.no_connect_count_cooling_interface_board = 0;
  global_data_A36507.no_connect_count_heater_magnet_board = 0;
  global_data_A36507.no_connect_count_gun_driver_board = 0;

  CXINTF = 0;
  _TRAPR = 0;
  _IOPUWR = 0;
  _EXTR = 0;
  _WDTO = 0;
  _SLEEP = 0;
  _IDLE = 0;
  _BOR = 0;
  _POR = 0;
  _SWR = 0;

}



void SendToEventLog(unsigned int log_id) {
  event_log.event_data[event_log.write_index].event_number = global_data_A36507.event_log_counter;
  event_log.event_data[event_log.write_index].event_time   = global_data_A36507.time_seconds_now;
  event_log.event_data[event_log.write_index].event_id     = log_id;
  event_log.write_index++;
  event_log.write_index &= 0x7F;
  global_data_A36507.event_log_counter++;
  // DPARKER need to check the EEPROM and TCP locations and advance them as nesseasry so that we don't pass them when advancing the write_index
}
