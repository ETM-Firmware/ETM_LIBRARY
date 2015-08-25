#include <xc.h>
#include <timer.h>
#include "P1395_CAN_MASTER.h"
#include "ETM_IO_PORTS.H"
#include "ETM_SCALE.H"
//#include "A36507.h"

// DPARKER fix these
extern unsigned int SendCalibrationData(unsigned int index, unsigned int scale, unsigned int offset);
unsigned int etm_can_active_debugging_board_id;

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


typedef struct {
  unsigned int no_connect_count_ion_pump_board;
  unsigned int no_connect_count_magnetron_current_board;
  unsigned int no_connect_count_pulse_sync_board;
  unsigned int no_connect_count_hv_lambda_board;
  unsigned int no_connect_count_afc_board;
  unsigned int no_connect_count_cooling_interface_board;
  unsigned int no_connect_count_heater_magnet_board;
  unsigned int no_connect_count_gun_driver_board;
  
  unsigned int event_log_counter;
  unsigned int time_seconds_now;
  unsigned int millisecond_counter;
  
  unsigned int buffer_a_ready_to_send;
  unsigned int buffer_a_sent;
  
  unsigned int buffer_b_ready_to_send;
  unsigned int buffer_b_sent;
} TYPE_GLOBAL_DATA_CAN_MASTER;

TYPE_GLOBAL_DATA_CAN_MASTER global_data_can_master;

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


// ---------- Pointers to CAN stucutres so that we can use CAN1 or CAN2
volatile unsigned int *CXEC_ptr;
volatile unsigned int *CXINTF_ptr;
volatile unsigned int *CXRX0CON_ptr;
volatile unsigned int *CXRX1CON_ptr;
volatile unsigned int *CXTX0CON_ptr;
volatile unsigned int *CXTX1CON_ptr;
volatile unsigned int *CXTX2CON_ptr;


// --------- Ram Structures that store the module status ---------- //
ETMCanBoardData local_data_ecb;
ETMCanBoardData mirror_hv_lambda;
ETMCanBoardData mirror_ion_pump;
ETMCanBoardData mirror_afc;
ETMCanBoardData mirror_cooling;
ETMCanBoardData mirror_htr_mag;
ETMCanBoardData mirror_gun_drv;
ETMCanBoardData mirror_magnetron_mon;
ETMCanBoardData mirror_pulse_sync;


// ---------------- CAN Message Defines ---------------------- //
ETMCanSyncMessage                etm_can_sync_message;                // This is the sync message that the ECB sends out, only word zero ande one are used at this time




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
void ETMCanMasterSendSync();                          // This gets sent out 1 time every 50ms
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

