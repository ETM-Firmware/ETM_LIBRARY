#include <xc.h>
#include <libpic30.h>
#include "ETM_EEPROM.h"
#include "ETM_I2C.h"
#include "ETM_SPI.h"
#include "ETM_IO_PORTS.h"
#include "ETM_CRC.h"


#if defined(__dsPIC30F6014A__)
#define INTERNAL_EEPROM_SIZE_WORDS  2048
#endif

#if defined(__dsPIC30F6010A__)
#define INTERNAL_EEPROM_SIZE_WORDS  2048
#endif

#ifndef INTERNAL_EEPROM_SIZE_WORDS
#define INTERNAL_EEPROM_SIZE_WORDS 0
#endif


// DPARKER need to extended for SPI EEPROM
typedef struct {
  unsigned char address;
  unsigned char i2c_port;
  unsigned int  size_bytes;
} TYPE_ETMEEPROM_I2C;

typedef struct {
  unsigned int  pin_chip_select_not;
  unsigned int  pin_hold;
  unsigned int  pin_write_protect;
  unsigned char spi_port;
  unsigned int  size_bytes;
} TYPE_ETMEEPROM_SPI;

static TYPE_ETMEEPROM_I2C external_eeprom_I2C;
static TYPE_ETMEEPROM_SPI external_eeprom_SPI;

static unsigned int etm_eeprom_selected_device;
static unsigned long long alternate_page_data;

typedef struct {
  unsigned int read_internal_error;
  unsigned int read_spi_error;
  unsigned int read_i2c_error;
  unsigned int write_internal_error;
  unsigned int write_spi_error;
  unsigned int write_i2c_error;
  unsigned int page_read_count_internal;
  unsigned int page_read_count_spi;
  unsigned int page_read_count_i2c;
  unsigned int page_write_count_internal;
  unsigned int page_write_count_spi;
  unsigned int page_write_count_i2c;
  unsigned int crc_error;

} TYPE_EEPROM_DEBUG_DATA;

static TYPE_EEPROM_DEBUG_DATA eeprom_debug_data;


static unsigned int ETMEEPromPrivateReadSinglePage(unsigned int page_number, unsigned int *page_data);
static unsigned int ETMEEPromPrivateWriteSinglePage(unsigned int page_number, unsigned int *page_data);

static unsigned int ETMEEPromPrivateWritePageInternal(unsigned int page_number, unsigned int *data);
static unsigned int ETMEEPromPrivateReadPageInternal(unsigned int page_number, unsigned int *data);
static unsigned int ETMEEPromPrivateWritePageI2C(unsigned int page_number, unsigned int *data);
static unsigned int ETMEEPromPrivateReadPageI2C(unsigned int page_number, unsigned int *data);
static unsigned int ETMEEPromPrivateWritePageSPI(unsigned int page_number, unsigned int *data);
static unsigned int ETMEEPromPrivateReadPageSPI(unsigned int page_number, unsigned int *data);

void ETMEEPromConfigureI2CDevice(unsigned int size_bytes, unsigned long fcy_clk, unsigned long i2c_baud_rate, unsigned char i2c_address, unsigned char i2c_port) {
  external_eeprom_I2C.address = i2c_address;
  external_eeprom_I2C.i2c_port = i2c_port;
  external_eeprom_I2C.size_bytes = size_bytes;
  ConfigureI2C(external_eeprom_I2C.i2c_port, I2CCON_DEFAULT_SETUP_PIC30F, i2c_baud_rate, fcy_clk, 0);
}


void ETMEEPromConfigureSPIDevice(unsigned int size_bytes,
				 unsigned long fcy_clk,
				 unsigned long spi_bit_rate,
				 unsigned int  spi_port,
				 unsigned long pin_chip_select_not,
				 unsigned long pin_hold,
				 unsigned long pin_write_protect) {
  // DPARKER need to write this

  external_eeprom_SPI.size_bytes = size_bytes;
}



void ETMEEPromUseInternal(void) {
  etm_eeprom_selected_device = ETM_EEPROM_INTERNAL_SELECTED;
}

void ETMEEPromUseI2C(void) {
  etm_eeprom_selected_device = ETM_EEPROM_I2C_SELECTED;
}

