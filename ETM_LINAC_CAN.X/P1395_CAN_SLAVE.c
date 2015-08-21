#include <xc.h>
#include <timer.h>   // DPARKER remove the need for timers.h here
#include "P1395_CAN_SLAVE.h"
#include "ETM.h"

const unsigned int p1395_can_slave_version = 0x0011;


// ----------- Can Timers T4 & T5 Configuration ----------- //
#define T4_FREQUENCY_HZ          10  // This is 100mS rate  // This is used to clock transmissions to the master.  A Status message and 1 of the 16 log messages is sent at every 100ms Interval
#define T5_FREQUENCY_HZ          4   // This is 250ms rate  // This is used as timeout on the CAN bus.  If a message is not recieved from the master before this overflows an error is generated


void ETMCanSlaveProcessMessage(void);
/*
  Message is decending priority

  Slave to Slave  - LEVEL_BROADCAST  - Processed in Can interrupt
  Master to Slave - SYNC_BROADCAST   - Processed in Can interrupt
  Slave to Master - SLAVE_MASTER     - Not processed by Slave
  Master to Slave - MASTER_SLAVE     - Processed by ETMCanSlaveProcessMessage
  Slave to Master - SLAVE_STATUS     - Not processed by Slave
  Master to Slave - MASTER_REQUEST   - Processed by ETMCanSlaveProcessMessage (not at the moment this is only Calibration Request)
  Slave to Master - SLAVE_LOG        - Not processed by Slave
  
*/


void ETMCanSlaveExecuteCMD(ETMCanMessage* message_ptr);           
/* 
   This is the generic subroutine to handle commands.
   It will check that the message is valid and then call
   ETMCanSlaveExecuteCMDCommon (if it is a generic message that applies to all boards)
   ETMCanSlaveExecuteCMDBoardSpecific (if it is a command processed only by this board)  -- THIS IS LOCATED in file XXXXX

*/


void ETMCanSlaveExecuteCMDCommon(ETMCanMessage* message_ptr);
/*
  This is a set value helper function
  If the index indicates a command command, this function will be called
  This handles commands that are common to all boards
*/


void ETMCanSlaveExecuteCMDBoardSpecific(ETMCanMessage* message_ptr);
/*
  This is a set value helper function
  If the index indicates a board specific command, this function will be called
  This function is custom for each board.  
  The source for this function must be included in the project directory of each board.
*/


void ETMCanSlaveSetCalibrationPair(ETMCanMessage* message_ptr);
/*
  This is a set value helper function
  If the index indicates a calibration point pair, this function will be called
  This function load the values in the message into EEPROM
  
  word 0 is offset data, this is stored at the register address (even)
  word 1 is scale data, this is stored at the register address + 1 (odd)

*/

void ETMCanSlaveReturnCalibrationPair(ETMCanMessage* message_ptr);
/*
  This returns the value of a calibration pair to the ECB
  It reads from the EEPROM and returns the results to the ECB
*/

void ETMCanSlaveTimedTransmit(void);
/*
  This uses TMR4 to schedule transmissions from the Slave to the Master
  TMR4 is set to expire at 100ms Interval
  Every 100ms a status message is set and 1 (of the 16) data log messages is sent
*/

void ETMCanSlaveSendStatus(void);
/*
  This is a ETMCanSlaveTimedTransmit helper function.
  This function sends the status from the slave board to the master
*/


void ETMCanSlaveLogData(unsigned int packet_id, unsigned int word3, unsigned int word2, unsigned int word1, unsigned int word0);
/*
  This is a ETMCanSlaveTimedTransmit helper function.
  It is used to generate the slave logging messages
*/


void ETMCanSlaveCheckForTimeOut(void);
/*
  This is a helper function to look for a can timeout by checing _T5IF bit
  TMR5 is cleared every time a sync message is recieved from the ECB
*/

void ETMCanSlaveSendUpdateIfNewNotReady(void);
/*
  This is used to immediately send a status message to the ECB when the _CONTROL_NOT_READY bit is first set 
*/

