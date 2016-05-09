#include <xc.h>
#include <timer.h>   // DPARKER remove the need for timers.h here
#include "P1395_CAN_SLAVE.h"
#include "ETM.h"


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
ETMCanBoardData           slave_board_data;            // This contains information that is always mirrored on ECB


// -------------------- local variables -------------------------- //
ETMCanBoardDebuggingData  etm_can_slave_debug_data;    // This information is only mirrored on ECB if this module is selected on the GUI
ETMCanSyncMessage         etm_can_slave_sync_message;  // This is the most recent sync message recieved from the ECB

ETMCanMessageBuffer etm_can_slave_rx_message_buffer;
ETMCanMessageBuffer etm_can_slave_tx_message_buffer;

unsigned int slave_data_log_index;      
unsigned int slave_data_log_sub_index;
// These two variables are used to rotate through all the possible logging messages
// One logging message is sent out once every 100ms

unsigned int previous_ready_status;  // DPARKER - Need better name

typedef struct {
  unsigned int reset_count;
  unsigned int can_timeout_count;
} PersistentData;
volatile PersistentData etm_can_persistent_data __attribute__ ((persistent));


typedef struct {
  unsigned int  address;
  unsigned long led;
  unsigned long flash_led;
  unsigned long not_ready_led;
} TYPE_CAN_PARAMETERS;

TYPE_CAN_PARAMETERS can_params;


// ------------------- #defines for placement of data into the local_data memory block ---------------//
#define etm_can_slave_next_pulse_level      slave_board_data.local_data[0]
#define etm_can_slave_next_pulse_count      slave_board_data.local_data[1]
#define etm_can_slave_com_loss              slave_board_data.local_data[2]
#define etm_can_slave_next_pulse_prf        slave_board_data.local_data[3]  // DPARKER provide functions for slave programs to read this data


// Board Configuration data - 0x06
#define config_agile_number_high_word      slave_board_data.config_data[0]
#define config_agile_number_low_word       slave_board_data.config_data[1]
#define config_agile_dash                  slave_board_data.config_data[2]
#define config_agile_rev_ascii             slave_board_data.config_data[3]

// Board Configuration data - 0x07
#define config_serial_number               slave_board_data.config_data[4]
#define config_firmware_agile_rev          slave_board_data.config_data[5]
#define config_firmware_branch             slave_board_data.config_data[6]
#define config_firmware_branch_rev         slave_board_data.config_data[7]


#define _SYNC_CONTROL_WORD                    *(unsigned int*)&etm_can_slave_sync_message.sync_0_control_word


