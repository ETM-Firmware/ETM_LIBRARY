#include <xc.h>
#include <libpic30.h>
#include "ETM_I2C.h"


/***************************************************

Portions written for devices with 2 i2c ports has been commented out- this functionality needs more work. 
Thea Henry 9/4/2015.

**************************************************/
unsigned int etm_i2c1_error_count = 0;

#if defined(_I2C1MD)  
unsigned int etm_i2c2_error_count = 0;
#endif

unsigned int etm_i2c_loop_timeout;

#ifdef __dsPIC30F6014A__
   #define I2C_1_CLK_TRIS  _TRISG2
   #define I2C_1_CLK_PIN   _LATG2
   #define I2C_1_DATA_TRIS _TRISG3
   #define I2C_1_DATA_PIN  _LATG3
#endif 

#ifdef __dsPIC30F6010A__
   #define I2C_1_CLK_TRIS  _TRISG2
   #define I2C_1_CLK_PIN   _LATG2
   #define I2C_1_DATA_TRIS _TRISG3
   #define I2C_1_DATA_PIN  _LATG3
#endif


void ConfigureI2C(unsigned char i2c_port, unsigned int configuration, unsigned long baud_rate, unsigned long fcy_clk, unsigned long pulse_gobbler_delay_fcy) {
  unsigned long baud_rate_register;

  ClearI2CBus(i2c_port);

  baud_rate_register = fcy_clk/baud_rate;
  etm_i2c_loop_timeout = baud_rate_register * 16;
  /*
    Each Loop that gets timed is at least 10 clock cycles.
    That means our timeout is at least 10*16 I2C Clocks = 160 I2C Clocks.
  */
  // DPARKER this is probably too long

  baud_rate_register -= fcy_clk/PULSE_GOBBLER_DELAY_HZ;
  baud_rate_register -= 1;

#if defined(__dsPIC33F__)
  baud_rate_register -= 1;
#endif 
  
  if (baud_rate_register < 2) {
    baud_rate_register = 2;
  }
  if (baud_rate_register > 0xFFFF) {
    baud_rate_register = 2;
  }

#if defined(_I2CMD)
  if ((i2c_port == 0) || (i2c_port == 1)) {
    I2CCON = configuration;
    I2CBRG = baud_rate_register;
  } 
#endif
 /************************** 
#if defined(_I2C1MD)  
  if (i2c_port = 1) {
    I2C1CON = configuration;
    I2C1BRG = baud_rate_register;
  }
#endif    
  
#if defined(_I2C2MD) 
  if (i2c_port = 2) {
    I2C2CON = configuration;
    I2C2BRG = baud_rate_register;
  }
#endif
*********************************/
}

#define I2C_CLOCK_HALF_PERIOD  50

void ClearI2CBus(unsigned char i2c_port) {
  unsigned int i2ccon_save;
  unsigned char n;
#if defined(_I2CMD)
  if ((i2c_port == 0) || (i2c_port == 1)) {
    i2ccon_save = I2CCON;
    I2C_1_CLK_TRIS = 1;
    I2C_1_DATA_TRIS = 1;
    I2C_1_DATA_PIN = 0;
    I2C_1_CLK_PIN  = 0;
    I2CCON = 0;
    
    // Send a start condition
    I2C_1_DATA_TRIS = 1; // This should allow the Data line to be pulled high (assuming a device is not holding it low)
    I2C_1_CLK_TRIS  = 1; // Allow the clock pin to be pulled high by external resistor
    __delay32(I2C_CLOCK_HALF_PERIOD*4);
    I2C_1_DATA_TRIS = 0; // Actively pull the data pin low
    __delay32(I2C_CLOCK_HALF_PERIOD*4);
    I2C_1_DATA_TRIS = 1; // Release the data pin
    
    // Send out 9 Clock Pulses
    for (n=0; n<9; n++) {
      I2C_1_CLK_TRIS = 0; // Actively pull the clock pin low
      __delay32(I2C_CLOCK_HALF_PERIOD);
      I2C_1_CLK_TRIS = 1; // Allow the clock pin to be pulled high by external resistor
      __delay32(I2C_CLOCK_HALF_PERIOD);
    }
    
    // Send a start condition
    I2C_1_DATA_TRIS = 1; // This should allow the Data line to be pulled high (assuming a device is not holding it low)
    I2C_1_CLK_TRIS  = 1; // Allow the clock pin to be pulled high by external resistor
    __delay32(I2C_CLOCK_HALF_PERIOD*4);
    I2C_1_DATA_TRIS = 0; // Actively pull the data pin low
    __delay32(I2C_CLOCK_HALF_PERIOD*4);
    
    // Send a stop condition
    I2C_1_DATA_TRIS = 0; // Actively pull the data line low
    I2C_1_CLK_TRIS  = 1; // Allow the clock pin to be pulled high by external resistor
    __delay32(I2C_CLOCK_HALF_PERIOD*4);
    I2C_1_DATA_TRIS = 1; // Allow the Data line to be pulled high
    __delay32(I2C_CLOCK_HALF_PERIOD*4);
    
    I2C_1_CLK_TRIS = 1;
    I2C_1_DATA_TRIS = 1;
    I2CCON = i2ccon_save;
  }
#endif  
}

