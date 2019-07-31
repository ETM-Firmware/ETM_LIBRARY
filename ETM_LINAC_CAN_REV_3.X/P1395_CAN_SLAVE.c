#include <xc.h>
#include <libpic30.h>
#include "P1395_CAN_SLAVE.h"
#include "ETM.h"


// DOSE LEVEL DEFINITIONS

#define DOSE_SELECT_REPEATE_DOSE_LEVEL_0           0x11
#define DOSE_SELECT_REPEATE_DOSE_LEVEL_1           0x12
#define DOSE_SELECT_REPEATE_DOSE_LEVEL_2           0x13
#define DOSE_SELECT_REPEATE_DOSE_LEVEL_3           0x14

#define DOSE_SELECT_INTERLEAVE_0_1_DOSE_LEVEL_0    0x21
#define DOSE_SELECT_INTERLEAVE_0_1_DOSE_LEVEL_1    0x22
#define DOSE_SELECT_INTERLEAVE_2_3_DOSE_LEVEL_2    0x23
#define DOSE_SELECT_INTERLEAVE_2_3_DOSE_LEVEL_3    0x24


#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_0               0x000
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_1               0x010
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_2               0x020
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_3               0x030
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_4               0x040
#define ETM_CAN_DATA_LOG_REGISTER_BOARD_SPECIFIC_5               0x050
#define ETM_CAN_DATA_LOG_REGISTER_CONFIG_0                       0x060
#define ETM_CAN_DATA_LOG_REGISTER_CONFIG_1                       0x070


#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_0                0x1C0
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_1                0x1D0
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_2                0x1E0
#define ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_3                0x1F0

#define ETM_CAN_DATA_LOG_REGISTER_RAM_WATCH_DATA                 0x200
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_ANALOG_DATA           0x210

#define ETM_CAN_DATA_LOG_REGISTER_CAN_DEBUG_0                    0x220
#define ETM_CAN_DATA_LOG_REGISTER_CAN_DEBUG_1                    0x230
#define ETM_CAN_DATA_LOG_REGISTER_CAN_DEBUG_2                    0x240
#define ETM_CAN_DATA_LOG_REGISTER_CAN_DEBUG_3                    0x250

#define ETM_CAN_DATA_LOG_REGISTER_EEPROM_INTERNAL_DEBUG          0x260
#define ETM_CAN_DATA_LOG_REGISTER_EEPROM_I2C_DEBUG               0x270
#define ETM_CAN_DATA_LOG_REGISTER_EEPROM_SPI_DEBUG               0x280
#define ETM_CAN_DATA_LOG_REGISTER_EEPROM_CRC_DEBUG               0x290

#define ETM_CAN_DATA_LOG_REGISTER_RESET_VERSION_DEBUG            0x2A0
#define ETM_CAN_DATA_LOG_REGISTER_I2C_SPI_SCALE_SELF_TEST_DEBUG  0x2B0
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_TBD_3           0x2C0
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_TBD_2           0x2D0

#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_TBD_1           0x2E0
#define ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_TBD_0           0x2F0

#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_0                  0x300
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_1                  0x310
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_2                  0x320
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_3                  0x330
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_4                  0x340
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_5                  0x350
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_6                  0x360
#define ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_7                  0x370


#define SLAVE_TRANSMIT_MILLISECONDS       100
#define SLAVE_TIMEOUT_MILLISECONDS        250

#if defined(__dsPIC30F6014A__)
#define RAM_SIZE_WORDS  4096
#endif

#if defined(__dsPIC30F6010A__)
#define RAM_SIZE_WORDS  4096
#endif

#ifndef RAM_SIZE_WORDS
#define RAM_SIZE_WORDS 0
#endif



#define EEPROM_PAGE_ANALOG_CALIBRATION_0_1_2   0x40
#define EEPROM_PAGE_ANALOG_CALIBRATION_3_4_5   0x41
#define EEPROM_PAGE_ANALOG_CALIBRATION_6_7     0x42
#define EEPROM_PAGE_BOARD_CONFIGURATION        0x00




//Local Funcations
static void ETMCanSlaveLoadDefaultCalibration(void);
static void ETMCanSlaveProcessMessage(void);
static void ETMCanSlaveExecuteCMD(ETMCanMessage* message_ptr);           
static void ETMCanSlaveTimedTransmit(void);
static void ETMCanSlaveSendStatus(void);
static void ETMCanSlaveLogData(unsigned int packet_id, unsigned int word3, unsigned int word2, unsigned int word1, unsigned int word0);
static void ETMCanSlaveCheckForTimeOut(void);
static void ETMCanSlaveSendUpdateIfNewNotReady(void);
static void ETMCanSlaveClearDebug(void);
static void DoCanInterrupt(void);  //  Helper function for Can interupt handler



// -------------------- local variables -------------------------- //
static ETMCanBoardData           slave_board_data;            // This contains information that is always mirrored on ECB

static ETMCanBoardDebuggingData  etm_can_slave_debug_data;    // This information is only mirrored on ECB if this module is selected on the GUI
static ETMCanSyncMessage         etm_can_slave_sync_message;  // This is the most recent sync message recieved from the ECB

static ETMCanMessageBuffer etm_can_slave_rx_message_buffer;   // RX message buffer
static ETMCanMessageBuffer etm_can_slave_tx_message_buffer;   // TX Message buffer

static unsigned long etm_can_slave_communication_timeout_holding_var;  // Used to check for timeout of SYNC messsage from the ECB
static unsigned char etm_can_slave_com_loss;  // Keeps track of if communicantion from ECB has timed out
static unsigned int setting_data_recieved;  // This keeps track of which settings registers have been recieved 
static BUFFERBYTE64 discrete_cmd_buffer;  // Buffer for discrete commands that come in.  This is quiered and processed by the main code as needed


typedef struct {
  unsigned int hvps_set_point_dose_level_3;
  unsigned int hvps_set_point_dose_level_2;
  unsigned int hvps_set_point_dose_level_1;
  unsigned int hvps_set_point_dose_level_0;

  unsigned int electromagnet_set_point_dose_level_3;
  unsigned int electromagnet_set_point_dose_level_2;
  unsigned int electromagnet_set_point_dose_level_1;
  unsigned int electromagnet_set_point_dose_level_0;

  unsigned int gun_driver_pulse_top_voltage_dose_level_3;
  unsigned int gun_driver_pulse_top_voltage_dose_level_2;
  unsigned int gun_driver_pulse_top_voltage_dose_level_1;
  unsigned int gun_driver_pulse_top_voltage_dose_level_0;

  unsigned int gun_driver_cathode_voltage_dose_level_3;
  unsigned int gun_driver_cathode_voltage_dose_level_2;
  unsigned int gun_driver_cathode_voltage_dose_level_1;
  unsigned int gun_driver_cathode_voltage_dose_level_0;

  unsigned int afc_home_position_dose_level_3;
  unsigned int afc_home_position_dose_level_2;
  unsigned int afc_home_position_dose_level_1;
  unsigned int afc_home_position_dose_level_0;

  unsigned int magnetron_heater_scaled_heater_current;
  unsigned int afc_aux_control_or_offset;
  unsigned int gun_driver_grid_bias_voltage;
  unsigned int gun_driver_heater_voltage;
  
  unsigned int afc_manual_target_position;
  
} TYPE_SLAVE_DATA;
static TYPE_SLAVE_DATA slave_data;