// ---------- Pointers to CAN stucutres so that we can use CAN1 or CAN2
volatile unsigned int *CXEC_ptr;
volatile unsigned int *CXINTF_ptr;
volatile unsigned int *CXRX0CON_ptr;
volatile unsigned int *CXRX1CON_ptr;
volatile unsigned int *CXTX0CON_ptr;
volatile unsigned int *CXTX1CON_ptr;
volatile unsigned int *CXTX2CON_ptr;

 
void ETMCanSlaveInitialize(unsigned int requested_can_port, unsigned long fcy, unsigned int etm_can_address,
			   unsigned long can_operation_led, unsigned int can_interrupt_priority,
			   unsigned long flash_led, unsigned long not_ready_led) {

  unsigned long timer_period_value;

  etm_can_slave_debug_data.reserved_1          = P1395_CAN_SLAVE_VERSION;

  if (can_interrupt_priority > 7) {
    can_interrupt_priority = 7;
  }
  
  can_params.address = etm_can_address;
  can_params.led = can_operation_led;
  can_params.flash_led = flash_led;
  can_params.not_ready_led = not_ready_led;

  ETMPinTrisOutput(can_params.led);
  ETMPinTrisOutput(can_params.flash_led);
  ETMPinTrisOutput(can_params.not_ready_led);


  etm_can_persistent_data.reset_count++;
  
  _SYNC_CONTROL_WORD = 0;
  etm_can_slave_sync_message.sync_1_ecb_state_for_fault_logic = 0;
  etm_can_slave_sync_message.sync_2 = 0;
  etm_can_slave_sync_message.sync_3 = 0;
  
  etm_can_slave_com_loss = 0;

  etm_can_slave_debug_data.reset_count = etm_can_persistent_data.reset_count;
  etm_can_slave_debug_data.can_timeout = etm_can_persistent_data.can_timeout_count;

  //ETMCanSlaveClearDebug();  DPARKER you can't call this because it will clear the reset count
  
  ETMCanBufferInitialize(&etm_can_slave_rx_message_buffer);
  ETMCanBufferInitialize(&etm_can_slave_tx_message_buffer);
  
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
    CXEC_ptr     = &C2EC;
    CXINTF_ptr   = &C2INTF;
    CXRX0CON_ptr = &C2RX0CON;
    CXRX1CON_ptr = &C2RX1CON;
    CXTX0CON_ptr = &C2TX0CON;
    CXTX1CON_ptr = &C2TX1CON;
    CXTX2CON_ptr = &C2TX2CON;

    _C2IE = 0;
    _C2IF = 0;
    _C2IP = can_interrupt_priority;
    
    C2INTF = 0;
    
    C2INTEbits.RX0IE = 1; // Enable RXB0 interrupt
    C2INTEbits.RX1IE = 1; // Enable RXB1 interrupt
    C2INTEbits.TX0IE = 1; // Enable TXB0 interrupt
    C2INTEbits.ERRIE = 1; // Enable Error interrupt
  
    // ---------------- Set up CAN Control Registers ---------------- //
    
    // Set Baud Rate
    C2CTRL = CXCTRL_CONFIG_MODE_VALUE;
    while(C2CTRLbits.OPMODE != 4);
    
    if (fcy == 25000000) {
      C2CFG1 = CXCFG1_25MHZ_FCY_VALUE;    
    } else if (fcy == 20000000) {
      C2CFG1 = CXCFG1_20MHZ_FCY_VALUE;    
    } else if (fcy == 10000000) {
      C2CFG1 = CXCFG1_10MHZ_FCY_VALUE;    
    } else {
      // If you got here we can't configure the can module
      // DPARKER WHAT TO DO HERE
    }
    
    C2CFG2 = CXCFG2_VALUE;
    
    
    // Load Mask registers for RX0 and RX1
    C2RXM0SID = ETM_CAN_SLAVE_RX0_MASK;
    C2RXM1SID = ETM_CAN_SLAVE_RX1_MASK;
    
    // Load Filter registers
    C2RXF0SID = ETM_CAN_SLAVE_MSG_FILTER_RF0;
    C2RXF1SID = ETM_CAN_SLAVE_MSG_FILTER_RF1;
    C2RXF2SID = (ETM_CAN_SLAVE_MSG_FILTER_RF2 | (can_params.address << 2));
    //C2RXF3SID = ETM_CAN_MSG_FILTER_OFF;
    //C2RXF4SID = ETM_CAN_MSG_FILTER_OFF;
    //C2RXF5SID = ETM_CAN_MSG_FILTER_OFF;
    
    // Set Transmitter Mode
    C2TX0CON = CXTXXCON_VALUE_LOW_PRIORITY;
    C2TX1CON = CXTXXCON_VALUE_MEDIUM_PRIORITY;
    C2TX2CON = CXTXXCON_VALUE_HIGH_PRIORITY;
    
    C2TX0DLC = CXTXXDLC_VALUE;
    C2TX1DLC = CXTXXDLC_VALUE;
    C2TX2DLC = CXTXXDLC_VALUE;
    
    
    // Set Receiver Mode
    C2RX0CON = CXRXXCON_VALUE;
    C2RX1CON = CXRXXCON_VALUE;
    

    // Switch to normal operation
    C2CTRL = CXCTRL_OPERATE_MODE_VALUE;
    while(C2CTRLbits.OPMODE != 0);
    
    // Enable Can interrupt
    _C2IE = 1;
    _C1IE = 0;
  }
  
  if (!ETMAnalogCheckEEPromInitialized()) {
    ETMAnalogLoadDefaultCalibration();
    ETMEEPromWriteWord(0x0180, 65100);
    ETMEEPromWriteWord(0x0181, 0x31);
  }
}

