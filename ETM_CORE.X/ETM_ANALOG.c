#include "ETM_ANALOG.h"
#include "ETM_SCALE.h"
#include "ETM_MATH.h"



void ETMAnalogInputInitialize(TYPE_ANALOG_INPUT*  ptr_analog_input, 
			      unsigned int  fixed_scale, 
			      signed int    fixed_offset, 
			      unsigned int  filter_samples_2_n) {
  
  ptr_analog_input->fixed_scale = fixed_scale;
  ptr_analog_input->fixed_offset = fixed_offset;  

  ptr_analog_input->calibration_internal_scale  = MACRO_DEC_TO_CAL_FACTOR_2(1);
  ptr_analog_input->calibration_internal_offset = 0;
  ptr_analog_input->calibration_external_scale  = MACRO_DEC_TO_CAL_FACTOR_2(1);
  ptr_analog_input->calibration_external_offset = 0;
  
  if (filter_samples_2_n > 15) {
    filter_samples_2_n = 15;
  }

  ptr_analog_input->samples_to_filter_2_to_n     = filter_samples_2_n;
  ptr_analog_input->adc_accumulator              = 0;
  ptr_analog_input->adc_accumulator_counter      = 0;  

  ptr_analog_input->fixed_over_trip_counter      = 0;  
  ptr_analog_input->fixed_under_trip_counter     = 0;
  ptr_analog_input->relative_over_trip_counter   = 0;  
  ptr_analog_input->relative_under_trip_counter  = 0;

  ptr_analog_input->fixed_over_trip_point        = 0xFFFF;
  ptr_analog_input->fixed_under_trip_point       = 0x0000;

  ptr_analog_input->relative_over_trip_point     = 0xFFFF;
  ptr_analog_input->relative_under_trip_point    = 0x0000;
  
  ptr_analog_input->fixed_counter_fault_limit    = 0x7FFF;
  ptr_analog_input->relative_counter_fault_limit = 0x7FFF;

}


void ETMAnalogInputLoadCalibration(TYPE_ANALOG_INPUT*  ptr_analog_input, 
				   unsigned int  internal_scale,
				   signed int    internal_offset,
				   unsigned int  external_scale,
				   signed int    external_offset) {
  
  ptr_analog_input->calibration_internal_scale  = internal_scale;
  ptr_analog_input->calibration_internal_offset = internal_offset;
  ptr_analog_input->calibration_external_scale  = external_scale;
  ptr_analog_input->calibration_external_offset = external_offset;
}


void ETMAnalogInputInitializeFixedTripLevels(TYPE_ANALOG_INPUT* ptr_analog_input,
					     unsigned int fixed_over_trip_point, 
					     unsigned int fixed_under_trip_point, 
					     unsigned int fixed_counter_fault_limit) {
  
  if (fixed_counter_fault_limit > 0x7FFF) {
    fixed_counter_fault_limit = 0x7FFF;
  }
  ptr_analog_input->fixed_counter_fault_limit = fixed_counter_fault_limit;
  
  ptr_analog_input->fixed_under_trip_point = fixed_under_trip_point;  
  ptr_analog_input->fixed_over_trip_point = fixed_over_trip_point;  
}



void ETMAnalogInputInitializeRelativeTripLevels(TYPE_ANALOG_INPUT* ptr_analog_input,
						unsigned int target_value,
						unsigned int relative_trip_point_scale, 
						unsigned int relative_trip_point_floor, 
						unsigned int relative_counter_fault_limit) {
  
  unsigned int compare_point;
  
  if (relative_counter_fault_limit > 0x7FFF) {
    relative_counter_fault_limit = 0x7FFF;
  }
  ptr_analog_input->relative_counter_fault_limit = relative_counter_fault_limit;
  
  
  compare_point = ETMScaleFactor2(target_value, relative_trip_point_scale, 0);
  if (compare_point < relative_trip_point_floor) {
    compare_point = relative_trip_point_floor;
  }
  
  ptr_analog_input->relative_over_trip_point  = ETMMath16Add(target_value, compare_point);
  ptr_analog_input->relative_under_trip_point = ETMMath16Sub(target_value, compare_point);
}