void ETMEEPromUseSPI(void) {
  etm_eeprom_selected_device = ETM_EEPROM_SPI_SELECTED;
}


unsigned int ETMEEPromReturnActiveEEProm(void) {
  return etm_eeprom_selected_device;
}


unsigned int ETMEEPromReadPage(unsigned int page_number, unsigned int *page_data) {
  /*
    Data will be returned in *page_data
    If the function return is 0xFFFF the data is valid
    If the function return is 0, the data is NOT valid
  */

  page_number = page_number * 2;
  if (ETMEEPromPrivateReadSinglePage(page_number, &page_data[0])) {
    return 0xFFFF;
  }
  
  if (ETMEEPromPrivateReadSinglePage(page_number + 1, &page_data[0])) {
    return 0xFFFF;
  }

  return 0;
}


unsigned int ETMEEPromWritePage(unsigned int page_number, unsigned int *page_data) {
  /* 
     Will return 0xFFFF if there were no write errors
     Will return 0 if there was a write error to one (or both) registers
  */
  unsigned int error_check = 0xFFFF;

  page_number = page_number * 2;
  
  error_check &= ETMEEPromPrivateWriteSinglePage(page_number, &page_data[0]);
  
  error_check &= ETMEEPromPrivateWriteSinglePage(page_number+1, &page_data[0]);
  
  return error_check;
}


unsigned int ETMEEPromWritePageFast(unsigned int page_number, unsigned int *page_data) {
  unsigned long long alternate_test = 0;
  unsigned int actual_page;
  /*
    Alternate writing between pages
    This should be used for something like pulse counter or off time where it doesn't cause a problem if we miss a single update
    Will return 0xFFFF if there were no write errors
    Will return 0 if there was a write error
  */
  
  actual_page = page_number * 2;

  if (page_number <= 63) {
    alternate_test = alternate_page_data >> page_number;
    alternate_test &= 1;

    if (alternate_test) {
      // use the alternate page register
      actual_page++;
    }
    
    alternate_test = 1;
    alternate_test <<= page_number;
    alternate_page_data ^= alternate_test;
  } 
  
  return (ETMEEPromPrivateWriteSinglePage(actual_page, &page_data[0]));
}


unsigned int ETMEEPromWritePageWithConfirmation(unsigned int page_number, unsigned int *page_data) {
  /*
    Writes to the A and B pages and confirms the values after writing
    This could take a long time to execute
    Will return 0xFFFF if the page data is confirmed in both registers
    Will return 0 if there were any errors (write/read/crc/confirmation)
  */


  unsigned int page_read[16];
  page_number = page_number * 2;
  
  // Write and check page A, abort on error
  if (ETMEEPromPrivateWriteSinglePage(page_number, &page_data[0]) == 0) {
    return 0;
  }
  
  if (ETMEEPromPrivateReadSinglePage(page_number, &page_read[0]) == 0) {
    return 0;
  }
  

  // DPARKER Figure out how to confirm the data sent matches the data read back
  
  
  // Repeat for B register
  if (ETMEEPromPrivateWriteSinglePage(page_number + 1, &page_data[0]) == 0) {
    return 0;
  }
  
  if (ETMEEPromPrivateReadSinglePage(page_number + 1, &page_read[0]) == 0) {
    return 0;
  }
  
  // DPARKER Figure out how to confirm the data sent matches the data read back

  return 0xFFFF; // The write operation was sucessful
}


unsigned int ETMEEPromWriteWordWithConfirmation(unsigned int register_location, unsigned int data) {
  unsigned int page_read[16];
  unsigned int page_number;

  page_number = (register_location >> 4);
  register_location &= 0x0F;
  
  // Load the existing contents of the register
  if (ETMEEPromReadPage(page_number, &page_read[0]) == 0) {
    return 0;
  }
  
  // Modify the value being changed
  *(&page_read[0] + register_location*2) = data;
  
  // Write the data back to EEProm
  if (ETMEEPromWritePageWithConfirmation(page_number, &page_read[0])) {
    return 0xFFFF;
  }
  
  return 0;
}


