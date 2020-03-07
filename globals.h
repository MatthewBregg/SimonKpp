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
constexpr uint32_t START_DELAY_INC = 15; // Wait step count increase (wraps in a byte)
constexpr uint32_t START_DSTEP_US = 8; // Microseconds per start delay step
constexpr uint32_t TIMEOUT_START = 10000; // Timeout per commutation for ZC during starting

// PWM Related

// Uhoh, these are 1s complement?!
// This is tricky. TODO(bregg): Potential bugs here...
// Based on comments in set_new_duty21, it looks like
// this the input is ones complemented, and then stored in here for up counting tcnt2.
// Therefore, leave this unsigned, and just be VERY careful that I keep those com instructions in!!
//
// Oh, I think I get it.  duty is how long we don't want to be doing X for in the period.
// IE, [ X X Y Y Y ], so we invert, do count down how many times we want to do Y,
// and then finish the period and reset?
volatile uint16_t duty = 0;	//   on duty cycle, one's complement
volatile uint16_t off_duty = 0;	//   on duty cycle, one's complement

// The PWM interrupt uses this byte to determine which action to take.
// Why an enum and not a function pointer? An enum should be 8 bits,
// and therefore loaded/mutated atomically, but a function pointer
// in AVR will be 16, so I then need to worry about ZL/ZH being out of sync.
// Then again, maybe I should just use the atomic type?
// Update: This looks slow as fuck based on godbolt.
// It's compiling to a series of cpi/breqs.
// Solution: Function pointer, wrap the sets in SEI/CLI.
// Very in depth solution: Go and ensure that these PWM functions are in the lower 8 bits
// for the function pointer, and then only set that half. But that's tricky and bug prone,
// and I'd rather just use a slower ut functional solution until the migration to a 32 bit platform.
enum PWM_STATUS_ENUM : byte {
    PWM_NOP = 0x00,
    PWM_OFF = 0x01,
    PWM_ON = 0x02,
    PWM_ON_FAST = 0x03,
    PWM_ON_FAST_HIGH = 0x04,
    PWM_ON_HIGH = 0x05
};
volatile PWM_STATUS_ENUM PWM_STATUS = PWM_NOP;
// This looks to be set as part of eval RC, so make sure this happens!
// AKA, I need to run set_new_duty!!
volatile PWM_STATUS_ENUM PWM_ON_PTR = PWM_NOP;

// Timer related
volatile bool oct1_pending = false;
volatile byte ocr1ax = 0; // third byte of OCR1A.
volatile byte tcnt1x = 0; // third byte of TCNT1.
volatile byte tcnt2h = 0; // 2nd byte of tcnt2.

// RC Timeout values
volatile byte rc_timeout = 0;
volatile byte rct_boot = 0; // Counter which increments while rc_timeout is 0 to jump to boot loader (TODO delete?)
volatile byte rct_beacon = 0; // Counter which increments while rc_timeout is 0 to disarm and beep occasionally (TODO delete?)

// Motor Driving flags
volatile bool set_duty = false;
volatile bool power_on = false;
volatile bool full_power = false;
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
