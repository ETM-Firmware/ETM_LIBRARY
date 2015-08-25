#ifndef __P1395_CAN_SLAVE_PUBLIC_H
#define __P1395_CAN_SLAVE_PUBLIC_H

#include "P1395_CAN_CORE.h"

extern const unsigned int p1395_can_slave_version;
extern unsigned int etm_can_slave_com_loss;

//------------ SLAVE PUBLIC FUNCTIONS AND VARIABLES ------------------- //

// Public Functions
void ETMCanSlaveInitialize(unsigned int requested_can_port, unsigned long fcy, unsigned int etm_can_address, unsigned long can_operation_led, unsigned int can_interrupt_priority);
/*
  This is called once when the processor starts up to initialize the can bus and all of the can variables
*/

void ETMCanSlaveLoadConfiguration(unsigned long agile_id, unsigned int agile_dash, unsigned int agile_rev, unsigned int firmware_agile_rev, unsigned int firmware_branch, unsigned int firmware_branch_rev, unsigned int serial_number);
/*
  This is called once when the prcoessor starts up to load the board configuration into RAM so it can be sent over CAN to the ECB
*/

// DPARKER how do you set the agile rev and the serial number?? Should this be done over can?



void ETMCanSlaveDoCan(void);
/*
  This function should be called every time through the processor execution loop (which should be on the order of 10-100s of uS)
  If will do the following
  1) Look for an execute can commands
  2) Look for changes in status bits, update the Fault/Inhibit bits and send out a new status command if nessesary
  3) Send out regularly schedule communication (On slaves this is status update and logging data)
*/


void ETMCanSlaveLogBoardData(unsigned int data_register);
/*
  This is use to manualy send a data logging message
*/

// Only for Pulse Sync Board
void ETMCanSlavePulseSyncSendNextPulseLevel(unsigned int next_pulse_level, unsigned int next_pulse_count);


// Only for Ion Pump Board
void ETMCanIonPumpSendTargetCurrentReading(unsigned int target_current_reading, unsigned int energy_level, unsigned int pulse_count);


#endif
