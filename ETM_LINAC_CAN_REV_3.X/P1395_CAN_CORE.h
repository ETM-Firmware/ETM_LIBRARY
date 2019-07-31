#ifndef __P1395_CAN_CORE_H
#define __P1395_CAN_CORE_H

// ---------------------- STATUS REGISTER DEFFENITIONS   ------------------------- //
#define CAN_REV 3

typedef struct {
  unsigned control_not_ready:1;
  unsigned control_not_configured:1;
  unsigned control_self_check_error:1;
  unsigned control_3_unused:1;
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



typedef struct {
  ETMCanStatusRegister    status;             // This is 4 words of status data for the slave

  // Can data log - 0x00Z -> 0x05Z
  unsigned int            log_data[24];       // This is 24 words (6 registers) of logging data passed to the GUI
                                              // This data should be managed by the user application program
                                              // use #define to map the relevant variables to these locations

  // Can data log - 0x06Z
  unsigned int            device_id_high_word;
  unsigned int            device_id_low_word;
  unsigned int            device_id_dash_number;
  unsigned int            device_rev_2x_ASCII;

  // Can data log - 0x07Z
  unsigned int            device_serial_number;
  unsigned int            device_firmware_rev_agile;
  unsigned int            device_firmware_branch;
  unsigned int            device_firmware_branch_rev;

  unsigned int            connection_timeout;                // On the ECB this is used to flag if the board connection has timed out or not.
  unsigned long           time_last_status_message_recieved; // On the ECB this is used to track the last time a status message was recieved from this board.  
  
  //unsigned int            cmd_data[12];     // On the ECB, this stores data to be sent to the slave boards, on slave boards, this stores the data recieved from ECB
                                            // This should be read only to the user application program
} ETMCanBoardData;







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
  unsigned st_trigger_fiber;
  unsigned st_spare_3:1;
  unsigned st_spare_2:1;
  unsigned st_spare_1:1;
  unsigned st_spare_0:1;
} ETMCanSelfTestRegister;


// --------------- Board Logging Data ----------------------- //
/*
  24 Words of board data
  8  Words of board configuration
  This is mirrored for all slave boards on the ECB
*/




// ------------ Debug Data log Structure ----------------- //
/*
  16 words of board specific debug data (this is used by the software developer)
  24 words of predefined debugging information (Reset, CAN)
  4  words of RAM monitor (ability to select RAM physical locations for monitoring)
  4  words of EEPROM Monitor
  16 words of predefined debugging (TBD what values, things like 5V, 24V, ect)
  This data is only mirrored on the ECB for a single board - the "active" window pane
*/

