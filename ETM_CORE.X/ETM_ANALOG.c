#include "ETM_ANALOG.h"
#include "ETM_EEPROM.h"

#define CALIBRATION_DATA_START_REGISTER 0x100

void ETMAnalogInitializeInput(AnalogInput* ptr_analog_input, unsigned int fixed_scale, signed int fixed_offset, unsigned char analog_port, unsigned int over_trip_point_absolute, unsigned int under_trip_point_absolute, unsigned int relative_trip_point_scale, unsigned int relative_trip_point_floor, unsigned int relative_counter_fault_limit, unsigned int absolute_counter_fault_limit) {

  unsigned int cal_data_address;

  if (absolute_counter_fault_limit > 0x7FFF) {
    absolute_counter_fault_limit = 0x7FFF;
  }

  if (relative_counter_fault_limit > 0x7FFF) {
    relative_counter_fault_limit = 0x7FFF;
  }
  
  cal_data_address = 0;

  ptr_analog_input->adc_accumulator = 0;
  ptr_analog_input->filtered_adc_reading = 0;
  ptr_analog_input->reading_scaled_and_calibrated = 0;
  
  ptr_analog_input->fixed_scale = fixed_scale;
  ptr_analog_input->fixed_offset = fixed_offset;
  if (analog_port >= 0x10) {
    // There are only 16 analog input ports of calibration data
    // this is a special case, set the calibration to gain of 1 and offset of zero
    ptr_analog_input->calibration_internal_scale = MACRO_DEC_TO_CAL_FACTOR_2(1);
    ptr_analog_input->calibration_internal_offset = 0;
    ptr_analog_input->calibration_external_scale = MACRO_DEC_TO_CAL_FACTOR_2(1);
    ptr_analog_input->calibration_external_offset = 0;
  } else {
    // read calibration data from EEPROM
    // read calibration data from the EEPROM
    cal_data_address = CALIBRATION_DATA_START_REGISTER + analog_port*4;
    ptr_analog_input->calibration_internal_offset = ETMEEPromReadWord(cal_data_address);
    ptr_analog_input->calibration_internal_scale  = ETMEEPromReadWord(cal_data_address+1);
    ptr_analog_input->calibration_external_offset = ETMEEPromReadWord(cal_data_address+2);
    ptr_analog_input->calibration_external_scale  = ETMEEPromReadWord(cal_data_address+3);        
  }

  ptr_analog_input->over_trip_point_absolute = over_trip_point_absolute;
  ptr_analog_input->under_trip_point_absolute = under_trip_point_absolute;
  ptr_analog_input->relative_trip_point_scale = relative_trip_point_scale;
  ptr_analog_input->relative_trip_point_floor = relative_trip_point_floor;
  ptr_analog_input->over_trip_counter = 0;
  ptr_analog_input->under_trip_counter = 0;
  ptr_analog_input->relative_counter_fault_limit = relative_counter_fault_limit;
  ptr_analog_input->absolute_counter_fault_limit = absolute_counter_fault_limit;
}



void ETMAnalogInitializeOutput(AnalogOutput* ptr_analog_output, unsigned int fixed_scale, signed int fixed_offset, unsigned char analog_port, unsigned int max_set_point, unsigned int min_set_point, unsigned int disabled_dac_set_point) {
  unsigned int cal_data_address;
  
  cal_data_address = 0;
  
  ptr_analog_output->enabled = 0;
  ptr_analog_output->max_set_point = max_set_point;
  ptr_analog_output->min_set_point = min_set_point;
  ptr_analog_output->disabled_dac_set_point = disabled_dac_set_point;


  ptr_analog_output->fixed_scale = fixed_scale;
  ptr_analog_output->fixed_offset = fixed_offset;

  if (analog_port >= 0x08) {
    // there are only 8 DAC calibration channels
    ptr_analog_output->calibration_internal_scale = MACRO_DEC_TO_CAL_FACTOR_2(1);
    ptr_analog_output->calibration_internal_offset = 0;
    ptr_analog_output->calibration_external_scale = MACRO_DEC_TO_CAL_FACTOR_2(1);
    ptr_analog_output->calibration_external_offset = 0;
  } else {
    // read calibration data from EEPROM
    cal_data_address = CALIBRATION_DATA_START_REGISTER + 0x40 + analog_port*4;
    ptr_analog_output->calibration_internal_offset = ETMEEPromReadWord(cal_data_address);
    ptr_analog_output->calibration_internal_scale  = ETMEEPromReadWord(cal_data_address+1);
    ptr_analog_output->calibration_external_offset = ETMEEPromReadWord(cal_data_address+2);
    ptr_analog_output->calibration_external_scale  = ETMEEPromReadWord(cal_data_address+3);
  }
}



