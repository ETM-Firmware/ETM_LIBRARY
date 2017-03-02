// ETM MODULE:  REAL TIME CLOCK OVER I2C
//#include <p30f6014A.h>
//#include <libpic30.h>
#include "DS3231.h"
#include "ETM_I2C.h"

unsigned char slave_address = RTC_SLAVE_ADDRESS;


#define YEAR_MULT  35942400
#define MONTH_MULT  2764800
#define DAY_MULT      86400
#define HOUR_MULT      3600
#define MIN_MULT         60



unsigned char ConvertToBCD(unsigned char decimal, unsigned char hour);
unsigned char ConvertFromBCD(unsigned char decimal, unsigned char hour);

unsigned char ConfigureDS3231(RTC_DS3231* ptr_REAL_TIME_CLOCK, unsigned char I2Cport, unsigned char config, unsigned long fcy_clk, unsigned long i2c_baud_rate) {
  ptr_REAL_TIME_CLOCK->control_register = config;
  ptr_REAL_TIME_CLOCK->I2Cport = I2Cport;
  
  ConfigureI2C(ptr_REAL_TIME_CLOCK->I2Cport, I2CCON_DEFAULT_SETUP_PIC30F, i2c_baud_rate, fcy_clk, 0);
  
  if (WaitForI2CBusIdle(ptr_REAL_TIME_CLOCK->I2Cport) == 0){
    if (GenerateI2CStart(ptr_REAL_TIME_CLOCK->I2Cport) == 0){
      if (WriteByteI2C(slave_address, ptr_REAL_TIME_CLOCK->I2Cport) == 0){
	if (WriteByteI2C(CONTROL_ADDRESS, ptr_REAL_TIME_CLOCK->I2Cport) == 0){
	  if (WriteByteI2C(ptr_REAL_TIME_CLOCK->control_register, ptr_REAL_TIME_CLOCK->I2Cport) == 0){
	    if (GenerateI2CStop(ptr_REAL_TIME_CLOCK->I2Cport) == 0){
	      return 0;
	    }
	    else
	      return 6;
	  }
	  else
	    return 5;
	}
	else
	  return 4;
      }
      else
	return 3;
    }
    else
      return 2;
  }
  else
    return 1;
}

