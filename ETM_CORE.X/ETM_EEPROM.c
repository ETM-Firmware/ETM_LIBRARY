#include <xc.h>
#include <libpic30.h>
#include "ETM_EEPROM.h"
#include "ETM_I2C.h"

void ETMEEPromWriteWordInternal(unsigned int register_location, unsigned int data);
unsigned int ETMEEPromReadWordInternal(unsigned int register_location);
void ETMEEPromWritePageInternal(unsigned int page_number, unsigned int words_to_write, unsigned int *data);
void ETMEEPromReadPageInternal(unsigned int page_number, unsigned int words_to_read, unsigned int *data);

void ETMEEPromWriteWordExternal(unsigned int register_location, unsigned int data);
unsigned int ETMEEPromReadWordExternal(unsigned int register_location);
void ETMEEPromWritePageExternal(unsigned int page_number, unsigned int words_to_write, unsigned int *data);
void ETMEEPromReadPageExternal(unsigned int page_number, unsigned int words_to_read, unsigned int *data);

typedef struct {
  unsigned char address;
  unsigned char i2c_port;
  unsigned int  size_bytes;
} ETMEEProm;

ETMEEProm external_eeprom;






// --------------- EEPROM GLUE FUNCTIONS ---------------------
unsigned int etm_eeprom_internal_selected = 1;

void ETMEEPromConfigureExternalDevice(unsigned int size_bytes, unsigned long fcy_clk, unsigned long i2c_baud_rate, unsigned char i2c_address, unsigned char i2c_port) {
  external_eeprom.address = i2c_address;
  external_eeprom.i2c_port = i2c_port;
  external_eeprom.size_bytes = size_bytes;
  ConfigureI2C(external_eeprom.i2c_port, I2CCON_DEFAULT_SETUP_PIC30F, i2c_baud_rate, fcy_clk, 0);
}

void ETMEEPromWriteWord(unsigned int register_location, unsigned int data) {
  if (ETMEEPromInternalSelected()) {
    ETMEEPromWriteWordInternal(register_location, data);
  } else {
    ETMEEPromWriteWordExternal(register_location, data);
  }
}

unsigned int ETMEEPromReadWord(unsigned int register_location) {
  if (ETMEEPromInternalSelected()) {
    return ETMEEPromReadWordInternal(register_location);
  } else {
    return ETMEEPromReadWordExternal(register_location);
  }
}

void ETMEEPromWritePage(unsigned int page_number, unsigned int words_to_write, unsigned int *data) {
  if (ETMEEPromInternalSelected()) {
    ETMEEPromWritePageInternal(page_number, words_to_write, &data[0]);
  } else {
    ETMEEPromWritePageExternal(page_number, words_to_write, &data[0]);
  }
}

void ETMEEPromReadPage(unsigned int page_number, unsigned int words_to_read, unsigned int *data) {
  if (ETMEEPromInternalSelected()) {
    ETMEEPromReadPageInternal(page_number, words_to_read, &data[0]);
  } else {
    ETMEEPromReadPageExternal(page_number, words_to_read, &data[0]);
  }
}

void ETMEEPromUseInternal(void) {
  etm_eeprom_internal_selected = 1;
}

void ETMEEPromUseExternal(void) {
  etm_eeprom_internal_selected = 0;
}

unsigned int ETMEEPromInternalSelected(void) {
  return etm_eeprom_internal_selected;
}


// ----------------------- INTERNAL EEPROM FUNCTIONS ----------------------------------

#if defined(__dsPIC30F6014A__)
#define INTERNAL_EEPROM_SIZE_WORDS  2048
#endif

#if defined(__dsPIC30F6010A__)
#define INTERNAL_EEPROM_SIZE_WORDS  2048
#endif

#ifndef INTERNAL_EEPROM_SIZE_WORDS
#define INTERNAL_EEPROM_SIZE_WORDS 0
#endif


#if INTERNAL_EEPROM_SIZE_WORDS == 0
void ETMEEPromWriteWordInternal(unsigned int register_location, unsigned int data) {}

unsigned int ETMEEPromReadWordInternal(unsigned int register_location) {
  return 0;
}

void ETMEEPromWritePageInternal(unsigned int page_number, unsigned int words_to_write, unsigned int *data) {
}

