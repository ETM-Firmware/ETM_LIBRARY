#ifndef __ETM_ANALOG
#define __ETM_ANALOG


/*
  This module provides functions for handeling Analog Inputs and Outputs

  Possible faults on analog input
  Over condition - Absolute (out of range for at least N milliseconds
  Under condition - Absolute (out of range for at least N milliseconds)
  
  Over condition - Relative to Set Point (out of range for at least N milliseconds)
  Under condition - Relative to Set Point (out of range for at least N milliseconds)
*/


#define ETM_ANALOG_VERSION    04

typedef struct {
  unsigned int private_data_no_touching[32]; // Touch at your own risk
} TYPE_PUBLIC_ANALOG_INPUT;

typedef struct {
  unsigned int private_data_no_touching[14]; // Touch at your own risk
} TYPE_PUBLIC_ANALOG_OUTPUT;


void ETMAnalogInputInitialize(TYPE_PUBLIC_ANALOG_INPUT*  ptr_analog_input, 
				 unsigned int  fixed_scale, 
				 signed int    fixed_offset, 
				 unsigned int  average_samples_2_n);
/*
  Initialize the Analog Input
  fixed_scale and fixed_offset are the Application Specfic Analog scaling
  filter_samples_2_n is the number of samples to be avaerage. 0=1 sample, 1=2 samples, 2=4 samples, . . . . up to a max of N=15 (32768 samples)
*/


void ETMAnalogInputLoadCalibration(TYPE_PUBLIC_ANALOG_INPUT*  ptr_analog_input, 
				   unsigned int  internal_scale,
				   signed int    internal_offset,
				   unsigned int  external_scale,
				   signed int    external_offset);

/*
  Optionally load non-default calibration data.
  If you don't call this, default calibration values will be used (gain of 1 and offset of 0)
  This could come from the EEPROM at board board of from external control system

  This subroutine must be called after ETMAnalogInputInitialize
*/


void ETMAnalogInputInitializeFixedTripLevels(TYPE_PUBLIC_ANALOG_INPUT* ptr_analog_input,
					     unsigned int fixed_over_trip_point, 
					     unsigned int fixed_under_trip_point, 
					     unsigned int fixed_fault_time_milliseconds);
/*
  Optionally load fixed over and under trip limits
  If you don't call this, these faults won't be enabled
  set fixed_fault_time_milliseconds to Zero to indicated a fault on the first evaluation that the condition is true

  This subroutine must be called after ETMAnalogInputInitialize
  The ETM_TICK module must be initialized before you call this function or timing will not work properly
*/


void ETMAnalogInputInitializeRelativeTripLevels(TYPE_PUBLIC_ANALOG_INPUT* ptr_analog_input,
						unsigned int target_value,
						unsigned int relative_trip_point_scale, 
						unsigned int relative_trip_point_floor, 
						unsigned int relative_fault_time_milliseconds);
/*
  Optionally load relative over and under trip limits
  If you don't call this, these faults won't be enabled
  You must call this subroutine to change the target value
  The trip limits are target_value +/- (CALCULATED_TRIP_LEVEL)
  CALCULATED_TRIP_LEVEL = maximum((target_value * relative_trip_point_scale), relative_trip_point_floor)
  set relative_fault_time_milliseconds to Zero to indicated a fault on the first evaluation that the condition is true

  This subroutine must be called after ETMAnalogInputInitialize
  The ETM_TICK module must be initialized before you call this function or timing will not work properly
*/


void ETMAnalogInputUpdate(TYPE_PUBLIC_ANALOG_INPUT* ptr_analog_input, unsigned int adc_reading);
/*
  Loads a raw ADC reading and average, scales, calibrates and updates .reading_scaled_and_calibrated as configured above
  Updates the relevant fault timer (over/under fixed/relative)
*/

unsigned int ETMAnalogInputGetReading(TYPE_PUBLIC_ANALOG_INPUT* ptr_analog_input);
/*
  Returns the averaged / scaled / calibratated ADC Reading
*/

unsigned int ETMAnalogInputFaultOverFixed(TYPE_PUBLIC_ANALOG_INPUT* ptr_analog_input);
/*
  Returns 0xFFFF if there is an over fixed fault
  Returns 0 otherwise
*/

unsigned int ETMAnalogInputFaultUnderFixed(TYPE_PUBLIC_ANALOG_INPUT* ptr_analog_input);
/*
  Returns 0xFFFF if there is an under fixed fault
  Returns 0 otherwise
*/

unsigned int ETMAnalogInputFaultOverRelative(TYPE_PUBLIC_ANALOG_INPUT* ptr_analog_input);
/*
  Returns 0xFFFF if there is an over relative fault
  Returns 0 otherwise
*/