void ETMCanSlaveLoadConfiguration(unsigned long agile_id, unsigned int agile_dash,
				  unsigned int firmware_agile_rev, unsigned int firmware_branch, 
				  unsigned int firmware_branch_rev) {
  
  config_agile_number_low_word = (agile_id & 0xFFFF);
  agile_id >>= 16;
  config_agile_number_high_word = agile_id;
  config_agile_dash = agile_dash;
  config_agile_rev_ascii = ETMEEPromReadWord(0x0181);

  config_firmware_agile_rev = firmware_agile_rev;
  config_firmware_branch = firmware_branch;
  config_firmware_branch_rev = firmware_branch_rev;
  config_serial_number = ETMEEPromReadWord(0x0180);
}


void ETMCanSlaveDoCan(void) {
  ETMCanSlaveProcessMessage();
  ETMCanSlaveTimedTransmit();
  ETMCanSlaveCheckForTimeOut();
  ETMCanSlaveSendUpdateIfNewNotReady();
  if (etm_can_slave_sync_message.sync_0_control_word.sync_F_clear_debug_data) {
    ETMCanSlaveClearDebug();
  }


  // Log debugging information
  etm_can_slave_debug_data.RCON_value = RCON;
  
  // Record the max TX counter
  if ((*CXEC_ptr & 0xFF00) > (etm_can_slave_debug_data.CXEC_reg_max & 0xFF00)) {
    etm_can_slave_debug_data.CXEC_reg_max &= 0x00FF;
    etm_can_slave_debug_data.CXEC_reg_max += (*CXEC_ptr & 0xFF00);
  }
  
  // Record the max RX counter
  if ((*CXEC_ptr & 0x00FF) > (etm_can_slave_debug_data.CXEC_reg_max & 0x00FF)) {
    etm_can_slave_debug_data.CXEC_reg_max &= 0xFF00;
    etm_can_slave_debug_data.CXEC_reg_max += (*CXEC_ptr & 0x00FF);
  }
}


void ETMCanSlavePulseSyncSendNextPulseLevel(unsigned int next_pulse_level, unsigned int next_pulse_count, unsigned int rep_rate_deci_herz) {
  ETMCanMessage message;
  message.identifier = ETM_CAN_MSG_LVL_TX | (can_params.address << 2); 
  message.word0      = next_pulse_count;
  if (next_pulse_level) {
    message.word1    = 0xFFFF;
  } else {
    message.word1    = 0;
  }
  message.word2 = rep_rate_deci_herz;

  ETMCanTXMessage(&message, CXTX2CON_ptr);
  etm_can_slave_debug_data.can_tx_2++;
}


void ETMCanSlaveProcessMessage(void) {
  ETMCanMessage next_message;
  while (ETMCanBufferNotEmpty(&etm_can_slave_rx_message_buffer)) {
    ETMCanReadMessageFromBuffer(&etm_can_slave_rx_message_buffer, &next_message);
    ETMCanSlaveExecuteCMD(&next_message);      
  }
  
  etm_can_slave_debug_data.can_tx_buf_overflow = etm_can_slave_tx_message_buffer.message_overwrite_count;
  etm_can_slave_debug_data.can_rx_buf_overflow = etm_can_slave_rx_message_buffer.message_overwrite_count;
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
    etm_can_slave_debug_data.can_invalid_index++;
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
    etm_can_slave_debug_data.can_invalid_index++;
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
    etm_can_slave_debug_data.can_invalid_index++;
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
  ETMCanAddMessageToBuffer(&etm_can_slave_tx_message_buffer, &return_msg);
  MacroETMCanCheckTXBuffer();  // DPARKER - Figure out how to build this into ETMCanAddMessageToBuffer()
}

