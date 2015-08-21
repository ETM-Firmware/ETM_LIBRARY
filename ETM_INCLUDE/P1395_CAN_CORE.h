#ifndef __P1395_CAN_CORE_H
#define __P1395_CAN_CORE_H

// ---------------------- STATUS REGISTER DEFFENITIONS   ------------------------- //

typedef struct {
  unsigned control_0_sum_fault:1;
  unsigned control_1_not_configured:1;
  unsigned control_2_not_ready:1;
  unsigned control_3_self_check_error:1;
  unsigned control_4_unused:1;
  unsigned control_5_unused:1;
  unsigned control_6_unused:1;
  unsigned control_7_unused:1;
  
  unsigned notice_0:1;
  unsigned notice_1:1;
  unsigned notice_2:1;
  unsigned notice_3:1;
  unsigned notice_4:1;
  unsigned notice_5:1;
  unsigned notice_6:1;
  unsigned notice_7:1;
} ETMCanStatusRegisterControlAndNoticeBits;

typedef struct {
  unsigned fault_0:1;
  unsigned fault_1:1;
  unsigned fault_2:1;
  unsigned fault_3:1;
  unsigned fault_4:1;
  unsigned fault_5:1;
  unsigned fault_6:1;
  unsigned fault_7:1;
  unsigned fault_8:1;
  unsigned fault_9:1;
  unsigned fault_A:1;
  unsigned fault_B:1;
  unsigned fault_C:1;
  unsigned fault_D:1;
  unsigned fault_E:1;
  unsigned fault_F:1;
} ETMCanStatusRegisterFaultBits;

typedef struct {
  unsigned warning_0:1;
  unsigned warning_1:1;
  unsigned warning_2:1;
  unsigned warning_3:1;
  unsigned warning_4:1;
  unsigned warning_5:1;
  unsigned warning_6:1;
  unsigned warning_7:1;
  unsigned warning_8:1;
  unsigned warning_9:1;
  unsigned warning_A:1;
  unsigned warning_B:1;
  unsigned warning_C:1;
  unsigned warning_D:1;
  unsigned warning_E:1;
  unsigned warning_F:1;
} ETMCanStatusRegisterWarningBits;

typedef struct {
  unsigned not_logged_0:1;
  unsigned not_logged_1:1;
  unsigned not_logged_2:1;
  unsigned not_logged_3:1;
  unsigned not_logged_4:1;
  unsigned not_logged_5:1;
  unsigned not_logged_6:1;
  unsigned not_logged_7:1;
  unsigned not_logged_8:1;
  unsigned not_logged_9:1;
  unsigned not_logged_A:1;
  unsigned not_logged_B:1;
  unsigned not_logged_C:1;
  unsigned not_logged_D:1;
  unsigned not_logged_E:1;
  unsigned not_logged_F:1;
} ETMCanStatusRegisterNotLoggedBits;



typedef struct {
  ETMCanStatusRegisterControlAndNoticeBits   control_notice_bits;  // 16 bits
  ETMCanStatusRegisterFaultBits              fault_bits;    // 16 bits
  ETMCanStatusRegisterWarningBits            warning_bits;  // 16 bits
  ETMCanStatusRegisterNotLoggedBits          not_logged_bits;   // 16 bits
} ETMCanStatusRegister;

extern ETMCanStatusRegister           etm_can_status_register;

#define _CONTROL_SUM_FAULT            etm_can_status_register.control_notice_bits.control_0_sum_fault
#define _CONTROL_NOT_CONFIGURED       etm_can_status_register.control_notice_bits.control_1_not_configured
#define _CONTROL_NOT_READY            etm_can_status_register.control_notice_bits.control_2_not_ready
#define _CONTROL_SELF_CHECK_ERROR     etm_can_status_register.control_notice_bits.control_3_self_check_error


#define _NOTICE_0                     etm_can_status_register.control_notice_bits.notice_0
#define _NOTICE_1                     etm_can_status_register.control_notice_bits.notice_1
#define _NOTICE_2                     etm_can_status_register.control_notice_bits.notice_2
#define _NOTICE_3                     etm_can_status_register.control_notice_bits.notice_3
#define _NOTICE_4                     etm_can_status_register.control_notice_bits.notice_4
#define _NOTICE_5                     etm_can_status_register.control_notice_bits.notice_5
#define _NOTICE_6                     etm_can_status_register.control_notice_bits.notice_6
#define _NOTICE_7                     etm_can_status_register.control_notice_bits.notice_7