void ETMCanMasterDataReturnFromSlave(ETMCanMessage* message_ptr);
/*
  This processes Return Commands (From slave board).
  This is used to return EEprom Data
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


void ETMCanMasterInitialize(unsigned int requested_can_port, unsigned long fcy, unsigned int etm_can_address, unsigned long can_operation_led, unsigned int can_interrupt_priority) {
  unsigned long timer_period_value;

  if (can_interrupt_priority > 7) {
    can_interrupt_priority = 7;
  }

  can_params.address = etm_can_address;
  can_params.led = can_operation_led;

  etm_can_persistent_data.reset_count++;
  
  _SYNC_CONTROL_WORD = 0;
  etm_can_sync_message.sync_1_ecb_state_for_fault_logic = 0;
  etm_can_sync_message.sync_2 = 0;
  etm_can_sync_message.sync_3 = 0;
  
  debug_data_ecb.reset_count = etm_can_persistent_data.reset_count;
  debug_data_ecb.can_timeout = etm_can_persistent_data.can_timeout_count;

  ETMCanBufferInitialize(&etm_can_rx_message_buffer);
  ETMCanBufferInitialize(&etm_can_tx_message_buffer);
  ETMCanBufferInitialize(&etm_can_rx_data_log_buffer);


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

  ETMPinTrisOutput(can_params.led);
  

  if (requested_can_port != CAN_PORT_2) {
    // Use CAN1
    
    CXEC_ptr     = &C1EC;
    CXINTF_ptr   = &C1INTF;
    CXRX0CON_ptr = &C1RX0CON;
    CXRX1CON_ptr = &C1RX1CON;
    CXTX0CON_ptr = &C1TX0CON;
    CXTX1CON_ptr = &C1TX1CON;
    CXTX2CON_ptr = &C1TX2CON;
    
    
    _C1IE = 0;
    _C1IF = 0;
    _C1IP = can_interrupt_priority;
    
    C1INTF = 0;
    
    C1INTEbits.RX0IE = 1; // Enable RXB0 interrupt
    C1INTEbits.RX1IE = 1; // Enable RXB1 interrupt
    C1INTEbits.TX0IE = 1; // Enable TXB0 interrupt
    C1INTEbits.ERRIE = 1; // Enable Error interrupt

  
  
    // ---------------- Set up CAN Control Registers ---------------- //
  
    // Set Baud Rate
    C1CTRL = CXCTRL_CONFIG_MODE_VALUE;
    while(C1CTRLbits.OPMODE != 4);
  
    if (fcy == 25000000) {
      C1CFG1 = CXCFG1_25MHZ_FCY_VALUE;    
    } else if (fcy == 20000000) {
      C1CFG1 = CXCFG1_20MHZ_FCY_VALUE;    
    } else if (fcy == 10000000) {
      C1CFG1 = CXCFG1_10MHZ_FCY_VALUE;    
    } else {
      // If you got here we can't configure the can module
      // Try to set to operate in 10MHZ mode.
      // Can probably won't talk
      C1CFG1 = CXCFG1_10MHZ_FCY_VALUE;    
    }
    
    C1CFG2 = CXCFG2_VALUE;
    
    // Load Mask registers for RX0 and RX1
    C1RXM0SID = ETM_CAN_MASTER_RX0_MASK;
    C1RXM1SID = ETM_CAN_MASTER_RX1_MASK;
    
    // Load Filter registers
    C1RXF0SID = ETM_CAN_MASTER_MSG_FILTER_RF0;
    C1RXF1SID = ETM_CAN_MASTER_MSG_FILTER_RF1;
    C1RXF2SID = ETM_CAN_MASTER_MSG_FILTER_RF2;
    //C1RXF3SID = ETM_CAN_MSG_FILTER_OFF;
    //C1RXF4SID = ETM_CAN_MSG_FILTER_OFF;
    //C1RXF5SID = ETM_CAN_MSG_FILTER_OFF;

    // Set Transmitter Mode
    C1TX0CON = CXTXXCON_VALUE_LOW_PRIORITY;
    C1TX1CON = CXTXXCON_VALUE_MEDIUM_PRIORITY;
    C1TX2CON = CXTXXCON_VALUE_HIGH_PRIORITY;
    
    C1TX0DLC = CXTXXDLC_VALUE;
    C1TX1DLC = CXTXXDLC_VALUE;
    C1TX2DLC = CXTXXDLC_VALUE;

    // Set Receiver Mode
    C1RX0CON = CXRXXCON_VALUE;
    C1RX1CON = CXRXXCON_VALUE;
  
    // Switch to normal operation
    C1CTRL = CXCTRL_OPERATE_MODE_VALUE;
    while(C1CTRLbits.OPMODE != 0);
    
    //etm_can_ethernet_board_data.status_received_register = 0x0000;
    
    // Enable Can interrupt
    _C1IE = 1;
  } else {
    // Use CAN2
  }
}


void ETMCanMasterLoadConfiguration(unsigned long agile_id, unsigned int agile_dash, unsigned int agile_rev, unsigned int firmware_agile_rev, unsigned int firmware_branch, unsigned int firmware_branch_rev, unsigned int serial_number) {
  
  config_agile_number_low_word = (agile_id & 0xFFFF);
  agile_id >>= 16;
  config_agile_number_high_word = agile_id;
  config_agile_dash = agile_dash;
  config_agile_rev_ascii = agile_rev;

  config_firmware_agile_rev = firmware_agile_rev;
  config_firmware_branch = firmware_branch;
  config_firmware_branch_rev= firmware_branch_rev;
  config_serial_number = serial_number;
}


void ETMCanMasterDoCan(void) {
  ETMCanMasterProcessMessage();
  ETMCanMasterTimedTransmit();
  ETMCanMasterProcessLogData();
  ETMCanMasterCheckForTimeOut();
  if (_SYNC_CONTROL_CLEAR_DEBUG_DATA) {
    ETMCanMasterClearDebug();
  }


  // Log Debugging Information
  // Record the RCON state
  debug_data_ecb.RCON_value = RCON;

  // Record the max TX counter
  if ((*CXEC_ptr & 0xFF00) > (debug_data_ecb.CXEC_reg_max & 0xFF00)) {
    debug_data_ecb.CXEC_reg_max &= 0x00FF;
    debug_data_ecb.CXEC_reg_max += (*CXEC_ptr & 0xFF00);
  }

  // Record the max RX counter
  if ((*CXEC_ptr & 0x00FF) > (debug_data_ecb.CXEC_reg_max & 0x00FF)) {
    debug_data_ecb.CXEC_reg_max &= 0xFF00;
    debug_data_ecb.CXEC_reg_max += (*CXEC_ptr & 0x00FF);
  }
}


void ETMCanMasterProcessMessage(void) {
  ETMCanMessage next_message;
  while (ETMCanBufferNotEmpty(&etm_can_rx_message_buffer)) {
    ETMCanReadMessageFromBuffer(&etm_can_rx_message_buffer, &next_message);
    if ((next_message.identifier & ETM_CAN_MASTER_RX0_MASK) == ETM_CAN_MSG_RTN_RX) {
      ETMCanMasterDataReturnFromSlave(&next_message);
    } else if ((next_message.identifier & ETM_CAN_MASTER_RX0_MASK) == ETM_CAN_MSG_STATUS_RX) {
      ETMCanMasterUpdateSlaveStatus(&next_message);
    } else {
      debug_data_ecb.can_unknown_msg_id++;
    } 
  }
  
  debug_data_ecb.can_tx_buf_overflow = etm_can_tx_message_buffer.message_overwrite_count;
  debug_data_ecb.can_rx_buf_overflow = etm_can_rx_message_buffer.message_overwrite_count;
  debug_data_ecb.can_rx_log_buf_overflow = etm_can_rx_data_log_buffer.message_overwrite_count;
}



// DPAKRER  - Need to evaluate how these are used under new control system
#define _STATUS_X_RAY_DISABLED     local_data_ecb.status.control_notice_bits.control_not_ready
#define _STATUS_PERSONALITY_LOADED local_data_ecb.status.control_notice_bits.control_not_configured


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
  sync_message.word2 = etm_can_sync_message.sync_1_ecb_state_for_fault_logic; // DPARKER update with the current state or point to the current state
  sync_message.word2 = etm_can_sync_message.sync_2;
  sync_message.word3 = etm_can_sync_message.sync_3;
  
  ETMCanTXMessage(&sync_message, CXTX1CON_ptr);
  debug_data_ecb.can_tx_1++;

  _STATUS_X_RAY_DISABLED = _SYNC_CONTROL_PULSE_SYNC_DISABLE_XRAY;
}

void ETMCanMasterHVLambdaUpdateOutput(void) {
  ETMCanMessage can_message;
  
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_HV_LAMBDA_BOARD << 2));
  can_message.word3 = ETM_CAN_REGISTER_HV_LAMBDA_SET_1_LAMBDA_SET_POINT;
  can_message.word2 = local_hv_lambda_low_en_set_point;
  can_message.word1 = local_hv_lambda_high_en_set_point;
  can_message.word0 = 0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterAFCUpdateHomeOffset(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_AFC_CONTROL_BOARD << 2));
  can_message.word3 = ETM_CAN_REGISTER_AFC_SET_1_HOME_POSITION_AND_OFFSET;
  can_message.word2 = local_afc_aft_control_voltage;
  can_message.word1 = 0;
  can_message.word0 = local_afc_home_position;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterHtrMagnetUpdateOutput(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_HEATER_MAGNET_BOARD << 2));
  can_message.word3 = ETM_CAN_REGISTER_HEATER_MAGNET_SET_1_CURRENT_SET_POINT;
  can_message.word2 = 0;
  can_message.word1 = local_heater_current_scaled_set_point;
  can_message.word0 = local_magnet_current_set_point;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterGunDriverUpdatePulseTop(void) {
  ETMCanMessage can_message;
    can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_GUN_DRIVER_BOARD << 2));
  can_message.word3 = ETM_CAN_REGISTER_GUN_DRIVER_SET_1_GRID_TOP_SET_POINT;
  can_message.word2 = 0;
  can_message.word1 = local_gun_drv_high_en_pulse_top_v;
  can_message.word0 = local_gun_drv_low_en_pulse_top_v;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterGunDriverUpdateHeaterCathode(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_GUN_DRIVER_BOARD << 2));
  can_message.word3 = ETM_CAN_REGISTER_GUN_DRIVER_SET_1_HEATER_CATHODE_SET_POINT;
  can_message.word2 = 0;
  can_message.word1 = local_gun_drv_cathode_set_point;
  can_message.word0 = local_gun_drv_heater_v_set_point;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterPulseSyncUpdateHighRegZero(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_PULSE_SYNC_BOARD << 2));
  can_message.word3 = ETM_CAN_REGISTER_PULSE_SYNC_SET_1_HIGH_ENERGY_TIMING_REG_0;
  can_message.word2 = local_pulse_sync_timing_reg_0_word_2;
  can_message.word1 = local_pulse_sync_timing_reg_0_word_1;
  can_message.word0 = local_pulse_sync_timing_reg_0_word_0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterPulseSyncUpdateHighRegOne(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_PULSE_SYNC_BOARD << 2));
  can_message.word3 = ETM_CAN_REGISTER_PULSE_SYNC_SET_1_HIGH_ENERGY_TIMING_REG_1;
  can_message.word2 = local_pulse_sync_timing_reg_1_word_2;
  can_message.word1 = local_pulse_sync_timing_reg_1_word_1;
  can_message.word0 = local_pulse_sync_timing_reg_1_word_0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterPulseSyncUpdateLowRegZero(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_PULSE_SYNC_BOARD << 2));
  can_message.word3 = ETM_CAN_REGISTER_PULSE_SYNC_SET_1_LOW_ENERGY_TIMING_REG_0;
  can_message.word2 = local_pulse_sync_timing_reg_2_word_2;
  can_message.word1 = local_pulse_sync_timing_reg_2_word_1;
  can_message.word0 = local_pulse_sync_timing_reg_2_word_0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

void ETMCanMasterPulseSyncUpdateLowRegOne(void) {
  ETMCanMessage can_message;
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (ETM_CAN_ADDR_PULSE_SYNC_BOARD << 2));
  can_message.word3 = ETM_CAN_REGISTER_PULSE_SYNC_SET_1_LOW_ENERGY_TIMING_REG_1;
  can_message.word2 = local_pulse_sync_timing_reg_3_word_2;
  can_message.word1 = local_pulse_sync_timing_reg_3_word_1;
  can_message.word0 = local_pulse_sync_timing_reg_3_word_0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}





void ETMCanMasterDataReturnFromSlave(ETMCanMessage* message_ptr) {
  unsigned int index_word;
  index_word = message_ptr->word3 & 0x0FFF;
  
  if ((index_word >= 0x0100) & (index_word < 0x0200)) {
    // This is Calibration data that was read from the slave EEPROM
    SendCalibrationData(message_ptr->word3, message_ptr->word1, message_ptr->word0);
    // DPARKER SendCalibrationData Sends data to the GUI over TCP/IP.  Could use a better name
  } else {
    // It was not a set value index 
    debug_data_ecb.can_invalid_index++;
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
  
  status_message.control_notice_bits = *(ETMCanStatusRegisterControlAndNoticeBits*)&message_ptr->word0;
  status_message.fault_bits          = *(ETMCanStatusRegisterFaultBits*)&message_ptr->word1;
  status_message.warning_bits        = *(ETMCanStatusRegisterWarningBits*)&message_ptr->word2;
  status_message.not_logged_bits     = *(ETMCanStatusRegisterNotLoggedBits*)&message_ptr->word3;
  ClrWdt();

  switch (source_board) {
    /*
      Place all board specific status updates here
    */

  case ETM_CAN_ADDR_ION_PUMP_BOARD:
    mirror_ion_pump.status = status_message;
    board_status_received.ion_pump_board = 1;
    break;

  case ETM_CAN_ADDR_MAGNETRON_CURRENT_BOARD:
    mirror_magnetron_mon.status = status_message;
    board_status_received.magnetron_current_board = 1;
    break;

  case ETM_CAN_ADDR_PULSE_SYNC_BOARD:
    mirror_pulse_sync.status = status_message;
    board_status_received.pulse_sync_board = 1;
    break;

  case ETM_CAN_ADDR_HV_LAMBDA_BOARD:
    mirror_hv_lambda.status = status_message;
    board_status_received.hv_lambda_board = 1;
    break;

  case ETM_CAN_ADDR_AFC_CONTROL_BOARD:
    mirror_afc.status = status_message;
    board_status_received.afc_board = 1;
    break;
    
  case ETM_CAN_ADDR_COOLING_INTERFACE_BOARD:
    mirror_cooling.status = status_message;
    board_status_received.cooling_interface_board = 1;
    break;

  case ETM_CAN_ADDR_HEATER_MAGNET_BOARD:
    mirror_htr_mag.status = status_message;
    board_status_received.heater_magnet_board = 1;
    break;

  case ETM_CAN_ADDR_GUN_DRIVER_BOARD:
    mirror_gun_drv.status = status_message;
    board_status_received.gun_driver_board = 1;
    break;


  default:
    debug_data_ecb.can_address_error++;
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
  unsigned int           board_id;
  unsigned int           log_id;

  ETMCanBoardData*       board_data_ptr;

  unsigned int           fast_log_buffer_index;
  ETMCanHighSpeedData*   ptr_high_speed_data;


  while (ETMCanBufferNotEmpty(&etm_can_rx_data_log_buffer)) {
    ETMCanReadMessageFromBuffer(&etm_can_rx_data_log_buffer, &next_message);
    data_log_index = next_message.identifier;
    data_log_index >>= 2;
    data_log_index &= 0x03FF;
    board_id = data_log_index & 0x000F;
    log_id = data_log_index & 0x03F0;



    if (log_id <= 0x03F) {
      // It is high speed logging data that must be handled manually
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
 	case ETM_CAN_DATA_LOG_REGISTER_HV_LAMBDA_FAST_LOG_0:
	  // Update the high speed data table
	  ptr_high_speed_data->hvlambda_readback_high_energy_lambda_program_voltage = next_message.word2;
	  ptr_high_speed_data->hvlambda_readback_low_energy_lambda_program_voltage = next_message.word1;
	  ptr_high_speed_data->hvlambda_readback_peak_lambda_voltage = next_message.word0;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_AFC_FAST_LOG_0:
	  ptr_high_speed_data->afc_readback_current_position = next_message.word2;
	  ptr_high_speed_data->afc_readback_target_position = next_message.word1;
	  // unused word 0
	  break;
	  
	case ETM_CAN_DATA_LOG_REGISTER_AFC_FAST_LOG_1:
	  ptr_high_speed_data->afc_readback_a_input = next_message.word2;
	  ptr_high_speed_data->afc_readback_b_input = next_message.word1;
	  ptr_high_speed_data->afc_readback_filtered_error_reading = next_message.word0;
	  break;
	  
	case ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_FAST_LOG_0:
	  ptr_high_speed_data->magmon_readback_magnetron_low_energy_current = next_message.word2;  // Internal DAC Reading
	  ptr_high_speed_data->magmon_readback_magnetron_high_energy_current = next_message.word1; // External DAC Reading
	  if (next_message.word0) {
	    ptr_high_speed_data->status_bits.arc_this_pulse = 1;
	  }
	  break;
	  
	case ETM_CAN_DATA_LOG_REGISTER_PULSE_SYNC_FAST_LOG_0:
	  ptr_high_speed_data->psync_readback_trigger_width_and_filtered_trigger_width = next_message.word2;
	  ptr_high_speed_data->psync_readback_high_energy_grid_width_and_delay = next_message.word1;
	  ptr_high_speed_data->psync_readback_low_energy_grid_width_and_delay = next_message.word0;
	  break;
	  
	default:
	  debug_data_ecb.can_unknown_msg_id++;
	  break;
	}
    } else if (log_id >= 0x100) {
      // It is debugging information, load into the common debugging register if that board is actively being debugged
      // DPARKER impliment this using used debug_data_slave_mirror
      if (board_id == etm_can_active_debugging_board_id) {
	switch (log_id) 
	  {
	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_0:
	    debug_data_slave_mirror.debug_0                 = next_message.word3;
	    debug_data_slave_mirror.debug_1                 = next_message.word2;
	    debug_data_slave_mirror.debug_2                 = next_message.word1;
	    debug_data_slave_mirror.debug_3                 = next_message.word0;
	    break;
	    
	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_1:
	    debug_data_slave_mirror.debug_4                 = next_message.word3;
	    debug_data_slave_mirror.debug_5                 = next_message.word2;
	    debug_data_slave_mirror.debug_6                 = next_message.word1;
	    debug_data_slave_mirror.debug_7                 = next_message.word0;
	    break;

	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_2:
	    debug_data_slave_mirror.debug_8                 = next_message.word3;
	    debug_data_slave_mirror.debug_9                 = next_message.word2;
	    debug_data_slave_mirror.debug_A                 = next_message.word1;
	    debug_data_slave_mirror.debug_B                 = next_message.word0;
	    break;

	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_3:
	    debug_data_slave_mirror.debug_C                 = next_message.word3;
	    debug_data_slave_mirror.debug_D                 = next_message.word2;
	    debug_data_slave_mirror.debug_E                 = next_message.word1;
	    debug_data_slave_mirror.debug_F                 = next_message.word0;
	    break;

	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_0:
	    debug_data_slave_mirror.can_tx_0                = next_message.word3;
	    debug_data_slave_mirror.can_tx_1                = next_message.word2;
	    debug_data_slave_mirror.can_tx_2                = next_message.word1;
	    debug_data_slave_mirror.CXEC_reg_max            = next_message.word0;
	    break;

	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_1:
	    debug_data_slave_mirror.can_rx_0_filt_0         = next_message.word3;
	    debug_data_slave_mirror.can_rx_0_filt_1         = next_message.word2;
	    debug_data_slave_mirror.can_rx_1_filt_2         = next_message.word1;
	    debug_data_slave_mirror.CXINTF_max              = next_message.word0;
	    break;

	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_2:
	    debug_data_slave_mirror.can_unknown_msg_id      = next_message.word3;
	    debug_data_slave_mirror.can_invalid_index       = next_message.word2;
	    debug_data_slave_mirror.can_address_error       = next_message.word1;
	    debug_data_slave_mirror.can_error_flag          = next_message.word0;
	    break;

	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_3:
	    debug_data_slave_mirror.can_tx_buf_overflow     = next_message.word3;
	    debug_data_slave_mirror.can_rx_buf_overflow     = next_message.word2;
	    debug_data_slave_mirror.can_rx_log_buf_overflow = next_message.word1;
	    debug_data_slave_mirror.can_timeout             = next_message.word0;
	    break;

	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_SYSTEM_ERROR_0:
	    debug_data_slave_mirror.reset_count             = next_message.word3;
	    debug_data_slave_mirror.RCON_value              = next_message.word2;
	    debug_data_slave_mirror.reserved_1              = next_message.word1;
	    debug_data_slave_mirror.reserved_0              = next_message.word0;
	    break;

	  case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_SYSTEM_ERROR_1:
	    debug_data_slave_mirror.i2c_bus_error_count     = next_message.word3;
	    debug_data_slave_mirror.spi_bus_error_count     = next_message.word2;
	    debug_data_slave_mirror.scale_error_count       = next_message.word1;
	    debug_data_slave_mirror.self_test_results       = *(ETMCanSelfTestRegister*)&next_message.word0;
	    break;
	    
	  default:
	    debug_data_ecb.can_unknown_msg_id++;
	    break;
	  }
      }
    } else {
      // It is data that needs to be stored for a specific board. 

      // First figure out which board the data is from
      switch (board_id) 
	{
	case ETM_CAN_ADDR_ION_PUMP_BOARD:
	  board_data_ptr = &mirror_ion_pump;
	  break;
	  
	case ETM_CAN_ADDR_MAGNETRON_CURRENT_BOARD:
	  board_data_ptr = &mirror_magnetron_mon;
	  break;

	case ETM_CAN_ADDR_PULSE_SYNC_BOARD:
	  board_data_ptr = &mirror_pulse_sync;
	  break;
	  
	case ETM_CAN_ADDR_HV_LAMBDA_BOARD:
	  board_data_ptr = &mirror_hv_lambda;
	  break;
	  
	case ETM_CAN_ADDR_AFC_CONTROL_BOARD:
	  board_data_ptr = &mirror_afc;
	  break;

	case ETM_CAN_ADDR_COOLING_INTERFACE_BOARD:
	  board_data_ptr = &mirror_cooling;
	  break;
	  
	case ETM_CAN_ADDR_HEATER_MAGNET_BOARD:
	  board_data_ptr = &mirror_htr_mag;
	  break;

	case ETM_CAN_ADDR_GUN_DRIVER_BOARD:
	  board_data_ptr = &mirror_gun_drv;
	  break;

	default:
	  debug_data_ecb.can_address_error++;
	  break;
	  
	}
      
      // Now figure out which data log it is
      switch (log_id)
	{
	case ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_0:
	  board_data_ptr->board_data[0]  = next_message.word0;
	  board_data_ptr->board_data[1]  = next_message.word1;
	  board_data_ptr->board_data[2]  = next_message.word2;
	  board_data_ptr->board_data[3]  = next_message.word3;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_1:
	  board_data_ptr->board_data[4]  = next_message.word0;
	  board_data_ptr->board_data[5]  = next_message.word1;
	  board_data_ptr->board_data[6]  = next_message.word2;
	  board_data_ptr->board_data[7]  = next_message.word3;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_2:
	  board_data_ptr->board_data[8]  = next_message.word0;
	  board_data_ptr->board_data[9]  = next_message.word1;
	  board_data_ptr->board_data[10] = next_message.word2;
	  board_data_ptr->board_data[11] = next_message.word3;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_3:
	  board_data_ptr->board_data[12] = next_message.word0;
	  board_data_ptr->board_data[13] = next_message.word1;
	  board_data_ptr->board_data[14] = next_message.word2;
	  board_data_ptr->board_data[15] = next_message.word3;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_4:
	  board_data_ptr->board_data[16] = next_message.word0;
	  board_data_ptr->board_data[17] = next_message.word1;
	  board_data_ptr->board_data[18] = next_message.word2;
	  board_data_ptr->board_data[19] = next_message.word3;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_5:
	  board_data_ptr->board_data[20] = next_message.word0;
	  board_data_ptr->board_data[21] = next_message.word1;
	  board_data_ptr->board_data[22] = next_message.word2;
	  board_data_ptr->board_data[23] = next_message.word3;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_0:
	  board_data_ptr->board_data[24] = next_message.word0;
	  board_data_ptr->board_data[25] = next_message.word1;
	  board_data_ptr->board_data[26] = next_message.word2;
	  board_data_ptr->board_data[27] = next_message.word3;
	  break;

	case ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_1:
	  board_data_ptr->board_data[28] = next_message.word0;
	  board_data_ptr->board_data[29] = next_message.word1;
	  board_data_ptr->board_data[30] = next_message.word2;
	  board_data_ptr->board_data[31] = next_message.word3;
	  break;
	  
	default:
	  debug_data_ecb.can_unknown_msg_id++;
	  break;
	} 
    }
  }
}