void ETMCanSlaveDoSync(ETMCanMessage* message_ptr);
/*
  This is a _CXInterrupt helper function
  This process the sync message from the ECB and loads it into RAM
*/

void ETMCanSlaveClearDebug(void);
/*
  This zeros all the debug data.
  It is called when the _SYNC_CONTROL_CLEAR_DEBUG_DATA bit is set
*/

void ETMCanSlaveLogData(unsigned int packet_id, unsigned int word3, unsigned int word2, unsigned int word1, unsigned int word0);
/*
  This is a ETMCanSlaveTimedTransmit helper function.
  It is used to generate the slave logging messages
*/






// ------------------- Global Variables --------------------- //
ETMCanMessageBuffer etm_can_rx_message_buffer;
ETMCanMessageBuffer etm_can_tx_message_buffer;

unsigned int etm_can_next_pulse_level;
unsigned int etm_can_next_pulse_count;
unsigned int etm_can_slave_com_loss;

// Public Debug and Status registers
unsigned int              etm_can_board_data[24];
ETMCanStatusRegister      etm_can_status_register;
ETMCanStandardLoggingData etm_can_log_data;
ETMCanSyncMessage         etm_can_sync_message;



// -------------------- local variables -------------------------- //
unsigned int slave_data_log_index;
unsigned int slave_data_log_sub_timer;
unsigned int previous_ready_status;  // DPARKER - Need better name

// ---------- Pointers to CAN stucutres so that we can use CAN1 or CAN2
volatile unsigned int *CXEC_ptr;
volatile unsigned int *CXINTF_ptr;
volatile unsigned int *CXRX0CON_ptr;
volatile unsigned int *CXRX1CON_ptr;
volatile unsigned int *CXTX0CON_ptr;
volatile unsigned int *CXTX1CON_ptr;
volatile unsigned int *CXTX2CON_ptr;


typedef struct {
  unsigned int reset_count;
  unsigned int can_timeout_count;
} PersistentData;
volatile PersistentData etm_can_persistent_data __attribute__ ((persistent));


typedef struct {
  unsigned int  address;
  unsigned long led;
} TYPE_CAN_PARAMETERS;

TYPE_CAN_PARAMETERS can_params;
 


void ETMCanSlaveInitialize(unsigned int requested_can_port, unsigned long fcy, unsigned int etm_can_address, unsigned long can_operation_led, unsigned int can_interrupt_priority) {
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
  
  etm_can_slave_com_loss = 0;

  etm_can_log_data.reset_count = etm_can_persistent_data.reset_count;
  etm_can_log_data.can_timeout = etm_can_persistent_data.can_timeout_count;

  ETMCanSlaveClearDebug();
  
  ETMCanBufferInitialize(&etm_can_rx_message_buffer);
  ETMCanBufferInitialize(&etm_can_tx_message_buffer);
  
  // Configure T4
  timer_period_value = fcy;
  timer_period_value >>= 8;
  timer_period_value /= T4_FREQUENCY_HZ;
  if (timer_period_value > 0xFFFF) {
    timer_period_value = 0xFFFF;
  }
  T4CON = (T4_OFF & T4_IDLE_CON & T4_GATE_OFF & T4_PS_1_256 & T4_32BIT_MODE_OFF & T4_SOURCE_INT);
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
  T5CON = (T5_OFF & T5_IDLE_CON & T5_GATE_OFF & T5_PS_1_256 & T5_SOURCE_INT);
  PR5 = timer_period_value;
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
      // DPARKER WHAT TO DO HERE
    }
    
    C1CFG2 = CXCFG2_VALUE;
    
    
    // Load Mask registers for RX0 and RX1
    C1RXM0SID = ETM_CAN_SLAVE_RX0_MASK;
    C1RXM1SID = ETM_CAN_SLAVE_RX1_MASK;
    
    // Load Filter registers
    C1RXF0SID = ETM_CAN_SLAVE_MSG_FILTER_RF0;
    C1RXF1SID = ETM_CAN_SLAVE_MSG_FILTER_RF1;
    C1RXF2SID = (ETM_CAN_SLAVE_MSG_FILTER_RF2 | (can_params.address << 2));
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
    
    // Enable Can interrupt
    _C1IE = 1;
    _C2IE = 0;
    
  } else {
    // Use CAN2
  }
}