typedef struct {
  unsigned int reset_count;
  unsigned int can_timeout_count;
} PersistentData;
static volatile PersistentData etm_can_persistent_data __attribute__ ((persistent));

typedef struct {
  unsigned int  address;
  unsigned char analog_eeprom;
  unsigned char eeprom_intialization_error;
  unsigned char eeprom_calibration_write_error;
  unsigned long led;
  unsigned long flash_led;
  unsigned long not_ready_led;
  
} TYPE_CAN_PARAMETERS;
static TYPE_CAN_PARAMETERS can_params;


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
			   unsigned long flash_led, unsigned long not_ready_led, unsigned int analog_calibration_eeprom_location) {

  unsigned int  eeprom_page_data[16];
  unsigned int  eeprom_select_backup;
  
  if (ETMTickNotInitialized()) {
    ETMTickInitialize(fcy, ETM_TICK_USE_TIMER_5);
  }

  BufferByte64Initialize(&discrete_cmd_buffer);

  setting_data_recieved &= 0b00000000;
  _CONTROL_NOT_CONFIGURED = 1;
  
  etm_can_slave_debug_data.can_build_version = P1395_CAN_SLAVE_VERSION;
  etm_can_slave_debug_data.library_build_version = ETM_LIBRARY_VERSION;

  if (can_interrupt_priority > 7) {
    can_interrupt_priority = 7;
  }
  
  can_params.address = etm_can_address;
  can_params.led = can_operation_led;
  can_params.flash_led = flash_led;
  can_params.not_ready_led = not_ready_led;
  can_params.analog_eeprom = analog_calibration_eeprom_location;

  ETMPinTrisOutput(can_params.led);
  ETMPinTrisOutput(can_params.flash_led);
  ETMPinTrisOutput(can_params.not_ready_led);


  etm_can_persistent_data.reset_count++;
  
  *(unsigned int*)&etm_can_slave_sync_message.sync_0_control_word = 0;
  etm_can_slave_sync_message.pulse_count = 0;
  etm_can_slave_sync_message.next_energy_level = 0;
  etm_can_slave_sync_message.prf_from_ecb = 0;
  etm_can_slave_sync_message.scope_A_select = 0;
  etm_can_slave_sync_message.scope_B_select = 0;
  
  
  
  etm_can_slave_com_loss = 0;

  etm_can_slave_debug_data.reset_count = etm_can_persistent_data.reset_count;
  etm_can_slave_debug_data.can_timeout = etm_can_persistent_data.can_timeout_count;

  //ETMCanSlaveClearDebug();  DPARKER you can't call this because it will clear the reset count
  
  ETMCanBufferInitialize(&etm_can_slave_rx_message_buffer);
  ETMCanBufferInitialize(&etm_can_slave_tx_message_buffer);
  
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


  // DPARKER - Who is responsible for EEPROM Select and configuration?  Is that done by this module or higher up???
  // Does the can module need to save the select EEPROM Type?

  eeprom_select_backup = ETMEEPromReturnActiveEEProm();
  
  if (can_params.analog_eeprom == ETM_EEPROM_I2C_SELECTED) {
    ETMEEPromUseI2C();
  } else if (can_params.analog_eeprom == ETM_EEPROM_SPI_SELECTED) {
    ETMEEPromUseSPI();
  } else {
    ETMEEPromUseInternal();
  }

  can_params.eeprom_intialization_error = 0;
  
  // Load the Analog Calibration
  if (ETMEEPromReadPage(EEPROM_PAGE_ANALOG_CALIBRATION_0_1_2, &eeprom_page_data[0]) == 0xFFFF) {
    // No error with the EEPROM read
    etm_can_slave_debug_data.calibration_0_internal_gain   = eeprom_page_data[0];
    etm_can_slave_debug_data.calibration_0_internal_offset = eeprom_page_data[1];
    etm_can_slave_debug_data.calibration_0_external_gain   = eeprom_page_data[2];
    etm_can_slave_debug_data.calibration_0_external_offset = eeprom_page_data[3];

    etm_can_slave_debug_data.calibration_1_internal_gain   = eeprom_page_data[4];
    etm_can_slave_debug_data.calibration_1_internal_offset = eeprom_page_data[5];
    etm_can_slave_debug_data.calibration_1_external_gain   = eeprom_page_data[6];
    etm_can_slave_debug_data.calibration_1_external_offset = eeprom_page_data[7];

    etm_can_slave_debug_data.calibration_2_internal_gain   = eeprom_page_data[8];
    etm_can_slave_debug_data.calibration_2_internal_offset = eeprom_page_data[9];
    etm_can_slave_debug_data.calibration_2_external_gain   = eeprom_page_data[10];
    etm_can_slave_debug_data.calibration_2_external_offset = eeprom_page_data[11];
    
  } else {
    // Data error, set default values and error flag
    can_params.eeprom_intialization_error = 1;

    etm_can_slave_debug_data.calibration_0_internal_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_0_internal_offset = ETM_ANALOG_OFFSET_ZERO;
    etm_can_slave_debug_data.calibration_0_external_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_0_external_offset = ETM_ANALOG_OFFSET_ZERO;

    etm_can_slave_debug_data.calibration_1_internal_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_1_internal_offset = ETM_ANALOG_OFFSET_ZERO;
    etm_can_slave_debug_data.calibration_1_external_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_1_external_offset = ETM_ANALOG_OFFSET_ZERO;

    etm_can_slave_debug_data.calibration_2_internal_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_2_internal_offset = ETM_ANALOG_OFFSET_ZERO;
    etm_can_slave_debug_data.calibration_2_external_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_2_external_offset = ETM_ANALOG_OFFSET_ZERO;
  }
  
  if (ETMEEPromReadPage(EEPROM_PAGE_ANALOG_CALIBRATION_3_4_5, &eeprom_page_data[0]) == 0xFFFF) {
    // No error with the EEPROM read
    etm_can_slave_debug_data.calibration_3_internal_gain   = eeprom_page_data[0];
    etm_can_slave_debug_data.calibration_3_internal_offset = eeprom_page_data[1];
    etm_can_slave_debug_data.calibration_3_external_gain   = eeprom_page_data[2];
    etm_can_slave_debug_data.calibration_3_external_offset = eeprom_page_data[3];

    etm_can_slave_debug_data.calibration_4_internal_gain   = eeprom_page_data[4];
    etm_can_slave_debug_data.calibration_4_internal_offset = eeprom_page_data[5];
    etm_can_slave_debug_data.calibration_4_external_gain   = eeprom_page_data[6];
    etm_can_slave_debug_data.calibration_4_external_offset = eeprom_page_data[7];

    etm_can_slave_debug_data.calibration_5_internal_gain   = eeprom_page_data[8];
    etm_can_slave_debug_data.calibration_5_internal_offset = eeprom_page_data[9];
    etm_can_slave_debug_data.calibration_5_external_gain   = eeprom_page_data[10];
    etm_can_slave_debug_data.calibration_5_external_offset = eeprom_page_data[11];
    
  } else {
    // Data error, set default values and error flag
    can_params.eeprom_intialization_error = 1;

    etm_can_slave_debug_data.calibration_3_internal_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_3_internal_offset = ETM_ANALOG_OFFSET_ZERO;
    etm_can_slave_debug_data.calibration_3_external_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_3_external_offset = ETM_ANALOG_OFFSET_ZERO;

    etm_can_slave_debug_data.calibration_4_internal_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_4_internal_offset = ETM_ANALOG_OFFSET_ZERO;
    etm_can_slave_debug_data.calibration_4_external_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_4_external_offset = ETM_ANALOG_OFFSET_ZERO;

    etm_can_slave_debug_data.calibration_5_internal_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_5_internal_offset = ETM_ANALOG_OFFSET_ZERO;
    etm_can_slave_debug_data.calibration_5_external_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_5_external_offset = ETM_ANALOG_OFFSET_ZERO;
  }

  if (ETMEEPromReadPage(EEPROM_PAGE_ANALOG_CALIBRATION_6_7, &eeprom_page_data[0]) == 0xFFFF) {
    // No error with the EEPROM read
    etm_can_slave_debug_data.calibration_6_internal_gain   = eeprom_page_data[0];
    etm_can_slave_debug_data.calibration_6_internal_offset = eeprom_page_data[1];
    etm_can_slave_debug_data.calibration_6_external_gain   = eeprom_page_data[2];
    etm_can_slave_debug_data.calibration_6_external_offset = eeprom_page_data[3];

    etm_can_slave_debug_data.calibration_7_internal_gain   = eeprom_page_data[4];
    etm_can_slave_debug_data.calibration_7_internal_offset = eeprom_page_data[5];
    etm_can_slave_debug_data.calibration_7_external_gain   = eeprom_page_data[6];
    etm_can_slave_debug_data.calibration_7_external_offset = eeprom_page_data[7];
    
  } else {
    // Data error, set default values and error flag
    can_params.eeprom_intialization_error = 1;

    etm_can_slave_debug_data.calibration_6_internal_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_6_internal_offset = ETM_ANALOG_OFFSET_ZERO;
    etm_can_slave_debug_data.calibration_6_external_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_6_external_offset = ETM_ANALOG_OFFSET_ZERO;

    etm_can_slave_debug_data.calibration_7_internal_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_7_internal_offset = ETM_ANALOG_OFFSET_ZERO;
    etm_can_slave_debug_data.calibration_7_external_gain   = ETM_ANALOG_CALIBRATION_SCALE_1;
    etm_can_slave_debug_data.calibration_7_external_offset = ETM_ANALOG_OFFSET_ZERO;
  }

  // Load the Board Configuration Data
  if (ETMEEPromReadPage(EEPROM_PAGE_BOARD_CONFIGURATION, &eeprom_page_data[0]) == 0xFFFF) {
    // No error with the EEPROM read
    slave_board_data.device_rev_2x_ASCII           = eeprom_page_data[0];
    slave_board_data.device_serial_number          = eeprom_page_data[1];
    
  } else {
    // Data error, set default values and error flag
    can_params.eeprom_intialization_error = 1;
    slave_board_data.device_rev_2x_ASCII           = 0x3333;
    slave_board_data.device_serial_number          = 44444;
  }


  if (eeprom_select_backup == ETM_EEPROM_I2C_SELECTED) {
    ETMEEPromUseI2C();
  } else if (eeprom_select_backup == ETM_EEPROM_SPI_SELECTED) {
    ETMEEPromUseSPI();
  } else {
    ETMEEPromUseInternal();
  }
  
}

