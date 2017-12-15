#include "ETM_ANALOG.h"
#include "ETM_SCALE.h"
#include "ETM_MATH.h"
#include "ETM_TICK.h"


typedef struct {
  unsigned long adc_accumulator;
  unsigned int  adc_accumulator_counter;
  unsigned int  filtered_adc_reading;
  unsigned int  reading_scaled_and_calibrated;
  unsigned int  samples_to_average_2_to_n;
  unsigned long previous_sample_time;
  
  // -------- These are used to calibrate and scale the ADC Reading to Engineering Units ---------
  // Word 8
  unsigned int  fixed_scale;
  signed int    fixed_offset;
  unsigned int  calibration_internal_scale;
  signed int    calibration_internal_offset;
  unsigned int  calibration_external_scale;
  signed int    calibration_external_offset;


  // --------  These are used for fixed fault detection ------------------
  // Word 14
  unsigned int  fixed_over_trip_point;          // If the value exceeds this it will trip
  unsigned int  fixed_under_trip_point;         // If the value is less than this it will trip
  unsigned long fixed_over_trip_timer;          // This counts the time over fixed_over_trip_point (will decrement when test is false)
  unsigned long fixed_under_trip_timer;         // This counts the time under fixed_under_trip_point (will decrement when test is false)
  unsigned long fixed_fault_time;               // The over / under trip counter must reach this time to generate a condition


  // --------  These are used for relative fault detection ------------------ 
  // Word 22
  unsigned int  relative_over_trip_point;       // If the value exceeds this it will trip
  unsigned int  relative_under_trip_point;      // If the value is less than this it will trip
  unsigned long relative_over_trip_timer;       // This counts the time over relative_over_trip_point (will decrement when test is false)
  unsigned long relative_under_trip_timer;      // This counts the time under relative_under_trip_point (will decrement when test is false)
  unsigned long relative_fault_time;            // The over / under trip counter must reach this time to generate a condition

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


#define ETM_ANALOG_NO_TRIP_SCALE                               MACRO_DEC_TO_CAL_FACTOR_2(1)
#define ETM_ANALOG_MAX_FAULT_LIMIT                             0x3FFFFFFF

void ETMAnalogInputInitialize(TYPE_PUBLIC_ANALOG_INPUT*  input_ptr,
			      unsigned int  fixed_scale, 
			      signed int    fixed_offset, 
			      unsigned int  average_samples_2_n) {
  
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;  

  ptr_analog_input->fixed_scale = fixed_scale;
  ptr_analog_input->fixed_offset = fixed_offset;  

  ptr_analog_input->calibration_internal_scale  = ETM_ANALOG_CALIBRATION_SCALE_1;
  ptr_analog_input->calibration_internal_offset = ETM_ANALOG_OFFSET_ZERO;
  ptr_analog_input->calibration_external_scale  = ETM_ANALOG_CALIBRATION_SCALE_1;
  ptr_analog_input->calibration_external_offset = ETM_ANALOG_OFFSET_ZERO;
  
  if (average_samples_2_n > 15) {
    average_samples_2_n = 15;
  }

  ptr_analog_input->samples_to_average_2_to_n     = average_samples_2_n;
  ptr_analog_input->adc_accumulator              = 0;
  ptr_analog_input->adc_accumulator_counter      = 0;  

  ptr_analog_input->fixed_over_trip_timer        = 0;  
  ptr_analog_input->fixed_under_trip_timer       = 0;
  ptr_analog_input->relative_over_trip_timer     = 0;
  ptr_analog_input->relative_under_trip_timer    = 0;

  ptr_analog_input->fixed_over_trip_point        = ETM_ANALOG_NO_OVER_TRIP;
  ptr_analog_input->fixed_under_trip_point       = ETM_ANALOG_NO_UNDER_TRIP;

  ptr_analog_input->relative_over_trip_point     = ETM_ANALOG_NO_OVER_TRIP;
  ptr_analog_input->relative_under_trip_point    = ETM_ANALOG_NO_UNDER_TRIP;
  
  ptr_analog_input->fixed_fault_time             = ETM_ANALOG_MAX_FAULT_LIMIT;
  ptr_analog_input->relative_fault_time          = ETM_ANALOG_MAX_FAULT_LIMIT;

}


void ETMAnalogInputLoadCalibration(TYPE_PUBLIC_ANALOG_INPUT* input_ptr, 
				   unsigned int  internal_scale,
				   signed int    internal_offset,
				   unsigned int  external_scale,
				   signed int    external_offset) {

  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;  

  ptr_analog_input->calibration_internal_scale  = internal_scale;
  ptr_analog_input->calibration_internal_offset = internal_offset;
  ptr_analog_input->calibration_external_scale  = external_scale;
  ptr_analog_input->calibration_external_offset = external_offset;
}


void ETMAnalogInputInitializeFixedTripLevels(TYPE_PUBLIC_ANALOG_INPUT* input_ptr,
					     unsigned int fixed_over_trip_point, 
					     unsigned int fixed_under_trip_point, 
					     unsigned int fixed_counter_fault_limit) {

  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;

  ptr_analog_input->fixed_fault_time = fixed_counter_fault_limit;
  ptr_analog_input->fixed_fault_time *= ETMTickGet1msMultiplier();
  
  if (ptr_analog_input->fixed_fault_time > ETM_ANALOG_MAX_FAULT_LIMIT) {
    ptr_analog_input->fixed_fault_time = ETM_ANALOG_MAX_FAULT_LIMIT;
  }
  
  ptr_analog_input->fixed_under_trip_point = fixed_under_trip_point;  
  ptr_analog_input->fixed_over_trip_point = fixed_over_trip_point;  
}



void ETMAnalogInputInitializeRelativeTripLevels(TYPE_PUBLIC_ANALOG_INPUT* input_ptr,
						unsigned int target_value,
						unsigned int relative_trip_point_scale, 
						unsigned int relative_trip_point_floor, 
						unsigned int relative_counter_fault_limit) {
  unsigned int compare_point;
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;
 
  ptr_analog_input->relative_fault_time = relative_counter_fault_limit;
  ptr_analog_input->relative_fault_time *= ETMTickGet1msMultiplier();

  if (ptr_analog_input->relative_fault_time > ETM_ANALOG_MAX_FAULT_LIMIT) {
    ptr_analog_input->relative_fault_time = ETM_ANALOG_MAX_FAULT_LIMIT;
  }
 
  
  
  compare_point = ETMScaleFactor2(target_value, relative_trip_point_scale, 0);
  if (compare_point < relative_trip_point_floor) {
    compare_point = relative_trip_point_floor;
  }
  
  ptr_analog_input->relative_over_trip_point  = ETMMath16Add(target_value, compare_point);
  ptr_analog_input->relative_under_trip_point = ETMMath16Sub(target_value, compare_point);
}



void ETMAnalogInputUpdate(TYPE_PUBLIC_ANALOG_INPUT* input_ptr, unsigned int adc_reading) {
  unsigned long current_time;
  unsigned long delta_time;
  unsigned int temp;
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;
  
  ptr_analog_input->adc_accumulator += adc_reading;
  ptr_analog_input->adc_accumulator_counter++;
  
  if (ptr_analog_input->adc_accumulator_counter >= (0x1 << ptr_analog_input->samples_to_average_2_to_n)) {
    // Average over 2^N readings
    ptr_analog_input->adc_accumulator >>= ptr_analog_input->samples_to_average_2_to_n;
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


    // Now check for faults
    current_time = ETMTickGet();
    delta_time = current_time - ptr_analog_input->previous_sample_time;
    ptr_analog_input->previous_sample_time = current_time;

    // Check Over Fixed 
    if (ptr_analog_input->reading_scaled_and_calibrated > ptr_analog_input->fixed_over_trip_point) {
      if (ptr_analog_input->fixed_over_trip_timer < (ptr_analog_input->fixed_fault_time << 1)) {
	ptr_analog_input->fixed_over_trip_timer += delta_time;
      } 
    } else {
      if (ptr_analog_input->fixed_over_trip_timer <= delta_time) {
	ptr_analog_input->fixed_over_trip_timer = 0;
      } else {
	ptr_analog_input->fixed_over_trip_timer -= delta_time;
      }
    }

    // Check Under Fixed
    if (ptr_analog_input->reading_scaled_and_calibrated < ptr_analog_input->fixed_under_trip_point) {
      if (ptr_analog_input->fixed_under_trip_timer < (ptr_analog_input->fixed_fault_time << 1)) {
	ptr_analog_input->fixed_under_trip_timer += delta_time;
      } 
    } else {
      if (ptr_analog_input->fixed_under_trip_timer <= delta_time) {
	ptr_analog_input->fixed_under_trip_timer = 0;
      } else {
	ptr_analog_input->fixed_under_trip_timer -= delta_time;
      }
    }

    // Check Over Relative 
    if (ptr_analog_input->reading_scaled_and_calibrated > ptr_analog_input->relative_over_trip_point) {
      if (ptr_analog_input->relative_over_trip_timer < (ptr_analog_input->relative_fault_time << 1)) {
	ptr_analog_input->relative_over_trip_timer += delta_time;
      } 
    } else {
      if (ptr_analog_input->relative_over_trip_timer <= delta_time) {
	ptr_analog_input->relative_over_trip_timer = 0;
      } else {
	ptr_analog_input->relative_over_trip_timer -= delta_time;
      }
    }

    // Check Under Relative
    if (ptr_analog_input->reading_scaled_and_calibrated < ptr_analog_input->relative_under_trip_point) {
      if (ptr_analog_input->relative_under_trip_timer < (ptr_analog_input->relative_fault_time << 1)) {
	ptr_analog_input->relative_under_trip_timer += delta_time;
      } 
    } else {
      if (ptr_analog_input->relative_under_trip_timer <= delta_time) {
	ptr_analog_input->relative_under_trip_timer = 0;
      } else {
	ptr_analog_input->relative_under_trip_timer -= delta_time;
      }
    }
  }
}


unsigned int ETMAnalogInputGetReading(TYPE_PUBLIC_ANALOG_INPUT* input_ptr) {
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;
  return ptr_analog_input->reading_scaled_and_calibrated;
}


#if(0)
void ETMAnalogInputUpdateFaults(TYPE_PUBLIC_ANALOG_INPUT* input_ptr) {
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;

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

#endif


unsigned int ETMAnalogInputFaultOverFixed(TYPE_PUBLIC_ANALOG_INPUT* input_ptr) {
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;

  if (ptr_analog_input->fixed_over_trip_timer >= ptr_analog_input->fixed_fault_time) {
    return 1;
  }
  return 0;
}


unsigned int ETMAnalogInputFaultUnderFixed(TYPE_PUBLIC_ANALOG_INPUT* input_ptr) {
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;

  if (ptr_analog_input->fixed_under_trip_timer >= ptr_analog_input->fixed_fault_time) {
    return 1;
  }
  return 0;
}


unsigned int ETMAnalogInputFaultOverRelative(TYPE_PUBLIC_ANALOG_INPUT* input_ptr) {
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;

  if (ptr_analog_input->relative_over_trip_timer >= ptr_analog_input->relative_fault_time) {
    return 1;
  }
  return 0;
}


unsigned int ETMAnalogInputFaultUnderRelative(TYPE_PUBLIC_ANALOG_INPUT* input_ptr) {
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;

  if (ptr_analog_input->relative_under_trip_timer >= ptr_analog_input->relative_fault_time) {
    return 1;
  }
  return 0;
}

 
void ETMAnalogInputClearFaultCounters(TYPE_PUBLIC_ANALOG_INPUT* input_ptr) {
  TYPE_ANALOG_INPUT *ptr_analog_input = (TYPE_ANALOG_INPUT*)input_ptr;

  ptr_analog_input->fixed_over_trip_timer = 0;
  ptr_analog_input->fixed_under_trip_timer = 0;
  ptr_analog_input->relative_over_trip_timer = 0;
  ptr_analog_input->relative_under_trip_timer = 0;
}  




void ETMAnalogOutputInitialize(TYPE_PUBLIC_ANALOG_OUTPUT* output_ptr, 
			       unsigned int fixed_scale, 
			       signed int   fixed_offset,
			       unsigned int max_set_point, 
			       unsigned int min_set_point, 
			       unsigned int disabled_set_point) {

  TYPE_ANALOG_OUTPUT *ptr_analog_output = (TYPE_ANALOG_OUTPUT*)output_ptr;  

  ptr_analog_output->max_set_point = max_set_point;
  ptr_analog_output->min_set_point = min_set_point;
  ptr_analog_output->disabled_set_point = disabled_set_point;

  ptr_analog_output->fixed_scale = fixed_scale;
  ptr_analog_output->fixed_offset = fixed_offset;

  ptr_analog_output->calibration_internal_scale = MACRO_DEC_TO_CAL_FACTOR_2(1);
  ptr_analog_output->calibration_internal_offset = 0;
  ptr_analog_output->calibration_external_scale = MACRO_DEC_TO_CAL_FACTOR_2(1);
  ptr_analog_output->calibration_external_offset = 0;
  
  ETMAnalogOutputSetPoint((TYPE_PUBLIC_ANALOG_OUTPUT*)ptr_analog_output, ptr_analog_output->disabled_set_point);
  ETMAnalogOutputDisable((TYPE_PUBLIC_ANALOG_OUTPUT*)ptr_analog_output);
}


void ETMAnalogOutputLoadCalibration(TYPE_PUBLIC_ANALOG_OUTPUT* output_ptr,
				    unsigned int internal_scale,
				    signed int   internal_offset,
				    unsigned int external_scale,
				    signed int   external_offset) {

  TYPE_ANALOG_OUTPUT *ptr_analog_output = (TYPE_ANALOG_OUTPUT*)output_ptr;  

  ptr_analog_output->calibration_internal_scale  = internal_scale;
  ptr_analog_output->calibration_internal_offset = internal_offset;
  ptr_analog_output->calibration_external_scale  = internal_scale;
  ptr_analog_output->calibration_external_offset = internal_offset;
}


void ETMAnalogOutputSetPoint(TYPE_PUBLIC_ANALOG_OUTPUT* output_ptr, unsigned int new_set_point) {
  TYPE_ANALOG_OUTPUT *ptr_analog_output = (TYPE_ANALOG_OUTPUT*)output_ptr;  
  
  if (new_set_point > ptr_analog_output->max_set_point) {
    new_set_point = ptr_analog_output->max_set_point;
  }
  
  if (new_set_point < ptr_analog_output->min_set_point) {
    new_set_point = ptr_analog_output->min_set_point;
  }
  ptr_analog_output->set_point = new_set_point;
}

void ETMAnalogOutputDisable(TYPE_PUBLIC_ANALOG_OUTPUT* output_ptr) {
  TYPE_ANALOG_OUTPUT *ptr_analog_output = (TYPE_ANALOG_OUTPUT*)output_ptr;  

  ptr_analog_output->output_enabled = 0;
}

void ETMAnalogOutputEnable(TYPE_PUBLIC_ANALOG_OUTPUT* output_ptr) {
  TYPE_ANALOG_OUTPUT *ptr_analog_output = (TYPE_ANALOG_OUTPUT*)output_ptr;  

  ptr_analog_output->output_enabled = 1;
}

unsigned int ETMAnalogOuputGetDACValue(TYPE_PUBLIC_ANALOG_OUTPUT* output_ptr) {
  unsigned int temp;
  TYPE_ANALOG_OUTPUT *ptr_analog_output = (TYPE_ANALOG_OUTPUT*)output_ptr;  

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