void ETMCanSlaveLoadConfiguration(unsigned long agile_id, unsigned int agile_dash, unsigned int firmware_agile_rev, unsigned int firmware_branch, unsigned int firmware_minor_rev) {

  etm_can_log_data.agile_number_low_word = (agile_id & 0xFFFF);
  agile_id >>= 16;
  etm_can_log_data.agile_number_high_word = agile_id;
  etm_can_log_data.agile_dash = agile_dash;
  etm_can_log_data.firmware_branch = firmware_branch;
  etm_can_log_data.firmware_major_rev = firmware_agile_rev;
  etm_can_log_data.firmware_minor_rev = firmware_minor_rev;

  // Load default values for agile rev and serial number at this time
  // Need some way to update these with the real numbers
  etm_can_log_data.agile_rev_ascii = 'A';
  etm_can_log_data.serial_number = 0;
}


void ETMCanSlaveDoCan(void) {
  ETMCanSlaveProcessMessage();
  ETMCanSlaveTimedTransmit();
  ETMCanSlaveCheckForTimeOut();
  ETMCanSlaveSendUpdateIfNewNotReady();
  if (_SYNC_CONTROL_CLEAR_DEBUG_DATA) {
    ETMCanSlaveClearDebug();
  }


  // Log debugging information
  etm_can_log_data.RCON_value = RCON;
  
  // Record the max TX counter
  if ((*CXEC_ptr & 0xFF00) > (etm_can_log_data.CXEC_reg_max & 0xFF00)) {
    etm_can_log_data.CXEC_reg_max &= 0x00FF;
    etm_can_log_data.CXEC_reg_max += (*CXEC_ptr & 0xFF00);
  }
  
  // Record the max RX counter
  if ((*CXEC_ptr & 0x00FF) > (etm_can_log_data.CXEC_reg_max & 0x00FF)) {
    etm_can_log_data.CXEC_reg_max &= 0xFF00;
    etm_can_log_data.CXEC_reg_max += (*CXEC_ptr & 0x00FF);
  }
}


void ETMCanSlavePulseSyncSendNextPulseLevel(unsigned int next_pulse_level, unsigned int next_pulse_count) {
  ETMCanMessage message;
  message.identifier = ETM_CAN_MSG_LVL_TX | (can_params.address << 2); 
  message.word0      = next_pulse_count;
  if (next_pulse_level) {
    message.word1    = 0xFFFF;
  } else {
    message.word1    = 0;
  }

  ETMCanTXMessage(&message, CXTX2CON_ptr);
  etm_can_log_data.can_tx_2++;
}


void ETMCanSlaveProcessMessage(void) {
  ETMCanMessage next_message;
  while (ETMCanBufferNotEmpty(&etm_can_rx_message_buffer)) {
    ETMCanReadMessageFromBuffer(&etm_can_rx_message_buffer, &next_message);
    ETMCanSlaveExecuteCMD(&next_message);      
  }
  
  etm_can_log_data.can_tx_buf_overflow = etm_can_tx_message_buffer.message_overwrite_count;
  etm_can_log_data.can_rx_buf_overflow = etm_can_rx_message_buffer.message_overwrite_count;
}



