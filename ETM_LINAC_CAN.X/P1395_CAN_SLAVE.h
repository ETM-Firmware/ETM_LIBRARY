#ifndef __P1395_CAN_SLAVE_PUBLIC_H
#define __P1395_CAN_SLAVE_PUBLIC_H

#include "P1395_CAN_CORE.h"

#define P1395_CAN_SLAVE_VERSION   0x0017

//------------ SLAVE PUBLIC FUNCTIONS AND VARIABLES ------------------- //

// Public Functions
void ETMCanSlaveInitialize(unsigned int requested_can_port, unsigned long fcy, unsigned int etm_can_address,
			   unsigned long can_operation_led, unsigned int can_interrupt_priority,
			   unsigned long flash_led, unsigned long not_ready_led);
/*
  This is called once when the processor starts up to initialize the can bus and all of the can variables
*/


void ETMCanSlaveLoadConfiguration(unsigned long agile_id, unsigned int agile_dash,
				  unsigned int firmware_agile_rev, unsigned int firmware_branch, 
				  unsigned int firmware_branch_rev);
/*
  This is called once when the prcoessor starts up to load the board configuration into RAM so it can be sent over CAN to the ECB
*/


void ETMCanSlaveDoCan(void);
/*
  This function should be called every time through the processor execution loop (which should be on the order of 10-100s of uS)
  If will do the following
  1) Look for an execute can commands
  2) Look for changes in status bits, update the Fault/Inhibit bits and send out a new status command if nessesary
  3) Send out regularly schedule communication (On slaves this is status update and logging data)
*/


void ETMCanSlaveLogPulseData(unsigned int packet_id, unsigned int word3, unsigned int word2, unsigned int word1, unsigned int word0);
/*
  This is used to log pulse by pulse data
*/


unsigned int ETMCanSlaveGetPulseLevel(void);
/*
  Returns the next pulse level (as set by the pulse sync board)
  Returns 0x0000 if the next pulse should be low energy
  Returns 0xFFFF if the next pulse should be high energy
*/

#define ETMCanSlaveIsNextPulseLevelHigh ETMCanSlaveGetPulseLevel
// Alternate name for ETMCanSlaveGetPulseLevel


void ETMCanSlaveSetPulseLevelLow(void);
/*
  Sets the local pulse level to low.
  This is used by the High Voltage Lambda to start charging to low energy until the
  "Pulse Level Command" is received from the pulse sync board
*/


unsigned int ETMCanSlaveGetPulseCount(void);
/*
  Returns the next pulse count (as set by the pulse sync board)
*/


unsigned int ETMCanSlaveGetComFaultStatus(void);
/*
  Returns 0 if communication with the ECB is normal
  Returns 1 if the communication with the ECB has timed out
*/

void ETMCanSlaveSetDebugRegister(unsigned int debug_register, unsigned int debug_value);
/*
  There are 16 debug registers, 0x0->0xF
  This stores the value to a particular register
  This information is available on the GUI and should be used to display real time inforamtion to help in debugging.
*/


unsigned int ETMCanSlaveGetSyncMsgResetEnable(void);
unsigned int ETMCanSlaveGetSyncMsgHighSpeedLogging(void);
unsigned int ETMCanSlaveGetSyncMsgPulseSyncDisableHV(void);
unsigned int ETMCanSlaveGetSyncMsgPulseSyncDisableXray(void);
unsigned int ETMCanSlaveGetSyncMsgSystemHVDisable(void);
unsigned int ETMCanSlaveGetSyncMsgCoolingFault(void);
unsigned int ETMCanSlaveGetSyncMsgClearDebug(void);
unsigned int ETMCanSlaveGetSyncMsgPulseSyncWarmupLED(void);
unsigned int ETMCanSlaveGetSyncMsgPulseSyncStandbyLED(void);
unsigned int ETMCanSlaveGetSyncMsgPulseSyncReadyLED(void);
unsigned int ETMCanSlaveGetSyncMsgPulseSyncFaultLED(void);
/*
  All of these 
  will return 0xFFFF if the corresponding bit is set in the sync Message
  will return 0x0000 otherwise
*/