void ETMCanMasterCheckForTimeOut(void) {
  
  // Check to see if a faulted board has regained communication.  If so, clear the fault bit and write to event log

  // Ion Pump Board
  if (board_status_received.ion_pump_board && board_com_fault.ion_pump_board) {
    // The slave board has regained communication
    SendToEventLog(LOG_ID_CONNECTED_ION_PUMP_BOARD);
    board_com_fault.ion_pump_board = 0;
  }
  
  // Pulse Current Monitor Board
  if (board_status_received.magnetron_current_board && board_com_fault.magnetron_current_board) {
    // The slave board has regained communication
    SendToEventLog(LOG_ID_CONNECTED_MAGNETRON_CURRENT_BOARD);
    board_com_fault.magnetron_current_board = 0;
  }
  
  // Pulse Sync Board
  if (board_status_received.pulse_sync_board && board_com_fault.pulse_sync_board) {
    // The slave board has regained communication
    SendToEventLog(LOG_ID_CONNECTED_PULSE_SYNC_BOARD);
    board_com_fault.pulse_sync_board = 0;
  }
  
  // HV Lambda Board
  if (board_status_received.hv_lambda_board && board_com_fault.hv_lambda_board) {
    SendToEventLog(LOG_ID_CONNECTED_HV_LAMBDA_BOARD);
    board_com_fault.hv_lambda_board = 0;
  }
  
  // AFC Board
  if (board_status_received.afc_board  && board_com_fault.afc_board) {
    SendToEventLog(LOG_ID_CONNECTED_AFC_BOARD);
    board_com_fault.afc_board = 0;
  }
  
  // Cooling Interface
  if (board_status_received.cooling_interface_board && board_com_fault.cooling_interface_board) {
    SendToEventLog(LOG_ID_CONNECTED_COOLING_BOARD);
    board_com_fault.cooling_interface_board = 0;
  }
  
  // Heater Magnet Supply
  if (board_status_received.heater_magnet_board && board_com_fault.heater_magnet_board) {
    SendToEventLog(LOG_ID_CONNECTED_HEATER_MAGNET_BOARD);
    board_com_fault.heater_magnet_board = 0;
  }
  
  // Gun Driver
  if (board_status_received.gun_driver_board && board_com_fault.gun_driver_board) {
    SendToEventLog(LOG_ID_CONNECTED_GUN_DRIVER_BOARD);
    board_com_fault.gun_driver_board = 0;
  }
  
  if (_T5IF) {
    // At least one board failed to communicate durring the T5 period
    _T5IF = 0;
    debug_data_ecb.can_timeout++;
    etm_can_persistent_data.can_timeout_count = debug_data_ecb.can_timeout;
    // _CONTROL_CAN_COM_LOSS = 1; // DPARKER change this to a fault
    
    // store which boards are not connect and write to the event log
    
    // Ion Pump Board
#ifndef __IGNORE_ION_PUMP_MODULE
    if (!board_status_received.ion_pump_board) {                          // The slave board is not connected
      global_data_can_master.no_connect_count_ion_pump_board++;
      if (!board_com_fault.ion_pump_board) {                              // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_ION_PUMP_BOARD);
      }
      board_com_fault.ion_pump_board = 1;
    }
#endif
    
    // Pulse Current Monitor Board
#ifndef __IGNORE_PULSE_CURRENT_MODULE
    if (!board_status_received.magnetron_current_board) {                 // The slave board is not connected
      global_data_can_master.no_connect_count_magnetron_current_board++;
      if (!board_com_fault.magnetron_current_board) {                     // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_MAGNETRON_CURRENT_BOARD);
      }
      board_com_fault.magnetron_current_board = 1;
    }
#endif
    

    // Pulse Sync Board
#ifndef __IGNORE_PULSE_SYNC_MODULE
    if (!board_status_received.pulse_sync_board) {                        // The slave board is not connected
      global_data_can_master.no_connect_count_pulse_sync_board++;
      if (!board_com_fault.pulse_sync_board) {                            // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_PULSE_SYNC_BOARD);
      }
      board_com_fault.pulse_sync_board = 1;
    }
#endif

    // HV Lambda Board
#ifndef __IGNORE_HV_LAMBDA_MODULE
    if (!board_status_received.hv_lambda_board) {                         // The slave board is not connected
      global_data_can_master.no_connect_count_hv_lambda_board++;
      if (!board_com_fault.hv_lambda_board) {                             // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_HV_LAMBDA_BOARD);
      }
      board_com_fault.hv_lambda_board = 1;
    }
#endif

    // AFC Board
#ifndef __IGNORE_AFC_MODULE
    if (!board_status_received.afc_board) {                               // The slave board is not connected
      global_data_can_master.no_connect_count_afc_board++;
      if (!board_com_fault.afc_board) {                                   // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_AFC_BOARD);
      }
      board_com_fault.afc_board = 1;
    }    
#endif

    // Cooling Interface
#ifndef __IGNORE_COOLING_INTERFACE_MODULE
    if (!board_status_received.cooling_interface_board) {                 // The slave board is not connected
      global_data_can_master.no_connect_count_cooling_interface_board++;
      if (!board_com_fault.cooling_interface_board) {                     // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_COOLING_BOARD);
      }
      board_com_fault.cooling_interface_board = 1;
    }
#endif

    // Heater Magnet Supply
#ifndef __IGNORE_HEATER_MAGNET_MODULE
    if (!board_status_received.heater_magnet_board) {                     // The slave board is not connected
      global_data_can_master.no_connect_count_heater_magnet_board++;
      if (!board_com_fault.heater_magnet_board) {                         // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_HEATER_MAGNET_BOARD);
      }
      board_com_fault.heater_magnet_board = 1;
    }
#endif

    // Gun Driver
#ifndef __IGNORE_GUN_DRIVER_MODULE
    if (!board_status_received.gun_driver_board) {                        // The slave board is not connected
      global_data_can_master.no_connect_count_gun_driver_board++;
      if (!board_com_fault.gun_driver_board) {                            // The slave board has lost communication
	SendToEventLog(LOG_ID_NOT_CONNECTED_GUN_DRIVER_BOARD);
      }
      board_com_fault.gun_driver_board = 1;
    }
#endif

    // Clear the status received register
    *(unsigned int*)&board_status_received = 0x0000;

  }
}