void ETMCanSlaveExecuteCMD(ETMCanMessage* message_ptr) {
  /*
    CMD Index allocations
    0xZ000 -> 0xZ0FF  -> Common Slave Commands and Set Values    - ETMCanSlaveExecuteCMDCommon()
    0xZ100 -> 0xZ1FF  -> Calibration Write Registers             - ETMCanSlaveSetCalibrationPair()
    0xZ200 -> 0xZ3FF  -> Slave Specific Commands and Set Values  - ETMCanSlaveExecuteCMD()
    0xZ900 -> 0xZ9FF  -> Calibration Read Registers              - ETMCanSlaveReturnCalibrationPair()
  */
  unsigned int index_word;
  index_word = message_ptr->word3;
  
  if ((index_word & 0xF000) != (can_params.address << 12)) {
    // The index is not addressed to this board
    etm_can_log_data.can_invalid_index++;
    return;
  }
  
  index_word &= 0x0FFF;

  if (index_word <= 0x00FF) {
    // It is a default command
    ETMCanSlaveExecuteCMDCommon(message_ptr);
  } else if (index_word <= 0x01FF) {
    // It is Calibration Pair Set Message
    ETMCanSlaveSetCalibrationPair(message_ptr);
  } else if (index_word <= 0x3FF) {
    // It is a board specific command
    ETMCanSlaveExecuteCMDBoardSpecific(message_ptr);
  } else if ((index_word >= 0x900) && (index_word <= 0x9FF)) {
    // It is Calibration Pair Request
    ETMCanSlaveReturnCalibrationPair(message_ptr);
  } else {
    // It was not a command ID
    etm_can_log_data.can_invalid_index++;
  }
}



void ETMCanSlaveExecuteCMDCommon(ETMCanMessage* message_ptr) {
  unsigned int index_word;
  index_word = message_ptr->word3;
  index_word &= 0x0FFF;
  
  switch (index_word) {
    
  case ETM_CAN_REGISTER_DEFAULT_CMD_RESET_MCU:
    __asm__ ("Reset");
    break;

  case ETM_CAN_REGISTER_DEFAULT_CMD_RESET_ANALOG_CALIBRATION:
    ETMAnalogLoadDefaultCalibration();
    break;
 
  default:
    // The default command was not recognized 
    etm_can_log_data.can_invalid_index++;
    break;
  }
}


void ETMCanSlaveSetCalibrationPair(ETMCanMessage* message_ptr) {
  unsigned int eeprom_register;

  eeprom_register = message_ptr->word3;
  eeprom_register &= 0x0FFF;
  ETMEEPromWriteWord(eeprom_register, message_ptr->word0);
  ETMEEPromWriteWord(eeprom_register + 1, message_ptr->word1);
}

void ETMCanSlaveReturnCalibrationPair(ETMCanMessage* message_ptr) {
  ETMCanMessage return_msg;
  unsigned int index_word;
  index_word = message_ptr->word3;
  
  index_word &= 0x0FFF;
  
  index_word -= 0x0800;
  // The request is valid, return the data stored in eeprom
  return_msg.identifier = ETM_CAN_MSG_RTN_TX | (can_params.address << 2);
  return_msg.word3 = message_ptr->word3 - 0x0800;
  return_msg.word2 = 0;
  return_msg.word1 = ETMEEPromReadWord(index_word + 1);
  return_msg.word0 = ETMEEPromReadWord(index_word);

  // Send Message Back to ECB with data
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &return_msg);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}


void ETMCanSlaveLogBoardData(unsigned int data_register) {
  unsigned int log_register;
  if (data_register >= 6) {
    return;
  }
  log_register = data_register;
  log_register <<= 4;
  log_register |= can_params.address;
  
  data_register *= 4;
  
  ETMCanSlaveLogData(log_register,
		     etm_can_board_data[data_register + 3],
		     etm_can_board_data[data_register + 2],
		     etm_can_board_data[data_register + 1],
		     etm_can_board_data[data_register]);
		     
}