void ETMAnalogScaleCalibrateDACSetting(AnalogOutput* ptr_analog_output) {
  unsigned int temp;
  // Confirm set point is within valid range
  if (ptr_analog_output->set_point > ptr_analog_output->max_set_point) {
    ptr_analog_output->set_point = ptr_analog_output->max_set_point;
  }

  if (ptr_analog_output->set_point < ptr_analog_output->min_set_point) {
    ptr_analog_output->set_point = ptr_analog_output->min_set_point;
  }

  // Convert from engineering units to the DAC scale
  temp = ETMScaleFactor16(ptr_analog_output->set_point, ptr_analog_output->fixed_scale, ptr_analog_output->fixed_offset);
  
  // Calibrate for known gain/offset errors of this board
  temp = ETMScaleFactor2(temp, ptr_analog_output->calibration_internal_scale, ptr_analog_output->calibration_internal_offset);

  // Calibrate the DAC output for known gain/offset errors of the external circuitry
  temp = ETMScaleFactor2(temp, ptr_analog_output->calibration_external_scale, ptr_analog_output->calibration_external_offset);

  if (!ptr_analog_output->enabled) {
    temp = ptr_analog_output->disabled_dac_set_point;
  }
  
  ptr_analog_output->dac_setting_scaled_and_calibrated = temp;
}

void ETMAnalogScaleCalibrateADCReading(AnalogInput* ptr_analog_input) {
  unsigned int temp;
  // Calibrate the adc reading based on the known gain/offset errors of the external circuitry
  temp = ETMScaleFactor2(ptr_analog_input->filtered_adc_reading, ptr_analog_input->calibration_external_scale, ptr_analog_input->calibration_external_offset);

  // Calibrate the adc reading based on the known gain/offset errors of this board
  temp = ETMScaleFactor2(temp, ptr_analog_input->calibration_internal_scale, ptr_analog_input->calibration_internal_offset);
  
  // Scale the analog input to engineering units based on the fixed scale (and offset but normally not required) for this application
  temp = ETMScaleFactor16(temp, ptr_analog_input->fixed_scale, ptr_analog_input->fixed_offset);

  ptr_analog_input->reading_scaled_and_calibrated = temp;
}

void ETMAnalogSetOutput(AnalogOutput* ptr_analog_output, unsigned int new_set_point) {
  if (new_set_point > ptr_analog_output->max_set_point) {
    new_set_point = ptr_analog_output->max_set_point;
  }

  if (new_set_point < ptr_analog_output->min_set_point) {
    new_set_point = ptr_analog_output->min_set_point;
  }
  ptr_analog_output->set_point = new_set_point;
}

unsigned int ETMAnalogCheckOverAbsolute(AnalogInput* ptr_analog_input) {
  if (ptr_analog_input->reading_scaled_and_calibrated > ptr_analog_input->over_trip_point_absolute) {
    if (ptr_analog_input->absolute_over_counter <= (ptr_analog_input->absolute_counter_fault_limit << 1)) {
      ptr_analog_input->absolute_over_counter++;
    }
  } else { 
    if (ptr_analog_input->absolute_over_counter) {
      ptr_analog_input->absolute_over_counter--;
    }
  }

  if (ptr_analog_input->absolute_over_counter > ptr_analog_input->absolute_counter_fault_limit) {
    return 1;
  } else {
    return 0;
  }
}

unsigned int ETMAnalogCheckUnderAbsolute(AnalogInput* ptr_analog_input) {
  if (ptr_analog_input->reading_scaled_and_calibrated < ptr_analog_input->under_trip_point_absolute) {
    if (ptr_analog_input->absolute_under_counter <= (ptr_analog_input->absolute_counter_fault_limit << 1)) {
      ptr_analog_input->absolute_under_counter++;
    }
  } else { 
    if (ptr_analog_input->absolute_under_counter) {
      ptr_analog_input->absolute_under_counter--;
    }
  }

  if (ptr_analog_input->absolute_under_counter > ptr_analog_input->absolute_counter_fault_limit) {
    return 1;
  } else {
    return 0;
  }
}


unsigned int ETMAnalogCheckOverRelative(AnalogInput* ptr_analog_input) {
  unsigned int compare_point;
  compare_point = ETMScaleFactor2(ptr_analog_input->target_value, ptr_analog_input->relative_trip_point_scale, 0);
  if (compare_point < ptr_analog_input->relative_trip_point_floor) {
    compare_point = ptr_analog_input->relative_trip_point_floor;
  }
  
  if ((0xFF00 - compare_point) > ptr_analog_input->target_value) {
    compare_point += ptr_analog_input->target_value;
  } else {
    compare_point = 0xFF00;// We can't set this to 0xFFFF otherwise we would never get an over trip relative
  }
  
  if (ptr_analog_input->reading_scaled_and_calibrated > compare_point) {
    // We are out of range
    if (ptr_analog_input->over_trip_counter <= (ptr_analog_input->relative_counter_fault_limit << 1)) {
	ptr_analog_input->over_trip_counter++;
    }
  } else {
    if (ptr_analog_input->over_trip_counter) {
      ptr_analog_input->over_trip_counter--;
    }
  }
  
  if (ptr_analog_input->over_trip_counter >= ptr_analog_input->relative_counter_fault_limit) {
    return 1;
  } else {
    return 0;
  }
}


