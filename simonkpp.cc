#include <avr/io.h>
#include <avr/interrupt.h>
#ifndef F_CPU
#define F_CPU 16000000UL // Must be set to CPU frequency for the util/delay function.
#endif
#include <util/delay.h>
#include "atmel.h"
#include "globals.h"
#include "commutations.h"
#include "interrupts.h"
#include "beep.h"
#include "byte_manipulation.h"
#include "ocr1a.h"
#include "wait_functions.h"

// REMEMBER: VARIABLES BEING set/access from an interrupt must be volatile!
// Big TODO: Move into proper .cc/.h files, and INLINE the world. I can use -Winline to make not inlining a warning.


// Afro NFet ESCs have an external 16mhz oscillator.
// We are disregarding the bootloader, and will be using USBASP.
// For laziness/convenience, we will be using https://github.com/carlosefr/atmega to flash.
//
// This does not set fuses, we must do so manually if it's a new chip.
//    With external oscillator: avrdude -U lfuse:w:0x3f:m -U hfuse:w:0xca:m

// Hacky declariations until we do this right in a .cc/.h file.
void start_from_running();
void restart_control();
void wait_startup();
void wait_commutation();
void wait_timeout_init();
void update_timing1(const uint32_t current_timing_period, const uint32_t last_tcnt1_copy);
void rc_duty_set(unsigned short new_rc_duty);
void set_new_duty_set(uint16_t rc_duty_copy, uint16_t new_duty);
void set_new_duty_21(uint16_t rc_duty_copy, uint16_t new_duty, const PWM_STATUS_ENUM next_pwm_status);

void start_failed() {
    // TODO: This better,
    // we probably should have a way to return to the control loop after a failed start after all.
    // Perhaps wait until we receive a no throttle command and then return up to the arming loop somehow?
    while(true) {
	redLedOff();
	greenLedOn();
	_delay_ms(2000);
	redLedOn();
	greenLedOff();
	_delay_ms(2000);
    }
}

// If sys_control_copy is less than or equal to the currently determined
// max_power (local_max_power), then set the real sys_control to sys_control_copy.
// Otherwise, sys_control is the local_max_power.
void run6_3(uint16_t sys_control_copy, uint16_t local_max_power) {
    if ( local_max_power > sys_control_copy ) {
	sys_control = sys_control_copy;
    } else {
	sys_control = local_max_power;
    }
    // In simonk, we go to run1 here,
    // but that won't work so well here, need to structure slightly differntly!
    return;
}

void run6_2(uint16_t sys_control_copy) {
    startup = false;
    start_fail = 0;
    start_modulate = 0;
    redLedOff();
//////////////////////////////////////////////////////
// Build up sys_control to MAX_POWER in steps.	    //
// If SLOW_THROTTLE is disabled, this only limits   //
// initial start ramp-up; once running, sys_control //
// will stay at MAX_POWER unless timing is lost.    //
//////////////////////////////////////////////////////
    sys_control_copy += (POWER_RANGE + 31)/32.0;
    // temp1/2 = MAX_POWER
    run6_3(sys_control_copy, MAX_POWER);
    return;
}