unsigned char SetDateAndTime(RTC_DS3231* ptr_REAL_TIME_CLOCK, RTC_TIME* ptr_TIME){
    if (WaitForI2CBusIdle(ptr_REAL_TIME_CLOCK->I2Cport) == 0){
        if (GenerateI2CStart(ptr_REAL_TIME_CLOCK->I2Cport) == 0){
            if (WriteByteI2C(slave_address, ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                if (WriteByteI2C(SECONDS_ADDRESS, ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                    if (WriteByteI2C(ConvertToBCD(ptr_TIME->second, 0), ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                        if (WriteByteI2C(ConvertToBCD(ptr_TIME->minute, 0), ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                            if (WriteByteI2C(ConvertToBCD(ptr_TIME->hour, 1), ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                                if (WriteByteI2C(ConvertToBCD(ptr_TIME->day, 0), ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                                    if (WriteByteI2C(ConvertToBCD(ptr_TIME->date, 0), ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                                        if (WriteByteI2C(ConvertToBCD(ptr_TIME->month, 0), ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                                            if (WriteByteI2C(ConvertToBCD(ptr_TIME->year, 0), ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                                                if (GenerateI2CStop(ptr_REAL_TIME_CLOCK->I2Cport) == 0)
                                                    return 0;
                                                else
                                                    return 0xC;
                                            }
                                            else
                                                return 0xB;
                                        }
                                        else
                                            return 0xA;
                                    }
                                    else
                                        return 9;
                                }
                                else
                                    return 8;
                            }
                            else
                                return 7;
                        }
                        else
                            return 6;
                    }
                    else
                        return 5;
                }
                else
                    return 4;
            }
            else
               return 3;
        }
        else
            return 2;
    }
    else
        return 1;
}

unsigned char ReadDateAndTime(RTC_DS3231* ptr_REAL_TIME_CLOCK, RTC_TIME* ptr_TIME){
    unsigned int data;
    unsigned char temp;
    
    if (WaitForI2CBusIdle(ptr_REAL_TIME_CLOCK->I2Cport) == 0){
        if (GenerateI2CStart(ptr_REAL_TIME_CLOCK->I2Cport) == 0){
            if (WriteByteI2C(slave_address, ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                if (WriteByteI2C(SECONDS_ADDRESS, ptr_REAL_TIME_CLOCK->I2Cport) == 0) {
                    if (GenerateI2CRestart(ptr_REAL_TIME_CLOCK->I2Cport) == 0) {
                        if (WriteByteI2C(slave_address + 1, ptr_REAL_TIME_CLOCK->I2Cport) == 0){
                            data = ReadByteI2C(ptr_REAL_TIME_CLOCK->I2Cport);
                            if (data == 0x1000)
                                return 7;
                            temp = data & 0x00FF;
                            ptr_TIME->second = ConvertFromBCD(temp, 0);
                            if (ptr_TIME->second > 60) {
                                ptr_TIME->second = 0;
                                return 8;
                            }
                            if (GenerateACK(ptr_REAL_TIME_CLOCK->I2Cport) != 0)
                                return 9;
                            data = ReadByteI2C(ptr_REAL_TIME_CLOCK->I2Cport);
                            if (data == 0x1000)
                                return 0xA;
                            temp = data & 0x00FF;
                            ptr_TIME->minute = ConvertFromBCD(temp, 0);
                            if (ptr_TIME->minute > 60) {
                                ptr_TIME->minute = 0;
                                return 0xB;
                            }
                            if (GenerateACK(ptr_REAL_TIME_CLOCK->I2Cport) != 0)
                                return 0xC;
                            data = ReadByteI2C(ptr_REAL_TIME_CLOCK->I2Cport);
                            if (data == 0x1000)
                                return 0xD;
                            temp = data & 0x00FF;
                            ptr_TIME->hour = ConvertFromBCD(temp, 1);
                            if (ptr_TIME->hour > 24) {
                                ptr_TIME->hour = 0;
                                return 0xE;
                            }
                            if (GenerateACK(ptr_REAL_TIME_CLOCK->I2Cport) != 0)
                                return 0xF;
                            data = ReadByteI2C(ptr_REAL_TIME_CLOCK->I2Cport);
                            if (data == 0x1000)
                                return 0x10;
                            temp = data & 0x00FF;
                            ptr_TIME->day = ConvertFromBCD(temp, 0);
                            if (ptr_TIME->day > 7) {
                                ptr_TIME->day = 0;
                                return 0x11;
                            }
                            if (GenerateACK(ptr_REAL_TIME_CLOCK->I2Cport) != 0)
                                return 0x12;
                            data = ReadByteI2C(ptr_REAL_TIME_CLOCK->I2Cport);
                            if (data == 0x1000)
                                return 0x13;
                            temp = data & 0x00FF;
                            ptr_TIME->date = ConvertFromBCD(temp, 0);
                            if (ptr_TIME->date > 31) {
                                ptr_TIME->date = 0;
                                return 0x14;
                            }
                            if (GenerateACK(ptr_REAL_TIME_CLOCK->I2Cport) != 0)
                                return 0x15;
                            data = ReadByteI2C(ptr_REAL_TIME_CLOCK->I2Cport);
                            if (data == 0x1000)
                                return 0x16;
                            temp = data & 0x00FF;
                            ptr_TIME->month = ConvertFromBCD(temp, 0);
                            if (ptr_TIME->month > 12) {
                                ptr_TIME->month = 0;
                                return 0x17;
                            }
                            if (GenerateACK(ptr_REAL_TIME_CLOCK->I2Cport) != 0)
                                return 0x18;
                            data = ReadByteI2C(ptr_REAL_TIME_CLOCK->I2Cport);
                            if (data == 0x1000)
                                return 0x19;
                            temp = data & 0x00FF;
                            ptr_TIME->year = ConvertFromBCD(temp, 0);
                            if (ptr_TIME->year > 99) {
                                ptr_TIME->year = 0;
                                return 0x1A;
                            }
                            if (GenerateNACK(ptr_REAL_TIME_CLOCK->I2Cport) != 0)
                                return 0x1B;
                            if (GenerateI2CStop(ptr_REAL_TIME_CLOCK->I2Cport) == 0) {
                                return 0;
                            }
                            else
                                return 0x1C;
                        }
                        else
                            return 6;
                    }
                    else
                        return 5;
                }
                else
                    return 4;
            }
            else
               return 3;
        }
        else
            return 2;
    }
    else
        return 1;
}

unsigned char ConvertFromBCD(unsigned char bcd, unsigned char hour){
    unsigned char twenty = 0;
    unsigned char ten = 0;
    unsigned char decimal = 0;

    if (hour){
        twenty = bcd & 0x20;
        ten = bcd & 0x10;
    }
    else {
        twenty = 0;
        ten = bcd & 0xF0;
    }

    decimal = ((twenty >> 5) * 20) + ((ten >> 4) * 10) + (bcd & 0xF);

    return decimal;
}

unsigned char ConvertToBCD(unsigned char decimal, unsigned char hour){
    unsigned char twenty = 0;
    unsigned char ten = 0;
    unsigned char bcd = 0;
    
    if (hour){
        twenty = decimal / 20;
        decimal -= twenty * 20;
        ten = decimal / 10;
        decimal -= ten * 10;
        if (twenty)
            bcd = 0x20;
        if (ten)
            bcd |= ten << 4;
        bcd |= decimal;
    }
    else {
        ten = decimal / 10;
        decimal -= ten * 10;
        if (ten)
            bcd = ten << 4;
        bcd |= decimal;
    }
    return bcd;
}

unsigned long RTCDateToSeconds(RTC_TIME* ptr_time) {
  unsigned long time;
  unsigned long temp;
  
  /*
    Could also be expressed as 
    Years * 35942400 +
    month *  2764800 +
    Date  *    86400 +
    hour  *     3600 +
    minute*       60 +
    Second*        1
   */


  // Year Calc
  temp = ptr_time->year;
  temp *= YEAR_MULT;
  time = temp;

  // Month Calc
  temp = ptr_time->month;
  temp *= MONTH_MULT;
  time += temp;

  // Day Calc
  temp = ptr_time->date;
  temp *= DAY_MULT;
  time += temp;

  // Hour Calc
  temp = ptr_time->hour;
  temp *= HOUR_MULT;
  time += temp;

  // Minute Calc
  temp = ptr_time->minute;
  temp *= MIN_MULT;
  time += temp;

  // Seconds Calc
  time += ptr_time->second;

  return time;
}

void RTCSecondsToDate(unsigned long sudo_seconds, RTC_TIME* ptr_time) {
  unsigned long int_part;

  // year calculation
  int_part = sudo_seconds / YEAR_MULT;
  sudo_seconds = sudo_seconds % YEAR_MULT;
  ptr_time->year = int_part;

  //month_calculation
  int_part = sudo_seconds / MONTH_MULT;
  sudo_seconds = sudo_seconds % MONTH_MULT;
  ptr_time->month = int_part;

  //date_calculation
  int_part = sudo_seconds / DAY_MULT;
  sudo_seconds = sudo_seconds % DAY_MULT;
  ptr_time->date = int_part;

  //hour_calculation
  int_part = sudo_seconds / HOUR_MULT;
  sudo_seconds = sudo_seconds % HOUR_MULT;
  ptr_time->hour = int_part;

  //minute_calculation
  int_part = sudo_seconds / MIN_MULT;
  sudo_seconds = sudo_seconds % MIN_MULT;
  ptr_time->minute = int_part;
  ptr_time->second = sudo_seconds;
}