unsigned int ETMCanSlaveGetSyncMsgECBState(void);
/*
  Returns the ECB state from the latest sync message
*/


void ETMCanSlaveIncrementInvalidIndex(void);
/* 
   Increments the Invalid Index Debugging counter
*/


// Only used by Pulse Sync Board
void ETMCanSlavePulseSyncSendNextPulseLevel(unsigned int next_pulse_level, unsigned int next_pulse_count, unsigned int rep_rate_deci_herz);


// Only used by Ion Pump Board
void ETMCanIonPumpSendTargetCurrentReading(unsigned int target_current_reading, unsigned int energy_level, unsigned int pulse_count);






extern ETMCanBoardData           slave_board_data;            // This contains information that is always mirrored on ECB


// --------------------------- #defines to access the status register --------------------------- //

#define _CONTROL_NOT_READY            slave_board_data.status.control_notice_bits.control_not_ready
#define _CONTROL_NOT_CONFIGURED       slave_board_data.status.control_notice_bits.control_not_configured
#define _CONTROL_SELF_CHECK_ERROR     slave_board_data.status.control_notice_bits.control_self_check_error
#define _CONTROL_3                    slave_board_data.status.control_notice_bits.control_3_unused
#define _CONTROL_4                    slave_board_data.status.control_notice_bits.control_4_unused
#define _CONTROL_5                    slave_board_data.status.control_notice_bits.control_5_unused
#define _CONTROL_6                    slave_board_data.status.control_notice_bits.control_6_unused
#define _CONTROL_7                    slave_board_data.status.control_notice_bits.control_7_unused


#define _NOTICE_0                     slave_board_data.status.control_notice_bits.notice_0
#define _NOTICE_1                     slave_board_data.status.control_notice_bits.notice_1
#define _NOTICE_2                     slave_board_data.status.control_notice_bits.notice_2
#define _NOTICE_3                     slave_board_data.status.control_notice_bits.notice_3
#define _NOTICE_4                     slave_board_data.status.control_notice_bits.notice_4
#define _NOTICE_5                     slave_board_data.status.control_notice_bits.notice_5
#define _NOTICE_6                     slave_board_data.status.control_notice_bits.notice_6
#define _NOTICE_7                     slave_board_data.status.control_notice_bits.notice_7

#define _FAULT_0                      slave_board_data.status.fault_bits.fault_0
#define _FAULT_1                      slave_board_data.status.fault_bits.fault_1
#define _FAULT_2                      slave_board_data.status.fault_bits.fault_2
#define _FAULT_3                      slave_board_data.status.fault_bits.fault_3
#define _FAULT_4                      slave_board_data.status.fault_bits.fault_4
#define _FAULT_5                      slave_board_data.status.fault_bits.fault_5
#define _FAULT_6                      slave_board_data.status.fault_bits.fault_6
#define _FAULT_7                      slave_board_data.status.fault_bits.fault_7
#define _FAULT_8                      slave_board_data.status.fault_bits.fault_8
#define _FAULT_9                      slave_board_data.status.fault_bits.fault_9
#define _FAULT_A                      slave_board_data.status.fault_bits.fault_A
#define _FAULT_B                      slave_board_data.status.fault_bits.fault_B
#define _FAULT_C                      slave_board_data.status.fault_bits.fault_C
#define _FAULT_D                      slave_board_data.status.fault_bits.fault_D
#define _FAULT_E                      slave_board_data.status.fault_bits.fault_E
#define _FAULT_F                      slave_board_data.status.fault_bits.fault_F

