#ifndef __ETM_ANALOG
#define __ETM_ANALOG
#include "ETM_SCALE.h"


#define ETM_ANALOG_VERSION    02


/*
  Possible faults on analog input
  
  Over condition - Absolute (X samples out of range)
  Under condition - Absolute (X samples out of range)
  
  Over condition - Relative (Y samples out of range)
  Under condition - Relative (Y samples out of range)

*/


typedef struct {
  unsigned long adc_accumulator;
  unsigned int filtered_adc_reading;
  unsigned int reading_scaled_and_calibrated;

  // -------- These are used to calibrate and scale the ADC Reading to Engineering Units ---------
  unsigned int fixed_scale;
  signed int   fixed_offset;
  unsigned int calibration_internal_scale;
  signed int   calibration_internal_offset;
  unsigned int calibration_external_scale;
  signed int   calibration_external_offset;


  // --------  These are used for fault detection ------------------ 
  unsigned int over_trip_point_absolute;          // If the value exceeds this it will trip immediatly
  unsigned int under_trip_point_absolute;         // If the value is less than this it will trip immediatly
  unsigned int target_value;                      // This is the target value (probably set point) in engineering units
  unsigned int relative_trip_point_scale;         // This will be something like 5%, 10%, ect
  unsigned int relative_trip_point_floor;         // If target_value * relative_trip_point_floor is less than the floor value, the floor value will be used instead
                                                  // Trip Points = target_value +/- GreaterOf [(target_value*relative_trip_point_scale) OR (relative_trip_point_floor)] 
  unsigned int over_trip_counter;                 // This counts the number of samples over the relative_over_trip_point (will decrement each sample over test is false)
  unsigned int under_trip_counter;                // This counts the number of samples under the relative_under_trip_point (will decrement each sample under test is false)
  unsigned int relative_counter_fault_limit;      // The over / under trip counter must reach this value to generate a fault

  unsigned int absolute_over_counter;
  unsigned int absolute_under_counter;
  unsigned int absolute_counter_fault_limit;

} AnalogInput;


typedef struct {
  unsigned int set_point;
  unsigned int dac_setting_scaled_and_calibrated;
  unsigned int enabled;

  unsigned int max_set_point;
  unsigned int min_set_point;
  unsigned int disabled_dac_set_point;

  // -------- These are used to calibrate and scale the ADC Reading to Engineering Units ---------
  unsigned int fixed_scale;
  signed int   fixed_offset;
  unsigned int calibration_internal_scale;
  signed int   calibration_internal_offset;
  unsigned int calibration_external_scale;
  signed int   calibration_external_offset;

} AnalogOutput;


void ETMAnalogInitializeInput(AnalogInput* ptr_analog_input,
			      unsigned int fixed_scale,
			      signed int fixed_offset,
			      unsigned char analog_port,
			      unsigned int over_trip_point_absolute,
			      unsigned int under_trip_point_absolute,
			      unsigned int relative_trip_point_scale,
			      unsigned int relative_trip_point_floor,
			      unsigned int relative_counter_fault_limit,
			      unsigned int absolute_counter_fault_limit);

void ETMAnalogInitializeOutput(AnalogOutput* ptr_analog_output,
			       unsigned int fixed_scale,
			       signed int fixed_offset,
			       unsigned char analog_port,
			       unsigned int max_set_point,
			       unsigned int min_set_point,
			       unsigned int disabled_dac_set_point);


unsigned int ETMAnalogCheckEEPromInitialized();
/*
  This checks to see if the EEPROM has been initialized
  It checks that EEPROM Register 0x01FF = 0xAAAA
  Returns 1 if the EEProm has been initialized, 0 otherwise
*/

void ETMAnalogLoadDefaultCalibration(void);
/*
  This loads default values into the EEPROM
*/


void ETMAnalogScaleCalibrateDACSetting(AnalogOutput* ptr_analog_output);
/*
  Converts the engineering units set point into a binary to be written to DAC

  1) Make sure the set point is within valid range
  2) Convert that set poing into DAC setting
  3a) If enabled load DAC setting into dac_setting_scaled_and_calibrated
  3b) If disabled load disabled_dac_set_point into dac_setting_scaled_and_calibrated
*/

void ETMAnalogScaleCalibrateADCReading(AnalogInput* ptr_analog_input);
/*
  Converts ADC binary into engineering units
*/



void ETMAnalogSetOutput(AnalogOutput* ptr_analog_output, unsigned int new_set_point);
/*
  This limits new_set_point to valid range then writes to .set_point
  This function is no longer required because the set_point valid range is now checked every time the DAC is written to
*/


unsigned int ETMAnalogCheckOverAbsolute(AnalogInput* ptr_analog_input);
/*
  Used to check for an Over Absolute Condition based on values at initialization
*/

unsigned int ETMAnalogCheckUnderAbsolute(AnalogInput* ptr_analog_input);
/*
  Used to check for an Under Absolute Condition based on values at initialization
*/

unsigned int ETMAnalogCheckOverRelative(AnalogInput* ptr_analog_input);
/*
  Used to check for an Over Relative Condition based on values at initialization
*/

unsigned int ETMAnalogCheckUnderRelative(AnalogInput* ptr_analog_input);
/*
  Used to check for an Under Relative Condition based on values at initialization
*/

void ETMAnalogClearFaultCounters(AnalogInput* ptr_analog_input);
/*
  This clears all of the fault counters
  Over Absolute, Under Absolute, Over Relative, Under Relative
  This is used when clear a fault so that there is no memory of the faulted condidtion
*/

  
#define ANALOG_INPUT_0      0x0
#define ANALOG_INPUT_1      0x1
#define ANALOG_INPUT_2      0x2
#define ANALOG_INPUT_3      0x3
#define ANALOG_INPUT_4      0x4
#define ANALOG_INPUT_5      0x5
#define ANALOG_INPUT_6      0x6
#define ANALOG_INPUT_7      0x7
#define ANALOG_INPUT_8      0x8
#define ANALOG_INPUT_9      0x9
#define ANALOG_INPUT_A      0xA
#define ANALOG_INPUT_B      0xB
#define ANALOG_INPUT_C      0xC
#define ANALOG_INPUT_D      0xD
#define ANALOG_INPUT_E      0xE
#define ANALOG_INPUT_F      0xF
#define ANALOG_INPUT_NO_CALIBRATION    0xFF


#define ANALOG_OUTPUT_0     0x0
#define ANALOG_OUTPUT_1     0x1
#define ANALOG_OUTPUT_2     0x2
#define ANALOG_OUTPUT_3     0x3
#define ANALOG_OUTPUT_4     0x4
#define ANALOG_OUTPUT_5     0x5
#define ANALOG_OUTPUT_6     0x6
#define ANALOG_OUTPUT_7     0x7
#define ANALOG_OUTPUT_NO_CALIBRATION   0xFF


#define OFFSET_ZERO                                 0
#define NO_OVER_TRIP                                0xFFFF
#define NO_UNDER_TRIP                               0x0000
#define NO_TRIP_SCALE                               MACRO_DEC_TO_CAL_FACTOR_2(1)
#define NO_FLOOR                                    0xFFFF
#define NO_COUNTER                                  0x7FFF
#define NO_RELATIVE_COUNTER                         NO_COUNTER
#define NO_ABSOLUTE_COUNTER                         NO_COUNTER


#endif