typedef struct {
  unsigned int debug_reg[16];       // 0x1CZ -> 0x1FZ

  // Can data log - 0x20Z
  unsigned int ram_monitor_a;
  unsigned int ram_monitor_b;
  unsigned int ram_monitor_c;
  unsigned int eeprom_read_result;

  // Can data log - 0x21Z
  unsigned int analog_1_nominal_5V;
  unsigned int analog_2_nominal_pos_15;
  unsigned int analog_3_nominal_neg_15;
  unsigned int analog_4_nominal_24;
  
  // Can data log - 0x22Z
  unsigned int can_tx_0;            // count of tx_0 transmits
  unsigned int can_tx_1;            // count of tx_1 transmits
  unsigned int can_tx_2;            // count of tx_2 transmits
  unsigned int CXEC_reg_max;        // MAX instead of instantaneous value of CXEC
  
  // Can data log - 0x23Z
  unsigned int can_rx_0_filt_0;     // count of messages received by this filter
  unsigned int can_rx_0_filt_1;     // count of messages received by this filter
  unsigned int can_rx_1_filt_2;     // count of messages received by this filter
  unsigned int CXINTF_max;          // logical or of the CXINTF register every time the can ISR is entered

  // Can data log - 0x24Z
  unsigned int can_unknown_msg_id;  // NOT POSSIBLE SLAVE// count of the number of unknown message ids
  unsigned int can_invalid_index;   // count of the number of invalid message index
  unsigned int can_address_error;   // NOT POSSIBLE SLAVE // count of the number of received messages not addressed to this board
  unsigned int can_error_flag;      // counts the number of CAN error flags interrupts

  // Can data log - 0x25Z
  unsigned int can_tx_buf_overflow; // overwrite count on etm_can_tx_message_buffer 
  unsigned int can_rx_buf_overflow; // overwrite count on etm_can_rx_message_buffer
  unsigned int can_rx_log_buf_overflow; // MASTER ONLY - overwrite count on the logging data buffer overflow count
  unsigned int can_timeout;         // count of the number of can timeouts

  // Can data log - 0x26Z
  unsigned int eeprom_internal_read_count;
  unsigned int eeprom_internal_read_error;
  unsigned int eeprom_internal_write_count;
  unsigned int eeprom_internal_write_error;

  // Can data log - 0x27Z
  unsigned int eeprom_i2c_read_count;
  unsigned int eeprom_i2c_read_error;
  unsigned int eeprom_i2c_write_count;
  unsigned int eeprom_i2c_write_error;

  // Can data log - 0x28Z
  unsigned int eeprom_spi_read_count;
  unsigned int eeprom_spi_read_error;
  unsigned int eeprom_spi_write_count;
  unsigned int eeprom_spi_write_error;

  // Can data log - 0x29Z
  unsigned int eeprom_crc_error_count;
  unsigned int cmd_data_register_read_invalid_index;
  unsigned int debugging_TBD_17;    // Debugging TBD
  unsigned int debugging_TBD_16;    // Debugging TBD

  // Can data log - 0x2AZ
  unsigned int reset_count;
  unsigned int RCON_value;
  unsigned int can_build_version;
  unsigned int library_build_version;
  
  // Can data log - 0x2BZ
  unsigned int i2c_bus_error_count;
  unsigned int spi_bus_error_count;
  unsigned int scale_error_count;
  ETMCanSelfTestRegister self_test_results;
  
  // Can data log - 0x2CZ
  unsigned int debugging_TBD_15;     // Debugging TBD
  unsigned int debugging_TBD_14;     // Debugging TBD
  unsigned int debugging_TBD_13;     // Debugging TBD
  unsigned int debugging_TBD_12;     // Debugging TBD

  // Can data log - 0x2DZ
  unsigned int debugging_TBD_11;     // Debugging TBD
  unsigned int debugging_TBD_10;     // Debugging TBD
  unsigned int debugging_TBD_9;      // Debugging TBD
  unsigned int debugging_TBD_8;      // Debugging TBD

  // Can data log - 0x2EZ
  unsigned int debugging_TBD_7;      // Debugging TBD
  unsigned int debugging_TBD_6;      // Debugging TBD
  unsigned int debugging_TBD_5;      // Debugging TBD
  unsigned int debugging_TBD_4;      // Debugging TBD

  // Can data log - 0x2FZ
  unsigned int debugging_TBD_3;      // Debugging TBD
  unsigned int debugging_TBD_2;      // Debugging TBD
  unsigned int debugging_TBD_1;      // Debugging TBD
  unsigned int debugging_TBD_0;      // Debugging TBD

  // Can data log - 0x30Z
  unsigned int calibration_0_internal_gain;
  unsigned int calibration_0_internal_offset;
  unsigned int calibration_0_external_gain;
  unsigned int calibration_0_external_offset;

  // Can data log - 0x31Z
  unsigned int calibration_1_internal_gain;
  unsigned int calibration_1_internal_offset;
  unsigned int calibration_1_external_gain;
  unsigned int calibration_1_external_offset;

  // Can data log - 0x32Z
  unsigned int calibration_2_internal_gain;
  unsigned int calibration_2_internal_offset;
  unsigned int calibration_2_external_gain;
  unsigned int calibration_2_external_offset;

  // Can data log - 0x33Z
  unsigned int calibration_3_internal_gain;
  unsigned int calibration_3_internal_offset;
  unsigned int calibration_3_external_gain;
  unsigned int calibration_3_external_offset;

  // Can data log - 0x34Z
  unsigned int calibration_4_internal_gain;
  unsigned int calibration_4_internal_offset;
  unsigned int calibration_4_external_gain;
  unsigned int calibration_4_external_offset;
  
  // Can data log - 0x35Z
  unsigned int calibration_5_internal_gain;
  unsigned int calibration_5_internal_offset;
  unsigned int calibration_5_external_gain;
  unsigned int calibration_5_external_offset;
  
  // Can data log - 0x36Z
  unsigned int calibration_6_internal_gain;
  unsigned int calibration_6_internal_offset;
  unsigned int calibration_6_external_gain;
  unsigned int calibration_6_external_offset;
  
  // Can data log - 0x37Z
  unsigned int calibration_7_internal_gain;
  unsigned int calibration_7_internal_offset;
  unsigned int calibration_7_external_gain;
  unsigned int calibration_7_external_offset;
    
} ETMCanBoardDebuggingData;