void ETMCanSlaveLoadConfiguration(unsigned long agile_id, unsigned int agile_dash,
				  unsigned int firmware_agile_rev, unsigned int firmware_branch, 
				  unsigned int firmware_branch_rev) {
  
  slave_board_data.device_id_low_word = (agile_id & 0xFFFF);
  slave_board_data.device_id_high_word = agile_id;
  slave_board_data.device_id_dash_number = agile_dash;
  
  slave_board_data.device_firmware_rev_agile = firmware_agile_rev;
  slave_board_data.device_firmware_branch = firmware_branch;
  slave_board_data.device_firmware_branch_rev = firmware_branch_rev;

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
  unsigned int cmd_id;
  unsigned int eeprom_page_data[16];
  unsigned int eeprom_write_error;
  unsigned int *ram_ptr;
  unsigned char discrete_cmd_id;

  cmd_id = message_ptr->identifier; // DPARKER manipulate these bits to get the cmd id

  switch(cmd_id) {

  case ETM_CAN_CMD_ID_RESET_MCU:
    if (message_ptr->word3 == can_params.address) {
      __delay32(10000000); // Delay for a while so that any EEPROM write process can complete
      __asm__ ("Reset");
    }
    break;

  case ETM_CAN_CMD_ID_LOAD_DEFAULT_CALIBRATION:
    if (message_ptr->word3 == can_params.address) {
      ETMCanSlaveLoadDefaultCalibration();
    }
    break;

  case ETM_CAN_CMD_ID_LOAD_REV_AND_SERIAL_NUMBER:
    if (message_ptr->word3 == can_params.address) {
      eeprom_write_error = 0xFFFF;
      eeprom_write_error &= ETMEEPromReadPage(EEPROM_PAGE_BOARD_CONFIGURATION, &eeprom_page_data[0]);
      if (eeprom_write_error == 0xFFFF) {
	eeprom_page_data[0] = message_ptr->word1; // update the device_rev_2x_ASCII
	eeprom_page_data[1] = message_ptr->word0; // update the device_serial_number
	eeprom_write_error &= ETMEEPromWritePageWithConfirmation(EEPROM_PAGE_BOARD_CONFIGURATION, &eeprom_page_data[0]);
      }
      if (eeprom_write_error != 0xFFFF) {
	can_params.eeprom_calibration_write_error = 1;
      }
    }
    break;

  case ETM_CAN_CMD_ID_SET_CAL_PAIR:
    // DPARKER NEED WORK HERE
    /*
      NEED TO DO A SWITCH ON THE CALIBRATION PAIR SO THAT WE GRAD THE RIGHT EEPROM DATA

    if (message_ptr->word3 == can_params.address) {
      eeprom_write_error = 0xFFFF;
      eeprom_write_error &= ETMEEPromReadPage(EEPROM_PAGE_ANALOG_CALIBRATION_0_1_2, &eeprom_page_data[0]);
      

    }
    */
    break;

    
  case ETM_CAN_CMD_ID_SET_RAM_DEBUG:
    if (message_ptr->word3 == can_params.address) {
      if (message_ptr->word2 < RAM_SIZE_WORDS) {
	ram_ptr = (unsigned int*)message_ptr->word2;
	etm_can_slave_debug_data.ram_monitor_c = *ram_ptr;
      }
      if (message_ptr->word1 < RAM_SIZE_WORDS) {
	ram_ptr = (unsigned int*)message_ptr->word1;
	etm_can_slave_debug_data.ram_monitor_b = *ram_ptr;
      }
      if (message_ptr->word1 < RAM_SIZE_WORDS) {
	ram_ptr = (unsigned int*)message_ptr->word0;
	etm_can_slave_debug_data.ram_monitor_a = *ram_ptr;
      }
    }
    break;

  case ETM_CAN_CMD_ID_SET_EEPROM_DEBUG:
    // DPARKER THERE IS NO MECHANISM FOR READING A SINGLE WORD FROM THE RAW EEPROM
    break;

  case ETM_CAN_CMD_ID_SET_IGNORE_FAULTS:
    // DPARKER IMPLIMENT THIS
    break;

  case ETM_CAN_CMD_ID_HVPS_SET_POINTS:
    slave_data.hvps_set_point_dose_level_3 = message_ptr->word3;
    slave_data.hvps_set_point_dose_level_2 = message_ptr->word2;
    slave_data.hvps_set_point_dose_level_1 = message_ptr->word1;
    slave_data.hvps_set_point_dose_level_0 = message_ptr->word0;
    setting_data_recieved &= 0b00000001;
    break;

  case ETM_CAN_CMD_ID_MAGNET_SET_POINTS:
    slave_data.electromagnet_set_point_dose_level_3 = message_ptr->word3;
    slave_data.electromagnet_set_point_dose_level_2 = message_ptr->word2;
    slave_data.electromagnet_set_point_dose_level_1 = message_ptr->word1;
    slave_data.electromagnet_set_point_dose_level_0 = message_ptr->word0;
    setting_data_recieved &= 0b00000010;
    break;

  case ETM_CAN_CMD_ID_AFC_HOME_POSTION:
    slave_data.afc_home_position_dose_level_3 = message_ptr->word3;
    slave_data.afc_home_position_dose_level_2 = message_ptr->word2;
    slave_data.afc_home_position_dose_level_1 = message_ptr->word1;
    slave_data.afc_home_position_dose_level_0 = message_ptr->word0;
    setting_data_recieved &= 0b00000100;
    break;

  case ETM_CAN_CMD_ID_GUN_PULSE_TOP_SET_POINTS:
    slave_data.gun_driver_pulse_top_voltage_dose_level_3 = message_ptr->word3;
    slave_data.gun_driver_pulse_top_voltage_dose_level_2 = message_ptr->word2;
    slave_data.gun_driver_pulse_top_voltage_dose_level_1 = message_ptr->word1;
    slave_data.gun_driver_pulse_top_voltage_dose_level_0 = message_ptr->word0;
    setting_data_recieved &= 0b00001000;
    break;

  case ETM_CAN_CMD_ID_GUN_CATHODE_SET_POINTS:
    slave_data.gun_driver_cathode_voltage_dose_level_3 = message_ptr->word3;
    slave_data.gun_driver_cathode_voltage_dose_level_2 = message_ptr->word2;
    slave_data.gun_driver_cathode_voltage_dose_level_1 = message_ptr->word1;
    slave_data.gun_driver_cathode_voltage_dose_level_0 = message_ptr->word0;
    setting_data_recieved &= 0b00010000;
    break;

  case ETM_CAN_CMD_ID_ALL_DOSE_SET_POINTS_REGISTER_A:
    slave_data.magnetron_heater_scaled_heater_current = message_ptr->word3;
    slave_data.afc_aux_control_or_offset              = message_ptr->word2;
    slave_data.gun_driver_grid_bias_voltage           = message_ptr->word1;
    slave_data.gun_driver_heater_voltage              = message_ptr->word0;
    setting_data_recieved &= 0b00100000;
    break;

  case ETM_CAN_CMD_ID_ALL_DOSE_SET_POINTS_REGISTER_B:
    slave_data.afc_manual_target_position             = message_ptr->word0;
    setting_data_recieved &= 0b01000000;
    break;

  case ETM_CAN_CMD_ID_DISCRETE_CMD:
    discrete_cmd_id = message_ptr->word3;
    BufferByte64WriteByte(&discrete_cmd_buffer, discrete_cmd_id);
    break;

  default:
    // DPARKER increment fault counter
    etm_can_slave_debug_data.can_invalid_index++;
    break;
			  
  }

  if (setting_data_recieved == 0b01111111) {
    _CONTROL_NOT_CONFIGURED = 0;
  }
}