unsigned int WaitForI2CBusIdle(unsigned char i2c_port) {
  
  unsigned int loop_counter;
  unsigned int i2c_result;

  i2c_result = 0;

#if defined(_I2CMD)
  if ((i2c_port == 0) || (i2c_port == 1)) {
    //Wait for bus Idle or Timeout
    loop_counter = 0;
    while (I2CSTATbits.TRSTAT) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0100;
	etm_i2c1_error_count++;
	break;	
      }
    }
  }
#endif
/************************************************
#if defined(_I2C1MD)
  if (i2c_port == 1) {
    //Wait for bus Idle or Timeout
    loop_counter = 0;
    while (I2C1STATbits.TRSTAT) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0100;
	etm_i2c1_error_count++;
	break;	
      }
    }
  }
#endif

#if defined(_I2C2MD)
  if (i2c_port == 2) {
    //Wait for bus Idle or Timeout
    loop_counter = 0;
    while (I2C2STATbits.TRSTAT) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0100;
	etm_i2c2_error_count++;
	break;
      }
    }
  }
#endif
   ***********************************************/
  return i2c_result;
}
 




unsigned int GenerateI2CStart(unsigned char i2c_port) {

  unsigned int loop_counter;
  unsigned int i2c_result;

  i2c_result = 0;
  
#if defined(_I2CMD)
  if ((i2c_port == 0) || (i2c_port == 1)) {
    I2CCONbits.SEN = 1;		                         //Generate Start COndition
    //Wait for Start COndition or Timeout
    loop_counter = 0;
    while (I2CCONbits.SEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0200;
	etm_i2c1_error_count++;
	break;
      }
    }
  }
#endif
  /**********************************
#if defined(_I2C1MD)
  if (i2c_port == 1) {
    I2C1CONbits.SEN = 1;	                         //Generate Start COndition
    //Wait for Start COndition or Timeout
    loop_counter = 0;
    while (I2C1CONbits.SEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0200;
	etm_i2c1_error_count++;
	break;
      }
    }
  }
#endif
  
#if defined(_I2C2MD)
  if (i2c_port == 2) {
    I2C2CONbits.SEN = 1;	                         //Generate Start COndition
    //Wait for Start COndition or Timeout
    loop_counter = 0;
    while (I2C2CONbits.SEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0200;
	etm_i2c2_error_count++;
	break;
      }
    }
  }
#endif
  ****************************************/
  return i2c_result;
}

  

unsigned int GenerateI2CRestart(unsigned char i2c_port) {
  unsigned int loop_counter;
  unsigned int i2c_result;
  
  i2c_result = 0;
  
#if defined(_I2CMD)
  if ((i2c_port == 0) || (i2c_port == 1)) {
    I2CCONbits.RSEN = 1;	                         //Generate Re-Start COndition
    //Wait for Re-Start COndition
    loop_counter = 0;
    while (I2CCONbits.RSEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0400;
	etm_i2c1_error_count++;
	break;
      }
    }
  }
#endif

/*******************************************************
#if defined(_I2C1MD)
  if (i2c_port == 1) {
    I2C1CONbits.RSEN = 1;		                 //Generate Re-Start COndition
    //Wait for Re-Start COndition
    loop_counter = 0;
    while (I2C1CONbits.RSEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0400;
	etm_i2c1_error_count++;
	break;
      }
    }
  }
#endif

#if defined(_I2C2MD)
  if (i2c_port == 2) {
    I2C2CONbits.RSEN = 1;		                 //Generate Re-Start COndition
    //Wait for Re-Start COndition
    loop_counter = 0;
    while (I2C2CONbits.RSEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0400;
	etm_i2c2_error_count++;
	break;
      }
    }
  }
#endif
    **************************************************/
  return i2c_result;
}