// -------------------------  SYNC MESSAGE DEFFENITIONS ----------------------- //

typedef struct {
  unsigned sync_0_reset_enable:1;
  unsigned sync_1_high_speed_logging_enabled:1;
  unsigned sync_2_unused:1;
  unsigned sync_3_unused:1;
  unsigned sync_4_cooling_fault:1;
  unsigned sync_5_system_hv_disable:1;
  unsigned sync_6_gun_driver_disable_heater:1;
  unsigned sync_7_unused:1;

  unsigned sync_8_unused:1;
  unsigned sync_9_unused:1;
  unsigned sync_A_unused:1;
  unsigned sync_B_unused:1;
  unsigned sync_C_interleave_energy:1;
  unsigned sync_D_scope_HV_HVMON_active:1;
  unsigned sync_E_ingnore_faults_enabled:1;
  unsigned sync_F_clear_debug_data:1;
} ETMCanSyncControlWord;


typedef struct {
  ETMCanSyncControlWord sync_0_control_word;
  unsigned char pulse_count;
  unsigned char next_energy_level;
  unsigned int  prf_from_ecb; 
  unsigned char scope_A_select;
  unsigned char scope_B_select;
} ETMCanSyncMessage;


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


// RECEIVE MODE                                  0bXXXCCCCDDDDDDDX0


// --------------- MASTER ---------------------- 
// RX0  Mask and Filters 

#define ETM_CAN_MASTER_RX0_MASK                  0b0001111000000000
//                                               0bXXX0000NNNNNNNX0
#define ETM_CAN_MASTER_MSG_FILTER_RF0            0b0000100000000000  // This is used to recieve RTSP response - Action TBD
//                                               0bXXX0001NNAAAAAX0
#define ETM_CAN_MASTER_MSG_FILTER_RF1            0b0000010000000000  // This will accept STATUS message from slaves.  Needs to be or'ed with ECB ADDRESS << 2

// RX1  Mask and Filters
//                                               0bXXX1PPPPPPAAAAX0
#define ETM_CAN_MASTER_RX1_MASK                  0b0001000000000000  // This is used by master to receive LOG messages
#define ETM_CAN_MASTER_MSG_FILTER_RF2            0b0001000000000000  // This will accept LOG msg from slaves


// --------------- SLAVE -----------------------
// RX0 Mask and Filters
#define ETM_CAN_SLAVE_RX0_MASK                   0b0001111000000000  
//                                               0bXXX0000NNNNNNNX0
#define ETM_CAN_SLAVE_MSG_FILTER_RF0             0b0000000000000000 // This will accept the SYNC/LVL broadcast
//                                               0bXXX0010NNNNNNNX0
#define ETM_CAN_SLAVE_MSG_FILTER_RF1             0b0000010000000000 // This will accept the cmd messages

// RX1  Mask and Filters
//                                               0bXXX111xTTTAAAAX0
#define ETM_CAN_SLAVE_RX1_MASK                   0b0000110000111100  // This is used by slaves to receive self Program messages
#define ETM_CAN_SLAVE_MSG_FILTER_RF2             0b0000110000000000  // When or'ed with address this will accept self program commands