unsigned int ETMAnalogCheckUnderRelative(AnalogInput* ptr_analog_input) {
  unsigned int compare_point;
  compare_point = ETMScaleFactor2(ptr_analog_input->target_value, ptr_analog_input->relative_trip_point_scale, 0);
  if (compare_point < ptr_analog_input->relative_trip_point_floor) {
    compare_point = ptr_analog_input->relative_trip_point_floor;
  }
  
  if (compare_point < ptr_analog_input->target_value) {
    compare_point = ptr_analog_input->target_value - compare_point;
  } else {
    // In this case we will never get an under relative fault
    compare_point = 0x0000; 
  }
  
  if (ptr_analog_input->reading_scaled_and_calibrated < compare_point) {
    // We are out of range
    if (ptr_analog_input->under_trip_counter <= (ptr_analog_input->relative_counter_fault_limit << 1)) {
      ptr_analog_input->under_trip_counter++;
    }
    
  } else {
    if (ptr_analog_input->under_trip_counter) {
      ptr_analog_input->under_trip_counter--;
    }
  }
  
  if (ptr_analog_input->under_trip_counter >= ptr_analog_input->relative_counter_fault_limit) {
    return 1;
  } else {
    return 0;
  }
}






#define EEPROM_CALIBRATION_PAGE_ADC_CHN0_CHN3       0x10
#define EEPROM_CALIBRATION_PAGE_ADC_CHN4_CHN7       0x11
#define EEPROM_CALIBRATION_PAGE_ADC_CHN8_CHN11      0x12
#define EEPROM_CALIBRATION_PAGE_ADC_CHN12_CHN15     0x13
#define EEPROM_CALIBRATION_PAGE_DAC_CHN0_CHN3       0x14
#define EEPROM_CALIBRATION_PAGE_DAC_CHN4_CHN7       0x15
#define EEPROM_CALIBRATION_PAGE_GENERAL_1           0x16
#define EEPROM_CALIBRATION_PAGE_GENERAL_2           0x17


const unsigned int default_calibration_data[16] = {0, 0x8000, 0, 0x8000, 0, 0x8000, 0, 0x8000, 0, 0x8000, 0, 0x8000, 0, 0x8000, 0, 0x8000};
const unsigned int default_zero_data[16]       = {0, 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0};

unsigned int ETMAnalogCheckEEPromInitialized() {
  if (ETMEEPromReadWord(0x01FF) == 0xAAAA) {
    return 1;
  } else {
    return 0;
  }
}

void ETMAnalogLoadDefaultCalibration(void) {
  ETMEEPromWritePage(EEPROM_CALIBRATION_PAGE_ADC_CHN0_CHN3, 16, (unsigned int*)&default_calibration_data);
  ETMEEPromWritePage(EEPROM_CALIBRATION_PAGE_ADC_CHN4_CHN7, 16, (unsigned int*)&default_calibration_data);
  ETMEEPromWritePage(EEPROM_CALIBRATION_PAGE_ADC_CHN8_CHN11, 16, (unsigned int*)&default_calibration_data);
  ETMEEPromWritePage(EEPROM_CALIBRATION_PAGE_ADC_CHN12_CHN15, 16, (unsigned int*)&default_calibration_data);
  ETMEEPromWritePage(EEPROM_CALIBRATION_PAGE_DAC_CHN0_CHN3, 16, (unsigned int*)&default_calibration_data);
  ETMEEPromWritePage(EEPROM_CALIBRATION_PAGE_DAC_CHN4_CHN7, 16, (unsigned int*)&default_calibration_data);
  ETMEEPromWritePage(EEPROM_CALIBRATION_PAGE_GENERAL_1, 16, (unsigned int*)&default_zero_data);
  ETMEEPromWritePage(EEPROM_CALIBRATION_PAGE_GENERAL_2, 16, (unsigned int*)&default_zero_data);
  ETMEEPromWriteWord(0x01FF, 0xAAAA);
}
void ETMAnalogClearFaultCounters(AnalogInput* ptr_analog_input) {
  ptr_analog_input->absolute_under_counter = 0;
  ptr_analog_input->absolute_over_counter = 0;
  ptr_analog_input->over_trip_counter = 0;
  ptr_analog_input->under_trip_counter = 0;
}
