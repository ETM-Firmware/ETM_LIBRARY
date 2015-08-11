#include <xc.h>
#include <timer.h>   // DPARKER remove the need for timers.h here
#include "P1395_CAN_SLAVE.h"
#include "ETM.h"

const unsigned int p1395_can_slave_version = 0x0010;


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


// ------------------- Global Variables --------------------- //
ETMCanMessageBuffer etm_can_rx_message_buffer;
ETMCanMessageBuffer etm_can_tx_message_buffer;

unsigned int etm_can_next_pulse_level;
unsigned int etm_can_next_pulse_count;


// Public Debug and Status registers
ETMCanSystemDebugData local_debug_data;
ETMCanStatusRegister  etm_can_status_register;
ETMCanAgileConfig     etm_can_my_configuration;
ETMCanCanStatus       local_can_errors;
ETMCanSyncMessage     etm_can_sync_message;



// -------------------- local variables -------------------------- //
unsigned int slave_data_log_index;
unsigned int previous_ready_status;  // DPARKER - Need better name

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
 






void ETMCanSlaveInitialize(unsigned long fcy, unsigned int etm_can_address, unsigned long can_operation_led, unsigned int can_interrupt_priority) {
  unsigned long timer_period_value;

  if (can_interrupt_priority > 7) {
    can_interrupt_priority = 7;
  }
  
  can_params.address = etm_can_address;
  can_params.led = can_operation_led;


  //can_params.fcy = fcy;
  //can_params.can_interrupt_priority = can_interrupt_priority;

  etm_can_persistent_data.reset_count++;
  
  _SYNC_CONTROL_WORD = 0;
  etm_can_sync_message.sync_1 = 1;
  etm_can_sync_message.sync_2 = 2;
  etm_can_sync_message.sync_3 = 3;
  
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

  // DPARKER - Zero all the counters in the error structure.
  
  ETMCanBufferInitialize(&etm_can_rx_message_buffer);
  ETMCanBufferInitialize(&etm_can_tx_message_buffer);

  
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
    // DPARKER WHAT TO DO HERE
  }
  

  CXCFG2 = CXCFG2_VALUE;
  
  
  // Load Mask registers for RX0 and RX1
  CXRXM0SID = ETM_CAN_SLAVE_RX0_MASK;
  CXRXM1SID = ETM_CAN_SLAVE_RX1_MASK;

  // Load Filter registers
  CXRXF0SID = ETM_CAN_MSG_LVL_FILTER;
  CXRXF1SID = ETM_CAN_MSG_SYNC_FILTER;
  CXRXF2SID = (ETM_CAN_MSG_SLAVE_FILTER | (can_params.address << 3));
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
  
  // Enable Can interrupt
  _CXIE = 1;
  
  
  // Configure T4
  timer_period_value = fcy;
  timer_period_value >>= 8;
  timer_period_value /= T4_FREQUENCY_HZ;
  if (timer_period_value > 0xFFFF) {
    timer_period_value = 0xFFFF;
  }
  T4CON = (T4_OFF & T4_IDLE_CON & T4_GATE_OFF & T4_PS_1_256 & T4_32BIT_MODE_OFF & T4_SOURCE_INT);
  //PR4 = (unsigned int)(10000000/256/T4_FREQUENCY_HZ);  
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
  //PR5 = (unsigned int)(10000000/256/T5_FREQUENCY_HZ);
  PR5 = timer_period_value;
  _T5IF = 0;
  _T5IE = 0;
  T5CONbits.TON = 1;

  ETMPinTrisOutput(can_params.led);
}