unsigned int ETMAnalogInputFaultUnderRelative(TYPE_PUBLIC_ANALOG_INPUT* ptr_analog_input);
/*
  Returns 0xFFFF if there is an under relative fault
  Returns 0 otherwise
*/


void ETMAnalogInputClearFaultCounters(TYPE_PUBLIC_ANALOG_INPUT* ptr_analog_input);
/*
  Sets all of the fault timers back to zero
*/


void ETMAnalogOutputInitialize(TYPE_PUBLIC_ANALOG_OUTPUT* ptr_analog_output, 
			       unsigned int fixed_scale, 
			       signed int   fixed_offset,
			       unsigned int max_set_point, 
			       unsigned int min_set_point, 
			       unsigned int disabled_set_point);
/*
  Initialize the Analog Output - Starts up in the disabled state
  fixed_scale and fixed_offset are the Application Specfic Analog scaling
  min_set_point and max_set_point are the minimum and maximum set points (in engineering units)
  disabled_set_point is the output when disabled (typically this will be zero, but can be whatever makes sense for the application)
  disabled_set_point is to NOT required to within the min->max set point range
*/


void ETMAnalogOutputLoadCalibration(TYPE_PUBLIC_ANALOG_OUTPUT* ptr_analog_output,
				    unsigned int internal_scale,
				    signed int   internal_offset,
				    unsigned int external_scale,
				    signed int   external_offset);
/*
  Optionally load non-default calibration data.
  This could come from the EEPROM at board board of from external control system

  This subroutine must be called after ETMAnalogOutputInitialize
*/


void ETMAnalogOutputSetPoint(TYPE_PUBLIC_ANALOG_OUTPUT* ptr_analog_output, unsigned int new_set_point);
/*
  Confirms that new_set_point is within the valid set point range
  If above the max value, will be set to max.
  If bellow the min value, will be set to min.

  This subroutine must be called after ETMAnalogOutputInitialize
*/

unsigned int ETMAnalogOutputGetDACValue(TYPE_PUBLIC_ANALOG_OUTPUT* ptr_analog_output);
/*
  This will return the value that needs to be written to the DAC to generate the requested output
  If the dac is disabled, this will return the DAC value associated with "disabled_set_point"
  If the dac is enabled, this will return the DAC value assoicated with "set_point"
*/

unsigned int ETMAnalogOutputGetSetPoint(TYPE_PUBLIC_ANALOG_OUTPUT* ptr_analog_output);
/*
  This returns the set point.
*/


void ETMAnalogOutputDisable(TYPE_PUBLIC_ANALOG_OUTPUT* ptr_analog_output);
/*
  Sets the disable flag which causes ETMAnalogOuputGetDACValue to return the DAC setting for 'disabled_set_point' from ETMAnalogOutputInitialize
*/

void ETMAnalogOutputEnable(TYPE_PUBLIC_ANALOG_OUTPUT* ptr_analog_output);
/*
  Clears the disable flag which causes ETMAnalogOuputGetDACValue to return the DAC setting for most recent set point from ETMAnalogOutputSetPoint
*/


#define ETM_ANALOG_CALIBRATION_SCALE_1                         MACRO_DEC_TO_CAL_FACTOR_2(1)
#define ETM_ANALOG_OFFSET_ZERO                                 0
#define ETM_ANALOG_NO_OVER_TRIP                                0xFFFF
#define ETM_ANALOG_NO_UNDER_TRIP                               0x0000
#define ETM_ANALOG_NO_FLOOR                                    0xFFFF

#define ETM_ANALOG_AVERAGE_1_SAMPLES                           0
#define ETM_ANALOG_AVERAGE_2_SAMPLES                           1
#define ETM_ANALOG_AVERAGE_4_SAMPLES                           2
#define ETM_ANALOG_AVERAGE_8_SAMPLES                           3
#define ETM_ANALOG_AVERAGE_16_SAMPLES                          4
#define ETM_ANALOG_AVERAGE_32_SAMPLES                          5
#define ETM_ANALOG_AVERAGE_64_SAMPLES                          6
#define ETM_ANALOG_AVERAGE_128_SAMPLES                         7
#define ETM_ANALOG_AVERAGE_256_SAMPLES                         8
#define ETM_ANALOG_AVERAGE_512_SAMPLES                         9
#define ETM_ANALOG_AVERAGE_1024_SAMPLES                        10
#define ETM_ANALOG_AVERAGE_2048_SAMPLES                        11
#define ETM_ANALOG_AVERAGE_4096_SAMPLES                        12



#endif