#define _FAULT_0                      etm_can_status_register.fault_bits.fault_0
#define _FAULT_1                      etm_can_status_register.fault_bits.fault_1
#define _FAULT_2                      etm_can_status_register.fault_bits.fault_2
#define _FAULT_3                      etm_can_status_register.fault_bits.fault_3
#define _FAULT_4                      etm_can_status_register.fault_bits.fault_4
#define _FAULT_5                      etm_can_status_register.fault_bits.fault_5
#define _FAULT_6                      etm_can_status_register.fault_bits.fault_6
#define _FAULT_7                      etm_can_status_register.fault_bits.fault_7
#define _FAULT_8                      etm_can_status_register.fault_bits.fault_8
#define _FAULT_9                      etm_can_status_register.fault_bits.fault_9
#define _FAULT_A                      etm_can_status_register.fault_bits.fault_A
#define _FAULT_B                      etm_can_status_register.fault_bits.fault_B
#define _FAULT_C                      etm_can_status_register.fault_bits.fault_C
#define _FAULT_D                      etm_can_status_register.fault_bits.fault_D
#define _FAULT_E                      etm_can_status_register.fault_bits.fault_E
#define _FAULT_F                      etm_can_status_register.fault_bits.fault_F

#define _WARNING_0                    etm_can_status_register.warning_bits.warning_0
#define _WARNING_1                    etm_can_status_register.warning_bits.warning_1
#define _WARNING_2                    etm_can_status_register.warning_bits.warning_2
#define _WARNING_3                    etm_can_status_register.warning_bits.warning_3
#define _WARNING_4                    etm_can_status_register.warning_bits.warning_4
#define _WARNING_5                    etm_can_status_register.warning_bits.warning_5
#define _WARNING_6                    etm_can_status_register.warning_bits.warning_6
#define _WARNING_7                    etm_can_status_register.warning_bits.warning_7
#define _WARNING_8                    etm_can_status_register.warning_bits.warning_8
#define _WARNING_9                    etm_can_status_register.warning_bits.warning_9
#define _WARNING_A                    etm_can_status_register.warning_bits.warning_A
#define _WARNING_B                    etm_can_status_register.warning_bits.warning_B
#define _WARNING_C                    etm_can_status_register.warning_bits.warning_C
#define _WARNING_D                    etm_can_status_register.warning_bits.warning_D
#define _WARNING_E                    etm_can_status_register.warning_bits.warning_E
#define _WARNING_F                    etm_can_status_register.warning_bits.warning_F


#define _CONTROL_REGISTER             *(unsigned int*)&etm_can_status_register.control_notice_bits
#define _FAULT_REGISTER               *(unsigned int*)&etm_can_status_register.fault_bits
#define _WARNING_REGISTER             *(unsigned int*)&etm_can_status_register.warning_bits




// -----------------------  DEBUG REGISTERS DEFFENTITIONS --------------------- //


typedef struct {
  unsigned st_5V_over_voltage:1;
  unsigned st_5V_under_voltage:1;
  unsigned st_15V_over_voltage:1;
  unsigned st_15V_under_voltage:1;
  unsigned st_N15V_over_voltage:1;
  unsigned st_N15V_under_voltage:1;
  unsigned st_ADC_over_voltage:1;
  unsigned st_ADC_under_voltage:1;
  unsigned st_ADC_EXT:1;
  unsigned st__EEPROM:1;
  unsigned st_DAC:1;
  unsigned st_spare_4:1;
  unsigned st_spare_3:1;
  unsigned st_spare_2:1;
  unsigned st_spare_1:1;
  unsigned st_spare_0:1;
} ETMCanSelfTestRegister;


// ------------ LOG DATA Structure ----------------- //
/*
  4 words of board specific status data
  This data is mirrored on the ECB for all boards all the time.
  

  32 words of board specific logging data (this is defined for each board)



  16 words of board specific debug data (this is used by the software developer)
  8 words of board configuration
  16 wrods of Can Module Debugging Data
  16 words of Board Debugging Data   
  This data is only mirrored on the ECB for the "active" window pane

*/

extern unsigned int etm_can_board_data[24];

#define board_data_msg_0_word_0 etm_can_board_data[0];
#define board_data_msg_0_word_1 etm_can_board_data[1];
#define board_data_msg_0_word_2 etm_can_board_data[2];
#define board_data_msg_0_word_3 etm_can_board_data[3];

#define board_data_msg_1_word_0 etm_can_board_data[4];
#define board_data_msg_1_word_1 etm_can_board_data[5];
#define board_data_msg_1_word_2 etm_can_board_data[6];
#define board_data_msg_1_word_3 etm_can_board_data[7];

#define board_data_msg_2_word_0 etm_can_board_data[8];
#define board_data_msg_2_word_1 etm_can_board_data[9];
#define board_data_msg_2_word_2 etm_can_board_data[10];
#define board_data_msg_2_word_3 etm_can_board_data[11];