/*
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
		     slave_board_data.log_data[data_register + 3],
		     slave_board_data.log_data[data_register + 2],
		     slave_board_data.log_data[data_register + 1],
		     slave_board_data.log_data[data_register]);
}
*/

void ETMCanSlaveTimedTransmit(void) {
  // Sends the debug information up as log data  
  if (_T4IF) {
    // should be true once every 100mS
    _T4IF = 0;
    
    // Set the Ready LED
    if (_CONTROL_NOT_READY) {
      ETMClearPin(can_params.not_ready_led); // This turns on the LED
    } else {
      ETMSetPin(can_params.not_ready_led);  // This turns off the LED
    }

    
    slave_data_log_index++;
    if (slave_data_log_index >= 10) {
      slave_data_log_index = 0;
      slave_data_log_sub_index++;
      slave_data_log_sub_index &= 0b11;
      // Flash the flashing LED
      if (ETMReadPinLatch(can_params.flash_led)) {
	ETMClearPin(can_params.flash_led);
      } else {
	ETMSetPin(can_params.flash_led);
      }
    }

    ETMCanSlaveSendStatus(); // Send out the status every 100mS

    // Also send out one bit of logging data every 100mS
    switch (slave_data_log_index) 
      {
      case 0:
      ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_0,
			 slave_board_data.log_data[3],
			 slave_board_data.log_data[2],
			 slave_board_data.log_data[1],
			 slave_board_data.log_data[0]);
	break;
	
      case 1:
      ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_1,
			 slave_board_data.log_data[7],
			 slave_board_data.log_data[6],
			 slave_board_data.log_data[5],
			 slave_board_data.log_data[4]);
	break;

      case 2:
      ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_2,
			 slave_board_data.log_data[11],
			 slave_board_data.log_data[10],
			 slave_board_data.log_data[9],
			 slave_board_data.log_data[8]);
	break;

      case 3:
      ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_3,
			 slave_board_data.log_data[15],
			 slave_board_data.log_data[14],
			 slave_board_data.log_data[13],
			 slave_board_data.log_data[12]);
	break;

      case 4:
      ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_4,
			 slave_board_data.log_data[19],
			 slave_board_data.log_data[18],
			 slave_board_data.log_data[17],
			 slave_board_data.log_data[16]);
	break;

      case 5:
      ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_5,
			 slave_board_data.log_data[23],
			 slave_board_data.log_data[22],
			 slave_board_data.log_data[21],
			 slave_board_data.log_data[20]);
	break;
      
      case 6:
	if ((slave_data_log_sub_index == 0) || (slave_data_log_sub_index == 2)) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_0,
			     etm_can_slave_debug_data.debug_reg[0],
			     etm_can_slave_debug_data.debug_reg[1],
			     etm_can_slave_debug_data.debug_reg[2],
			     etm_can_slave_debug_data.debug_reg[3]);
	} else {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_1,
			     etm_can_slave_debug_data.debug_reg[4],
			     etm_can_slave_debug_data.debug_reg[5],
			     etm_can_slave_debug_data.debug_reg[6],
			     etm_can_slave_debug_data.debug_reg[7]);
	}
	break;

      case 7:
	if ((slave_data_log_sub_index == 0) || (slave_data_log_sub_index == 2)) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_2,
			     etm_can_slave_debug_data.debug_reg[8],
			     etm_can_slave_debug_data.debug_reg[9],
			     etm_can_slave_debug_data.debug_reg[10],
			     etm_can_slave_debug_data.debug_reg[11]);
	} else {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_3,
			     etm_can_slave_debug_data.debug_reg[12],
			     etm_can_slave_debug_data.debug_reg[13],
			     etm_can_slave_debug_data.debug_reg[14],
			     etm_can_slave_debug_data.debug_reg[15]);
	}
	break;

      case 8:
	if (slave_data_log_sub_index == 0) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_0,
			     etm_can_slave_debug_data.can_tx_0,
			     etm_can_slave_debug_data.can_tx_1,
			     etm_can_slave_debug_data.can_tx_2,
			     etm_can_slave_debug_data.CXEC_reg_max);
			     



	} else if (slave_data_log_sub_index == 1) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_1,
			     etm_can_slave_debug_data.can_rx_0_filt_0,
			     etm_can_slave_debug_data.can_rx_0_filt_1,
			     etm_can_slave_debug_data.can_rx_1_filt_2,
			     etm_can_slave_debug_data.CXINTF_max);
	  
	} else if (slave_data_log_sub_index == 2) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_2,
			     etm_can_slave_debug_data.can_unknown_msg_id,
			     etm_can_slave_debug_data.can_invalid_index,
			     etm_can_slave_debug_data.can_address_error,
			     etm_can_slave_debug_data.can_error_flag);

	} else {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_3,
			     etm_can_slave_debug_data.can_tx_buf_overflow,
			     etm_can_slave_debug_data.can_rx_buf_overflow,
			     etm_can_slave_debug_data.can_rx_log_buf_overflow,
			     etm_can_slave_debug_data.can_timeout);
	}
	break;

      case 9:
	if (slave_data_log_sub_index == 0) {
	    ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_0,
			       config_agile_number_high_word,
			       config_agile_number_low_word,
			       config_agile_dash,
			       config_agile_rev_ascii);

	} else if (slave_data_log_sub_index == 1) {
	    ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_1,
			       config_serial_number,
			       config_firmware_agile_rev,
			       config_firmware_branch,
			       config_firmware_branch_rev);

	} else if (slave_data_log_sub_index == 2) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_SYSTEM_ERROR_0, 
			     etm_can_slave_debug_data.reset_count, 
			     etm_can_slave_debug_data.RCON_value,
			     etm_can_slave_debug_data.reserved_1, 
			     0x5050);//etm_can_slave_debug_data.reserved_0);	  	  

	} else {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_SYSTEM_ERROR_1, 
			     etm_can_slave_debug_data.i2c_bus_error_count, 
			     etm_can_slave_debug_data.spi_bus_error_count,
			     etm_can_slave_debug_data.scale_error_count,
			     *(unsigned int*)&etm_can_slave_debug_data.self_test_results);      
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
  message.word3 = _NOT_LOGGED_REGISTER;

  ETMCanTXMessage(&message, CXTX1CON_ptr);  
  etm_can_slave_debug_data.can_tx_1++;
  
  _NOTICE_0 = 0;
  _NOTICE_1 = 0;
  _NOTICE_2 = 0;
  _NOTICE_3 = 0;
  _NOTICE_4 = 0;
  _NOTICE_5 = 0;
  _NOTICE_6 = 0;
  _NOTICE_7 = 0;
  

}


