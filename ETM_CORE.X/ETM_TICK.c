#include <xc.h>
//#include "ETM_TICK.h"
#include "ETM_TICK.h"
/*
  Tick is defined as number of clock cycles
*/

volatile unsigned int *etm_tick_TMR_ptr;

unsigned int etm_tick_timer_select;
unsigned int etm_tick_delay_1ms;
unsigned int etm_tick_delay_100us;

unsigned int ETMTickPrivateGreaterThan(unsigned long start_tick, unsigned long difference);




unsigned int ETMTickNotInitialized(void) {

#ifdef T1CON
  if (etm_tick_timer_select == ETM_TICK_USE_TIMER_1) {
    return 0;
  }
#endif

#ifdef T2CON
  if (etm_tick_timer_select == ETM_TICK_USE_TIMER_2) {
    return 0;
  }
#endif

#ifdef T3CON
  if (etm_tick_timer_select == ETM_TICK_USE_TIMER_3) {
    return 0;
  }
#endif

#ifdef T4CON
  if (etm_tick_timer_select == ETM_TICK_USE_TIMER_4) {
    return 0;
  }
#endif

#ifdef T5CON
  if (etm_tick_timer_select == ETM_TICK_USE_TIMER_5) {
    return 0;
  }
#endif
  
  return 1;
}


void ETMTickInitialize(unsigned long fcy_clk, char timer_select) {
  unsigned long temp;
  temp = fcy_clk / 1000 / 8;
  etm_tick_delay_1ms = temp;
  temp = fcy_clk / 10000 / 8;
  etm_tick_delay_100us = temp;
  etm_tick_timer_select = 0xFF;

  switch (timer_select) {
    
#ifdef T1CON
  case ETM_TICK_USE_TIMER_1:
    etm_tick_timer_select = ETM_TICK_USE_TIMER_1;
    etm_tick_TMR_ptr = &TMR1;
    T1CON = 0x8010; // Timer on, 1:8 prescale
    break;
#endif

#ifdef T2CON
  case ETM_TICK_USE_TIMER_2:
    etm_tick_timer_select = ETM_TICK_USE_TIMER_2;
    etm_tick_TMR_ptr = &TMR2;
    T2CON = 0x8010; // Timer on, 1:8 prescale, 16 bit timer
    break;
#endif
    
#ifdef T3CON
  case ETM_TICK_USE_TIMER_3:
    etm_tick_timer_select = ETM_TICK_USE_TIMER_3;
    etm_tick_TMR_ptr = &TMR3;
    T3CON = 0x8010; // Timer on, 1:8 prescale, 16 bit timer
    break;
#endif
    
#ifdef T4CON
  case ETM_TICK_USE_TIMER_4:
    etm_tick_timer_select = ETM_TICK_USE_TIMER_4;
    etm_tick_TMR_ptr = &TMR4;
    T4CON = 0x8010; // Timer on, 1:8 prescale, 16 bit timer
    break;
#endif
    
#ifdef T5CON
  case ETM_TICK_USE_TIMER_5:
    etm_tick_timer_select = ETM_TICK_USE_TIMER_5;
    etm_tick_TMR_ptr = &TMR5;
    T5CON = 0x8010; // Timer on, 1:8 prescale, 16 bit timer
    break;
#endif
    
  }
}


unsigned int ETMTickGreaterThanNMilliseconds(unsigned int delay_milliseconds, unsigned long start_tick) {
  unsigned long test;
  test  = delay_milliseconds;
  test *= etm_tick_delay_1ms;

  if (etm_tick_timer_select == 0xFF) {
    return 0;
  }

  if (ETMTickPrivateGreaterThan(start_tick, test)) {
    return 1;
  }
  
  return 0;
}


unsigned int ETMTickGreaterThanN100uS(unsigned int delay_100us, unsigned long start_tick) {
  unsigned long test;
  
  if (etm_tick_timer_select == 0xFF) {
    return 0;
  }
  
  test  = delay_100us;
  test *= etm_tick_delay_100us;
  
  return ETMTickPrivateGreaterThan(start_tick, test);
}




unsigned int ETMTickRunOnceEveryNMilliseconds(unsigned int interval_milliseconds, unsigned long *ptr_holding_var) {
  unsigned long test;
  test  = interval_milliseconds;
  test *= etm_tick_delay_1ms;

  if (etm_tick_timer_select == 0xFF) {
    return 0;
  }

  if (ETMTickPrivateGreaterThan(*ptr_holding_var, test)) {
    *ptr_holding_var = *ptr_holding_var + test;
    while (ETMTickPrivateGreaterThan(*ptr_holding_var, test)) {
      // If it has been a long time since you made this test, this will ensure that it only tests true once.
      *ptr_holding_var = *ptr_holding_var + test;
    }
    return 1;
  }
 
  return 0;
}



unsigned int ETMTickPrivateGreaterThan(unsigned long start_tick, unsigned long difference) {
  // Assume that what we are timing is less than the full period
  // current_time = ETMTickGet();
  
  //test_time = current_time - start_tick;
  
  if ((ETMTickGet() - start_tick) > difference) {
    return 1;
  }
  
  return 0;
}


unsigned int ETMTickGet1msMultiplier(void) {
  ETMTickGet();
  return etm_tick_delay_1ms;
}