unsigned int WriteByteI2C(unsigned char data, unsigned char i2c_port) {
  unsigned int loop_counter;
  unsigned int i2c_result;
  
  i2c_result = 0;
  
#if defined(_I2CMD)
  if ((i2c_port == 0) || (i2c_port == 1)) {
    I2CTRN = (data);                                            //Load data to the transmit buffer
    // Wait for the transmit process to start
    loop_counter = 0;
    while (!I2CSTATbits.TRSTAT) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0800;
	etm_i2c1_error_count++;
	break;
      }
    }
    // Wait for the transmit process to complete (cleared at end of Slave ACK)
    while (I2CSTATbits.TRSTAT) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0800;
	etm_i2c1_error_count++;
	break;
      }
    }
  }
#endif

/************************************************************
#if defined(_I2C1MD)
  if (i2c_port == 1) {
    I2C1TRN = (data);                                           //Load data to the transmit buffer
    // Wait for the transmit process to start
    loop_counter = 0;
    while (!I2C1STATbits.TRSTAT) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0800;
	etm_i2c1_error_count++;
	break;
      }
    }
    // Wait for the transmit process to complete (cleared at end of Slave ACK)
    while (I2C1STATbits.TRSTAT) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0800;
	etm_i2c1_error_count++;
	break;
      }
    }
  }
#endif

#if defined(_I2C2MD)
  if (i2c_port == 2) {
    I2C2TRN = (data);                                           //Load data to the transmit buffer
    // Wait for the transmit process to start
    loop_counter = 0;
    while (!I2C2STATbits.TRSTAT) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0800;
	etm_i2c2_error_count++;
	break;
      }
    }
    // Wait for the transmit process to complete (cleared at end of Slave ACK)
    while (I2C2STATbits.TRSTAT) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x0800;
	etm_i2c2_error_count++;
	break;
      }
    }
  }
#endif
    ***************************************************/
  return i2c_result;
}

unsigned int ReadByteI2C(unsigned char i2c_port) {
  unsigned int loop_counter;
  unsigned int i2c_result;
  
  i2c_result = 0;

#if defined(_I2CMD)
  if ((i2c_port == 0) || (i2c_port == 1)) {
    I2CCONbits.RCEN = 1;			                 //Start Master receive
    //Wait for data transfer
    loop_counter = 0;
    while (I2CCONbits.RCEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x1000;
	etm_i2c1_error_count++;
	break;
      }
    }
    //Wait for receive bufer to be full
    while (!I2CSTATbits.RBF) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x1000;
	etm_i2c1_error_count++;
	break;
      }
    }
    if (i2c_result == 0) {
      i2c_result = (I2CRCV & 0x00FF);
    }
  }
#endif

/***********************************************************
#if defined(_I2C1MD)
  if (i2c_port == 1) {
    I2C1CONbits.RCEN = 1;			                 //Start Master receive
    //Wait for data transfer
    loop_counter = 0;
    while (I2C1CONbits.RCEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x1000;
	etm_i2c1_error_count++;
	break;
      }
    }
    //Wait for receive bufer to be full
    while (!I2C1STATbits.RBF) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x1000;
	etm_i2c1_error_count++;
	break;
      }
    }
    if (i2c_result == 0) {
      i2c_result = (I2C1RCV & 0x00FF);
    }
  }
#endif

#if defined(_I2C2MD)
  if (i2c_port == 2) {
    I2C2CONbits.RCEN = 1;			                 //Start Master receive
    //Wait for data transfer
    loop_counter = 0;
    while (I2C2CONbits.RCEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x1000;
	etm_i2c2_error_count++;
	break;
      }
    }
    //Wait for receive bufer to be full
    while (!I2C2STATbits.RBF) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x1000;
	etm_i2c2_error_count++;
	break;
      }
    }
    if (i2c_result == 0) {
      i2c_result = (I2C2RCV & 0x00FF);
    }
  }
#endif
 **********************************************************/   
  return i2c_result;
}