void ETMAnalogInputUpdate(TYPE_ANALOG_INPUT* ptr_analog_input, unsigned int adc_reading) {
  unsigned int temp;
  
  ptr_analog_input->adc_accumulator += adc_reading;
  ptr_analog_input->adc_accumulator_counter++;
  
  if (ptr_analog_input->adc_accumulator_counter >= (0x1 << ptr_analog_input->samples_to_filter_2_to_n)) {
    // Average over 2^N readings
    ptr_analog_input->adc_accumulator >>= ptr_analog_input->samples_to_filter_2_to_n;
    ptr_analog_input->filtered_adc_reading = ptr_analog_input->adc_accumulator;
    ptr_analog_input->adc_accumulator_counter = 0;
    ptr_analog_input->adc_accumulator = 0;

    
    // Scale the reading to engineering units

    // Calibrate the adc reading based on the known gain/offset errors of the external circuitry
    temp = ETMScaleFactor2(ptr_analog_input->filtered_adc_reading, ptr_analog_input->calibration_external_scale, ptr_analog_input->calibration_external_offset);
    
    // Calibrate the adc reading based on the known gain/offset errors of this board
    temp = ETMScaleFactor2(temp, ptr_analog_input->calibration_internal_scale, ptr_analog_input->calibration_internal_offset);
    
    // Scale the analog input to engineering units based on the fixed scale (and offset but normally not required) for this application
    temp = ETMScaleFactor16(temp, ptr_analog_input->fixed_scale, ptr_analog_input->fixed_offset);
    
    ptr_analog_input->reading_scaled_and_calibrated = temp;
  }
}


void ETMAnalogInputUpdateFaults(TYPE_ANALOG_INPUT* ptr_analog_input) {
  // Fixed Limits
  if (ptr_analog_input->reading_scaled_and_calibrated > ptr_analog_input->fixed_over_trip_point) {
    // Increase fixed_over_trip_counter, decrease fixed_under_trip_counter
    if (ptr_analog_input->fixed_over_trip_counter <= (ptr_analog_input->fixed_counter_fault_limit << 1)) {
      ptr_analog_input->fixed_over_trip_counter++;
    }
    if (ptr_analog_input->fixed_under_trip_counter) {
      ptr_analog_input->fixed_under_trip_counter--;
    }
    
  } else if (ptr_analog_input->reading_scaled_and_calibrated < ptr_analog_input->fixed_under_trip_point) {
    // decrease fixed_over_trip_counter, increase fixed_under_trip_counter
    if (ptr_analog_input->fixed_under_trip_counter <= (ptr_analog_input->fixed_counter_fault_limit << 1)) {
      ptr_analog_input->fixed_under_trip_counter++;
    }
    if (ptr_analog_input->fixed_over_trip_counter) {
      ptr_analog_input->fixed_over_trip_counter--;
    }
  } else {
    // decrease both counters
    if (ptr_analog_input->fixed_under_trip_counter) {
      ptr_analog_input->fixed_under_trip_counter--;
    }
    if (ptr_analog_input->fixed_over_trip_counter) {
      ptr_analog_input->fixed_over_trip_counter--;
    }
  }

  // Relative Limits
  if (ptr_analog_input->reading_scaled_and_calibrated > ptr_analog_input->relative_over_trip_point) {
    // Increase relative_over_trip_counter, decrease relative_under_trip_counter
    if (ptr_analog_input->relative_over_trip_counter <= (ptr_analog_input->relative_counter_fault_limit << 1)) {
      ptr_analog_input->relative_over_trip_counter++;
    }
    if (ptr_analog_input->relative_under_trip_counter) {
      ptr_analog_input->relative_under_trip_counter--;
    }
    
  } else if (ptr_analog_input->reading_scaled_and_calibrated < ptr_analog_input->relative_under_trip_point) {
    // decrease relative_over_trip_counter, increase relative_under_trip_counter
    if (ptr_analog_input->relative_under_trip_counter <= (ptr_analog_input->relative_counter_fault_limit << 1)) {
      ptr_analog_input->relative_under_trip_counter++;
    }
    if (ptr_analog_input->relative_over_trip_counter) {
      ptr_analog_input->relative_over_trip_counter--;
    }
  } else {
    // decrease both counters
    if (ptr_analog_input->relative_under_trip_counter) {
      ptr_analog_input->relative_under_trip_counter--;
    }
    if (ptr_analog_input->relative_over_trip_counter) {
      ptr_analog_input->relative_over_trip_counter--;
    }
  }
}
 
unsigned int ETMAnalogFaultOverFixed(TYPE_ANALOG_INPUT* ptr_analog_input) {
  if (ptr_analog_input->fixed_over_trip_counter >= ptr_analog_input->fixed_counter_fault_limit) {
    return 1;
  }
  return 0;
}

