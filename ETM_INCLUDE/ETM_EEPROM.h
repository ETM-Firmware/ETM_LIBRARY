#ifndef __ETM_EEPROM_H
#define __ETM_EEPROM_H
/*
  This is a common module that can be used with internal or external (Flash or FRAM based) eeprom
*/

#define ETM_EEPROM_VERSION 0x0002

void ETMEEPromWriteWord(unsigned int register_location, unsigned int data);
/*
  This writes a single word to the EEprom
  register_location is NOT the internal address. It is the position in words (instead of bytes)
*/

unsigned int ETMEEPromReadWord(unsigned int register_location);
/*
  This reads a single word from the EEprom
  register_location is NOT the internal address. It is the position in words (instead of bytes)
*/

void ETMEEPromWritePage(unsigned int page_number, unsigned int words_to_write, unsigned int *data);
/*
  This writes data from the data array into the eeprom.

  These are page aligned instructions (16 words per page) even on devices that do not have pages.
  If words_to_write > 16 it will be truncated to 16.

*/

void ETMEEPromReadPage(unsigned int page_number, unsigned int words_to_read, unsigned int *data);
/*
  This reads data from the eeprom into the data array

  These are page aligned instructions (16 words per page) even on devices that do not have pages.
  If words_to_read > 16 it will be truncated to 16.
  
  NOTE!!!!
  DO NOT READ MORE WORDS THAN YOU HAVE ALLOCATED TO YOUR DATA ARRAY
  IF YOU DO, THE VALUES IN RAM AT ADDRESSES PROCEEDING YOUR ARRAY WILL GET CLOBBERED 
*/

void ETMEEPromUseInternal(void);
/*
  Tells the module to direct all EEPROM read/writes to the internal EEPROM
  If the device does not have an enternal EEPROM, the read commands will all return zero
  
  The ETMEEPromUseInternal and ETMEEPromUseExternal can be used to switch between the EEProms within the application
*/

void ETMEEPromUseExternal(void);
/*
  Tells the module to direct all EEPROM read/writes to the external EEPROM
  In order to use the external EEPROM, ETMEEPromConfigureExternalDevice must be called
  If the devide does not have an I2C port or an external EEPROM, weird things may happen
  
  The ETMEEPromUseInternal and ETMEEPromUseExternal can be used to switch between the EEProms within the application
*/


unsigned int ETMEEPromInternalSelected(void);
/*
  Returns 1 if the internal EEPROM is currently selected for read/write operations
  Reutrns 0 if the external EEPROM is currently selected for read/write operations
*/



void ETMEEPromConfigureExternalDevice(unsigned int size_bytes, unsigned long fcy_clk, unsigned long i2c_baud_rate, unsigned char i2c_address, unsigned char i2c_port);
/*
  This configures the EEPROM and initializes the eeprom
  It should only be called if using an external EEPROM
  If the internal EEPROM is used this will do nothing
*/


// Used with external eeprom
#define EEPROM_I2C_ADDRESS_0                 0b10100000
#define EEPROM_I2C_ADDRESS_1                 0b10100010
#define EEPROM_I2C_ADDRESS_2                 0b10100100
#define EEPROM_I2C_ADDRESS_3                 0b10100110
#define EEPROM_I2C_ADDRESS_4                 0b10101000
#define EEPROM_I2C_ADDRESS_5                 0b10101010
#define EEPROM_I2C_ADDRESS_6                 0b10101100
#define EEPROM_I2C_ADDRESS_7                 0b10101110

#define EEPROM_SIZE_8K_BYTES                 0x2000
#define EEPROM_SIZE_64K_BITS                 0x2000


#endif
