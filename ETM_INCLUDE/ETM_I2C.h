/*
  This is an extension of the built in I2C functions that provides more robust operation and easier use.
  This has been written work on devices with 1 or 2 I2C ports.

  
  Dan Parker
  2013_04_02


  Portions written for devices with 2 i2c ports has been commented out- this functionality needs more work. 
  Thea Henry 9/4/2015.

*/

/*
  This module has been validated on A34760 using MCP23017 functions (pic30F6014A).
  All other applications are suspect
*/


#ifndef __ETM_I2C_H
#define __ETM_I2C_H

#define ETM_I2C_VERSION          03


#define I2CCON_DEFAULT_SETUP_30F6014A         0b1011000000100000
#define I2CCON_DEFAULT_SETUP_PIC30F           0b1011000000100000


#define I2C_WRITE_CONTROL_BIT   0b00000000
#define I2C_READ_CONTROL_BIT    0b00000001

#define ETM_I2C_400K_BAUD       400000
#define ETM_I2C_100K_BAUD       100000


void ConfigureI2C(unsigned char i2c_port, unsigned int configuration, unsigned long baud_rate, unsigned long fcy_clk, unsigned long pulse_gobbler_delay_fcy);
/*
  This function sets up the selected i2c_port with the configuation parameter that is passed in.
  It also calculates the baud rate register based on the selected baud rate and the system clock.

  NOTE - pulse_gobbler_delay_fcy is no longer unused,  set it to zero (or any other value), it has not been removed to preserve compatability
*/


unsigned int WaitForI2CBusIdle(unsigned char i2c_port);
/*
  This will wait for bus idle on the specified I2C Bus
  
  The result will be 0x0000 if everything works properly
  The result will be 0x0100 if there is an I2C bus fault
*/

unsigned int GenerateI2CStart(unsigned char i2c_port);
/*
  This will generate a start condition on the specified I2C bus
  
  The result will be 0x0000 if everything works properly
  The result will be 0x0200 if there is an I2C bus fault
*/

unsigned int GenerateI2CRestart(unsigned char i2c_port);
/*
  This will generate a re-start condition on the specified I2C bus
  
  The result will be 0x0000 if everything works properly
  The result will be 0x0400 if there is an I2C bus fault
*/

unsigned int WriteByteI2C(unsigned char data, unsigned char i2c_port);
/*
  This will write a byte to the specified I2C bus
  
  The result will be 0x0000 if everything works properly
  The result will be 0x0800 if there is an I2C bus fault
*/

unsigned int ReadByteI2C(unsigned char i2c_port);
/*
  This will read a byte from the specified I2C bus
  
  The result will be 0x00dd if everything works properly
  The result will be 0x1000 if there is an I2C bus fault
*/


unsigned int GenerateI2CStop(unsigned char i2c_port);
/*
  This will generate a stop condition on the specified I2C bus
  
  The result will be 0x0000 if everything works properly
  The result will be 0x2000 if there is an I2C bus fault
*/

unsigned int GenerateACK(unsigned char i2c_port);
/*
  This will generate a master ACK sequence

  The result will be 0x0000 if everything works properly
  The result will be 0x4000 if there is an I2C bus fault
*/
#define GenerateI2CAck GenerateACK

unsigned int GenerateNACK(unsigned char i2c_port);
/*
  This will generate a master NACK sequence

  The result will be 0x0000 if everything works properly
  The result will be 0x8000 if there is an I2C bus fault
*/
#define GenerateI2CNack GenerateNACK

void ClearI2CBus(unsigned char i2c_port);
/*
  This function is used to clear an I2C bus that is being help by a slave after reset.
  It sends manually clocks out an 
  I2CStart
  9 Clocks
  I2CStart
  I2CStop

  This will clear a bus that is being help by an external slave device

  It will restore the I2CCON state after clearing the bus.

  This function is called as part of ConfigureI2C, but can also be called independently
*/



#define I2C_PORT                 0
#define I2C_PORT_1               1
#define I2C_PORT_2               2

extern unsigned int etm_i2c1_error_count; // This global variable counts the number of i2c_errors.  It may be useful for code bebugging and validation

#if defined(_I2C2MD)
extern unsigned int etm_i2c2_error_count; // This global variable counts the number of i2c_errors.  It may be useful for code bebugging and validation
#endif


#ifdef __dsPIC30F6014A__
#define PULSE_GOBBLER_DELAY_HZ 3333333
#else
#define PULSE_GOBBLER_DELAY_HZ 3333333
#endif



#endif
