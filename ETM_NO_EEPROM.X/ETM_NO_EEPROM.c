#include <xc.h>
#include "ETM_EEPROM.h"
#include <libpic30.h>


#define INTERNAL_EEPROM_SIZE_WORDS 0




void ETMEEPromConfigureExternalDevice(unsigned int size_bytes, unsigned long fcy_clk, unsigned long i2c_baud_rate, unsigned char i2c_address, unsigned char i2c_port) {
}

void ETMEEPromWriteWord(unsigned int register_location, unsigned int data) {
}


unsigned int ETMEEPromReadWord(unsigned int register_location) {
    return 0xFFFF;
}


void ETMEEPromWritePage(unsigned int page_number, unsigned int words_to_write, unsigned int *data) {
}

void ETMEEPromReadPage(unsigned int page_number, unsigned int words_to_read, unsigned int *data) {
}