#define board_data_msg_3_word_0 etm_can_board_data[12];
#define board_data_msg_3_word_1 etm_can_board_data[13];
#define board_data_msg_3_word_2 etm_can_board_data[14];
#define board_data_msg_3_word_3 etm_can_board_data[15];

#define board_data_msg_4_word_0 etm_can_board_data[16];
#define board_data_msg_4_word_1 etm_can_board_data[17];
#define board_data_msg_4_word_2 etm_can_board_data[18];
#define board_data_msg_4_word_3 etm_can_board_data[19];

#define board_data_msg_5_word_0 etm_can_board_data[20];
#define board_data_msg_5_word_1 etm_can_board_data[21];
#define board_data_msg_5_word_2 etm_can_board_data[22];
#define board_data_msg_5_word_3 etm_can_board_data[23];



typedef struct {
  // Software Debug Data - 0x10
  unsigned int debug_0;
  unsigned int debug_1;
  unsigned int debug_2;
  unsigned int debug_3;

  // Software Debug Data - 0x11
  unsigned int debug_4;
  unsigned int debug_5;
  unsigned int debug_6;
  unsigned int debug_7;

  // Software Debug Data - 0x12
  unsigned int debug_8;
  unsigned int debug_9;
  unsigned int debug_A;
  unsigned int debug_B;

  // Software Debug Data - 0x13
  unsigned int debug_C;
  unsigned int debug_D;
  unsigned int debug_E;
  unsigned int debug_F;

  // Board Configuration data - 0x14
  unsigned int  agile_number_high_word;
  unsigned int  agile_number_low_word;
  unsigned int  agile_dash;
  unsigned int  agile_rev_ascii;

  // Board Configuration data - 0x15
  unsigned int  serial_number;
  unsigned int  firmware_branch;
  unsigned int  firmware_major_rev;
  unsigned int  firmware_minor_rev;

  // Can data log - 0x20
  unsigned int can_tx_0;            // count of tx_0 transmits
  unsigned int can_tx_1;            // count of tx_1 transmits
  unsigned int can_tx_2;            // count of tx_2 transmits
  unsigned int CXEC_reg_max;    // MAX instead of instantaneous value of CXEC
  
  // Can data log - 0x21
  unsigned int can_rx_0_filt_0;     // count of messages received by this filter
  unsigned int can_rx_0_filt_1;     // count of messages received by this filter
  unsigned int can_rx_1_filt_2;     // count of messages received by this filter
  unsigned int CXINTF_max;      // logical or of the CXINTF register every time the can ISR is entered

  // Can data log - 0x22
  unsigned int can_unknown_msg_id;  // NOT POSSIBLE SLAVE// count of the number of unknown message ids
  unsigned int can_invalid_index;   // count of the number of invalid message index
  unsigned int can_address_error;   // NOT POSSIBLE SLAVE // count of the number of received messages not addressed to this board
  unsigned int can_error_flag;      // counts the number of CAN error flags interrupts

  // Can data log - 0x23
  unsigned int can_tx_buf_overflow; // overwrite count on etm_can_tx_message_buffer 
  unsigned int can_rx_buf_overflow; // overwrite count on etm_can_rx_message_buffer
  unsigned int can_rx_log_buf_overflow; // MASTER ONLY - overwrite count on the logging data buffer overflow count
  unsigned int can_timeout;         // count of the number of can timeouts
  
  // Board Debug Data - 0x24
  // DPARKER are there better things we could be storing
  unsigned int reset_count;     // This counts the number of processor resets since cleared by the user
  unsigned int RCON_value;      // The current value of RCON
  unsigned int reserved_1;
  unsigned int reserved_0;

  // Board Debug Data - 0x25
  // DPARKER are there better things we could be storing?
  unsigned int i2c_bus_error_count;
  unsigned int spi_bus_error_count;
  unsigned int scale_error_count;
  ETMCanSelfTestRegister self_test_results;

} ETMCanStandardLoggingData;

extern ETMCanStandardLoggingData etm_can_log_data;


// -------------------------  SYNC MESSAGE DEFFENITIONS ----------------------- //

typedef struct {
  unsigned sync_0_reset_enable:1;
  unsigned sync_1_high_speed_logging_enabled:1;
  unsigned sync_2_pulse_sync_disable_hv:1;
  unsigned sync_3_pulse_sync_disable_xray:1;
  unsigned sync_4_cooling_fault:1;
  unsigned sync_5_unused:1;
  unsigned sync_6_unused:1;
  unsigned sync_7_unused:1;

  unsigned sync_8_unused:1;
  unsigned sync_9_unused:1;
  unsigned sync_A_pulse_sync_warmup_led_on:1;
  unsigned sync_B_pulse_sync_standby_led_on:1;
  unsigned sync_C_pulse_sync_ready_led_on:1;
  unsigned sync_D_pulse_sync_fault_led_on:1;
  unsigned sync_E_unused:1;
  unsigned sync_F_clear_debug_data:1;
} ETMCanSyncControlWord;