/*
  
void ETMCanSlaveExecuteCMDCommon(ETMCanMessage* message_ptr) {
  unsigned int index_word;
  unsigned int eeprom_page_data[16];
  unsigned int eeprom_write_error;

  index_word = message_ptr->word3;
  index_word &= 0xFFF0;

  
  switch (index_word) {
    
  case ETM_CAN_REGISTER_DEFAULT_CMD_RESET_MCU:
    __asm__ ("Reset");
    break;

  case ETM_CAN_REGISTER_DEFAULT_CMD_RESET_ANALOG_CALIBRATION:
    ETMCanSlaveLoadDefaultCalibration();
    break;

  case ETM_CAN_REGISTER_DEFAULT_CMD_SET_REV_SN:
    eeprom_write_error = 0xFFFF;
    eeprom_write_error &= ETMEEPromReadPage(EEPROM_PAGE_BOARD_CONFIGURATION, &eeprom_page_data[0]);
    if (eeprom_write_error == 0xFFFF) {
      eeprom_page_data[0] = message_ptr->word1; // update the device_rev_2x_ASCII
      eeprom_page_data[1] = message_ptr->word0; // update the device_serial_number
      eeprom_write_error &= ETMEEPromWritePageWithConfirmation(EEPROM_PAGE_BOARD_CONFIGURATION, &eeprom_page_data[0]);
    }
    if (eeprom_write_error != 0xFFFF) {
      can_params.eeprom_calibration_write_error = 1;
    }
    break;

  case ETM_CAN_REGISTER_REQUEST_RTSP:
    // Not implimented yet
    break;

  case ETM_CAN_REGISTER_CONFIRM_RTSP_REQUEST:
    // Not implimented yet
    break;
    
  case ETM_CAN_REGISTER_SET_CALIBRATION_PAIR_INTERNAL_0:
    eeprom_write_error = 0xFFFF;
    eeprom_write_error &= ETMEEPromReadPage(EEPROM_PAGE_ANALOG_CALIBRATION_0_1_2, &eeprom_page_data[0]);
    if (eeprom_write_error == 0xFFFF) {
      eeprom_page_data[0] = message_ptr->word1; // update internal gain 0
      eeprom_page_data[1] = message_ptr->word0; // update internal offset 0
      eeprom_write_error &= ETMEEPromWritePageWithConfirmation(EEPROM_PAGE_ANALOG_CALIBRATION_0_1_2, &eeprom_page_data[0]);
    }
    if (eeprom_write_error != 0xFFFF) {
      can_params.eeprom_calibration_write_error = 1;
    }
    break;
    
  case ETM_CAN_REGISTER_SET_CALIBRATION_PAIR_EXTERNAL_0:
    eeprom_write_error = 0xFFFF;
    eeprom_write_error &= ETMEEPromReadPage(EEPROM_PAGE_ANALOG_CALIBRATION_0_1_2, &eeprom_page_data[0]);
    if (eeprom_write_error == 0xFFFF) {
      eeprom_page_data[2] = message_ptr->word1; // update external gain 0
      eeprom_page_data[3] = message_ptr->word0; // update external offset 0
      eeprom_write_error &= ETMEEPromWritePageWithConfirmation(EEPROM_PAGE_ANALOG_CALIBRATION_0_1_2, &eeprom_page_data[0]);
    }
    if (eeprom_write_error != 0xFFFF) {
      can_params.eeprom_calibration_write_error = 1;
    }
    break;

    //--------------------- DPARKER - ADD ALL THE OTHER CALIBRATION PAIRS (1-7)----------------------//



    
  case ETM_CAN_REGISTER_SET_RAM_DEBUG_ALL_LOCATIONS:
    ETMCanSlaveSetRAMWatchLocations(message_ptr->word2, message_ptr->word1, message_ptr->word0);
    break;

  case ETM_CAN_REGISTER_READ_EEPROM_LOCATION:
    // DPARKER - READ RAM LOCATION . . ..
    break;

  case ETM_CAN_REGISTER_IGNORE_FAULT:
    // DPARKER THINK ABOUT THIS MORE
    break;

  case ETM_CAN_REGISTER_SET_DATA_A:
    slave_board_data.cmd_data[0] = message_ptr->word0;
    slave_board_data.cmd_data[1] = message_ptr->word1;
    slave_board_data.cmd_data[2] = message_ptr->word2;    
    break;

  case ETM_CAN_REGISTER_SET_DATA_B:
    slave_board_data.cmd_data[3] = message_ptr->word0;
    slave_board_data.cmd_data[4] = message_ptr->word1;
    slave_board_data.cmd_data[5] = message_ptr->word2;    
    break;

  case ETM_CAN_REGISTER_SET_DATA_C:
    slave_board_data.cmd_data[6] = message_ptr->word0;
    slave_board_data.cmd_data[7] = message_ptr->word1;
    slave_board_data.cmd_data[8] = message_ptr->word2;    
    break;
    
  case ETM_CAN_REGISTER_SET_DATA_D:
    slave_board_data.cmd_data[9] = message_ptr->word0;
    slave_board_data.cmd_data[10] = message_ptr->word1;
    slave_board_data.cmd_data[11] = message_ptr->word2;    
    break;
    
    
  default:
    // The default command was not recognized 
    etm_can_slave_debug_data.can_invalid_index++;
    break;
  }
}

*/


