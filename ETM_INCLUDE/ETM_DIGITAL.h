#ifndef __ETM_DIGITAL
#define __ETM_DIGITAL


typedef struct {
  unsigned int filtered_reading;
  unsigned int accumulator;
  unsigned int filter_time;
} TYPE_DIGITAL_INPUT;



void ETMDigitalInitializeInput(TYPE_DIGITAL_INPUT* input, unsigned int initial_value, unsigned int filter_time);
/*
  This is initializes a digital input.
  Select the initial value
  Select filter time - This is number times ETMDigitalUpdateInput must be called for the filtered_reading to change after a change in state of "current_value"
*/

void ETMDigitalUpdateInput(TYPE_DIGITAL_INPUT* input, unsigned int current_value);
/*
  This is should be called at a fixed time scale and will filter the digital input of glitches
*/

unsigned int ETMDigitalFilteredOutput(TYPE_DIGITAL_INPUT* input);
/*
  Will return 0 if the filtered digital level is low
  Will return 1 if the fitlered digital level is high
*/


#endif