typedef struct {
  ETMCanSyncControlWord sync_0_control_word;
  unsigned int sync_1_ecb_state_for_fault_logic;
  unsigned int sync_2;
  unsigned int sync_3;
} ETMCanSyncMessage;

extern ETMCanSyncMessage     etm_can_sync_message;


#define _SYNC_CONTROL_RESET_ENABLE            etm_can_sync_message.sync_0_control_word.sync_0_reset_enable
#define _SYNC_CONTROL_HIGH_SPEED_LOGGING      etm_can_sync_message.sync_0_control_word.sync_1_high_speed_logging_enabled
#define _SYNC_CONTROL_PULSE_SYNC_DISABLE_HV   etm_can_sync_message.sync_0_control_word.sync_2_pulse_sync_disable_hv
#define _SYNC_CONTROL_PULSE_SYNC_DISABLE_XRAY etm_can_sync_message.sync_0_control_word.sync_3_pulse_sync_disable_xray
#define _SYNC_CONTROL_COOLING_FAULT           etm_can_sync_message.sync_0_control_word.sync_4_cooling_fault
#define _SYNC_CONTROL_CLEAR_DEBUG_DATA        etm_can_sync_message.sync_0_control_word.sync_F_clear_debug_data

#define _SYNC_CONTROL_PULSE_SYNC_WARMUP_LED   etm_can_sync_message.sync_0_control_word.sync_A_pulse_sync_warmup_led_on
#define _SYNC_CONTROL_PULSE_SYNC_STANDBY_LED  etm_can_sync_message.sync_0_control_word.sync_B_pulse_sync_standby_led_on
#define _SYNC_CONTROL_PULSE_SYNC_READY_LED    etm_can_sync_message.sync_0_control_word.sync_C_pulse_sync_ready_led_on
#define _SYNC_CONTROL_PULSE_SYNC_FAULT_LED    etm_can_sync_message.sync_0_control_word.sync_D_pulse_sync_fault_led_on


#define _SYNC_CONTROL_WORD                 *(unsigned int*)&etm_can_sync_message.sync_0_control_word



// Public Variables
extern unsigned int etm_can_next_pulse_level;  // This value will get updated in RAM as when a next pulse level command is received
extern unsigned int etm_can_next_pulse_count;  // This value will get updated in RAM as when a next pulse level command is received
#define ETM_CAN_HIGH_ENERGY           1
#define ETM_CAN_LOW_ENERGY            0



typedef struct {
  unsigned int identifier;
  unsigned int word0;
  unsigned int word1;
  unsigned int word2;
  unsigned int word3;
} ETMCanMessage;


typedef struct {
  unsigned int message_write_index;
  unsigned int message_read_index;
  unsigned int message_write_count;
  unsigned int message_overwrite_count;
  ETMCanMessage message_data[16];
} ETMCanMessageBuffer;


// Can Module Buffers
extern ETMCanMessageBuffer etm_can_rx_message_buffer;
extern ETMCanMessageBuffer etm_can_tx_message_buffer;




void ETMCanRXMessage(ETMCanMessage* message_ptr, volatile unsigned int* rx_register_address);
/*
  This stores the data selected by rx_register_address (C1RX0CON,C1RX1CON,C2RX0CON,C2RX1CON) into the message
  If there is no data RX Buffer then error information is placed into the message
  This clears the RXFUL bit so that the buffer is available to receive another message
  see ETM_CAN_UTILITY.s
*/


void ETMCanRXMessageBuffer(ETMCanMessageBuffer* buffer_ptr, volatile unsigned int* rx_data_address);
/*
  This stores the data selected by rx_data_address (C1RX0CON,C1RX1CON,C2RX0CON,C2RX1CON) into the next available slot in the selected buffer.
  If the message buffer is full the data in the RX buffer is discarded.
  If the RX buffer is empty, nothing is added to the message buffer
  This clears the RXFUL bit so that the buffer is available to receive another message
  see ETM_CAN_UTILITY.s
*/


void ETMCanTXMessage(ETMCanMessage* message_ptr, volatile unsigned int* tx_register_address);
/*
  This moves the message data to the TX register indicated by tx_register_address (C1TX0CON, C1TX1CON, C1TX2CON)
  If the TX register is not empty, the data will be overwritten.
  Also sets the transmit bit to queue transmission
  see ETM_CAN_UTILITY.s
*/