void ETMCanSlaveTimedTransmit(void) {
  // Sends the debug information up as log data  
  if (_T4IF) {
    // should be true once every 100mS
    _T4IF = 0;
    
    slave_data_log_index++;
    if (slave_data_log_index >= 10) {
      slave_data_log_index = 0;
      
      slave_data_log_sub_timer++;
      slave_data_log_sub_timer &= 0b11;
    }
    
    ETMCanSlaveSendStatus(); // Send out the status every 100mS

    // Also send out one bit of logging data every 100mS
    switch (slave_data_log_index) 
      {
      case 0x0:
	ETMCanSlaveLogBoardData(0);
	break;
	
      case 0x1:
	ETMCanSlaveLogBoardData(1);
	break;

      case 0x2:
	ETMCanSlaveLogBoardData(2);
	break;

      case 0x3:
	ETMCanSlaveLogBoardData(3);
	break;

      case 0x4:
	ETMCanSlaveLogBoardData(4);
	break;

      case 0x5:
	ETMCanSlaveLogBoardData(5);
	break;
      
      case 0x6:
	if ((slave_data_log_sub_timer == 0) || (slave_data_log_sub_timer == 2)) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_0,
			     etm_can_log_data.debug_0,
			     etm_can_log_data.debug_1,
			     etm_can_log_data.debug_2,
			     etm_can_log_data.debug_3);
	} else {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_1,
			     etm_can_log_data.debug_4,
			     etm_can_log_data.debug_5,
			     etm_can_log_data.debug_6,
			     etm_can_log_data.debug_7);
	}
	break;

      case 0x7:
	if ((slave_data_log_sub_timer == 0) || (slave_data_log_sub_timer == 2)) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_2,
			     etm_can_log_data.debug_8,
			     etm_can_log_data.debug_9,
			     etm_can_log_data.debug_A,
			     etm_can_log_data.debug_B);
	} else {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_3,
			     etm_can_log_data.debug_C,
			     etm_can_log_data.debug_D,
			     etm_can_log_data.debug_E,
			     etm_can_log_data.debug_F);
	}
	break;

      case 0x8:
	if (slave_data_log_sub_timer == 0) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_0,
			     etm_can_log_data.can_tx_0,
			     etm_can_log_data.can_tx_1,
			     etm_can_log_data.can_tx_2,
			     etm_can_log_data.CXEC_reg_max);
			     



	} else if (slave_data_log_sub_timer == 1) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_1,
			     etm_can_log_data.can_rx_0_filt_0,
			     etm_can_log_data.can_rx_0_filt_1,
			     etm_can_log_data.can_rx_1_filt_2,
			     etm_can_log_data.CXINTF_max);
	  
	} else if (slave_data_log_sub_timer == 2) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_2,
			     etm_can_log_data.can_unknown_msg_id,
			     etm_can_log_data.can_invalid_index,
			     etm_can_log_data.can_address_error,
			     etm_can_log_data.can_error_flag);

	} else {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_3,
			     etm_can_log_data.can_tx_buf_overflow,
			     etm_can_log_data.can_rx_buf_overflow,
			     etm_can_log_data.can_rx_log_buf_overflow,
			     etm_can_log_data.can_timeout);
	}
	break;

      case 0x9:
	if (slave_data_log_sub_timer == 0) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_0,
			     etm_can_log_data.agile_number_high_word,
			     etm_can_log_data.agile_number_low_word,
			     etm_can_log_data.agile_dash,
			     etm_can_log_data.agile_rev_ascii);
	  
	} else if (slave_data_log_sub_timer == 1) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_1,
			     etm_can_log_data.serial_number,
			     etm_can_log_data.firmware_branch,
			     etm_can_log_data.firmware_major_rev,
			     etm_can_log_data.firmware_minor_rev);
	  
	} else if (slave_data_log_sub_timer == 2) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_SYSTEM_ERROR_0, 
			     etm_can_log_data.reset_count, 
			     etm_can_log_data.RCON_value,
			     etm_can_log_data.reserved_1, 
			     etm_can_log_data.reserved_0);	  	  

	} else {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_SYSTEM_ERROR_1, 
			     etm_can_log_data.i2c_bus_error_count, 
			     etm_can_log_data.spi_bus_error_count,
			     etm_can_log_data.scale_error_count,
			     *(unsigned int*)&etm_can_log_data.self_test_results);      
	}
	break;
      }
  }
}