unsigned int ETMEEPromPrivateReadSinglePage(unsigned int page_number, unsigned int *page_data) {
  unsigned int crc_check;

  switch (ETMEEPromReturnActiveEEProm()) {
    
  case ETM_EEPROM_INTERNAL_SELECTED:
    eeprom_debug_data.page_read_count_internal++;
    if (ETMEEPromPrivateReadPageInternal(page_number, &page_data[0]) == 0) {
      eeprom_debug_data.read_internal_error++;
      return 0;
    } 
    break;
    
  case ETM_EEPROM_I2C_SELECTED:
    eeprom_debug_data.page_read_count_i2c++;
    if (ETMEEPromPrivateReadPageI2C(page_number, &page_data[0]) == 0) {
      eeprom_debug_data.read_i2c_error++;
      return 0;
    }
    break;
    
  case ETM_EEPROM_SPI_SELECTED:
    eeprom_debug_data.page_read_count_spi++;
    if (ETMEEPromPrivateReadPageSPI(page_number, &page_data[0]) == 0) {
      eeprom_debug_data.read_spi_error++;
      return 0;
    }
    break;

  default:
    return 0;
    break;
    
  }

  crc_check = ETMCRCModbus(&page_data[0], 30);
  if (crc_check != page_data[15]) {
    eeprom_debug_data.crc_error++;
    return 0;
  }
  return 0xFFFF;
}


unsigned int ETMEEPromPrivateWriteSinglePage(unsigned int page_number, unsigned int *page_data) {
  unsigned int crc;
  crc = ETMCRCModbus(&page_data[0], 30);
  page_data[15] = crc;

  switch (ETMEEPromReturnActiveEEProm()) {
    
  case ETM_EEPROM_INTERNAL_SELECTED:
    eeprom_debug_data.page_write_count_internal++;
    if (ETMEEPromPrivateWritePageInternal(page_number, &page_data[0])) {
      return 0xFFFF;
    } else {
      eeprom_debug_data.write_internal_error++;
      return 0;
    } 
    break;

  case ETM_EEPROM_I2C_SELECTED:
    eeprom_debug_data.page_write_count_i2c++;
    if (ETMEEPromPrivateWritePageI2C(page_number, &page_data[0])) {
      return 0xFFFF;
    } else {
      eeprom_debug_data.write_i2c_error++;
      return 0;
    }
    break;

  case ETM_EEPROM_SPI_SELECTED:
    eeprom_debug_data.page_write_count_spi++;
    if (ETMEEPromPrivateWritePageSPI(page_number, &page_data[0])) {
      return 0xFFFF;
    } else {
      eeprom_debug_data.write_spi_error++;
      return 0;
    }
    break;

  default:
    return 0;
    break;
  }

  return 0;
}




#if INTERNAL_EEPROM_SIZE_WORDS == 0
unsigned int ETMEEPromPrivateWritePageInternal(unsigned int page_number, unsigned int *data) {
  return 0;
}

unsigned int ETMEEPromPrivateReadPageInternal(unsigned int page_number, unsigned int *data) {
  return 0;
}

#else

__eds__ unsigned int internal_eeprom[INTERNAL_EEPROM_SIZE_WORDS] __attribute__ ((space(eedata))) = {};

unsigned int ETMEEPromPrivateWritePageInternal(unsigned int page_number, unsigned int *data) {
  _prog_addressT write_address;
  unsigned int n = 0;
  int data_ram[16];
  unsigned int words_to_write;

  // DPARKER, remove the data_ram variable and write directly with data pointer

  words_to_write = 16;
  
  while (n < words_to_write) {
    data_ram[n] = (int)data[n];
    n++;
  }
  
  if (page_number > (INTERNAL_EEPROM_SIZE_WORDS >> 4)) {
    return 0;
  }
    
  write_address = __builtin_tbladdress(internal_eeprom);
  write_address += page_number << 5;
  _wait_eedata();
  _erase_eedata(write_address, _EE_ROW);
  _wait_eedata();
  _write_eedata_row(write_address, data_ram);
  
  return 0xFFFF;
}


unsigned int ETMEEPromPrivateReadPageInternal(unsigned int page_number, unsigned int *data) {
  unsigned int starting_register;
  unsigned int n;
  unsigned int words_to_read;

  words_to_read = 16;
  starting_register = page_number*16;
  _wait_eedata();
 
  if (page_number > (INTERNAL_EEPROM_SIZE_WORDS >> 4)) {
    return 0;
  }
   
  for (n = 0; n < words_to_read ; n++) {
    data[n] = internal_eeprom[starting_register + n];
  }
  
  return 0xFFFF;
}