void ETMCanTXMessageBuffer(ETMCanMessageBuffer* buffer_ptr, volatile unsigned int* tx_register_address);
/*
  This moves the oldest message in the buffer to to the TX register indicated by tx_register_address (C1TX0CON, C1TX1CON, C1TX2CON)
  If the TX register is not empty, no data will be transfered and the message buffer state will remain unchanged
  If the message buffer is empty, no data will be transmited and no error will be generated
  Also sets the transmit bit to queue transmission
  see ETM_CAN_UTILITY.s
*/


void ETMCanAddMessageToBuffer(ETMCanMessageBuffer* buffer_ptr, ETMCanMessage* message_ptr);
/*
  This adds a message to the buffer
  If the buffer is full the data is discarded.
  see ETM_CAN_UTILITY.s
*/


void ETMCanReadMessageFromBuffer(ETMCanMessageBuffer* buffer_ptr, ETMCanMessage* message_ptr);
/*
  This moves the oldest message in the buffer to the message_ptr
  If the buffer is empty it returns the error identifier (0b0000111000000000) and fills the data with Zeros.
  see ETM_CAN_UTILITY.s
*/


void ETMCanBufferInitialize(ETMCanMessageBuffer* buffer_ptr);
/*
  This initializes a can message buffer.
  see ETM_CAN_UTILITY.s
*/


unsigned int ETMCanBufferRowsAvailable(ETMCanMessageBuffer* buffer_ptr);
/*
  This returns 0 if the buffer is full, otherwise returns the number of available rows
  see ETM_CAN_UTILITY.s
*/


unsigned int ETMCanBufferNotEmpty(ETMCanMessageBuffer* buffer_ptr);
/*
  Returns 0 if the buffer is Empty, otherwise returns the number messages in the buffer
  see ETM_CAN_UTILITY.s
*/



// ---------- Define RX SID Masks and Filters ---------------


// RECEIVE MODE                                  0bXXXCCCCCCCAAAAX0
// MASTER MASKS
#define ETM_CAN_MASTER_RX0_MASK                  0b0001111110000000  // This is used by master to receive LVL/STATUS/RTN
#define ETM_CAN_MASTER_RX1_MASK                  0b0001000000000000  // This is used by master to receive LOG

// SLAVE MASKS
#define ETM_CAN_SLAVE_RX0_MASK                   0b0001111111111100  // This is used by slaves to receive LVL/SYNC
#define ETM_CAN_SLAVE_RX1_MASK                   0b0001111111111100  // This is used by slaves to receive CMD msg


// Define RX SID Filters
// MASTER FILTERS
#define ETM_CAN_MASTER_MSG_FILTER_RF0            0b0000000000000000  // This will accept the LVL broadcast
#define ETM_CAN_MASTER_MSG_FILTER_RF1            0b0000001000000000  // This will accept STATUS/RTN from slaves
#define ETM_CAN_MASTER_MSG_FILTER_RF2            0b0001000000000000  // This will accept LOG msg from slaves


// SLAVE FILTERS
#define ETM_CAN_SLAVE_MSG_FILTER_RF0             0b0000000000000000  // This will accept the LVL broadcast
#define ETM_CAN_SLAVE_MSG_FILTER_RF1             0b0000000100000000  // This will accept the SYNC broadcast
#define ETM_CAN_SLAVE_MSG_FILTER_RF2             0b0000110000000000  // When or'ed with address this will accept CMD msg


// ------- Define the bits for particular commands ---------------- 
// The TX and RX SID buffers within the PIC are not mapped the same.
// That is why the RX and TX are defined seperately

// MASTER receive Message Identifiers
// RECEIVE MODE                                  0bXXXCCCCCCCAAAAX0
#define ETM_CAN_MSG_RTN_RX                       0b0000001000000000  // When the address is removed this is a RTN message 
#define ETM_CAN_MSG_STATUS_RX                    0b0000001001000000  // When the address is removed this is a STATUS message 


// Define TX SID VALUES
// TRASMIT MODE                                  0bCCCCCXXXCCAAAA00
#define ETM_CAN_MSG_LVL_TX                       0b0000000000000000 
#define ETM_CAN_MSG_SYNC_TX                      0b0000100000000000
#define ETM_CAN_MSG_RTN_TX                       0b0001000000000000 // When or'ed with slave address
#define ETM_CAN_MSG_STATUS_TX                    0b0001000001000000 // When or'ed with slave address
#define ETM_CAN_MSG_CMD_TX                       0b0110000000000000 // When or'ed with slave address 
#define ETM_CAN_MSG_LOG_TX                       0b1000000000000000 // When or'ed with salve address and packet id




