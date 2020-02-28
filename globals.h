#include "esc_config.h"

#ifndef GLOBALS_H
#define GLOBALS_H

// Constants
constexpr bool HIGH_SIDE_PWM = false;
constexpr unsigned short MIN_DUTY = 56 * cpuMhz/16;
constexpr unsigned short POWER_RANGE = 1500U * cpuMhz/16 + MIN_DUTY;
constexpr unsigned short MAX_POWER = POWER_RANGE-1;
constexpr unsigned short PWR_MIN_START = POWER_RANGE/6;
constexpr byte RCP_TOT = 2U; // Number of 65536us periods before considering rc pulse lost
 // This many start cycles without timeout will transition to running mode
// (tm4 experimental 05-01-18 - stock 12)
constexpr byte ENOUGH_GOODIES = 6;
constexpr unsigned short ZC_CHECK_MIN = 3U;
//  Number of ZC checkloops under which the PWM noise should not matter.
constexpr unsigned short ZC_CHECK_FAST = 12U;
constexpr unsigned short ZC_CHECK_MAX = POWER_RANGE/32; // Limit ZC checking to about 1/2 PWM interval
constexpr unsigned short MASKED_ZC_CHECK_MIN = 0x00FFu & ZC_CHECK_MIN;
constexpr unsigned short MASKED_ZC_CHECK_MAX = 0x00FFu & ZC_CHECK_MAX;


// PWM Related

// The PWM interrupt uses this byte to determine which action to take.
// TODO: Replace with enum.
volatile byte PWM_STATUS = 0x00;

// Timer related
volatile bool oct1_pending = false;
volatile byte ocr1ax = 0; // third byte of OCR1A.
volatile byte tcnt1x = 0; // third byte of TCNT1.

// RC Timeout values
volatile byte rc_timeout = 0;
volatile byte rct_boot = 0; // Counter which increments while rc_timeout is 0 to jump to boot loader (TODO delete?)
volatile byte rct_beacon = 0; // Counter which increments while rc_timeout is 0 to disarm and beep occasionally (TODO delete?)

// Motor Driving flags
volatile bool set_duty = false;
volatile bool power_on = false;
volatile bool startup = false;
volatile bool aco_edge_high = false;

// Motor timing information.
volatile byte power_skip = 6U;
volatile byte goodies = 0U;
volatile unsigned short timing = 0x00U; // Interval of 2 commutations.

#endif