void ETMCanSlaveLoadConfiguration(unsigned long agile_id, unsigned int agile_dash, unsigned int firmware_agile_rev, unsigned int firmware_branch, unsigned int firmware_minor_rev) {

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


void ETMCanSlaveDoCan(void) {
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
  local_debug_data.reserved_1 = RCON;
  local_debug_data.reserved_0 = 0x0000;

  ETMCanSlaveProcessMessage();
  ETMCanSlaveTimedTransmit();
  ETMCanSlaveCheckForTimeOut();
  ETMCanSlaveSendUpdateIfNewNotReady();
  if (_SYNC_CONTROL_CLEAR_DEBUG_DATA) {
    ETMCanSlaveClearDebug();
  }


  local_debug_data.can_bus_error_count = local_can_errors.timeout;
  local_debug_data.can_bus_error_count += local_can_errors.message_tx_buffer_overflow;
  local_debug_data.can_bus_error_count += local_can_errors.message_rx_buffer_overflow;
  local_debug_data.can_bus_error_count += local_can_errors.data_log_rx_buffer_overflow;
  local_debug_data.can_bus_error_count += local_can_errors.address_error;
  local_debug_data.can_bus_error_count += local_can_errors.invalid_index;
  local_debug_data.can_bus_error_count += local_can_errors.unknown_message_identifier;
}


void ETMCanSlavePulseSyncSendNextPulseLevel(unsigned int next_pulse_level, unsigned int next_pulse_count) {
  ETMCanMessage message;
  message.identifier = ETM_CAN_MSG_LVL_TX | (can_params.address << 3); 
  message.word0      = next_pulse_count;
  if (next_pulse_level) {
    message.word1    = 0xFFFF;
  } else {
    message.word1    = 0;
  }
  local_debug_data.debug_D = message.word1;
  ETMCanTXMessage(&message, &CXTX2CON);
  local_can_errors.tx_2++;
}


void ETMCanSlaveProcessMessage(void) {
  ETMCanMessage next_message;
  while (ETMCanBufferNotEmpty(&etm_can_rx_message_buffer)) {
    ETMCanReadMessageFromBuffer(&etm_can_rx_message_buffer, &next_message);
    
    if ((next_message.identifier & 0b0000000001111000) != (can_params.address << 3)) {
      // It was not addressed to this board
      local_can_errors.address_error++;
    } else if (next_message.identifier == (ETM_CAN_MSG_CMD_RX | (can_params.address << 3))) {
      ETMCanSlaveExecuteCMD(&next_message);      
    } else if (next_message.identifier == (ETM_CAN_MSG_REQUEST_RX | (can_params.address << 3))) {
      ETMCanSlaveReturnCalibrationPair(&next_message);
    } else {
      local_can_errors.unknown_message_identifier++;
    } 
  }
  
  local_can_errors.message_tx_buffer_overflow = etm_can_tx_message_buffer.message_overwrite_count;
  local_can_errors.message_rx_buffer_overflow = etm_can_rx_message_buffer.message_overwrite_count;
}



void ETMCanSlaveExecuteCMD(ETMCanMessage* message_ptr) {
  /*
    Register allocations
    0xZ000 -> 0xZ0FF  -> Common Slave Commands and Set Values    - ETMCanSlaveExecuteCMDCommon()
    0xZ100 -> 0xZ1FF  -> Calibration Read/Write Registers        - ETMCanSlaveSetCalibrationPair()
    0xZ200 -> 0xZ3FF  -> Slave Specific Commands and Set Values  - ETMCanSlaveExecuteCMD()
  */
  unsigned int index_word;
  index_word = message_ptr->word3;
  
  if ((index_word & 0xF000) != (can_params.address << 12)) {
    // The index is not addressed to this board
    local_can_errors.invalid_index++;
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
  } else {
    // It was not a command ID
    local_can_errors.invalid_index++;
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
    local_can_errors.invalid_index++;
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
  
  if ((index_word & 0xF000) != (can_params.address << 12)) {
    // The index is not addressed to this board
    local_can_errors.invalid_index++;
    return;
  }
  
  index_word &= 0x0FFF;
  
  if ((index_word < 0x0900) || (index_word >= 0x0A00)) {
    // this is not a valid calibration request index
    local_can_errors.invalid_index++;
    return;
  }
  
  index_word -= 0x0800;
  // The request is valid, return the data stored in eeprom
  return_msg.identifier = ETM_CAN_MSG_SET_2_TX | (can_params.address << 3);
  return_msg.word3 = message_ptr->word3 - 0x0800;
  return_msg.word2 = 0;
  return_msg.word1 = ETMEEPromReadWord(index_word + 1);
  return_msg.word0 = ETMEEPromReadWord(index_word);

  // Send Message Back to ECB with data
  ETMCanAddMessageToBuffer(&etm_can_tx_message_buffer, &return_msg);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}


void ETMCanSlaveTimedTransmit(void) {
  // Sends the debug information up as log data  
  if (_T4IF) {
    // should be true once every 100mS
    _T4IF = 0;
    
    slave_data_log_index++;
    slave_data_log_index &= 0xF;
    
    ETMCanSlaveSendStatus(); // Send out the status every 100mS
    
    switch (slave_data_log_index) 
      {
      case 0x0:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_ERROR_0, 
			   local_debug_data.i2c_bus_error_count, 
			   local_debug_data.spi_bus_error_count,
			   local_debug_data.can_bus_error_count,
			   local_debug_data.scale_error_count);      
	break;
	
      case 0x1:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_ERROR_1, 
			   local_debug_data.reset_count, 
			   *(unsigned int*)&local_debug_data.self_test_results, 
			   local_debug_data.reserved_1, 
			   local_debug_data.reserved_0);
	break;
	
      case 0x2:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_0,
			   local_debug_data.debug_0, 
			   local_debug_data.debug_1,
			   local_debug_data.debug_2,
			   local_debug_data.debug_3);  
	break;
	
      case 0x3:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_1,
			   local_debug_data.debug_4,
			   local_debug_data.debug_5,
			   local_debug_data.debug_6,
			   local_debug_data.debug_7);
	break;
	
      case 0x4:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_2,
			   local_debug_data.debug_8,
			   local_debug_data.debug_9,
			   local_debug_data.debug_A,
			   local_debug_data.debug_B);
	break;
	
      case 0x5:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_3,
			   local_debug_data.debug_C,
			   local_debug_data.debug_D,
			   local_debug_data.debug_E,
			   local_debug_data.debug_F);
	break;
	
      case 0x6:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_0,
			   local_can_errors.CXEC_reg,
			   local_can_errors.error_flag,
			   local_can_errors.tx_1,
			   local_can_errors.tx_2);
	break;
	
      case 0x7:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_1,
			   local_can_errors.rx_0_filt_0,
			   local_can_errors.rx_0_filt_1,
			   local_can_errors.rx_1_filt_2,
			   local_can_errors.isr_entered);
	break;
	
      case 0x8:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_2,
			   local_can_errors.unknown_message_identifier,
			   local_can_errors.invalid_index,
			   local_can_errors.address_error,
			   local_can_errors.tx_0);
	break;
	
      case 0x9:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_3,
			   local_can_errors.message_tx_buffer_overflow,
			   local_can_errors.message_rx_buffer_overflow,
			   local_can_errors.data_log_rx_buffer_overflow,
			   local_can_errors.timeout);
	break;
	
      case 0xA:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_0,
			   etm_can_my_configuration.agile_number_high_word,
			   etm_can_my_configuration.agile_number_low_word,
			   etm_can_my_configuration.agile_dash,
			   etm_can_my_configuration.agile_rev_ascii);
	break;

      case 0xB:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_1,
			   etm_can_my_configuration.serial_number,
			   etm_can_my_configuration.firmware_branch,
			   etm_can_my_configuration.firmware_major_rev,
			   etm_can_my_configuration.firmware_minor_rev);
	break;
	
      case 0xC:
	ETMCanSlaveLogCustomPacketC();
	break;
	
      case 0xD:
	ETMCanSlaveLogCustomPacketD();
	break;

      case 0xE:
	ETMCanSlaveLogCustomPacketE();
	break;

      case 0xF:
	ETMCanSlaveLogCustomPacketF();
	break;
      }
  }
}


