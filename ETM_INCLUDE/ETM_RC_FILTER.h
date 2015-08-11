#ifndef __ETM_RC_FILTER_H
#define __ETM_RC_FILTER_H

unsigned int RCFilterNTau(unsigned int previous_value, unsigned int reading, unsigned int FILTER_TAU_BITS);
/*
  This performs a fast "almost RC" filter where Tau is 2^FILTER_TAU_BITS samples.
  The max value of FILTER_TAU_BITS is 15

  If "previous_value" is zero, this is a special case where the output will be set to reading.  
  This is so that when this is first called (previous value = zero), the filtered value will not start at zero (which could take a long time to reach correct value)
*/
#define RC_FILTER_2_TAU   1
#define RC_FILTER_4_TAU   2
#define RC_FILTER_8_TAU   3
#define RC_FILTER_16_TAU  4
#define RC_FILTER_32_TAU  5
#define RC_FILTER_64_TAU  6
#define RC_FILTER_128_TAU 7
#define RC_FILTER_256_TAU 8
#define RC_FILTER_512_TAU 9


#endif
