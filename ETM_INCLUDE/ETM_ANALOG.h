#ifndef __ETM_ANALOG
#define __ETM_ANALOG


#define ETM_ANALOG_VERSION    04


/*
  Possible faults on analog input
  
  Over condition - Absolute (X samples out of range)
  Under condition - Absolute (X samples out of range)
  
  Over condition - Relative (Y samples out of range)
  Under condition - Relative (Y samples out of range)

*/


typedef struct {
  unsigned long adc_accumulator;
  unsigned int  adc_accumulator_counter;
  unsigned int  filtered_adc_reading;
  unsigned int  reading_scaled_and_calibrated;
  unsigned int  samples_to_filter_2_to_n;

  // -------- These are used to calibrate and scale the ADC Reading to Engineering Units ---------
  unsigned int fixed_scale;
  signed int   fixed_offset;
  unsigned int calibration_internal_scale;
  signed int   calibration_internal_offset;
  unsigned int calibration_external_scale;
  signed int   calibration_external_offset;


  // --------  These are used for fixed fault detection ------------------ 
  unsigned int fixed_over_trip_point;          // If the value exceeds this it will trip immediatly
  unsigned int fixed_under_trip_point;         // If the value is less than this it will trip immediatly
  unsigned int fixed_over_trip_counter;        // This counts the number of samples over the fixed_over_trip_point (will decrement each sample over test is false)
  unsigned int fixed_under_trip_counter;       // This counts the number of samples under the fixed_under_trip_point (will decrement each sample under test is false)
  unsigned int fixed_counter_fault_limit;      // The over / under trip counter must reach this value to generate a fault


  // --------  These are used for relative fault detection ------------------ 
  unsigned int target_value;                      // This is the target value (probably set point) in engineering units
  unsigned int relative_over_trip_point;
  unsigned int relative_under_trip_point;
  unsigned int relative_over_trip_counter;        // This counts the number of samples over the relative_over_trip_point (will decrement each sample over test is false)
  unsigned int relative_under_trip_counter;       // This counts the number of samples under the relative_under_trip_point (will decrement each sample under test is false)
  unsigned int relative_counter_fault_limit;      // The over / under trip counter must reach this value to generate a fault

} TYPE_ANALOG_INPUT;


typedef struct {
  unsigned int set_point;
  unsigned int max_set_point;
  unsigned int min_set_point;
  unsigned int disabled_set_point;
  unsigned int output_enabled;

  // -------- These are used to calibrate and scale the ADC Reading to Engineering Units ---------
  unsigned int fixed_scale;
  signed int   fixed_offset;

  unsigned int calibration_internal_scale;
  signed int   calibration_internal_offset;
  unsigned int calibration_external_scale;
  signed int   calibration_external_offset;

} TYPE_ANALOG_OUTPUT;


void ETMAnalogInputInitialize(TYPE_ANALOG_INPUT*  ptr_analog_input, 
			      unsigned int  fixed_scale, 
			      signed int    fixed_offset, 
			      unsigned int  filter_samples_2_n);
/*
  Initialize the Analog Input
  fixed_scale and fixed_offset are the Application Specfic Analog scaling
  filter_samples_2_n is the number of samples to be avaerage. 0=1 sample, 1=2 samples, 2=4 samples, . . . . up to a max of N=15 (32768 samples)
*/


void ETMAnalogInputLoadCalibration(TYPE_ANALOG_INPUT*  ptr_analog_input, 
				   unsigned int  internal_scale,
				   signed int    internal_offset,
				   unsigned int  external_scale,
				   signed int    external_offset);

/*
  Optionally load non-default calibration data.
  This could come from the EEPROM at board board of from external control system
*/


void ETMAnalogInputInitializeFixedTripLevels(TYPE_ANALOG_INPUT* ptr_analog_input,
					     unsigned int fixed_over_trip_point, 
					     unsigned int fixed_under_trip_point, 
					     unsigned int fixed_counter_fault_limit);
/*
  Optionally load fixed over and under trip limits
  See ETMAnalogInputUpdateFaults for more info
*/


void ETMAnalogInputInitializeRelativeTripLevels(TYPE_ANALOG_INPUT* ptr_analog_input,
						unsigned int target_value,
						unsigned int relative_trip_point_scale, 
						unsigned int relative_trip_point_floor, 
						unsigned int relative_counter_fault_limit);
/*
  Optionally load relative over and under trip limits
  The trip limits are target_value +/- (CALCULATED_TRIP_LEVEL)
  CALCULATED_TRIP_LEVEL = maximum((target_value * relative_trip_point_scale), relative_trip_point_floor)
  See ETMAnalogInputUpdateFaults for more info
*/


void ETMAnalogInputUpdate(TYPE_ANALOG_INPUT* ptr_analog_input, unsigned int adc_reading);
/*
  Loads a raw ADC reading and average, scales, calibrates and updates .reading_scaled_and_calibrated as configured above
*/



void ETMAnalogInputUpdateFaults(TYPE_ANALOG_INPUT* ptr_analog_input);
/*
  Compares .reading_scaled_and_calibrated to the over and under limits
  Updates the 
  
*/


void ETMAnalogInputClearFaultCounters(TYPE_ANALOG_INPUT* ptr_analog_input);
/*
  Sets all of the fault counters back to zero
*/

unsigned int ETMAnalogFaultOverFixed(TYPE_ANALOG_INPUT* ptr_analog_input);
unsigned int ETMAnalogFaultUnderFixed(TYPE_ANALOG_INPUT* ptr_analog_input);
unsigned int ETMAnalogFaultOverRelative(TYPE_ANALOG_INPUT* ptr_analog_input);
unsigned int ETMAnalogFaultUnderRelative(TYPE_ANALOG_INPUT* ptr_analog_input);




void ETMAnalogOutputInitialize(TYPE_ANALOG_OUTPUT* ptr_analog_output, 
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


void ETMAnalogOutputLoadCalibration(TYPE_ANALOG_OUTPUT* ptr_analog_output,
				    unsigned int internal_scale,
				    signed int   internal_offset,
				    unsigned int external_scale,
				    signed int   external_offset);
/*
  Optionally load non-default calibration data.
  This could come from the EEPROM at board board of from external control system
*/


void ETMAnalogOutputSetPoint(TYPE_ANALOG_OUTPUT* ptr_analog_output, unsigned int new_set_point);
/*
  Confirms that new_set_point is within the valid set point range
  If above the max value, will be set to max.
  If bellow the min value, will be set to min.
*/

unsigned int ETMAnalogOuputGetDACValue(TYPE_ANALOG_OUTPUT* ptr_analog_output);
/*
  This will return the value that needs to be written to the DAC to generate the requested output
  If the dac is disabled, this will return the DAC value associated with "disabled_set_point"
  If the dac is enabled, this will return the DAC value assoicated with "set_point"
*/

void ETMAnalogOutputDisable(TYPE_ANALOG_OUTPUT* ptr_analog_output);
void ETMAnalogOutputEnable(TYPE_ANALOG_OUTPUT* ptr_analog_output);


#define OFFSET_ZERO                                 0
#define NO_OVER_TRIP                                0xFFFF
#define NO_UNDER_TRIP                               0x0000
#define NO_TRIP_SCALE                               MACRO_DEC_TO_CAL_FACTOR_2(1)
#define NO_FLOOR                                    0xFFFF
#define NO_COUNTER                                  0x7FFF
#define NO_RELATIVE_COUNTER                         NO_COUNTER
#define NO_ABSOLUTE_COUNTER                         NO_COUNTER


#endif
