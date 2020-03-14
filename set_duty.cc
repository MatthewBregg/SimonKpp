#include "set_duty.h"
#include "globals.h"
#include "byte_manipulation.h"
#include "interrupts.h"


// rc_duty_copy = yl/yh, new_duty = temp1/2.
void set_new_duty_21(uint16_t rc_duty_copy, uint16_t new_duty, void(* const next_pwm_status)(void)) {
    // set_new_duty21:
    new_duty = (new_duty & 0xFF00u) | (~get_low(new_duty) & 0xFFu);
    rc_duty_copy = (rc_duty_copy & 0xFF00u) | (~get_low(rc_duty_copy) & 0xFFu);
    cli();
    // Duty is set atomically in the ASM, but not promised here!
    duty = rc_duty_copy;
    // These must be set atomically!
    off_duty = new_duty;
    pwm_on_ptr = next_pwm_status;
    // End set atomically!
    sei();
    return;
}

// rc_duty_copy = yl/yh.
void set_new_duty_l(uint16_t rc_duty_copy) {
    if ( timing_duty <= rc_duty_copy ) {
	rc_duty_copy = timing_duty;
    }
    // set_new_duty_10.
    if ( sys_control <= rc_duty_copy ) {
	rc_duty_copy = sys_control;
    }
    // Set new duty limit 11
    // SLOW_THROTTLE
    //////////////////////////////////////////////////////////////
    // If sys_control is higher than twice the current duty,    //
    // limit it to that. This means that a steady-state duty    //
    // cycle can double at any time, but any larger change will //
    // be rate-limited.					        //
    //////////////////////////////////////////////////////////////
    // temp1/2, eventually becomes the new off duty.
    uint16_t new_duty = PWR_MIN_START;
    // New duty is at least PWR_MIN_START, but if rc_duty_copy is higher duty, use that.
    if ( PWR_MIN_START <= rc_duty_copy ) {
	new_duty = rc_duty_copy;
    }

    new_duty = new_duty << 1;

    if ( new_duty <= sys_control ) {
	sys_control = new_duty;
    }
    // Set new duty 13.
    new_duty = PWR_MAX_START;
    // Calculate OFF duty.
    new_duty -= rc_duty_copy;
    if (new_duty == 0) {
	full_power = true;
	power_on = true;
	set_new_duty_set(rc_duty_copy, new_duty);
	return;
    }
    if ( rc_duty_copy == 0 ) {
	full_power = false;
	power_on = false;
	set_new_duty_21(rc_duty_copy, new_duty, &pwm_off);
	return;
    }
    // Not off, and not full power
    full_power = false;
    power_on = true;
    // At higher PWM frequencies, halve the frequency
    // when starting -- this helps hard drive startup

    if (POWER_RANGE < (1700 * (cpu_mhz / 16.0))){ // 1700 is a torukmakto4 change
	if (startup) {
	    new_duty = new_duty << 1;
	    rc_duty_copy = rc_duty_copy << 1;
	}
    }
    set_new_duty_set(rc_duty_copy, new_duty);
    return;
}
// rc_duty_copy = yl/yh, new_duty = temp1/2.
void set_new_duty_set(uint16_t rc_duty_copy, uint16_t new_duty) {
    // set_new_duty_set:
    void (*next_pwm_status)() = &pwm_on; // Off period <0x100
    if ( (new_duty & 0xFF00u) != 0 ) { // Is the upper byte of new_duty 0?
	next_pwm_status = &pwm_on_high; // Off period >= 0x100.
    }
    set_new_duty_21(rc_duty_copy, new_duty, next_pwm_status);
    return;
}

void set_new_duty() {
    set_new_duty_l(rc_duty);
    return;
}

// Sets the speed that the ESC will try to rev to!
// Takes PARAM in YL/YH.
// Set YL/YH to MAX_POWER for full power, or 0 for off.
void rc_duty_set(uint16_t new_rc_duty) {
    rc_duty = new_rc_duty;
    if (set_duty) {
	rc_timeout = RCP_TOT;
	set_new_duty_l(rc_duty);
	return;
    } else {
	if ( 12 > rc_timeout ) {
	    ++rc_timeout;
	}
	return;
    }
}