#endif

unsigned int ETMEEPromPrivateWritePageI2C(unsigned int page_number, unsigned int *data) {
  unsigned int temp;
  unsigned char adr_high_byte;
  unsigned char adr_low_byte;
  unsigned char data_low_byte;
  unsigned char data_high_byte;
  unsigned int error_check;
  unsigned int n;
  unsigned int busy_count;
  unsigned int busy;
  unsigned int words_to_write;

  words_to_write = 16;

  if (page_number > (external_eeprom_I2C.size_bytes >> 5)) {
    // The requeted page address is outside the boundry of this device.
    return 0;
  }

  
  temp = (page_number << 5);
  adr_high_byte = (temp >> 8);
  adr_low_byte = (temp & 0x00FF);
  
  
  error_check = WaitForI2CBusIdle(external_eeprom_I2C.i2c_port);
  error_check |= GenerateI2CStart(external_eeprom_I2C.i2c_port);
  error_check |= WriteByteI2C(external_eeprom_I2C.address | I2C_WRITE_CONTROL_BIT, external_eeprom_I2C.i2c_port);  
  busy = _ACKSTAT;
  busy_count = 0;
  
  // DPARKER, change busy count to use ETM_TICK
  while (busy && (busy_count <= 200)) {
    error_check |= GenerateI2CStop(external_eeprom_I2C.i2c_port);
    error_check = WaitForI2CBusIdle(external_eeprom_I2C.i2c_port);
    error_check |= GenerateI2CStart(external_eeprom_I2C.i2c_port);
    error_check |= WriteByteI2C(external_eeprom_I2C.address | I2C_WRITE_CONTROL_BIT, external_eeprom_I2C.i2c_port);
    busy = _ACKSTAT;
    busy_count++;
  }
  
  
  error_check |= WriteByteI2C(adr_high_byte, external_eeprom_I2C.i2c_port);
  error_check |= WriteByteI2C(adr_low_byte, external_eeprom_I2C.i2c_port);
  
  for (n = 0; n < words_to_write ; n++) {
    data_high_byte = (data[n] >> 8);
    data_low_byte = (data[n] & 0x00FF);
    error_check |= WriteByteI2C(data_low_byte, external_eeprom_I2C.i2c_port);
    error_check |= WriteByteI2C(data_high_byte, external_eeprom_I2C.i2c_port);    
  }

    
  error_check |= GenerateI2CStop(external_eeprom_I2C.i2c_port);
  
  if (error_check) {
    return 0;
  }
  return 0xFFFF;
}