void SendCalibrationSetPointToSlave(unsigned int index, unsigned int data_1, unsigned int data_0) {
  ETMCanMessage can_message;
  unsigned int board_id;
  board_id = index & 0xF000;
  board_id >>= 12;
  
  
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (board_id << 2));
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
  
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (board_id << 2));
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
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (board_id << 2));
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
  can_message.identifier = (ETM_CAN_MSG_CMD_TX | (board_id << 2));
  can_message.word3 = (board_id << 12) + ETM_CAN_REGISTER_DEFAULT_CMD_RESET_MCU;
  can_message.word2 = 0;
  can_message.word1 = 0;
  can_message.word0 = 0;
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &can_message);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()  
}


void ETMCanMasterClearDebug(void) {
  debug_data_ecb.debug_0             = 0;
  debug_data_ecb.debug_1             = 0;
  debug_data_ecb.debug_2             = 0;
  debug_data_ecb.debug_3             = 0;

  debug_data_ecb.debug_4             = 0;
  debug_data_ecb.debug_5             = 0;
  debug_data_ecb.debug_6             = 0;
  debug_data_ecb.debug_7             = 0;

  debug_data_ecb.debug_8             = 0;
  debug_data_ecb.debug_9             = 0;
  debug_data_ecb.debug_A             = 0;
  debug_data_ecb.debug_B             = 0;

  debug_data_ecb.debug_C             = 0;
  debug_data_ecb.debug_D             = 0;
  debug_data_ecb.debug_E             = 0;
  debug_data_ecb.debug_F             = 0;


  debug_data_ecb.can_tx_0            = 0;
  debug_data_ecb.can_tx_1            = 0;
  debug_data_ecb.can_tx_2            = 0;
  debug_data_ecb.CXEC_reg_max        = 0;

  debug_data_ecb.can_rx_0_filt_0     = 0;
  debug_data_ecb.can_rx_0_filt_1     = 0;
  debug_data_ecb.can_rx_1_filt_2     = 0;
  debug_data_ecb.CXINTF_max          = 0;

  debug_data_ecb.can_unknown_msg_id  = 0;
  debug_data_ecb.can_invalid_index   = 0;
  debug_data_ecb.can_address_error   = 0;
  debug_data_ecb.can_error_flag      = 0;

  debug_data_ecb.can_tx_buf_overflow = 0;
  debug_data_ecb.can_rx_buf_overflow = 0;
  debug_data_ecb.can_rx_log_buf_overflow = 0;
  debug_data_ecb.can_timeout         = 0;

  debug_data_ecb.reset_count         = 0;
  debug_data_ecb.RCON_value          = 0;
  debug_data_ecb.reserved_1          = 0;
  debug_data_ecb.reserved_0          = 0;

  debug_data_ecb.i2c_bus_error_count = 0;
  debug_data_ecb.spi_bus_error_count = 0;
  debug_data_ecb.scale_error_count   = 0;
  //self test results

  etm_can_tx_message_buffer.message_overwrite_count = 0;
  etm_can_rx_message_buffer.message_overwrite_count = 0;
  etm_can_rx_data_log_buffer.message_overwrite_count = 0;
  etm_can_persistent_data.reset_count = 0;
  etm_can_persistent_data.can_timeout_count = 0;


  _TRAPR = 0;
  _IOPUWR = 0;
  _EXTR = 0;
  _WDTO = 0;
  _SLEEP = 0;
  _IDLE = 0;
  _BOR = 0;
  _POR = 0;
  _SWR = 0;

  *CXINTF_ptr = 0;




  global_data_can_master.no_connect_count_ion_pump_board = 0;
  global_data_can_master.no_connect_count_magnetron_current_board = 0;
  global_data_can_master.no_connect_count_pulse_sync_board = 0;
  global_data_can_master.no_connect_count_hv_lambda_board = 0;
  global_data_can_master.no_connect_count_afc_board = 0;
  global_data_can_master.no_connect_count_cooling_interface_board = 0;
  global_data_can_master.no_connect_count_heater_magnet_board = 0;
  global_data_can_master.no_connect_count_gun_driver_board = 0;

}



