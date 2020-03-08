#include "esc_config.h"

#ifndef GLOBALS_H
#define GLOBALS_H

// SimonK tuning notes: http://torukmakto4.blogspot.com/2018/05/project-t19-part-12-flywheel-drive.html

// Constants
constexpr byte MOTOR_ADVANCE = 13; // Degrees of timing advance (0 - 30, 30 meaning no delay)
constexpr bool HIGH_SIDE_PWM = false;
constexpr unsigned short MIN_DUTY = 56 * cpu_mhz/16;
constexpr unsigned short POWER_RANGE = 1500U * cpu_mhz/16 + MIN_DUTY;
constexpr unsigned short PWR_MAX_RPM1 = (POWER_RANGE/6); //  Power limit when running slower than TIMING_RANGE1
constexpr unsigned short MAX_POWER = POWER_RANGE-1;
constexpr unsigned short PWR_MIN_START = POWER_RANGE/6; // Power limit while starting (to start)
constexpr unsigned short PWR_MAX_START = POWER_RANGE/6; // Power limit while starting (if still not running)
constexpr unsigned short PWR_COOL_START = (POWER_RANGE/24); // Power limit while starting to reduce heating
// 8192us per commutation
constexpr uint16_t TIMING_MIN = 0x8000;
 // tm4 change - start ramping duty earlier due to less PWR_MAX_RPM1 (stock 0x4000 - 4096us per commutation)
constexpr uint16_t TIMING_RANGE1 = 0x6000;
// 2048us per commutation (Note this is unused and for legacy "Powershift" versions --tm4)
constexpr uint16_t TIMING_RANGE2 = 0x2000;
//1024us per commutation
constexpr uint16_t TIMING_RANGE3 = 0x1000;

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
constexpr uint8_t START_MOD_INC = 4; // Start power modulation step count increase (wraps in a byte)
constexpr uint8_t START_FAIL_INC = 16; // start_tries step count increase (wraps in a byte, upon which we disarm)
constexpr uint8_t START_MOD_LIMIT = 48; // Value at which power is reduced to avoid overheating
constexpr uint32_t TIMEOUT_START = 10000; // Timeout per commutation for ZC during starting
constexpr uint32_t TIMING_MAX = 0x0080u; // ; Fixed or safety governor (no less than 0x0080, 321500eRPM).

// Non Constants
uint16_t safety_governor = 0x00FFu;

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
volatile uint16_t timing_duty = 0; // timing duty limit.
// How much the RC command is telling us to peg the throttle at?
volatile uint16_t rc_duty = 0x00;

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
volatile uint32_t last_tcnt1 = 0x00u; // Last Timer1 value.
volatile uint32_t last2_tcnt1 = 0x00u; // Last last Timer1 value.

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
// timing_l, timing_h, timing_x
volatile  uint32_t timing = 0x00U; // Interval of 2 commutations.
// com_time_l, com_time_h, com_time_x
volatile uint32_t com_timing = 0x00U; // time of last commutation.


volatile uint16_t sys_control = 0x00u; // duty limit

// FET STATUS
volatile bool a_fet = false;
volatile bool b_fet = false;
volatile bool c_fet = false;

// Startup vars
volatile byte start_delay = 0x00u;
 // Start modulation counter (to reduce heating from PWR_MAX_START if stuck)
volatile byte start_modulate = 0x00u;
// Number of start_modulate loops for eventual failure and disarm
volatile byte start_fail = 0x00u;

#endif