void ETMCanSlaveLoadDefaultCalibration(void) {
  unsigned int eeprom_page_data[16];
  unsigned int write_error;
  eeprom_page_data[0] = ETM_ANALOG_CALIBRATION_SCALE_1;
  eeprom_page_data[1] = ETM_ANALOG_OFFSET_ZERO;
  eeprom_page_data[2] = ETM_ANALOG_CALIBRATION_SCALE_1;
  eeprom_page_data[3] = ETM_ANALOG_OFFSET_ZERO;

  eeprom_page_data[4] = ETM_ANALOG_CALIBRATION_SCALE_1;
  eeprom_page_data[5] = ETM_ANALOG_OFFSET_ZERO;
  eeprom_page_data[6] = ETM_ANALOG_CALIBRATION_SCALE_1;
  eeprom_page_data[7] = ETM_ANALOG_OFFSET_ZERO;
  
  eeprom_page_data[8] = ETM_ANALOG_CALIBRATION_SCALE_1;
  eeprom_page_data[9] = ETM_ANALOG_OFFSET_ZERO;
  eeprom_page_data[10] = ETM_ANALOG_CALIBRATION_SCALE_1;
  eeprom_page_data[11] = ETM_ANALOG_OFFSET_ZERO;

  eeprom_page_data[12] = 0;
  eeprom_page_data[13] = 0;
  eeprom_page_data[14] = 0;

  write_error = 0xFFFF;
  write_error &= ETMEEPromWritePageWithConfirmation(EEPROM_PAGE_ANALOG_CALIBRATION_0_1_2 ,&eeprom_page_data[0]);
  write_error &= ETMEEPromWritePageWithConfirmation(EEPROM_PAGE_ANALOG_CALIBRATION_3_4_5 ,&eeprom_page_data[0]);
  write_error &= ETMEEPromWritePageWithConfirmation(EEPROM_PAGE_ANALOG_CALIBRATION_6_7 ,&eeprom_page_data[0]);

  if (write_error != 0xFFFF) {
    can_params.eeprom_calibration_write_error = 1;
  }
}