void SendToEventLog(unsigned int log_id) {
  event_log.event_data[event_log.write_index].event_number = global_data_can_master.event_log_counter;
  event_log.event_data[event_log.write_index].event_time   = global_data_can_master.time_seconds_now;
  event_log.event_data[event_log.write_index].event_id     = log_id;
  event_log.write_index++;
  event_log.write_index &= 0x7F;
  global_data_can_master.event_log_counter++;
  // DPARKER need to check the EEPROM and TCP locations and advance them as nesseasry so that we don't pass them when advancing the write_index
}





/*
  This is the CAN Interrupt for the Master module
*/
void DoCanInterrupt(void);

void __attribute__((interrupt(__save__(CORCON,SR)), no_auto_psv)) _C1Interrupt(void) {
  _C1IF = 0;
  DoCanInterrupt();
}

void __attribute__((interrupt(__save__(CORCON,SR)), no_auto_psv)) _C2Interrupt(void) {
  _C2IF = 0;
  DoCanInterrupt();
}

void DoCanInterrupt(void) {
  ETMCanMessage can_message;
  unsigned int fast_log_buffer_index;
  unsigned int pulse_time;
  
  // Calculate the time (in milliseconds) that this interrupt occured
  // This is used for logging pulse messages
  pulse_time = global_data_can_master.millisecond_counter;
  pulse_time += ETMScaleFactor2((TMR5>>11),MACRO_DEC_TO_CAL_FACTOR_2(.8192),0);
  if (_T2IF) {
    pulse_time += 10;
  }

  debug_data_ecb.CXINTF_max |= *CXINTF_ptr;

  
  if (*CXRX0CON_ptr & BUFFER_FULL_BIT) {
    /*
      A message has been received in Buffer Zero
    */
    if (!(*CXRX0CON_ptr & FILTER_SELECT_BIT)) {
      // The command was received by Filter 0
      // It is a Next Pulse Level Command 
      debug_data_ecb.can_rx_0_filt_0++;
      ETMCanRXMessage(&can_message, CXRX0CON_ptr);
      etm_can_next_pulse_level = can_message.word1;
      etm_can_next_pulse_count = can_message.word0;

      if (_SYNC_CONTROL_HIGH_SPEED_LOGGING) {
	// Prepare the buffer to store the data
	fast_log_buffer_index = etm_can_next_pulse_count & 0x000F;
	if (etm_can_next_pulse_count & 0x0010) {
	  // We are putting data into buffer A
	  global_data_can_master.buffer_a_ready_to_send = 0;
	  global_data_can_master.buffer_a_sent = 0;
	  if (fast_log_buffer_index >= 3) {
	    global_data_can_master.buffer_b_ready_to_send = 1;
	  }
	  
	  *(unsigned int*)&high_speed_data_buffer_a[fast_log_buffer_index].status_bits = 0; // clear the status bits register
	  high_speed_data_buffer_a[fast_log_buffer_index].pulse_count = etm_can_next_pulse_count;
	  if (etm_can_next_pulse_level) {
	    high_speed_data_buffer_a[fast_log_buffer_index].status_bits.high_energy_pulse = 1;
	  }

	  high_speed_data_buffer_a[fast_log_buffer_index].x_ray_on_seconds_lsw = global_data_can_master.time_seconds_now;
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
	  global_data_can_master.buffer_b_ready_to_send = 0;
	  global_data_can_master.buffer_b_sent = 0;
	  if (fast_log_buffer_index >= 3) {
	    global_data_can_master.buffer_a_ready_to_send = 1;
	  }
	  
	  *(unsigned int*)&high_speed_data_buffer_b[fast_log_buffer_index].status_bits = 0; // Clear the status bits register
	  high_speed_data_buffer_b[fast_log_buffer_index].pulse_count = etm_can_next_pulse_count;
	  if (etm_can_next_pulse_level) {
	    high_speed_data_buffer_b[fast_log_buffer_index].status_bits.high_energy_pulse = 1;
	  }
	  
	  high_speed_data_buffer_b[fast_log_buffer_index].x_ray_on_seconds_lsw = global_data_can_master.time_seconds_now;
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
      // The command is a data log.  Add it to the data log buffer
      debug_data_ecb.can_rx_0_filt_1++;
      ETMCanRXMessageBuffer(&etm_can_rx_data_log_buffer, CXRX0CON_ptr);
    }
    *CXINTF_ptr &= RX1_INT_FLAG_BIT; // Clear the RX1 Interrupt Flag
  }
 
  if (*CXRX1CON_ptr & BUFFER_FULL_BIT) { 
    /* 
       A message has been recieved in Buffer 1
       This command gets pushed onto the command message buffer
    */
    debug_data_ecb.can_rx_1_filt_2++;
    ETMCanRXMessageBuffer(&etm_can_rx_message_buffer, CXRX1CON_ptr);
    *CXINTF_ptr &= RX1_INT_FLAG_BIT; // Clear the RX1 Interrupt Flag
  }

  if (!(*CXTX0CON_ptr & TX_REQ_BIT) && ((ETMCanBufferNotEmpty(&etm_can_tx_message_buffer)))) {  
    /*
      TX0 is empty and there is a message waiting in the transmit message buffer
      Load the next message into TX0
    */
    ETMCanTXMessageBuffer(&etm_can_tx_message_buffer, CXTX0CON_ptr);
    *CXINTF_ptr &= 0xFFFB; // Clear the TX0 Interrupt Flag
    debug_data_ecb.can_tx_0++;
  }
  
  if (*CXINTF_ptr & ERROR_FLAG_BIT) {
    // There was some sort of CAN Error
    debug_data_ecb.can_error_flag++;
    *CXINTF_ptr &= ~ERROR_FLAG_BIT; // Clear the ERR Flag
  } else {
    // FLASH THE CAN LED
    if (ETMReadPinLatch(can_params.led)) {
      ETMClearPin(can_params.led);
    } else {
      ETMSetPin(can_params.led);
    }
  }

}