void ETMEEPromReadPageInternal(unsigned int page_number, unsigned int words_to_read, unsigned int *data) {
  unsigned int n;
  
  if (words_to_read > 0) {
    if (words_to_read > 16) {
      words_to_read = 16;
    }
    for (n = 0; n < words_to_read ; n++) {
      data[n] = 0;
    }
  }
}

#else

__eds__ unsigned int internal_eeprom[INTERNAL_EEPROM_SIZE_WORDS] __attribute__ ((space(eedata))) = {};


void ETMEEPromWriteWordInternal(unsigned int register_location, unsigned int data) {
  _prog_addressT write_address;
  
  if (register_location < INTERNAL_EEPROM_SIZE_WORDS) {
    write_address = __builtin_tbladdress(internal_eeprom);
    _wait_eedata();
    _erase_eedata((write_address + register_location*2), _EE_WORD);
    _wait_eedata();
    _write_eedata_word((write_address + register_location*2), data);  
  }
}


unsigned int ETMEEPromReadWordInternal(unsigned int register_location) {
  if (register_location < INTERNAL_EEPROM_SIZE_WORDS) {
    _wait_eedata();
    return internal_eeprom[register_location];
  } else {
    return 0xFFFF;
  }
}


void ETMEEPromWritePageInternal(unsigned int page_number, unsigned int words_to_write, unsigned int *data) {
  _prog_addressT write_address;
  unsigned int n = 0;
  int data_ram[16];

  if (words_to_write > 0) {
    if (words_to_write > 16) {
      words_to_write = 16;
    }
    
    ETMEEPromReadPage(page_number, 16, (unsigned int*)&data_ram);
    
    while (n < words_to_write) {
      data_ram[n] = (int)data[n];
      n++;
    }
    
    if (page_number < (INTERNAL_EEPROM_SIZE_WORDS >> 4)) {
      write_address = __builtin_tbladdress(internal_eeprom);
      write_address += page_number << 5;
      // The requeted page address is within the boundry of this device.
      _wait_eedata();
      _erase_eedata(write_address, _EE_ROW);
      _wait_eedata();
      _write_eedata_row(write_address, data_ram);
    }
  }
}

void ETMEEPromReadPageInternal(unsigned int page_number, unsigned int words_to_read, unsigned int *data) {
  unsigned int starting_register;
  unsigned int n;
  
  starting_register = page_number*16;
  _wait_eedata();
  if (words_to_read > 0) {
    if (words_to_read > 16) {
      words_to_read = 16;
    }
    if (page_number < (INTERNAL_EEPROM_SIZE_WORDS >> 4)) {
      for (n = 0; n < words_to_read ; n++) {
	data[n] = internal_eeprom[starting_register + n];
      }
    }
  }
}

#endif



// -----------------------  EXTERNAL EEPROM FUNCTIONS -------------------------------- //


void ETMEEPromWriteWordExternal(unsigned int register_location, unsigned int data) {
  unsigned int temp;
  unsigned char adr_high_byte;
  unsigned char adr_low_byte;
  unsigned char data_low_byte;
  unsigned char data_high_byte;
  unsigned int error_check;
  unsigned int busy_count;
  unsigned int busy;
   
   
  if (register_location < (external_eeprom.size_bytes >> 1) ) {
    // The requeted register address is within the boundry of this device.
    
    data_high_byte = (data >> 8);
    data_low_byte = (data & 0x00FF);
    
    temp = (register_location << 1);
    adr_high_byte = (temp >> 8);
    adr_low_byte = (temp & 0x00FF);
    
    error_check = 1;
    error_check = WaitForI2CBusIdle(external_eeprom.i2c_port);
    error_check |= GenerateI2CStart(external_eeprom.i2c_port);
    error_check |= WriteByteI2C(external_eeprom.address | I2C_WRITE_CONTROL_BIT, external_eeprom.i2c_port);

    busy = _ACKSTAT;
    busy_count = 0;
    while (busy && (busy_count <= 200)) {
      error_check |= GenerateI2CStop(external_eeprom.i2c_port);
      error_check = WaitForI2CBusIdle(external_eeprom.i2c_port);
      error_check |= GenerateI2CStart(external_eeprom.i2c_port);
      error_check |= WriteByteI2C(external_eeprom.address | I2C_WRITE_CONTROL_BIT, external_eeprom.i2c_port);
      busy = _ACKSTAT;
      busy_count++;
    }
    
    error_check |= WriteByteI2C(adr_high_byte, external_eeprom.i2c_port);
    error_check |= WriteByteI2C(adr_low_byte, external_eeprom.i2c_port);
    error_check |= WriteByteI2C(data_low_byte, external_eeprom.i2c_port);
    error_check |= WriteByteI2C(data_high_byte, external_eeprom.i2c_port);
    error_check |= GenerateI2CStop(external_eeprom.i2c_port);
  } else {
    // The requested address to write is outside the boundry of this device
    error_check = 0xFFFF;
  }

  // DPARKER - what to do if error check != 0?
}