//  See run1 in simonk source.
// TODO: Rename this to run, place the wait_for_low() ... com2com1()
// under a bool reverse and add the run_forward function in also!
void run_reverse() {
    while ( true ) {
	wait_for_low();
	com1com6();
	sync_on();
	wait_for_high();
	com6com5();
	wait_for_low();
	com5com4();
	wait_for_high();
	com4com3();
	sync_off();
	wait_for_low();
	com3com2();
	wait_for_high();
	com2com1();
    ///////////
    // run6: //
    ///////////
	// IF last commutation timed out and power is off, return to restart control
	if (!power_on && goodies == 0) {
	    // TODO: Risk of stack overflow here eventually if we restart control enough?!
	    // Eventually does it make sense to just make restartControl a loop perhaps?
	    // Can we replace this with a break perhaps?
	    //
	    // Trap here for 4 seconds so it's very noticable when we fail to start.
	    _delay_ms(4000);
	    restart_control();
	    return;
	}
	// Each time TIMING_MAX is hit, sys_control is lsr'd
	// If zero, try startin over with powerskipping.
	// yl/yh.
	uint16_t sys_control_copy = sys_control;
	if (sys_control_copy == 0) {
	    start_from_running();
	    return;
	}

	if ( ENOUGH_GOODIES <= goodies ) {
	    // temp1 = goodies, yl/yh sys_control
	    run6_2(sys_control_copy);
	    // Loops again at run1.
	    continue;
	}
	++goodies;
	// Build up sys_control to PWR_MAX_START in steps.
	sys_control_copy += (POWER_RANGE + 47) / 48.0;
	//temp1/temp2 are now power_max_start.

	// If we've been trying to start for a while, modulate power to reduce heating.
	// temp3 = start_fail, temp4 = start_modulate which is then subtracted and then stored.
	start_modulate += START_MOD_INC;

	// If start_modulate == 0, do not skip over this section to run6_1.
	if (start_modulate == 0) {
	    // If we've been trying for a long while, give up.
	    if ( (start_fail + START_FAIL_INC) == 0) {
		start_failed();
		return;
	    } else {
		start_fail += START_FAIL_INC;
	    }

	}


	// Run 6_1:
	// Allow first loop at full power, then modulate.
	if ( START_FAIL_INC > start_fail || START_MOD_LIMIT > start_modulate ) {
	    run6_3(sys_control_copy, PWR_MAX_START);
	    // Loops again at run1.
	    continue;
	}
	// temp1/2 are now POWER_COOL_START
	run6_3(sys_control_copy, PWR_COOL_START);
	// Loops again at run1.
	continue;
    }
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
	set_new_duty_21(rc_duty_copy, new_duty, PWM_OFF);
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
void set_new_duty_21(uint16_t rc_duty_copy, uint16_t new_duty, const PWM_STATUS_ENUM next_pwm_status) {
    // set_new_duty21:
    new_duty = (new_duty & 0xFF00u) | (~get_low(new_duty) & 0xFFu);
    rc_duty_copy = (rc_duty_copy & 0xFF00u) | (~get_low(rc_duty_copy) & 0xFFu);
    cli();
    // Duty is set atomically in the ASM, but not promised here!
    duty = rc_duty_copy;
    // These must be set atomically!
    off_duty = new_duty;
    PWM_ON_PTR = next_pwm_status;
    // End set atomically!
    sei();
    return;
}
// rc_duty_copy = yl/yh, new_duty = temp1/2.
void set_new_duty_set(uint16_t rc_duty_copy, uint16_t new_duty) {
    // set_new_duty_set:
    PWM_STATUS_ENUM next_pwm_status = PWM_ON; // Off period < 0x100
    if ( (new_duty & 0xFF00u) != 0 ) { // Is the upper byte of new_duty 0?
	next_pwm_status = PWM_ON_HIGH; // Off period >= 0x100.
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
void rc_duty_set(unsigned short new_rc_duty) {
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

void start_from_running() {
    // Not quite where we run rc_duty_set normally,
    // but should be fine to drop it in here for now!
    rc_duty_set(MAX_POWER/16);
    switchPowerOff();
    init_comparator();
    greenLedOff();
    redLedOff();

    sys_control = PWR_MIN_START;
    set_duty = true;
    wait_timeout_init();

    // Initialization.
    // TODO: Update this comment to account for new control scheme.
    // Start with a short timeout to stop quickly
    // if we see no further pulses after the first.
    // Do not enable FETs during first cycle to
    // see if motor is running, and align to it.
    // If we can follow without a timeout, do not
    // continue in startup mode (long ZC filtering).
    // Enable PWM (ZL has been set to pwm_wdr)
    start_delay = 0;
    start_modulate = 0;
    start_fail  = 0;
    rc_timeout = RCP_TOT;
    power_skip = 6U;
    goodies = ENOUGH_GOODIES;
    enablePwmInterrupt();
    run_reverse();
}

// Also encapsulates wait_for_power_*
void restart_control() {
    switchPowerOff();
    set_duty = false;
    rc_duty_set(MAX_POWER/16);
    greenLedOn();
    redLedOff();
    // Idle beeping happened here in simonk.
    // dib_l/h set here (although not rc_duty?).
    // YL/rc_duty is set however, which seems to correspond to power?
    // After a bunch of puls input validation yadda yadda, we get to start_from_running
    start_from_running();
}

int main() {
    setDefaultRegisterValues();
    // Start off with the beepies!
    beepF1();
    _delay_ms(2000/8);
    beepF2();
    _delay_ms(2000/8);
    beepF3();
    _delay_ms(2000/8);
    beepF4();
    //_delay_ms(2000);
    while(false) {
	greenLedOn();
	_delay_ms(1000);
	redLedOn();
	_delay_ms(1000);
	redLedOff();
	_delay_ms(1000);
	greenLedOff();
	_delay_ms(1000);
    }
    while (true) {
	enableTimerInterrupts();
	sei();
	restart_control();
    }
}