void ETMCanSlaveSendStatus(void) {
  ETMCanMessage message;
  message.identifier = ETM_CAN_MSG_STATUS_TX | (can_params.address << 2);

  message.word0 = _CONTROL_REGISTER;
  message.word1 = _FAULT_REGISTER;
  message.word2 = _WARNING_REGISTER;
  message.word3 = 0;

  ETMCanTXMessage(&message, CXTX1CON_ptr);  
  etm_can_log_data.can_tx_1++;
}


void ETMCanSlaveLogData(unsigned int packet_id, unsigned int word3, unsigned int word2, unsigned int word1, unsigned int word0) {
  ETMCanMessage log_message;
  
  packet_id |= can_params.address; // Add the board address to the packet_id
  packet_id |= 0b0000010000000000; // Add the Data log identifier
  
  // Format the packet Id for the PIC Register
  packet_id <<= 2;
  log_message.identifier = packet_id;
  log_message.identifier &= 0xFF00;
  log_message.identifier <<= 3;
  log_message.identifier |= (packet_id & 0x00FF);
  
  log_message.word0 = word0;
  log_message.word1 = word1;
  log_message.word2 = word2;
  log_message.word3 = word3;
  
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &log_message);
  MacroETMCanCheckTXBuffer();
}

void ETMCanSlaveCheckForTimeOut(void) {
  if (_T5IF) {
    _T5IF = 0;
    etm_can_log_data.can_timeout++;
    etm_can_persistent_data.can_timeout_count = etm_can_log_data.can_timeout;
    etm_can_slave_com_loss = 1;
  }
}


// DPARKER this needs to get fixed
void ETMCanSlaveSendUpdateIfNewNotReady(void) {
  if ((previous_ready_status == 0) && (_CONTROL_NOT_READY)) {
    // There is new condition that is causing this board to inhibit operation.
    // Send a status update upstream to Master
    ETMCanSlaveSendStatus();
  }
  previous_ready_status = _CONTROL_NOT_READY;
}



void ETMCanSlaveDoSync(ETMCanMessage* message_ptr) {
  // Sync data is available in CXRX0B1->CXRX0B4
  // DPARKER move to assembly and issure W0-W3, SR usage

  // It should be noted that if any of these registers are written ANYWHERE else, then they can be bashed
  // Therefore the Slave boards should NEVER WRITE ANYTHING in _SYNC_CONTROL_WORD

  _SYNC_CONTROL_WORD = message_ptr->word0;
  etm_can_sync_message.sync_1_ecb_state_for_fault_logic = message_ptr->word1;
  etm_can_sync_message.sync_2 = message_ptr->word2;
  etm_can_sync_message.sync_3 = message_ptr->word3;
  
  ClrWdt();
  etm_can_slave_com_loss = 0;
  
  TMR5 = 0;

  // DPARKER this will not work as implimented.  Confirm it is handled by the pusle sync board
  /*
#ifdef __A36487
  // The Pulse Sync Board needs to see if it needs to inhibit X_RAYs
  // This can be based by an update to read/modify update to this PORT that is currently in process
  // It will get fixed the next time through the control loop, but there is nothing else we can do
  // Hopefully we are using good coding principles and not bashing a PORT register.
  if (_SYNC_CONTROL_PULSE_SYNC_DISABLE_HV || _SYNC_CONTROL_PULSE_SYNC_DISABLE_XRAY) {
    PIN_CPU_XRAY_ENABLE_OUT = !OLL_CPU_XRAY_ENABLE;
    // DPARKER how do we ensure that this is not bashed by a command in progress???
  }
#endif
  */
}