unsigned int ETMEEPromReadWordExternal(unsigned int register_location) {
  unsigned int error_check;
  unsigned int temp;
  unsigned char adr_high_byte;
  unsigned char adr_low_byte;
  unsigned char data_low_byte;
  unsigned char data_high_byte;
  unsigned int busy_count;
  unsigned int busy;

  if (register_location < (external_eeprom.size_bytes >> 1) ) {
    // The requeted register address is within the boundry of this device.
    
    temp = (register_location << 1);
    adr_high_byte = (temp >> 8);
    adr_low_byte = (temp & 0x00FF);
 
    error_check = WaitForI2CBusIdle(external_eeprom.i2c_port);
    error_check |= GenerateI2CStart(external_eeprom.i2c_port);
    error_check |= WriteByteI2C(external_eeprom.address | I2C_WRITE_CONTROL_BIT, external_eeprom.i2c_port);

    busy = _ACKSTAT;
    busy_count = 0;
    while (busy && (busy_count <= 200)) {
      error_check |= GenerateI2CStop(external_eeprom.i2c_port);
      error_check = WaitForI2CBusIdle(external_eeprom.i2c_port);
      error_check |= GenerateI2CStart(external_eeprom.i2c_port);
      error_check |= WriteByteI2C(external_eeprom.address | I2C_WRITE_CONTROL_BIT, external_eeprom.i2c_port);
      busy = _ACKSTAT;
      busy_count++;
    }

    error_check |= WriteByteI2C(adr_high_byte, external_eeprom.i2c_port);
    error_check |= WriteByteI2C(adr_low_byte, external_eeprom.i2c_port);
    
    error_check |= GenerateI2CRestart(external_eeprom.i2c_port);
    error_check |= WriteByteI2C(external_eeprom.address | I2C_READ_CONTROL_BIT, external_eeprom.i2c_port);
    data_low_byte = ReadByteI2C(external_eeprom.i2c_port);
    error_check |= GenerateI2CAck(external_eeprom.i2c_port);
    data_high_byte = ReadByteI2C(external_eeprom.i2c_port);
    error_check |= GenerateI2CNack(external_eeprom.i2c_port);

    error_check |= GenerateI2CStop(external_eeprom.i2c_port);
  
  } else {
    // The requested address to write is outside the boundry of this device
    error_check = 0xFFFF;
    data_low_byte = 0;
    data_high_byte = 0;
  }

  // DPARKER - what to do if error check != 0?  Probably return zero and increment error counter

  temp = data_high_byte;
  temp <<= 8; 
  temp += data_low_byte;
  return temp;

}


void ETMEEPromWritePageExternal(unsigned int page_number, unsigned int words_to_write, unsigned int *data) {
  unsigned int temp;
  unsigned char adr_high_byte;
  unsigned char adr_low_byte;
  unsigned char data_low_byte;
  unsigned char data_high_byte;
  unsigned int error_check;
  unsigned int n;
  unsigned int busy_count;
  unsigned int busy;

  if (words_to_write > 0) {
    if (words_to_write > 16) {
      words_to_write = 16;
    }
    if (page_number < (external_eeprom.size_bytes >> 5)) {
      // The requeted page address is within the boundry of this device.
      
      temp = (page_number << 5);
      adr_high_byte = (temp >> 8);
      adr_low_byte = (temp & 0x00FF);
      
      
      error_check = WaitForI2CBusIdle(external_eeprom.i2c_port);
      error_check |= GenerateI2CStart(external_eeprom.i2c_port);
      error_check |= WriteByteI2C(external_eeprom.address | I2C_WRITE_CONTROL_BIT, external_eeprom.i2c_port);  
      busy = _ACKSTAT;
      busy_count = 0;
      while (busy && (busy_count <= 200)) {
	error_check |= GenerateI2CStop(external_eeprom.i2c_port);
	error_check = WaitForI2CBusIdle(external_eeprom.i2c_port);
	error_check |= GenerateI2CStart(external_eeprom.i2c_port);
	error_check |= WriteByteI2C(external_eeprom.address | I2C_WRITE_CONTROL_BIT, external_eeprom.i2c_port);
	busy = _ACKSTAT;
	busy_count++;
      }
      

      error_check |= WriteByteI2C(adr_high_byte, external_eeprom.i2c_port);
      error_check |= WriteByteI2C(adr_low_byte, external_eeprom.i2c_port);
      
      for (n = 0; n < words_to_write ; n++) {
	data_high_byte = (data[n] >> 8);
	data_low_byte = (data[n] & 0x00FF);
	error_check |= WriteByteI2C(data_low_byte, external_eeprom.i2c_port);
	error_check |= WriteByteI2C(data_high_byte, external_eeprom.i2c_port);    
      }
      
      
      error_check |= GenerateI2CStop(external_eeprom.i2c_port);
      
    } else {
      // The requested page to write is outside the boundry of this device
      error_check = 0xFFFF;
    }
    // DPARKER - what to do if error check != 0?
  }

}