void ETMCanSlaveSendStatus(void) {
  ETMCanMessage message;
  message.identifier = ETM_CAN_MSG_STATUS_TX | (can_params.address << 3);

  message.word0 = _CONTROL_REGISTER;
  message.word1 = _FAULT_REGISTER;
  message.word2 = etm_can_status_register.data_word_A;
  message.word3 = etm_can_status_register.data_word_B;
  
  ETMCanTXMessage(&message, &CXTX1CON);
  local_can_errors.tx_1++;
}


void ETMCanSlaveLogData(unsigned int packet_id, unsigned int word3, unsigned int word2, unsigned int word1, unsigned int word0) {
  
  ETMCanMessage log_message;
  
  packet_id &= 0x000F;
  packet_id |= (can_params.address << 4);
  packet_id <<= 1;
  packet_id |= 0b0000011000000000;
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
    local_can_errors.timeout++;
    etm_can_persistent_data.can_timeout_count = local_can_errors.timeout;
    _CONTROL_CAN_COM_LOSS = 1;
  }
}

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
  // At this time all that happens is that the chip watchdog is reset
  // DPARKER move to assembly and issure W0-W3, SR usage

  // It should be noted that if any of these registers are written ANYWHERE else, then they can be bashed
  // Therefore the Slave boards should NEVER WRITE ANYTHING in _SYNC_CONTROL_WORD

  _SYNC_CONTROL_WORD = message_ptr->word0;
  etm_can_sync_message.sync_1 = message_ptr->word1;
  etm_can_sync_message.sync_2 = message_ptr->word2;
  etm_can_sync_message.sync_3 = 0xFFFF;  // Use this to indicate that a sync message has been recieved
  
  ClrWdt();
  _CONTROL_CAN_COM_LOSS = 0;
  
  TMR5 = 0;

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

}