// ------- Define the bits for particular commands ---------------- 
// The TX and RX SID buffers within the PIC are not mapped the same.
// That is why the RX and TX are defined seperately


// Define TX SID VALUES
// MASTER TRASMIT MODE                           0bCCCCxXXXxxxxxx00
#define ETM_CAN_MSG_SYNC_TX                      0b0000000000000000
//                                               0bCCCCxXXXPPPPPP00
#define ETM_CAN_MSG_CMD_TX                       0b0010000000000000 // When or'ed with bit shifted command ID

// SLAVE TRASMIT MODE                            0bCCCCxXXXxxAAAA00
#define ETM_CAN_MSG_STATUS_TX                    0b0001000000000000 // When or'ed with slave address in the address bits (2-5)
//                                               0b1PPPPXXXPPAAAA00
#define ETM_CAN_MSG_LOG_TX                       0b1000000000000000 // When or'ed with salve address (2-5) and packet ID (6,7,11,12,13,14)

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

#define ETM_CAN_ADDR_ION_PUMP_BOARD                                     0x0000
#define ETM_CAN_ADDR_MAGNETRON_CURRENT_BOARD                            0x0001
#define ETM_CAN_ADDR_HV_LAMBDA_BOARD                                    0x0002
#define ETM_CAN_ADDR_AFC_CONTROL_BOARD                                  0x0003
#define ETM_CAN_ADDR_COOLING_INTERFACE_BOARD                            0x0004
#define ETM_CAN_ADDR_HEATER_MAGNET_BOARD                                0x0005
#define ETM_CAN_ADDR_GUN_DRIVER_BOARD                                   0x0006
#define ETM_CAN_ADDR_TARGET_CURRENT_BOARD                               0x0007
#define ETM_CAN_ADDR_PFN_BOARD                                          0x0008
#define ETM_CAN_ADDR_DOSE_MONITOR_BOARD                                 0x0009
#define ETM_CAN_ADDR_ETHERNET_BOARD                                     0x000F


// Board Specific Register Locations

#define ETM_CAN_CMD_ID_RESET_MCU                        0x00
#define ETM_CAN_CMD_ID_LOAD_DEFAULT_CALIBRATION         0x01
#define ETM_CAN_CMD_ID_LOAD_REV_AND_SERIAL_NUMBER       0x02
#define ETM_CAN_CMD_ID_SET_CAL_PAIR                     0x03
#define ETM_CAN_CMD_ID_SET_RAM_DEBUG                    0x04
#define ETM_CAN_CMD_ID_SET_EEPROM_DEBUG                 0x05
#define ETM_CAN_CMD_ID_SET_IGNORE_FAULTS                0x06

#define ETM_CAN_CMD_ID_HVPS_SET_POINTS                  0x10
#define ETM_CAN_CMD_ID_MAGNET_SET_POINTS                0x11
#define ETM_CAN_CMD_ID_AFC_HOME_POSTION                 0x12
#define ETM_CAN_CMD_ID_GUN_PULSE_TOP_SET_POINTS         0x13
#define ETM_CAN_CMD_ID_GUN_CATHODE_SET_POINTS           0x14
#define ETM_CAN_CMD_ID_ALL_DOSE_SET_POINTS_REGISTER_A   0x15
#define ETM_CAN_CMD_ID_ALL_DOSE_SET_POINTS_REGISTER_B   0x16


#define ETM_CAN_CMD_ID_DISCRETE_CMD                     0x1F                       

#define DISCRETE_CMD_BUFFER_EMPTY                       0x00
#define DISCRETE_CMD_AFC_DO_AUTO_ZERO                   0x01
#define DISCRETE_CMD_AFC_SELECT_MANUAL_MODE             0x02
#define DISCRETE_CMD_AFC_SELECT_AUTOMATIC_MODE          0x03
#define DISCRETE_CMD_COOLING_ALLOW_25_SF6_PULSES        0x04
#define DISCRETE_CMD_COOLING_OVERRIDE_MINIMUM_FILL      0x05
#define DISCRETE_CMD_COOLING_RESET_BOTTLE_COUNT         0x06