unsigned int GenerateI2CStop(unsigned char i2c_port) {
  unsigned int loop_counter;
  unsigned int i2c_result;
  
  i2c_result = 0;

#if defined(_I2CMD)
  if ((i2c_port == 0) || (i2c_port == 1)) {
    I2CCONbits.PEN = 1;	                                 //Generate Stop COndition
    //Wait for Stop COndition or Timeout
    loop_counter = 0;
    while (I2CCONbits.PEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x2000;
	etm_i2c1_error_count++;
	break;
      }
    }
  }
#endif

/**************************************************************
#if defined(_I2C1MD)
  if (i2c_port == 1) {
    I2C1CONbits.PEN = 1;		                 //Generate stop COndition
    //Wait for Stop COndition or Timeout
    loop_counter = 0;
    while (I2C1CONbits.PEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x2000;
	etm_i2c1_error_count++;
	break;
      }
    }
  }
#endif

#if defined(_I2C2MD)
  if (i2c_port == 2) {
    I2C2CONbits.PEN = 1;		                 //Generate stop COndition
    //Wait for Stop COndition or Timeout
    loop_counter = 0;
    while (I2C2CONbits.PEN) {
      loop_counter++;
      if (loop_counter > etm_i2c_loop_timeout) {
	i2c_result = 0x2000;
	etm_i2c2_error_count++;
	break;
      }
    }
  }
#endif
************************************************************/
  return i2c_result;
}

unsigned int GenerateACK(unsigned char i2c_port) {
    unsigned int loop_counter;
    unsigned int ret;
    loop_counter = 0;
    ret = 0;

#if defined(_I2CMD)
    if ((i2c_port == 0) || (i2c_port == 1)) {
        I2CCONbits.ACKDT = 0;   //ACK
        I2CCONbits.ACKEN = 1;
        while (I2CCONbits.ACKEN) {
        loop_counter++;
        if (loop_counter > etm_i2c_loop_timeout) {
            ret = 0x4000;
            etm_i2c1_error_count++;
            break;
          }
        }
    }
#endif

/**********************************************************************
#if defined(_I2C1MD)
    if ((i2c_port == 1) {
        I2C1CONbits.ACKDT = 0;   //ACK
        I2C1CONbits.ACKEN = 1;
        while (I2C1CONbits.ACKEN) {
        loop_counter++;
        if (loop_counter > etm_i2c_loop_timeout) {
            ret = 0x4000;
            etm_i2c1_error_count++;
            break;
          }
        }
    }
#endif

#if defined(_I2C2MD)
    if ((i2c_port == 2) {
        I2C2CONbits.ACKDT = 0;   //ACK
        I2C2CONbits.ACKEN = 1;
        while (I2C2CONbits.ACKEN) {
        loop_counter++;
        if (loop_counter > etm_i2c_loop_timeout) {
            ret = 0x4000;
            etm_i2c1_error_count++;
            break;
          }
        }
    }
#endif
******************************************************************/
    return ret;
}

unsigned int GenerateNACK(unsigned char i2c_port) {
    unsigned int loop_counter;
    unsigned int ret;
    loop_counter = 0;
    ret = 0;

#if defined(_I2CMD)
    if ((i2c_port == 0) || (i2c_port == 1)) {
        I2CCONbits.ACKDT = 1;   //NACK
        I2CCONbits.ACKEN = 1;
        while (I2CCONbits.ACKEN) {
        loop_counter++;
        if (loop_counter > etm_i2c_loop_timeout) {
            ret = 0x8000;
            etm_i2c1_error_count++;
            break;
          }
        }
    }
#endif

/********************************************************************
#if defined(_I2C1MD)
    if ((i2c_port == 0) || (i2c_port == 1)) {
        I2C1CONbits.ACKDT = 1;   //NACK
        I2C1CONbits.ACKEN = 1;
        while (I2C1CONbits.ACKEN) {
        loop_counter++;
        if (loop_counter > etm_i2c_loop_timeout) {
            ret = 0x8000;
            etm_i2c1_error_count++;
            break;
          }
        }
    }
#endif

#if defined(_I2C2MD)
    if ((i2c_port == 0) || (i2c_port == 1)) {
        I2C2CONbits.ACKDT = 1;   //NACK
        I2C2CONbits.ACKEN = 1;
        while (I2C2CONbits.ACKEN) {
        loop_counter++;
        if (loop_counter > etm_i2c_loop_timeout) {
            ret = 0x8000;
            etm_i2c1_error_count++;
            break;
          }`
        }
    }
#endif
***********************************************************************/
    return ret;
}