void ETMCanSlaveClearDebug(void) {
  etm_can_log_data.debug_0             = 0;
  etm_can_log_data.debug_1             = 0;
  etm_can_log_data.debug_2             = 0;
  etm_can_log_data.debug_3             = 0;

  etm_can_log_data.debug_4             = 0;
  etm_can_log_data.debug_5             = 0;
  etm_can_log_data.debug_6             = 0;
  etm_can_log_data.debug_7             = 0;

  etm_can_log_data.debug_8             = 0;
  etm_can_log_data.debug_9             = 0;
  etm_can_log_data.debug_A             = 0;
  etm_can_log_data.debug_B             = 0;

  etm_can_log_data.debug_C             = 0;
  etm_can_log_data.debug_D             = 0;
  etm_can_log_data.debug_E             = 0;
  etm_can_log_data.debug_F             = 0;


  etm_can_log_data.can_tx_0            = 0;
  etm_can_log_data.can_tx_1            = 0;
  etm_can_log_data.can_tx_2            = 0;
  etm_can_log_data.CXEC_reg_max        = 0;

  etm_can_log_data.can_rx_0_filt_0     = 0;
  etm_can_log_data.can_rx_0_filt_1     = 0;
  etm_can_log_data.can_rx_1_filt_2     = 0;
  etm_can_log_data.CXINTF_max          = 0;

  etm_can_log_data.can_unknown_msg_id  = 0;
  etm_can_log_data.can_invalid_index   = 0;
  etm_can_log_data.can_address_error   = 0;
  etm_can_log_data.can_error_flag      = 0;

  etm_can_log_data.can_tx_buf_overflow = 0;
  etm_can_log_data.can_rx_buf_overflow = 0;
  etm_can_log_data.can_rx_log_buf_overflow = 0;
  etm_can_log_data.can_timeout         = 0;

  etm_can_log_data.reset_count         = 0;
  etm_can_log_data.RCON_value          = 0;
  etm_can_log_data.reserved_1          = 0;
  etm_can_log_data.reserved_0          = 0;

  etm_can_log_data.i2c_bus_error_count = 0;
  etm_can_log_data.spi_bus_error_count = 0;
  etm_can_log_data.scale_error_count   = 0;
  //self test results

  etm_can_tx_message_buffer.message_overwrite_count = 0;
  etm_can_rx_message_buffer.message_overwrite_count = 0;
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
}


// Can interrupt ISR for slave modules
#define BUFFER_FULL_BIT    0x0080
#define FILTER_SELECT_BIT  0x0001
#define TX_REQ_BIT         0x0008
#define RX0_INT_FLAG_BIT   0xFFFE
#define RX1_INT_FLAG_BIT   0xFFFD
#define ERROR_FLAG_BIT     0x0020
  

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

  etm_can_log_data.CXINTF_max |= *CXINTF_ptr;
  
  if (*CXRX0CON_ptr & BUFFER_FULL_BIT) {
    // A message has been received in Buffer Zero
    if (!(*CXRX0CON_ptr & FILTER_SELECT_BIT)) {
      // The command was received by Filter 0
      // It is a Next Pulse Level Command
      etm_can_log_data.can_rx_0_filt_0++;
      ETMCanRXMessage(&can_message, CXRX0CON_ptr);
      etm_can_next_pulse_level = can_message.word1;
      etm_can_next_pulse_count = can_message.word0;
    } else {
      // The commmand was received by Filter 1
      // The command is a sync command.
      etm_can_log_data.can_rx_0_filt_1++;
      ETMCanRXMessage(&can_message, CXRX0CON_ptr);
      ETMCanSlaveDoSync(&can_message);
    }
    *CXINTF_ptr &= RX0_INT_FLAG_BIT; // Clear the RX0 Interrupt Flag
  }
  
  if (*CXRX1CON_ptr & BUFFER_FULL_BIT) {
    /* 
       A message has been recieved in Buffer 1
       This command gets pushed onto the command message buffer
    */
    etm_can_log_data.can_rx_1_filt_2++;
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
    etm_can_log_data.can_tx_0++;
  }
  
  if (*CXINTF_ptr & ERROR_FLAG_BIT) {
    // There was some sort of CAN Error
    // DPARKER - figure out which error and fix/reset
    etm_can_log_data.CXINTF_max |= *CXINTF_ptr;
    etm_can_log_data.can_error_flag++;
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