void ETMCanSlaveLogPulseData(unsigned int packet_id, unsigned int word3, unsigned int word2, unsigned int word1, unsigned int word0) {
  ETMCanSlaveLogData(packet_id, word3, word2, word1, word0);
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
  
  ETMCanAddMessageToBuffer(&etm_can_slave_tx_message_buffer, &log_message);
  MacroETMCanCheckTXBuffer();
}

void ETMCanSlaveCheckForTimeOut(void) {
  if (_T5IF) {
    _T5IF = 0;
    etm_can_slave_debug_data.can_timeout++;
    etm_can_persistent_data.can_timeout_count = etm_can_slave_debug_data.can_timeout;
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


unsigned int test0;
unsigned int test1;
unsigned int test2;
unsigned int test3;



void ETMCanSlaveDoSync(ETMCanMessage* message_ptr) {
  

  // Sync data is available in CXRX0B1->CXRX0B4
  // DPARKER move to assembly and issure W0-W3, SR usage

  // It should be noted that if any of these registers are written ANYWHERE else, then they can be bashed
  // Therefore the Slave boards should NEVER WRITE ANYTHING in _SYNC_CONTROL_WORD


  _SYNC_CONTROL_WORD = message_ptr->word0;
  etm_can_slave_sync_message.sync_1_ecb_state_for_fault_logic = message_ptr->word1;
  etm_can_slave_sync_message.sync_2 = message_ptr->word2;
  etm_can_slave_sync_message.sync_3 = message_ptr->word3;


  test0 = _SYNC_CONTROL_WORD;
  test1 = etm_can_slave_sync_message.sync_1_ecb_state_for_fault_logic;
  test2 = etm_can_slave_sync_message.sync_2;
  test3 = etm_can_slave_sync_message.sync_3;


  
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
  etm_can_slave_debug_data.debug_reg[0]        = 0;
  etm_can_slave_debug_data.debug_reg[1]        = 0;
  etm_can_slave_debug_data.debug_reg[2]        = 0;
  etm_can_slave_debug_data.debug_reg[3]        = 0;

  etm_can_slave_debug_data.debug_reg[4]        = 0;
  etm_can_slave_debug_data.debug_reg[5]        = 0;
  etm_can_slave_debug_data.debug_reg[6]        = 0;
  etm_can_slave_debug_data.debug_reg[7]        = 0;

  etm_can_slave_debug_data.debug_reg[8]        = 0;
  etm_can_slave_debug_data.debug_reg[9]        = 0;
  etm_can_slave_debug_data.debug_reg[10]       = 0;
  etm_can_slave_debug_data.debug_reg[11]       = 0;

  etm_can_slave_debug_data.debug_reg[12]       = 0;
  etm_can_slave_debug_data.debug_reg[13]       = 0;
  etm_can_slave_debug_data.debug_reg[14]       = 0;
  etm_can_slave_debug_data.debug_reg[15]       = 0;


  etm_can_slave_debug_data.can_tx_0            = 0;
  etm_can_slave_debug_data.can_tx_1            = 0;
  etm_can_slave_debug_data.can_tx_2            = 0;
  etm_can_slave_debug_data.CXEC_reg_max        = 0;

  etm_can_slave_debug_data.can_rx_0_filt_0     = 0;
  etm_can_slave_debug_data.can_rx_0_filt_1     = 0;
  etm_can_slave_debug_data.can_rx_1_filt_2     = 0;
  etm_can_slave_debug_data.CXINTF_max          = 0;

  etm_can_slave_debug_data.can_unknown_msg_id  = 0;
  etm_can_slave_debug_data.can_invalid_index   = 0;
  etm_can_slave_debug_data.can_address_error   = 0;
  etm_can_slave_debug_data.can_error_flag      = 0;

  etm_can_slave_debug_data.can_tx_buf_overflow = 0;
  etm_can_slave_debug_data.can_rx_buf_overflow = 0;
  etm_can_slave_debug_data.can_rx_log_buf_overflow = 0;
  etm_can_slave_debug_data.can_timeout         = 0;

  etm_can_slave_debug_data.reset_count         = 0;
  etm_can_slave_debug_data.RCON_value          = 0;
  etm_can_slave_debug_data.reserved_1          = P1395_CAN_SLAVE_VERSION;
  etm_can_slave_debug_data.reserved_0          = 0;

  etm_can_slave_debug_data.i2c_bus_error_count = 0;
  etm_can_slave_debug_data.spi_bus_error_count = 0;
  etm_can_slave_debug_data.scale_error_count   = 0;
  //self test results

  etm_can_slave_tx_message_buffer.message_overwrite_count = 0;
  etm_can_slave_rx_message_buffer.message_overwrite_count = 0;
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

  etm_can_slave_debug_data.CXINTF_max |= *CXINTF_ptr;
  
  if (*CXRX0CON_ptr & BUFFER_FULL_BIT) {
    // A message has been received in Buffer Zero
    if (!(*CXRX0CON_ptr & FILTER_SELECT_BIT)) {
      // The command was received by Filter 0
      // It is a Next Pulse Level Command
      etm_can_slave_debug_data.can_rx_0_filt_0++;
      ETMCanRXMessage(&can_message, CXRX0CON_ptr);
      etm_can_slave_next_pulse_count = can_message.word0;
      etm_can_slave_next_pulse_level = can_message.word1;
      etm_can_slave_next_pulse_prf = can_message.word2;
    } else {
      // The commmand was received by Filter 1
      // The command is a sync command.
      etm_can_slave_debug_data.can_rx_0_filt_1++;
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
    etm_can_slave_debug_data.can_rx_1_filt_2++;
    ETMCanRXMessageBuffer(&etm_can_slave_rx_message_buffer, CXRX1CON_ptr);
    *CXINTF_ptr &= RX1_INT_FLAG_BIT; // Clear the RX1 Interrupt Flag
  }

  if (!(*CXTX0CON_ptr & TX_REQ_BIT) && ((ETMCanBufferNotEmpty(&etm_can_slave_tx_message_buffer)))) {
    /*
      TX0 is empty and there is a message waiting in the transmit message buffer
      Load the next message into TX0
    */
    ETMCanTXMessageBuffer(&etm_can_slave_tx_message_buffer, CXTX0CON_ptr);
    *CXINTF_ptr &= 0xFFFB; // Clear the TX0 Interrupt Flag
    etm_can_slave_debug_data.can_tx_0++;
  }
  
  if (*CXINTF_ptr & ERROR_FLAG_BIT) {
    // There was some sort of CAN Error
    // DPARKER - figure out which error and fix/reset
    etm_can_slave_debug_data.CXINTF_max |= *CXINTF_ptr;
    etm_can_slave_debug_data.can_error_flag++;
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

void ETMCanSlaveSetDebugRegister(unsigned int debug_register, unsigned int debug_value) {
  if (debug_register > 0x000F) {
    return;
  }
  etm_can_slave_debug_data.debug_reg[debug_register] = debug_value;
}

unsigned int ETMCanSlaveGetPulseLevel(void) {
  return etm_can_slave_next_pulse_level;
}

void ETMCanSlaveSetPulseLevelLow(void) {
  etm_can_slave_next_pulse_level = 0;
}

unsigned int ETMCanSlaveGetPulseCount(void) {
  return etm_can_slave_next_pulse_count;
}

unsigned int ETMCanSlaveGetComFaultStatus(void) {
  return etm_can_slave_com_loss;
}

unsigned int ETMCanSlaveGetSyncMsgResetEnable(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_0_reset_enable) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgHighSpeedLogging(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_1_high_speed_logging_enabled) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgPulseSyncDisableHV(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_2_pulse_sync_disable_hv) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgPulseSyncDisableXray(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_3_pulse_sync_disable_xray) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgSystemHVDisable(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_5_system_hv_disable) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgGunDriverDisableHeater(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_6_gun_driver_disable_heater) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgCoolingFault(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_4_cooling_fault) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgClearDebug(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_F_clear_debug_data) {
    return 0xFFFF;
  } else {
    return 0;
  }
}




unsigned int ETMCanSlaveGetSyncMsgPulseSyncWarmupLED(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_A_pulse_sync_warmup_led_on) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgPulseSyncStandbyLED(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_B_pulse_sync_standby_led_on) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgPulseSyncReadyLED(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_C_pulse_sync_ready_led_on) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgPulseSyncFaultLED(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_D_pulse_sync_fault_led_on) {
    return 0xFFFF;
  } else {
    return 0;
  }
}

unsigned int ETMCanSlaveGetSyncMsgECBState(void) {
  return etm_can_slave_sync_message.sync_1_ecb_state_for_fault_logic;
}

void ETMCanSlaveIncrementInvalidIndex(void) {
  etm_can_slave_debug_data.can_invalid_index++;
}