// Can configuration parameters with Fcan = 4xFcy
#define CXCTRL_CONFIG_MODE_VALUE                 0b0000010000000000      // This sets Fcan to 4xFcy
#define CXCTRL_OPERATE_MODE_VALUE                0b0000000000000000      // This sets Fcan to 4xFcy
#define CXCTRL_LOOPBACK_MODE_VALUE               0b0000001000000000      // This sets Fcan to 4xFcy

#define CXCFG1_10MHZ_FCY_VALUE                   0b0000000000000001      // This sets TQ to 4/Fcan
#define CXCFG1_20MHZ_FCY_VALUE                   0b0000000000000011      // This sets TQ to 8/Fcan
#define CXCFG1_25MHZ_FCY_VALUE                   0b0000000000000100      // This sets TQ to 10/Fcan



//#define CXCFG2_VALUE                             0b0000001110010001      // This will created a bit timing of 10x TQ
/*
  Can Bit Timing
  Syncchronization segment     - 1xTq
  Propagation Time Segments    - 2xTq
  Phase Buffer Segment 1       - 3xTq
  Sample Type                  - Single Sample
  Phase Buffer Segment 2       - 4xTq
  Maximum Jump Width(CXCFG1)   - 1xTq

*/


/*
  Can Bit Timing
  Syncchronization segment     - 1xTq
  Propagation Time Segments    - 3xTq
  Phase Buffer Segment 1       - 4xTq
  Sample Type                  - Single Sample
  Phase Buffer Segment 2       - 4xTq
  Maximum Jump Width(CXCFG1)   - 1xTq

*/
#define CXCFG2_VALUE                             0b0000001110011010      // This will created a bit timing of 12x TQ
/*
  DPARKER -  In order to get the CAN network to work it was necessary to increase the time of each bit.
  With testing we may be able to get the values back down to 10xTQ which is 1Mbit
*/



#define CXTXXCON_VALUE_HIGH_PRIORITY             0b0000000000000011
#define CXTXXCON_VALUE_MEDIUM_PRIORITY           0b0000000000000010
#define CXTXXCON_VALUE_LOW_PRIORITY              0b0000000000000001
#define CXRXXCON_VALUE                           0b0000000000000000

#define CXTXXDLC_VALUE                           0b0000000011000000


//------------------------------- Specific Board and Command Defines -------------------------- // 

#define ETM_CAN_ADDR_ETHERNET_BOARD                                     14
#define ETM_CAN_ADDR_ION_PUMP_BOARD                                     1
#define ETM_CAN_ADDR_MAGNETRON_CURRENT_BOARD                            2
#define ETM_CAN_ADDR_PULSE_SYNC_BOARD                                   3
#define ETM_CAN_ADDR_HV_LAMBDA_BOARD                                    4
#define ETM_CAN_ADDR_AFC_CONTROL_BOARD                                  5
#define ETM_CAN_ADDR_COOLING_INTERFACE_BOARD                            6
#define ETM_CAN_ADDR_HEATER_MAGNET_BOARD                                7
#define ETM_CAN_ADDR_GUN_DRIVER_BOARD                                   8




typedef struct {
  unsigned unused_0:1;
  unsigned ion_pump_board:1;
  unsigned magnetron_current_board:1;
  unsigned pulse_sync_board:1;
  unsigned hv_lambda_board:1;
  unsigned afc_board:1;
  unsigned cooling_interface_board:1;
  unsigned heater_magnet_board:1;
  unsigned gun_driver_board:1;
  unsigned unused_9:1;
  unsigned unused_10:1;
  unsigned unused_11:1;
  unsigned unused_12:1;
  unsigned unused_13:1;
  unsigned ethernet_board:1;
  unsigned unused_15:1;
} P1395BoardBits;

// Default Register Locations
#define ETM_CAN_REGISTER_DEFAULT_CMD_RESET_MCU                          0x001
#define ETM_CAN_REGISTER_DEFAULT_CMD_RESET_ANALOG_CALIBRATION           0x003


// Board Specific Register Locations
#define ETM_CAN_REGISTER_HV_LAMBDA_SET_1_LAMBDA_SET_POINT               0x4200

#define ETM_CAN_REGISTER_AFC_SET_1_HOME_POSITION_AND_OFFSET             0x5200
#define ETM_CAN_REGISTER_AFC_CMD_DO_AUTO_ZERO                           0x5201
#define ETM_CAN_REGISTER_AFC_CMD_SELECT_AFC_MODE                        0x5202
#define ETM_CAN_REGISTER_AFC_CMD_SELECT_MANUAL_MODE                     0x5203
#define ETM_CAN_REGISTER_AFC_CMD_SET_MANUAL_TARGET_POSITION             0x5204
#define ETM_CAN_REGISTER_AFC_CMD_RELATIVE_MOVE_MANUAL_TARGET            0x5205