unsigned int ETMEEPromPrivateReadPageI2C(unsigned int page_number, unsigned int *data) {
  unsigned int error_check;
  unsigned int temp;
  unsigned char adr_high_byte;
  unsigned char adr_low_byte;
  unsigned char data_low_byte;
  unsigned char data_high_byte;
  unsigned int n;
  unsigned int busy_count;
  unsigned int busy;
  unsigned int words_to_read;

  words_to_read = 16;

  if (page_number > (external_eeprom_I2C.size_bytes >> 5) ) {
    // The requeted page address is outside the boundry of this device.
    return 0;
  }
    
  temp = (page_number << 5);
  adr_high_byte = (temp >> 8);
  adr_low_byte = (temp & 0x00FF);
  
  error_check = WaitForI2CBusIdle(external_eeprom_I2C.i2c_port);
  error_check |= GenerateI2CStart(external_eeprom_I2C.i2c_port);      
  error_check |= WriteByteI2C(external_eeprom_I2C.address | I2C_WRITE_CONTROL_BIT, external_eeprom_I2C.i2c_port);
  busy = _ACKSTAT;
  busy_count = 0;
  while (busy && (busy_count <= 200)) {
    error_check |= GenerateI2CStop(external_eeprom_I2C.i2c_port);
    error_check = WaitForI2CBusIdle(external_eeprom_I2C.i2c_port);
    error_check |= GenerateI2CStart(external_eeprom_I2C.i2c_port);
    error_check |= WriteByteI2C(external_eeprom_I2C.address | I2C_WRITE_CONTROL_BIT, external_eeprom_I2C.i2c_port);
    busy = _ACKSTAT;
    busy_count++;
  }
  
  error_check |= WriteByteI2C(adr_high_byte, external_eeprom_I2C.i2c_port);
  error_check |= WriteByteI2C(adr_low_byte, external_eeprom_I2C.i2c_port);
  
  error_check |= GenerateI2CRestart(external_eeprom_I2C.i2c_port);
  error_check |= WriteByteI2C(external_eeprom_I2C.address | I2C_READ_CONTROL_BIT, external_eeprom_I2C.i2c_port);
  
  for (n = 0; n < words_to_read-1 ; n++) {
    data_low_byte = ReadByteI2C(external_eeprom_I2C.i2c_port);
    error_check |= GenerateI2CAck(external_eeprom_I2C.i2c_port);
    data_high_byte = ReadByteI2C(external_eeprom_I2C.i2c_port);
    error_check |= GenerateI2CAck(external_eeprom_I2C.i2c_port);
    data[n] = data_high_byte;
    data[n] <<= 8;
    data[n] += data_low_byte; 
  }
  
  data_low_byte = ReadByteI2C(external_eeprom_I2C.i2c_port);
  error_check |= GenerateI2CAck(external_eeprom_I2C.i2c_port);
  data_high_byte = ReadByteI2C(external_eeprom_I2C.i2c_port);
  error_check |= GenerateI2CNack(external_eeprom_I2C.i2c_port);
  data[n] = data_high_byte;
  data[n] <<= 8;
  data[n] += data_low_byte; 
  
    
  error_check |= GenerateI2CStop(external_eeprom_I2C.i2c_port);
  
  if (error_check) {
    return 0;
  }
  
  return 0xFFFF;
}



unsigned int ETMEEPromPrivateWritePageSPI(unsigned int page_number, unsigned int *data) {
  return 0;
}

unsigned int ETMEEPromPrivateReadPageSPI(unsigned int page_number, unsigned int *data) {
  return 0;
}


unsigned int ETMEEPromReturnDebugData(unsigned int debug_data_index) {
  unsigned int error_return;
  switch (debug_data_index) {
    
  case ETM_EEPROM_DEBUG_DATA_READ_INTERNAL_ERROR:
    error_return = eeprom_debug_data.read_internal_error;
    break;

  case ETM_EEPROM_DEBUG_DATA_READ_INTERNAL_COUNT:
    error_return = eeprom_debug_data.page_read_count_internal;
    break;

  case ETM_EEPROM_DEBUG_DATA_WRITE_INTERNAL_ERROR:
    error_return = eeprom_debug_data.write_internal_error;
    break;

  case ETM_EEPROM_DEBUG_DATA_WRITE_INTERNAL_COUNT:
    error_return = eeprom_debug_data.page_write_count_internal;
    break;

  case ETM_EEPROM_DEBUG_DATA_READ_SPI_ERROR:
    error_return = eeprom_debug_data.read_spi_error;
    break;

  case ETM_EEPROM_DEBUG_DATA_READ_SPI_COUNT:
    error_return = eeprom_debug_data.page_read_count_spi;
    break;

  case ETM_EEPROM_DEBUG_DATA_WRITE_SPI_ERROR:
    error_return = eeprom_debug_data.write_spi_error;
    break;

  case ETM_EEPROM_DEBUG_DATA_WRITE_SPI_COUNT:
    error_return = eeprom_debug_data.page_write_count_spi;
    break;

  case ETM_EEPROM_DEBUG_DATA_READ_I2C_ERROR:
    error_return = eeprom_debug_data.read_i2c_error;
    break;

  case ETM_EEPROM_DEBUG_DATA_READ_I2C_COUNT:
    error_return = eeprom_debug_data.page_read_count_i2c;
    break;

  case ETM_EEPROM_DEBUG_DATA_WRITE_I2C_ERROR:
    error_return = eeprom_debug_data.write_i2c_error;
    break;

  case ETM_EEPROM_DEBUG_DATA_WRITE_I2C_COUNT:
    error_return = eeprom_debug_data.page_write_count_i2c;
    break;

  case ETM_EEPROM_DEBUG_DATA_CRC_ERROR:
    error_return = eeprom_debug_data.crc_error;
    break;

  }
  return error_return;

}

