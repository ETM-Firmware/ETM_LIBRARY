// ETM MODULE:   REAL TIME CLOCK OVER I2C
//
// MODULE REQUIREMENTS:
//  1. ETM_I2C module
//  2. real_time_clock_I2C_port variable contains a valid port number
//
// MODULE DESCRIPTION:
//      A structure consisting of the Date, Time and Temperature is established
//          and will be used to write/read the variables
//
// MODULE USAGE:
//      Configuration:
//          1. Use the RTC_DEFAULT_CONFIG for standard implementations
//          2. Call ConfigureDS3231().  At a minimum this should be called
//              for only initial power applied.  But it is good practice to
//              call this everytime the system powers up.
//          3. The processor will be busy while this is loaded into the RTC
//          4. If zero is returned then the operation was successful
//      To Set the Date and Time:
//          1. Store the new date and time in the structure
//          2. Call SetDateAndTime Routine
//          3. The processor will be busy while all these parameters are written
//          4. If zero is returned then the operation was successful
//      To Get the current Date and Time
//          1. Call ReadDateAndTime()
//          2. The structure will be updated with the current RTC
//          3. The processor will be busy while these settings are written
//          4. If zero is returned then the operation was successful
//      To Get the Temperature
//          1. Call ReadRTCTemperature()
//          2. The structure will be updated with the current temperature
//          3. The processor will be busy while this is completed
//          4. An integer temperature (in celsius) in returned
//
// MODULE AUTHOR: Devin Henderlong 11/4/2014

#ifndef __DS3231_H
#define __DS3231_H
#include "ETM_I2C.h"

#define DS3231_VERSION 02

// 
typedef struct {
  unsigned char year;              //00-99
  unsigned char month;            //01-12
  unsigned char day;              //1-7
  unsigned char date;             //01-31
  unsigned char hour;             //0-23
  unsigned char minute;           //0-59
  unsigned char second;           //0-59
} RTC_TIME;


typedef struct {
  unsigned char I2Cport;          //DS3231 I2C port
  unsigned char control_register; //See ConfigureDS3231()
  unsigned char status_register;  //See ConfigureDS3231()
} RTC_DS3231;


/*
// Global Real Time Clock Variable
typedef struct{
    unsigned int year;              //00-99
    unsigned char month;            //01-12
    unsigned char day;              //1-7
    unsigned char date;             //01-31
    unsigned char hour;             //0-23
    unsigned char minute;           //0-59
    unsigned char second;           //0-59
    unsigned char I2Cport;          //DS3231 I2C port
    unsigned char control_register; //See ConfigureDS3231()
    unsigned char status_register;  //See ConfigureDS3231()
    int temperature;                //2's complement signed, Celsius
} REAL_TIME_CLOCK;
*/


// Real Time Clock Slave Address
#define RTC_SLAVE_ADDRESS 0b11010000   //LSB = Read/Write

//Default configuration
#define RTC_DEFAULT_CONFIG 0b00000100

unsigned char ConfigureDS3231(RTC_DS3231* ptr_REAL_TIME_CLOCK, unsigned char i2c_port, unsigned char rtc_config, unsigned long fcy_clk, unsigned long i2c_baud_rate);
//
//  Return:
//      0x00 = worked properly
//      0x01 = I2C bus not idle
//      0x02 = Generate Start fail
//      0x03 = Write slave address fail
//      0x04 = Write data address fail
//      0x05 = Write control register fail
//      0x06 = Generate Stop fail
//
        // control register contains the following control bits (set in .c)
        //  Bit 7: Enable Oscillator, Active Low
        //  Bit 6: Enable Battery Backed Square Wave, Active High
        //  Bit 5: Convert Temperature and update capacitance array to oscillator , Active High
        //  Bit 4: Square Wave Rate Select (1) (not used here)
        //  Bit 3: Square Wave Rate Select (2) (not used here)
        //  Bit 2: Interrupt Control, Active Low (not used here)
        //  Bit 1: Alarm 2 Interrupt Enable, Active High
        //  Bit 0: Alarm 1 Interrupt Enable, Active High