unsigned int ETMAnalogFaultUnderFixed(TYPE_ANALOG_INPUT* ptr_analog_input) {
  if (ptr_analog_input->fixed_under_trip_counter >= ptr_analog_input->fixed_counter_fault_limit) {
    return 1;
  }
  return 0;
}

unsigned int ETMAnalogFaultOverRelative(TYPE_ANALOG_INPUT* ptr_analog_input) {
  if (ptr_analog_input->relative_over_trip_counter >= ptr_analog_input->relative_counter_fault_limit) {
    return 1;
  }
  return 0;
}

unsigned int ETMAnalogFaultUnderRelative(TYPE_ANALOG_INPUT* ptr_analog_input) {
  if (ptr_analog_input->relative_under_trip_counter >= ptr_analog_input->relative_counter_fault_limit) {
    return 1;
  }
  return 0;
}

 
void ETMAnalogInputClearFaultCounters(TYPE_ANALOG_INPUT* ptr_analog_input) {
  ptr_analog_input->fixed_over_trip_counter = 0;
  ptr_analog_input->fixed_under_trip_counter = 0;
  ptr_analog_input->relative_over_trip_counter = 0;
  ptr_analog_input->relative_under_trip_counter = 0;
}  




void ETMAnalogOutputInitialize(TYPE_ANALOG_OUTPUT* ptr_analog_output, 
			       unsigned int fixed_scale, 
			       signed int   fixed_offset,
			       unsigned int max_set_point, 
			       unsigned int min_set_point, 
			       unsigned int disabled_set_point) {

  ptr_analog_output->max_set_point = max_set_point;
  ptr_analog_output->min_set_point = min_set_point;
  ptr_analog_output->disabled_set_point = disabled_set_point;

  ptr_analog_output->fixed_scale = fixed_scale;
  ptr_analog_output->fixed_offset = fixed_offset;

  ptr_analog_output->calibration_internal_scale = MACRO_DEC_TO_CAL_FACTOR_2(1);
  ptr_analog_output->calibration_internal_offset = 0;
  ptr_analog_output->calibration_external_scale = MACRO_DEC_TO_CAL_FACTOR_2(1);
  ptr_analog_output->calibration_external_offset = 0;
  
  ETMAnalogOutputSetPoint(ptr_analog_output, ptr_analog_output->disabled_set_point);
  ETMAnalogOutputDisable(ptr_analog_output);
}


void ETMAnalogOutputLoadCalibration(TYPE_ANALOG_OUTPUT* ptr_analog_output,
				    unsigned int internal_scale,
				    signed int   internal_offset,
				    unsigned int external_scale,
				    signed int   external_offset) {
  
  ptr_analog_output->calibration_internal_scale  = internal_scale;
  ptr_analog_output->calibration_internal_offset = internal_offset;
  ptr_analog_output->calibration_external_scale  = internal_scale;
  ptr_analog_output->calibration_external_offset = internal_offset;
}


void ETMAnalogOutputSetPoint(TYPE_ANALOG_OUTPUT* ptr_analog_output, unsigned int new_set_point) {
  
  if (new_set_point > ptr_analog_output->max_set_point) {
    new_set_point = ptr_analog_output->max_set_point;
  }
  
  if (new_set_point < ptr_analog_output->min_set_point) {
    new_set_point = ptr_analog_output->min_set_point;
  }
  ptr_analog_output->set_point = new_set_point;
}

void ETMAnalogOutputDisable(TYPE_ANALOG_OUTPUT* ptr_analog_output) {
  ptr_analog_output->output_enabled = 0;
}

void ETMAnalogOutputEnable(TYPE_ANALOG_OUTPUT* ptr_analog_output) {
  ptr_analog_output->output_enabled = 1;
}

unsigned int ETMAnalogOuputGetDACValue(TYPE_ANALOG_OUTPUT* ptr_analog_output) {
  unsigned int temp;
  if (ptr_analog_output->output_enabled == 0) {
    temp = ptr_analog_output->disabled_set_point;
  } else {
    temp = ptr_analog_output->set_point;
  }

  // Convert from engineering units to the DAC scale
  temp = ETMScaleFactor16(temp, ptr_analog_output->fixed_scale, ptr_analog_output->fixed_offset);
  
  // Calibrate for known gain/offset errors of this board
  temp = ETMScaleFactor2(temp, ptr_analog_output->calibration_internal_scale, ptr_analog_output->calibration_internal_offset);
  
  // Calibrate the DAC output for known gain/offset errors of the external circuitry
  temp = ETMScaleFactor2(temp, ptr_analog_output->calibration_external_scale, ptr_analog_output->calibration_external_offset);
  
  return temp;
}