void ETMCanSlaveTimedTransmit(void) {
  static unsigned int slave_data_log_index;      
  static unsigned int slave_data_log_sub_index;
  static unsigned long etm_can_slave_timed_transmit_holding_var; // Used to time the transmit of CAN Messaged back to the ECB
  
  // Sends the debug information up as log data  
  if (ETMTickRunOnceEveryNMilliseconds(SLAVE_TRANSMIT_MILLISECONDS, &etm_can_slave_timed_transmit_holding_var)) {
    // Will be true once every 100ms

    ETMCanSlaveSendStatus(); // Send out the status every 100mS
    
    // Set the Ready LED
    if (_CONTROL_NOT_READY) {
      ETMClearPin(can_params.not_ready_led); // This turns on the LED
    } else {
      ETMSetPin(can_params.not_ready_led);  // This turns off the LED
    }
        
    slave_data_log_index++;
    if (slave_data_log_index >= 18) {
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
    
    // Also send out one logging data message every 100mS
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
	if (slave_data_log_sub_index == 0) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CONFIG_0,
			     slave_board_data.device_id_high_word,
			     slave_board_data.device_id_low_word,
			     slave_board_data.device_id_dash_number,
			     slave_board_data.device_rev_2x_ASCII);
	  
	  
	} else if (slave_data_log_sub_index == 1) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CONFIG_1,
			     slave_board_data.device_serial_number,
			     slave_board_data.device_firmware_rev_agile,
			     slave_board_data.device_firmware_branch,
			     slave_board_data.device_firmware_branch_rev);
	  
	} else if (slave_data_log_sub_index == 2) {
    	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_TBD_1, 
			     etm_can_slave_debug_data.debugging_TBD_7, 
			     etm_can_slave_debug_data.debugging_TBD_6,
			     etm_can_slave_debug_data.debugging_TBD_5,
			     etm_can_slave_debug_data.debugging_TBD_4);      
	} else {
    	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_TBD_0, 
			     etm_can_slave_debug_data.debugging_TBD_3, 
			     etm_can_slave_debug_data.debugging_TBD_2,
			     etm_can_slave_debug_data.debugging_TBD_1,
			     etm_can_slave_debug_data.debugging_TBD_0);      
	}
	
      case 7:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_0,
			   etm_can_slave_debug_data.debug_reg[0],
			   etm_can_slave_debug_data.debug_reg[1],
			   etm_can_slave_debug_data.debug_reg[2],
			   etm_can_slave_debug_data.debug_reg[3]);
	break;
	
      case 8:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_1,
			   etm_can_slave_debug_data.debug_reg[4],
			   etm_can_slave_debug_data.debug_reg[5],
			   etm_can_slave_debug_data.debug_reg[6],
			   etm_can_slave_debug_data.debug_reg[7]);
	break;
	
      case 9:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_2,
			   etm_can_slave_debug_data.debug_reg[8],
			   etm_can_slave_debug_data.debug_reg[9],
			   etm_can_slave_debug_data.debug_reg[10],
			   etm_can_slave_debug_data.debug_reg[11]);
	break;
	
      case 10:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_DEFAULT_DEBUG_3,
			   etm_can_slave_debug_data.debug_reg[12],
			   etm_can_slave_debug_data.debug_reg[13],
			   etm_can_slave_debug_data.debug_reg[14],
			   etm_can_slave_debug_data.debug_reg[15]);
	break;

      case 11:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_RAM_WATCH_DATA,
			   0,
			   0,
			   0,
			   0);
	// DPARKER ADD RAM WATCH DATA
	break;
	
      case 12:
	ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_STANDARD_ANALOG_DATA,
			   0,
			   0,
			   0,
			   0);
	// DPARKER ADD ANALOG DATA
	break;
	
      case 13:
	if (slave_data_log_sub_index == 0) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CAN_DEBUG_0,
			     etm_can_slave_debug_data.can_tx_0,
			     etm_can_slave_debug_data.can_tx_1,
			     etm_can_slave_debug_data.can_tx_2,
			     etm_can_slave_debug_data.CXEC_reg_max);
	  
	} else if (slave_data_log_sub_index == 1) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CAN_DEBUG_1,
			     etm_can_slave_debug_data.can_rx_0_filt_0,
			     etm_can_slave_debug_data.can_rx_0_filt_1,
			     etm_can_slave_debug_data.can_rx_1_filt_2,
			     etm_can_slave_debug_data.CXINTF_max);
	  
	} else if (slave_data_log_sub_index == 2) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CAN_DEBUG_2,
			     etm_can_slave_debug_data.can_unknown_msg_id,
			     etm_can_slave_debug_data.can_invalid_index,
			     etm_can_slave_debug_data.can_address_error,
			     etm_can_slave_debug_data.can_error_flag);
	  
	} else {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CAN_DEBUG_3,
			     etm_can_slave_debug_data.can_tx_buf_overflow,
			     etm_can_slave_debug_data.can_rx_buf_overflow,
			     etm_can_slave_debug_data.can_rx_log_buf_overflow,
			     etm_can_slave_debug_data.can_timeout);
	}
	
	break;
	

      case 14:
	if (slave_data_log_sub_index == 0) {
	  etm_can_slave_debug_data.eeprom_internal_read_count = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_READ_INTERNAL_COUNT);
	  etm_can_slave_debug_data.eeprom_internal_read_error = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_READ_INTERNAL_ERROR);
	  etm_can_slave_debug_data.eeprom_internal_write_count = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_WRITE_INTERNAL_COUNT);
	  etm_can_slave_debug_data.eeprom_internal_write_error = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_WRITE_INTERNAL_ERROR);

	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_EEPROM_INTERNAL_DEBUG,
			     etm_can_slave_debug_data.eeprom_internal_read_count, 
			     etm_can_slave_debug_data.eeprom_internal_read_error,
			     etm_can_slave_debug_data.eeprom_internal_write_count,
			     etm_can_slave_debug_data.eeprom_internal_write_error);      
	  
	} else if (slave_data_log_sub_index == 1) {
	  etm_can_slave_debug_data.eeprom_i2c_read_count = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_READ_I2C_COUNT);
	  etm_can_slave_debug_data.eeprom_i2c_read_error = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_READ_I2C_ERROR);
	  etm_can_slave_debug_data.eeprom_i2c_write_count = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_WRITE_I2C_COUNT);
	  etm_can_slave_debug_data.eeprom_i2c_write_error = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_WRITE_I2C_ERROR);

	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_EEPROM_I2C_DEBUG,
			     etm_can_slave_debug_data.eeprom_i2c_read_count, 
			     etm_can_slave_debug_data.eeprom_i2c_read_error,
			     etm_can_slave_debug_data.eeprom_i2c_write_count,
			     etm_can_slave_debug_data.eeprom_i2c_write_error);      

	  
	} else if (slave_data_log_sub_index == 2) {
	  etm_can_slave_debug_data.eeprom_spi_read_count = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_READ_SPI_COUNT);
	  etm_can_slave_debug_data.eeprom_spi_read_error = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_READ_SPI_ERROR);
	  etm_can_slave_debug_data.eeprom_spi_write_count = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_WRITE_SPI_COUNT);
	  etm_can_slave_debug_data.eeprom_spi_write_error = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_WRITE_SPI_ERROR);
	  
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_EEPROM_SPI_DEBUG,
			     etm_can_slave_debug_data.eeprom_spi_read_count, 
			     etm_can_slave_debug_data.eeprom_spi_read_error,
			     etm_can_slave_debug_data.eeprom_spi_write_count,
			     etm_can_slave_debug_data.eeprom_spi_write_error);      

	  
	} else {
	  etm_can_slave_debug_data.eeprom_crc_error_count = ETMEEPromReturnDebugData(ETM_EEPROM_DEBUG_DATA_CRC_ERROR);

	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_EEPROM_CRC_DEBUG, 
			     etm_can_slave_debug_data.eeprom_crc_error_count, 
			     etm_can_slave_debug_data.debugging_TBD_10,
			     etm_can_slave_debug_data.debugging_TBD_9,
			     etm_can_slave_debug_data.debugging_TBD_8);      
	}
	break;

	
      case 15:
	if (slave_data_log_sub_index == 0) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_RESET_VERSION_DEBUG, 
			     etm_can_slave_debug_data.reset_count, 
			     etm_can_slave_debug_data.RCON_value,
			     etm_can_slave_debug_data.can_build_version, 
			     etm_can_slave_debug_data.library_build_version);
	  
	} else if (slave_data_log_sub_index == 1) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_I2C_SPI_SCALE_SELF_TEST_DEBUG, 
			     etm_can_slave_debug_data.i2c_bus_error_count, 
			     etm_can_slave_debug_data.spi_bus_error_count,
			     etm_can_slave_debug_data.scale_error_count,
			     *(unsigned int*)&etm_can_slave_debug_data.self_test_results);      
	  
	} else if (slave_data_log_sub_index == 2) {
    	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_TBD_3, 
			     etm_can_slave_debug_data.debugging_TBD_15, 
			     etm_can_slave_debug_data.debugging_TBD_14,
			     etm_can_slave_debug_data.debugging_TBD_13,
			     etm_can_slave_debug_data.debugging_TBD_12);      
	} else {
    	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_STANDARD_DEBUG_TBD_2, 
			     etm_can_slave_debug_data.debugging_TBD_11, 
			     etm_can_slave_debug_data.debugging_TBD_10,
			     etm_can_slave_debug_data.debugging_TBD_9,
			     etm_can_slave_debug_data.debugging_TBD_8);      
	}
	break;
		
	
      case 16:
	// CALIBRATION DATA
	if (slave_data_log_sub_index == 0) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_0,
			     etm_can_slave_debug_data.calibration_0_internal_gain, 
			     etm_can_slave_debug_data.calibration_0_internal_offset,
			     etm_can_slave_debug_data.calibration_0_internal_gain,
			     etm_can_slave_debug_data.calibration_0_internal_offset);      
			  
	} else if (slave_data_log_sub_index == 1) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_1,
			     etm_can_slave_debug_data.calibration_1_internal_gain, 
			     etm_can_slave_debug_data.calibration_1_internal_offset,
			     etm_can_slave_debug_data.calibration_1_internal_gain,
			     etm_can_slave_debug_data.calibration_1_internal_offset);      

	} else if (slave_data_log_sub_index == 2) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_2,
			     etm_can_slave_debug_data.calibration_2_internal_gain, 
			     etm_can_slave_debug_data.calibration_2_internal_offset,
			     etm_can_slave_debug_data.calibration_2_internal_gain,
			     etm_can_slave_debug_data.calibration_2_internal_offset);      
	} else {
    	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_3,
			     etm_can_slave_debug_data.calibration_3_internal_gain, 
			     etm_can_slave_debug_data.calibration_3_internal_offset,
			     etm_can_slave_debug_data.calibration_3_internal_gain,
			     etm_can_slave_debug_data.calibration_3_internal_offset);      
	}

	break;


      case 17:
	// CALIBRATION DATA
	if (slave_data_log_sub_index == 0) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_4,
			     etm_can_slave_debug_data.calibration_4_internal_gain, 
			     etm_can_slave_debug_data.calibration_4_internal_offset,
			     etm_can_slave_debug_data.calibration_4_internal_gain,
			     etm_can_slave_debug_data.calibration_4_internal_offset);      
			  
	} else if (slave_data_log_sub_index == 1) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_5,
			     etm_can_slave_debug_data.calibration_5_internal_gain, 
			     etm_can_slave_debug_data.calibration_5_internal_offset,
			     etm_can_slave_debug_data.calibration_5_internal_gain,
			     etm_can_slave_debug_data.calibration_5_internal_offset);      

	} else if (slave_data_log_sub_index == 2) {
	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_6,
			     etm_can_slave_debug_data.calibration_6_internal_gain, 
			     etm_can_slave_debug_data.calibration_6_internal_offset,
			     etm_can_slave_debug_data.calibration_6_internal_gain,
			     etm_can_slave_debug_data.calibration_6_internal_offset);      
	} else {
    	  ETMCanSlaveLogData(ETM_CAN_DATA_LOG_REGISTER_CALIBRATION_7,
			     etm_can_slave_debug_data.calibration_7_internal_gain, 
			     etm_can_slave_debug_data.calibration_7_internal_offset,
			     etm_can_slave_debug_data.calibration_7_internal_gain,
			     etm_can_slave_debug_data.calibration_7_internal_offset);      
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
  if (ETMTickGreaterThanNMilliseconds(SLAVE_TIMEOUT_MILLISECONDS, etm_can_slave_communication_timeout_holding_var)) {
    etm_can_slave_debug_data.can_timeout++;
    etm_can_persistent_data.can_timeout_count = etm_can_slave_debug_data.can_timeout;
    etm_can_slave_com_loss = 1;
  }
}


