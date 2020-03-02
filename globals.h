#include "esc_config.h"

#ifndef GLOBALS_H
#define GLOBALS_H

// SimonK tuning notes: http://torukmakto4.blogspot.com/2018/05/project-t19-part-12-flywheel-drive.html

// Constants
constexpr bool HIGH_SIDE_PWM = false;
constexpr unsigned short MIN_DUTY = 56 * cpu_mhz/16;
constexpr unsigned short POWER_RANGE = 1500U * cpu_mhz/16 + MIN_DUTY;
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

constexpr uint32_t START_DELAY_US = 0x00u; // Initial post-commutation wait during starting
constexpr uint32_t START_DSTEP_US = 8; // Microseconds per start delay step
constexpr uint32_t TIMEOUT_START = 10000; // Timeout per commutation for ZC during starting


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
volatile bool timing_fast = false; // Does timing fit in 16 bits?

// Motor timing information.
volatile byte power_skip = 6U;
// Goodies: Amount of Good zero-cross detections, good commutations, that's all.
volatile byte goodies = 0U;
// Both of these are 24 bits in simonk with _x, we will need to bitmask for the fast methods.
// Making these 32 bit might not be as efficient, but using 3 bytes is stupid messy,
// and coercing the compiler to actually make a 24 bit int type seems tricky, so...
// This is a problem that won't exist on a 32 bit platform anywho.
volatile  uint32_t timing = 0x00U; // Interval of 2 commutations.
volatile uint32_t com_timing = 0x00U; // time of last commutation.

// FET STATUS
volatile bool all_fets = false;
volatile bool a_fet = false;
volatile bool b_fet = false;
volatile bool c_fet = false;

// Startup vars
volatile byte start_delay = 0x00u;

#endif