//
        // status register contains the following:  (not currently used)
        //  Bit 7: Oscillator Stop Flag, Active High
        //  Bit 3: Enable 32kHz Output, Active High
        //  Bit 2: Busy, Active High (check before Convert Temperature)
        //  Bit 1: Alarm 2 Interrupt Flag
        //  Bit 0: Alarm 1 Interrupt Flag
//

unsigned char SetDateAndTime(RTC_DS3231* ptr_REAL_TIME_CLOCK, RTC_TIME* ptr_TIME);
//
//  Description: Call this function to set the date and time to what is stored
//                  at the memory address of the pointer passed in
//  Return:
//          0x00 = worked properly
//          0x01 = I2C bus not idle
//          0x02 = I2C start issue
//          0x03 = Error writing slave address to I2C
//          0x04 = Error writing data addres to I2C
//          0x05 = Error writing seconds to I2C
//          0x06 = Error writing minutes to I2C
//          0x07 = Error writing hours to I2C
//          0x08 = Error writing the day to I2C
//          0x09 = Error writing the date to I2C
//          0x0A = Error writing the month to I2C
//          0x0B = Error writing the year to I2C
//          0x0C = I2C stop issue
//

unsigned char ReadDateAndTime(RTC_DS3231* ptr_REAL_TIME_CLOCK, RTC_TIME* ptr_TIME);
//
//  Description: Call this function to read the date and time which is then
//                  stored at the memory address of the pointer passed in
//  Return: This function will return 0 if everything worked properly
//          0x01 = I2C bus not idle
//          0x02 = I2C start issue
//          0x03 = Error writing slave address to I2C
//          0x04 = Error writing data addres to I2C
//          0x05 = Error generating restart
//          0x06 = Error writing slave address to I2C
//          0x07 = Error reading seconds
//          0X08 = Seconds > 60 error
//          0x09 = Error generating ACK
//          0x0A = Error reading minutes
//          0x0B = Minutes > 60 error
//          0x0C = Error generating ACK
//          0x0D = Error reading hours
//          0x0E = Hours > 24 error
//          0x0F = Error generating ACK
//          0x10 = Error reading day
//          0x11 = Day > 7 error
//          0x12 = Error generating ACK
//          0x13 = Error reading date
//          0x14 = Date > 31 error
//          0x15 = Error generating ACK
//          0x16 = Error reading month
//          0x17 = Month > 12 error
//          0x18 = Error generating ACK
//          0x19 = Error reading year
//          0x1A = Year > 99 error
//          0x1B = Error generating NACK
//          0x1C = Error generating stop
//

//unsigned int ReadRTCTemperature(REAL_TIME_CLOCK* ptr_REAL_TIME_CLOCK);
//
//  Description: Call this function to read the temperature which is then
//                  stored at the memory address of the pointer passed in
//  Return: This function will return an integer temperature in Celsius
//



// Sudo Second Count = (Year*31622400 + Month*2678400 + Day*86400 + hour*3600 + minute*60 + second)
unsigned long RTCDateToSeconds(RTC_TIME* ptr_time);
// This converts an RTC_TIME into a sudo second count



void RTCSecondsToDate(unsigned long sudo_seconds, RTC_TIME* ptr_time);
// This converts a sudo second count into an RTC_TIME 


//These are the data addresses of the RTC module
#define SECONDS_ADDRESS 0x00
#define MINUTES_ADDRESS 0x01
#define HOURS_ADDRESS 0x02
#define DAYS_ADDRESS 0x03
#define DATE_ADDRESS 0x04
#define MONTH_ADDRESS 0x05
#define YEAR_ADDRESS 0x06
#define CONTROL_ADDRESS 0x0E
#define STATUS_ADDRESS 0x0F
#define TEMPERATURE_ADDRESS_MSB 0x11 //(sign = MSB, the rest is integer data)
#define TEMPERATURE_ADDRESS_LSB 0x12 //(fractional is MSB and MSB-1, the rest is 0)


#endif