#define ETM_CAN_REGISTER_COOLING_CMD_SF6_PULSE_LIMIT_OVERRIDE           0x6200
#define ETM_CAN_REGISTER_COOLING_CMD_SF6_LEAK_LIMIT_OVERRIDE            0x6201
#define ETM_CAN_REGISTER_COOLING_CMD_RESET_BOTTLE_COUNT                 0x6202

#define ETM_CAN_REGISTER_HEATER_MAGNET_SET_1_CURRENT_SET_POINT          0x7200

#define ETM_CAN_REGISTER_GUN_DRIVER_SET_1_GRID_TOP_SET_POINT            0x8200
#define ETM_CAN_REGISTER_GUN_DRIVER_SET_1_HEATER_CATHODE_SET_POINT      0x8201

#define ETM_CAN_REGISTER_PULSE_SYNC_SET_1_HIGH_ENERGY_TIMING_REG_0      0x3200
#define ETM_CAN_REGISTER_PULSE_SYNC_SET_1_HIGH_ENERGY_TIMING_REG_1      0x3201
#define ETM_CAN_REGISTER_PULSE_SYNC_SET_1_LOW_ENERGY_TIMING_REG_0       0x3202
#define ETM_CAN_REGISTER_PULSE_SYNC_SET_1_LOW_ENERGY_TIMING_REG_1       0x3203
#define ETM_CAN_REGISTER_PULSE_SYNC_SET_1_CUSTOMER_LED_OUTPUT           0x3204

#define ETM_CAN_REGISTER_ECB_SET_2_HIGH_ENERGY_TARGET_CURRENT_MON       0xE200
#define ETM_CAN_REGISTER_ECB_SET_2_LOW_ENERGY_TARGET_CURRENT_MON        0xE201


//------------------ DATA LOGGING REGISTERS --------------------------//

// Default data logging registers

#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_0                      0x000 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_1                      0x010 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_2                      0x020 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_3                      0x030 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_4                      0x040 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_5                      0x050 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_0                       0x100 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_1                       0x110 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_2                       0x120 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_3                       0x130 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_0                      0x180 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CONFIG_1                      0x190 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_0                   0x200 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_1                   0x210 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_2                   0x220 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_CAN_ERROR_3                   0x230 // This gets or'd with board address


#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_SYSTEM_ERROR_0                0x280 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_SYSTEM_ERROR_1                0x290 // This gets or'd with board address




#define ETM_CAN_DATA_LOG_REGISTER_HV_LAMBDA_FAST_PROGRAM_VOLTAGE        ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_0 | ETM_CAN_ADDR_HV_LAMBDA_BOARD
#define ETM_CAN_DATA_LOG_REGISTER_HV_LAMBDA_SLOW_SET_POINT              ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_1 | ETM_CAN_ADDR_HV_LAMBDA_BOARD

#define ETM_CAN_DATA_LOG_REGISTER_ION_PUMP_SLOW_MONITORS                0x1C

#define ETM_CAN_DATA_LOG_REGISTER_AFC_FAST_POSITION                     0x5C
#define ETM_CAN_DATA_LOG_REGISTER_AFC_FAST_READINGS                     0x5D
#define ETM_CAN_DATA_LOG_REGISTER_AFC_SLOW_SETTINGS                     0x5E

#define ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_FAST_PREVIOUS_PULSE     0x2C
#define ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_SLOW_FILTERED_PULSE     0x2D
#define ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_SLOW_ARCS               0x2E
#define ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_SLOW_PULSE_COUNT        0x2F

#define ETM_CAN_DATA_LOG_REGISTER_COOLING_SLOW_FLOW_0                   0x6C
#define ETM_CAN_DATA_LOG_REGISTER_COOLING_SLOW_FLOW_1                   0x6D
#define ETM_CAN_DATA_LOG_REGISTER_COOLING_SLOW_ANALOG_READINGS          0x6E

#define ETM_CAN_DATA_LOG_REGISTER_HEATER_MAGNET_SLOW_READINGS           0x7C
#define ETM_CAN_DATA_LOG_REGISTER_HEATER_MAGNET_SLOW_SET_POINTS         0x7D

#define ETM_CAN_DATA_LOG_REGISTER_GUN_DRIVER_SLOW_PULSE_TOP_MON         0x8C
#define ETM_CAN_DATA_LOG_REGISTER_GUN_DRIVER_SLOW_HEATER_MON            0x8D
#define ETM_CAN_DATA_LOG_REGISTER_GUN_DRIVER_SLOW_SET_POINTS            0x8E
#define ETM_CAN_DATA_LOG_REGISTER_GUN_DRIVER_FPGA_DATA                  0x8F          