//------------------ DATA LOGGING REGISTERS --------------------------//



/*
// Default data logging registers
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_0                      0x000 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_1                      0x010 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_2                      0x020 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_3                      0x030 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_4                      0x040 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_5                      0x050 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_ANALOG_DATA                  0x060 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_RAM_WATCH_DATA                        0x070 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_REGISTER_CONFIG_0                              0x080 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_CONFIG_1                              0x090 // This gets or'd with board address

// 0x0A0 - > 0x0F0 Available future Use

#define ETM_CAN_DATA_LOG_REGISTER_FAST_LOG_0                            0x100 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_FAST_LOG_1                            0x110 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_FAST_LOG_2                            0x120 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_FAST_LOG_3                            0x130 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_SCOPE_A_DATA_16_BIT                            0x140 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_SCOPE_A_DATA_8_BIT                             0x150 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_SCOPE_A_DATA_12_BIT                            0x160 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_SCOPE_A_DATA_10_BIT                            0x170 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_SCOPE_B_DATA_16_BIT                            0x180 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_SCOPE_B_DATA_8_BIT                             0x190 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_SCOPE_B_DATA_12_BIT                            0x1A0 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_SCOPE_B_DATA_10_BIT                            0x1B0 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_SCOPE_PULSE_CURRENT                            0x1C0 // This gets or'd with board address

// 0x1D0 - Unused

#define ETM_CAN_DATA_LOG_SCOPE_HV_VMON_12_BIT                           0x1E0 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_SCOPE_HV_VMON_8_BIT                            0x1F0 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_REGISTER_DEBUG_0                               0x200 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEBUG_1                               0x210 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEBUG_2                               0x220 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_DEBUG_3                               0x230 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_0                      0x240 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_1                      0x250 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_2                      0x260 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_3                      0x270 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_4                      0x280 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_5                      0x290 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_6                      0x2A0 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_7                      0x2B0 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_8                      0x2C0 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_9                      0x2D0 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_10                     0x2E0 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_11                     0x2F0 // This gets or'd with board address

#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_DATA_0                    0x300 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_DATA_1                    0x310 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_DATA_2                    0x320 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_DATA_3                    0x330 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_DATA_4                    0x340 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_DATA_5                    0x350 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_DATA_6                    0x360 // This gets or'd with board address
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_DATA_7                    0x370 // This gets or'd with board address

//0x380 -> 0x3F0 Available for Future Calibration data channels 8->F
*/

// Custom High Speed Data Logging Registers
#define ETM_CAN_DATA_LOG_REGISTER_HV_LAMBDA_FAST_LOG_0                  ETM_CAN_DATA_LOG_REGISTER_FAST_LOG_0 | ETM_CAN_ADDR_HV_LAMBDA_BOARD
#define ETM_CAN_DATA_LOG_REGISTER_MAGNETRON_MON_FAST_LOG_0              ETM_CAN_DATA_LOG_REGISTER_FAST_LOG_0 | ETM_CAN_ADDR_MAGNETRON_CURRENT_BOARD
#define ETM_CAN_DATA_LOG_REGISTER_AFC_FAST_LOG_0                        ETM_CAN_DATA_LOG_REGISTER_FAST_LOG_0 | ETM_CAN_ADDR_AFC_CONTROL_BOARD
#define ETM_CAN_DATA_LOG_REGISTER_TARGET_MON_FAST_LOG_0                 ETM_CAN_DATA_LOG_REGISTER_FAST_LOG_0 | ETM_CAN_ADDR_TARGET_CURRENT_BOARD
#define ETM_CAN_DATA_LOG_REGISTER_DOSE_LOG_0                            ETM_CAN_DATA_LOG_REGISTER_DOSE_LOG_0 | ETM_CAN_ADDR_DOSE_MONITOR_BOARD

// DPARKER NEED TO DECREASE THE AMOUNT OF PULSE BY PULSE DATA LOGGING ON 800HZ SYSTEM.  CONDENSE DATA TO SEND EVERY OTHER PULSE



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