void ETMEEPromReadPageExternal(unsigned int page_number, unsigned int words_to_read, unsigned int *data) {
  unsigned int error_check;
  unsigned int temp;
  unsigned char adr_high_byte;
  unsigned char adr_low_byte;
  unsigned char data_low_byte;
  unsigned char data_high_byte;
  unsigned int n;
  unsigned int busy_count;
  unsigned int busy;

  if (words_to_read > 0) {
    if (words_to_read > 16) {
      words_to_read = 16;
    }
    if (page_number < (external_eeprom.size_bytes >> 5) ) {
      // The requeted page address is within the boundry of this device.
      
      temp = (page_number << 5);
      adr_high_byte = (temp >> 8);
      adr_low_byte = (temp & 0x00FF);

      error_check = WaitForI2CBusIdle(external_eeprom.i2c_port);
      error_check |= GenerateI2CStart(external_eeprom.i2c_port);      
      error_check |= WriteByteI2C(external_eeprom.address | I2C_WRITE_CONTROL_BIT, external_eeprom.i2c_port);
      busy = _ACKSTAT;
      busy_count = 0;
      while (busy && (busy_count <= 200)) {
	error_check |= GenerateI2CStop(external_eeprom.i2c_port);
	error_check = WaitForI2CBusIdle(external_eeprom.i2c_port);
	error_check |= GenerateI2CStart(external_eeprom.i2c_port);
	error_check |= WriteByteI2C(external_eeprom.address | I2C_WRITE_CONTROL_BIT, external_eeprom.i2c_port);
	busy = _ACKSTAT;
	busy_count++;
      }

      error_check |= WriteByteI2C(adr_high_byte, external_eeprom.i2c_port);
      error_check |= WriteByteI2C(adr_low_byte, external_eeprom.i2c_port);
      
      error_check |= GenerateI2CRestart(external_eeprom.i2c_port);
      error_check |= WriteByteI2C(external_eeprom.address | I2C_READ_CONTROL_BIT, external_eeprom.i2c_port);
      
      for (n = 0; n < words_to_read-1 ; n++) {
	data_low_byte = ReadByteI2C(external_eeprom.i2c_port);
	error_check |= GenerateI2CAck(external_eeprom.i2c_port);
	data_high_byte = ReadByteI2C(external_eeprom.i2c_port);
	error_check |= GenerateI2CAck(external_eeprom.i2c_port);
	data[n] = data_high_byte;
	data[n] <<= 8;
	data[n] += data_low_byte; 
      }
      
      data_low_byte = ReadByteI2C(external_eeprom.i2c_port);
      error_check |= GenerateI2CAck(external_eeprom.i2c_port);
      data_high_byte = ReadByteI2C(external_eeprom.i2c_port);
      error_check |= GenerateI2CNack(external_eeprom.i2c_port);
      data[n] = data_high_byte;
      data[n] <<= 8;
      data[n] += data_low_byte; 

      
      error_check |= GenerateI2CStop(external_eeprom.i2c_port);
      
    } else {
      // The requested page to read is outside the boundry of this device
      error_check = 0xFFFF;
      data_low_byte = 0;
      data_high_byte = 0;
    }
   // DPARKER - what to do if error check != 0?  Probably return zero and increment error counter
  }
}