#define ETM_CAN_DATA_LOG_REGISTER_PULSE_SYNC_FAST_TRIGGER_DATA          0x3C
#define ETM_CAN_DATA_LOG_REGISTER_PULSE_SYNC_SLOW_TIMING_DATA_0         0x3D
#define ETM_CAN_DATA_LOG_REGISTER_PULSE_SYNC_SLOW_TIMING_DATA_1         0x3E
#define ETM_CAN_DATA_LOG_REGISTER_PULSE_SYNC_SLOW_TIMING_DATA_2         0x3F


// This allows the module to be configured to use CAN1 or CAN2  (CAN1 is default)
/*
#ifdef __USE_CAN_2

#define _CXIE                       _C2IE
#define _CXIF                       _C2IF
#define _CXIP                       _C2IP
#define CXTX0CON                    C2TX0CON
#define CXTX1CON                    C2TX1CON
#define CXINTF                      C2INTF
#define CXCTRL                      C2CTRL
#define CXCFG1                      C2CFG1
#define CXCFG2                      C2CFG2
#define CXRXM0SID                   C2RXM0SID
#define CXRXM1SID                   C2RXM1SID
#define CXRXF0SID                   C2RXF0SID
#define CXRXF1SID                   C2RXF1SID
#define CXRXF2SID                   C2RXF2SID
#define CXRXF3SID                   C2RXF3SID
#define CXRXF4SID                   C2RXF4SID
#define CXRXF5SID                   C2RXF5SID
#define CXTX2CON                    C2TX2CON
#define CXTX0DLC                    C2TX0DLC
#define CXTX1DLC                    C2TX1DLC
#define CXTX2DLC                    C2TX2DLC
#define CXRX0CON                    C2RX0CON
#define CXRX1CON                    C2RX1CON
#define CXRX1SID                    C2RX1SID
#define CXRX1B1                     C2RX1B1
#define CXEC                        C2EC
#define CXRX1CONbits                C2RX1CONbits
#define CXCTRLbits                  C2CTRLbits
#define CXTX0CONbits                C2TX0CONbits
#define CXTX1CONbits                C2TX1CONbits
#define CXINTFbits                  C2INTFbits
#define CXRX0CONbits                C2RX0CONbits
#define CXINTEbits                  C2INTEbits
#define _CXInterrupt                _C2Interrupt

#else

#define _CXIE                       _C1IE
#define _CXIF                       _C1IF
#define _CXIP                       _C1IP
#define CXTX0CON                    C1TX0CON
#define CXTX1CON                    C1TX1CON
#define CXINTF                      C1INTF
#define CXCTRL                      C1CTRL
#define CXCFG1                      C1CFG1
#define CXCFG2                      C1CFG2
#define CXRXM0SID                   C1RXM0SID
#define CXRXM1SID                   C1RXM1SID
#define CXRXF0SID                   C1RXF0SID
#define CXRXF1SID                   C1RXF1SID
#define CXRXF2SID                   C1RXF2SID
#define CXRXF3SID                   C1RXF3SID
#define CXRXF4SID                   C1RXF4SID
#define CXRXF5SID                   C1RXF5SID
#define CXTX2CON                    C1TX2CON
#define CXTX0DLC                    C1TX0DLC
#define CXTX1DLC                    C1TX1DLC
#define CXTX2DLC                    C1TX2DLC
#define CXRX0CON                    C1RX0CON
#define CXRX1CON                    C1RX1CON
#define CXRX1SID                    C1RX1SID
#define CXRX1B1                     C1RX1B1
#define CXEC                        C1EC
#define CXRX1CONbits                C1RX1CONbits
#define CXCTRLbits                  C1CTRLbits
#define CXTX0CONbits                C1TX0CONbits
#define CXTX1CONbits                C1TX1CONbits
#define CXINTFbits                  C1INTFbits
#define CXRX0CONbits                C1RX0CONbits
#define CXINTEbits                  C1INTEbits
#define _CXInterrupt                _C1Interrupt


#endif
*/


// Can interrupt ISR for slave modules
#define BUFFER_FULL_BIT    0x0080
#define FILTER_SELECT_BIT  0x0001
#define TX_REQ_BIT         0x0008
#define RX0_INT_FLAG_BIT   0xFFFE
#define RX1_INT_FLAG_BIT   0xFFFD
#define ERROR_FLAG_BIT     0x0020
  



//#define MacroETMCanCheckTXBuffer() if (!CXTX0CONbits.TXREQ) { _CXIF = 1; }
// DPARKER this macro needs to be extended to work with both CAN PORTS
#define MacroETMCanCheckTXBuffer() if (!(*CXTX0CON_ptr & TX_REQ_BIT)) { if (_C1IE) {_C1IF = 1;} else {_C2IF = 1;}}




#define CAN_PORT_2  2
#define CAN_PORT_1  1

#endif