void ETMCanSlaveClearDebug(void) {
  etm_can_tx_message_buffer.message_overwrite_count = 0;
  etm_can_rx_message_buffer.message_overwrite_count = 0;
  etm_can_persistent_data.reset_count = 0;
  etm_can_persistent_data.can_timeout_count = 0;

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






// Can interrupt ISR for slave modules

void __attribute__((interrupt(__save__(CORCON,SR)), no_auto_psv)) _CXInterrupt(void) {
  ETMCanMessage can_message;
    
  _CXIF = 0;
  //local_can_errors.isr_entered++;
  local_can_errors.isr_entered |= CXINTF;
    
  if(CXRX0CONbits.RXFUL) {
    /*
      A message has been received in Buffer Zero
    */
    if (!CXRX0CONbits.FILHIT0) {
      // The command was received by Filter 0
      // It is a Next Pulse Level Command
      local_can_errors.rx_0_filt_0++;
      ETMCanRXMessage(&can_message, &CXRX0CON);
      etm_can_next_pulse_level = can_message.word1;
      etm_can_next_pulse_count = can_message.word0;
    } else {
      // The commmand was received by Filter 1
      // The command is a sync command.
      local_can_errors.rx_0_filt_1++;
      ETMCanRXMessage(&can_message, &CXRX0CON);
      ETMCanSlaveDoSync(&can_message);
    }
    CXINTFbits.RX0IF = 0; // Clear the Interuppt Status bit
  }
    
  if(CXRX1CONbits.RXFUL) {
    /* 
       A message has been recieved in Buffer 1
       This command gets pushed onto the command message buffer
    */
    local_can_errors.rx_1_filt_2++;
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
    // DPARKER - figure out which error and fix/reset
    local_can_errors.data_log_rx_buffer_overflow |= CXINTF;
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
  //local_can_errors.CXEC_reg = CXEC;
  
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