#define _LOGGED_FAULT_0               _FAULT_0
#define _LOGGED_FAULT_1               _FAULT_1
#define _LOGGED_FAULT_2               _FAULT_2
#define _LOGGED_FAULT_3               _FAULT_3
#define _LOGGED_FAULT_4               _FAULT_4
#define _LOGGED_FAULT_5               _FAULT_5
#define _LOGGED_FAULT_6               _FAULT_6
#define _LOGGED_FAULT_7               _FAULT_7
#define _LOGGED_FAULT_8               _FAULT_8
#define _LOGGED_FAULT_9               _FAULT_9
#define _LOGGED_FAULT_A               _FAULT_A
#define _LOGGED_FAULT_B               _FAULT_B
#define _LOGGED_FAULT_C               _FAULT_C
#define _LOGGED_FAULT_D               _FAULT_D
#define _LOGGED_FAULT_E               _FAULT_E
#define _LOGGED_FAULT_F               _FAULT_F


#define _WARNING_0                    slave_board_data.status.warning_bits.warning_0
#define _WARNING_1                    slave_board_data.status.warning_bits.warning_1
#define _WARNING_2                    slave_board_data.status.warning_bits.warning_2
#define _WARNING_3                    slave_board_data.status.warning_bits.warning_3
#define _WARNING_4                    slave_board_data.status.warning_bits.warning_4
#define _WARNING_5                    slave_board_data.status.warning_bits.warning_5
#define _WARNING_6                    slave_board_data.status.warning_bits.warning_6
#define _WARNING_7                    slave_board_data.status.warning_bits.warning_7
#define _WARNING_8                    slave_board_data.status.warning_bits.warning_8
#define _WARNING_9                    slave_board_data.status.warning_bits.warning_9
#define _WARNING_A                    slave_board_data.status.warning_bits.warning_A
#define _WARNING_B                    slave_board_data.status.warning_bits.warning_B
#define _WARNING_C                    slave_board_data.status.warning_bits.warning_C
#define _WARNING_D                    slave_board_data.status.warning_bits.warning_D
#define _WARNING_E                    slave_board_data.status.warning_bits.warning_E
#define _WARNING_F                    slave_board_data.status.warning_bits.warning_F

#define _LOGGED_STATUS_0              _WARNING_0
#define _LOGGED_STATUS_1              _WARNING_1
#define _LOGGED_STATUS_2              _WARNING_2
#define _LOGGED_STATUS_3              _WARNING_3
#define _LOGGED_STATUS_4              _WARNING_4
#define _LOGGED_STATUS_5              _WARNING_5
#define _LOGGED_STATUS_6              _WARNING_6
#define _LOGGED_STATUS_7              _WARNING_7
#define _LOGGED_STATUS_8              _WARNING_8
#define _LOGGED_STATUS_9              _WARNING_9
#define _LOGGED_STATUS_A              _WARNING_A
#define _LOGGED_STATUS_B              _WARNING_B
#define _LOGGED_STATUS_C              _WARNING_C
#define _LOGGED_STATUS_D              _WARNING_D
#define _LOGGED_STATUS_E              _WARNING_E
#define _LOGGED_STATUS_F              _WARNING_F


#define _NOT_LOGGED_0                 slave_board_data.status.not_logged_bits.not_logged_0
#define _NOT_LOGGED_1                 slave_board_data.status.not_logged_bits.not_logged_1
#define _NOT_LOGGED_2                 slave_board_data.status.not_logged_bits.not_logged_2
#define _NOT_LOGGED_3                 slave_board_data.status.not_logged_bits.not_logged_3
#define _NOT_LOGGED_4                 slave_board_data.status.not_logged_bits.not_logged_4
#define _NOT_LOGGED_5                 slave_board_data.status.not_logged_bits.not_logged_5
#define _NOT_LOGGED_6                 slave_board_data.status.not_logged_bits.not_logged_6
#define _NOT_LOGGED_7                 slave_board_data.status.not_logged_bits.not_logged_7
#define _NOT_LOGGED_8                 slave_board_data.status.not_logged_bits.not_logged_8
#define _NOT_LOGGED_9                 slave_board_data.status.not_logged_bits.not_logged_9
#define _NOT_LOGGED_A                 slave_board_data.status.not_logged_bits.not_logged_A
#define _NOT_LOGGED_B                 slave_board_data.status.not_logged_bits.not_logged_B
#define _NOT_LOGGED_C                 slave_board_data.status.not_logged_bits.not_logged_C
#define _NOT_LOGGED_D                 slave_board_data.status.not_logged_bits.not_logged_D
#define _NOT_LOGGED_E                 slave_board_data.status.not_logged_bits.not_logged_E
#define _NOT_LOGGED_F                 slave_board_data.status.not_logged_bits.not_logged_F

