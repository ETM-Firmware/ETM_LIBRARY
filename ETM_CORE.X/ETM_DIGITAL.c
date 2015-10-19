#include "ETM_DIGITAL.h"

void ETMDigitalInitializeInput(TYPE_DIGITAL_INPUT* input, unsigned int initial_value, unsigned int filter_time) {
  if (filter_time > 0x7000) {
    filter_time = 0x7000;
  }
  input->filter_time = filter_time;
  if (initial_value == 0) {
    input->accumulator = 0;
    input->filtered_reading = 0;
  } else {
    input->accumulator = (filter_time << 1);
    input->filtered_reading = 1;
  }
}


void ETMDigitalUpdateInput(TYPE_DIGITAL_INPUT* input, unsigned int current_value) {
  if (input->filter_time < 2) {
    input->filtered_reading = current_value;
  } else {
    if (current_value) {
      if (++input->accumulator > (input->filter_time << 1)) {
	input->accumulator--;
      }
    } else {
      if (input->accumulator) {
	input->accumulator--;
      }
    }
    if (input->accumulator >= input->filter_time) {
      if (input->filtered_reading == 0) {
	// we are changing state from low to high
	input->accumulator = (input->filter_time << 1);
      }
      input->filtered_reading = 1;
    } else {
      if (input->filtered_reading == 1) {
	// we are changing state from high to low
	input->accumulator = 0;
      }
      input->filtered_reading = 0;
    }
  }
}

unsigned int ETMDigitalFilteredOutput(TYPE_DIGITAL_INPUT* input) {
  return input->filtered_reading;
}
