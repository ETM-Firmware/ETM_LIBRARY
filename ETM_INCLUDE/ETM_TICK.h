#ifndef __ETM_TICK_H
#define __ETM_TICK_H


/*
  This modules generates a 32 bit timer to use for timing multiple events
  1 LSB of this timer corresponds to 8 clock cycles

  ETM Tick can not be used to measure times greater than 2^32*8 clock cycles
  19.08 Minutes at 30Mhz
  28.63 Minutes at 20Mhz
  57.26 Minutes at 10Mhz

  If you need to meaure times more than about 15 minutes, use ETMTick to measure seconds or minutes and count minutes/seconds.

  NOTE!!!  This module uses no interrupts
  Because it does not use interrupts, the internal timer must be updated at least once every 2^16*8 clock cycles
  Calling any of the ETMTick* functions will update the internal timer.
  Again this must happen at least once every 2^16*8 clock cycles.
  17.47mS at 30MHz
  26.21ms at 20MHz
  52.42ms at 10MHz

*/


#define ETM_TICK_USE_TIMER_1   1
#define ETM_TICK_USE_TIMER_2   2
#define ETM_TICK_USE_TIMER_3   3
#define ETM_TICK_USE_TIMER_4   4
#define ETM_TICK_USE_TIMER_5   5

extern unsigned int etm_tick_delay_1ms;

void ETMTickInitialize(unsigned long fcy_clk, char timer_select);
/*
  This configures the timers and sets up the module
*/

unsigned int ETMTickNotInitialized(void);
/*
  Will return 1 if the module has NOT already been initialized
 
  Will return 0 otherwise
*/

unsigned int ETMTickGreaterThanNMilliseconds(unsigned int delay_milliseconds, unsigned long start_tick);
/*
  This will return 1 if more than delay_milliseconds has passed since start tick
  0 otherwise
*/


unsigned int ETMTickGreaterThanN100uS(unsigned int delay_100us, unsigned long start_tick);
/*
  This will return 1 if more than delay_100us has passed since start tick
  0 otherwise
*/


unsigned int ETMTickRunOnceEveryNMilliseconds(unsigned int interval_milliseconds, unsigned long *ptr_holding_var);
/*
  This will reutrn 1 if more than interval_milliseconds has passed since the last time this function returned true when called with a particular pointer
  0 otherwise
*/


unsigned long ETMTickGet(void);
/*
  Returns the current 32 bit timer
*/


#endif