#define _NOT_LOGGED_STATUS_0          _NOT_LOGGED_0
#define _NOT_LOGGED_STATUS_1          _NOT_LOGGED_1
#define _NOT_LOGGED_STATUS_2          _NOT_LOGGED_2
#define _NOT_LOGGED_STATUS_3          _NOT_LOGGED_3
#define _NOT_LOGGED_STATUS_4          _NOT_LOGGED_4
#define _NOT_LOGGED_STATUS_5          _NOT_LOGGED_5
#define _NOT_LOGGED_STATUS_6          _NOT_LOGGED_6
#define _NOT_LOGGED_STATUS_7          _NOT_LOGGED_7
#define _NOT_LOGGED_STATUS_8          _NOT_LOGGED_8
#define _NOT_LOGGED_STATUS_9          _NOT_LOGGED_9
#define _NOT_LOGGED_STATUS_A          _NOT_LOGGED_A
#define _NOT_LOGGED_STATUS_B          _NOT_LOGGED_B
#define _NOT_LOGGED_STATUS_C          _NOT_LOGGED_C
#define _NOT_LOGGED_STATUS_D          _NOT_LOGGED_D
#define _NOT_LOGGED_STATUS_E          _NOT_LOGGED_E
#define _NOT_LOGGED_STATUS_F          _NOT_LOGGED_F



#define _CONTROL_REGISTER             *(unsigned int*)&slave_board_data.status.control_notice_bits
#define _FAULT_REGISTER               *(unsigned int*)&slave_board_data.status.fault_bits
#define _WARNING_REGISTER             *(unsigned int*)&slave_board_data.status.warning_bits
#define _NOT_LOGGED_REGISTER          *(unsigned int*)&slave_board_data.status.not_logged_bits

// ------------------------- #defines to access the board log and configuration data -------------------------------- //
/*
  24 Words of board data
  8  Words of board configuration
*/

// Board data - 0x00
#define board_data_msg_0_word_0 slave_board_data.log_data[0]
#define board_data_msg_0_word_1 slave_board_data.log_data[1]
#define board_data_msg_0_word_2 slave_board_data.log_data[2]
#define board_data_msg_0_word_3 slave_board_data.log_data[3]

// Board data - 0x01
#define board_data_msg_1_word_0 slave_board_data.log_data[4]
#define board_data_msg_1_word_1 slave_board_data.log_data[5]
#define board_data_msg_1_word_2 slave_board_data.log_data[6]
#define board_data_msg_1_word_3 slave_board_data.log_data[7]

// Board data - 0x02
#define board_data_msg_2_word_0 slave_board_data.log_data[8]
#define board_data_msg_2_word_1 slave_board_data.log_data[9]
#define board_data_msg_2_word_2 slave_board_data.log_data[10]
#define board_data_msg_2_word_3 slave_board_data.log_data[11]

// Board data - 0x03
#define board_data_msg_3_word_0 slave_board_data.log_data[12]
#define board_data_msg_3_word_1 slave_board_data.log_data[13]
#define board_data_msg_3_word_2 slave_board_data.log_data[14]
#define board_data_msg_3_word_3 slave_board_data.log_data[15]

// Board data - 0x04
#define board_data_msg_4_word_0 slave_board_data.log_data[16]
#define board_data_msg_4_word_1 slave_board_data.log_data[17]
#define board_data_msg_4_word_2 slave_board_data.log_data[18]
#define board_data_msg_4_word_3 slave_board_data.log_data[19]

// Board data - 0x05
#define board_data_msg_5_word_0 slave_board_data.log_data[20]
#define board_data_msg_5_word_1 slave_board_data.log_data[21]
#define board_data_msg_5_word_2 slave_board_data.log_data[22]
#define board_data_msg_5_word_3 slave_board_data.log_data[23]



#endif