// DPARKER this needs to get fixed -- WHY?? what is the problem???
void ETMCanSlaveSendUpdateIfNewNotReady(void) {
  static unsigned int previous_ready_status;  // DPARKER - Need better name
  
  if ((previous_ready_status == 0) && (_CONTROL_NOT_READY)) {
    // There is new condition that is causing this board to inhibit operation.
    // Send a status update upstream to Master
    ETMCanSlaveSendStatus();
  }
  previous_ready_status = _CONTROL_NOT_READY;
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
  etm_can_slave_debug_data.can_build_version   = P1395_CAN_SLAVE_VERSION;
  etm_can_slave_debug_data.library_build_version  = ETM_LIBRARY_VERSION;

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

  etm_can_slave_debug_data.can_build_version = P1395_CAN_SLAVE_VERSION;
  etm_can_slave_debug_data.library_build_version = ETM_LIBRARY_VERSION;
}




void ETMCanSlaveSetLogDataRegister(unsigned int log_register, unsigned int data_for_log) {
  if (log_register > 0x0017) {
    return;
  }
  slave_board_data.log_data[log_register] = data_for_log;
}


void ETMCanSlaveSetDebugRegister(unsigned int debug_register, unsigned int debug_value) {
  if (debug_register > 0x000F) {
    return;
  }
  etm_can_slave_debug_data.debug_reg[debug_register] = debug_value;
}


unsigned int ETMCanSlaveGetComFaultStatus(void) {
  return etm_can_slave_com_loss;
}


unsigned int ETMCanSlaveGetSyncMsgHighSpeedLogging(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_1_high_speed_logging_enabled) {
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


unsigned int ETMCanSlaveGetSyncMsgScopeHVVmonEnabled(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_D_scope_HV_HVMON_active) {
    return 0xFFFF;
  } else {
    return 0;
  }
}


unsigned int ETMCanSlaveGetSyncMsgEnableFaultIgnore(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_E_ingnore_faults_enabled) {
    return 0xFFFF;
  } else {
    return 0;
  }
}


// DPARKER Chage reset enable to a command that gets sent as a command instead of a status big
unsigned int ETMCanSlaveGetSyncMsgResetEnable(void) {
  if (etm_can_slave_sync_message.sync_0_control_word.sync_0_reset_enable) {
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



unsigned int last_return_value_level_high_word_count_low_word;  // Pulse Level High Word, Pulse Count Low Word

void ETMCanSlaveTriggerRecieved(void) {
  // Does the slave need to send anything out or just update the next energy level
  unsigned char last_pulse_count;
  last_pulse_count = last_return_value_level_high_word_count_low_word;

  // Disable the Can Interrupt
  _C1IE = 0;
  _C2IE = 0;
  
  if (last_pulse_count == etm_can_slave_sync_message.pulse_count) {
    // we have not recieved a new sync message yet
    // Update pulse_count and pulse_level with the next expected value
    // Check to see if we are in interleave mode

    etm_can_slave_sync_message.pulse_count = (last_pulse_count + 1);
    switch (etm_can_slave_sync_message.next_energy_level)
      {
      
      case DOSE_SELECT_INTERLEAVE_0_1_DOSE_LEVEL_0:
	etm_can_slave_sync_message.next_energy_level = DOSE_SELECT_INTERLEAVE_0_1_DOSE_LEVEL_1;
	break;
	
      case DOSE_SELECT_INTERLEAVE_0_1_DOSE_LEVEL_1:
	etm_can_slave_sync_message.next_energy_level = DOSE_SELECT_INTERLEAVE_0_1_DOSE_LEVEL_0;
	break;
	
      case DOSE_SELECT_INTERLEAVE_2_3_DOSE_LEVEL_2:
	etm_can_slave_sync_message.next_energy_level = DOSE_SELECT_INTERLEAVE_2_3_DOSE_LEVEL_3;
	break;
	
      case DOSE_SELECT_INTERLEAVE_2_3_DOSE_LEVEL_3:
	etm_can_slave_sync_message.next_energy_level = DOSE_SELECT_INTERLEAVE_2_3_DOSE_LEVEL_2;
	break;
	
      default:
	// Don't change the pulse level because we aren't in interleaved mode
	break;
      }
  }

  // Reenable the relevant can interrupt
  if (CXEC_ptr == &C1EC) {
    // We are using CAN1.
    _C1IE = 1;
  } else {
    _C2IE = 1;
  }
}



unsigned int ETMCanSlaveGetPulseLevelAndCount(void) {
  unsigned int  return_value;
  
  // Disable the Can Interrupt
  _C1IE = 0;
  _C2IE = 0;

  return_value = etm_can_slave_sync_message.next_energy_level;
  return_value <<=9;
  return_value |= etm_can_slave_sync_message.pulse_count;
  
  // Reenable the relevant can interrupt
  if (CXEC_ptr == &C1EC) {
    // We are using CAN1.
    _C1IE = 1;
  } else {
    _C2IE = 1;
  }
  
  last_return_value_level_high_word_count_low_word = return_value;
  return return_value;
}


unsigned int ETMCanSlaveGetSetting(unsigned char setting_select) {
  unsigned int *data_ptr;
  data_ptr = (unsigned int*)&slave_data;
  data_ptr += setting_select;
  return *data_ptr;
}


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
      // It is a SYNC / Next Pulse Level Command

      etm_can_slave_debug_data.can_rx_0_filt_0++;
      ETMCanRXMessage(&can_message, CXRX0CON_ptr);
      *(unsigned int*)&etm_can_slave_sync_message.sync_0_control_word = can_message.word0;
      *(unsigned int*)&etm_can_slave_sync_message.pulse_count = can_message.word1;       // DPARKER CONFIRM THAT THIS ASSIGNMENT IS ALIGNED PROPERLY
      etm_can_slave_sync_message.prf_from_ecb = can_message.word2;
      *(unsigned int*)&etm_can_slave_sync_message.scope_A_select = can_message.word3;    // DPARKER CONFIRM THAT THIS ASSIGNMENT IS ALIGNED PROPERLY
      ClrWdt();
      etm_can_slave_com_loss = 0;
      etm_can_slave_communication_timeout_holding_var = ETMTickGet();
      
    } else {
      // The commmand was received by Filter 1
      // This is a setting/command from the ECB
      ETMCanRXMessageBuffer(&etm_can_slave_rx_message_buffer, CXRX1CON_ptr);
      etm_can_slave_debug_data.can_rx_0_filt_1++;
    }
    *CXINTF_ptr &= RX0_INT_FLAG_BIT; // Clear the RX0 Interrupt Flag
  }
  
  if (*CXRX1CON_ptr & BUFFER_FULL_BIT) {
    /* 
       A message has been recieved in Buffer 1
       This is a RTSP message - no other use for now
    */
    etm_can_slave_debug_data.can_rx_1_filt_2++;
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
