#include "atmel.h"
#include "globals.h"
#include "commutations.h"
#include "interrupts.h"
#include "beep.h"
// REMEMBER: VARIABLES BEING set/access from an interrupt must be volatile!

// Afro NFet ESCs have an external 16mhz oscillator.
// We are disregarding the bootloader, and will be using USBASP.
// For laziness/convenience, we will be using https://github.com/carlosefr/atmega to flash.
//
// This does not set fuses, we must do so manually if it's a new chip.
//    With external oscillator: avrdude -U lfuse:w:0x3f:m -U hfuse:w:0xca:m
void setup() {
  // put your setup code here, to run once:
    setDefaultRegisterValues();
    // Start off with the beepies!
    beepF1();
    delay(2000);
    beepF2();
    delay(2000);
    beepF3();
    delay(2000);
    beepF4();
    delay(2000);
}

void loop() {
    enableTimerInterrupts();
    sei();
    return;
}

void wait_startup() {}

void set_timing_degrees(byte temp) {}
void wait_OCT1_tot() {}

void demag_timeout() {}

void set_ocr1a_zct() {}

void wait_for_edge_fast(byte quartered_timing_lower) {
    set_ocr1a_zct();
    wait_for_edge1(quartered_timing_lower);
}
void wait_for_edge_fast_min() {
    wait_for_edge_fast(ZC_CHECK_MIN);
}
void wait_for_edge_below_max(byte quartered_timing_lower) {
    if ( quartered_timing_lower < ZC_CHECK_FAST ) {
	wait_for_edge_fast(quartered_timing_lower);
	return;
    }
    set_timing_degrees(24*256/120);
    wait_for_edge1(quartered_timing_lower);
}

void wait_for_edge0() {
    unsigned short quartered_timing =  timing >> 2;
    if ( quartered_timing <  MASKED_ZC_CHECK_MIN ) {
	wait_for_edge_fast_min();
	return;
    }
    if ( quartered_timing == MASKED_ZC_CHECK_MIN ) {
	wait_for_edge_fast((0xFFu & quartered_timing));
	return;
    }

    if ( quartered_timing < MASKED_ZC_CHECK_MAX ) {
	wait_for_edge_below_max((0xFFu & quartered_timing));
	return;
    }
    // simonk falls through to wait_for_edge_below_max
    wait_for_edge_below_max((0xFFu & MASKED_ZC_CHECK_MAX));
}

void wait_timeout(byte quartered_timing_higher, byte quartered_timing_lower) {}

void wait_for_edge2(byte quartered_timing_higher, byte quartered_timing_lower) {
    bool opposite_level;
    do {
	// If we don't have an oct1_pending, go to demag_timeout.
	if (!oct1_pending) {
	    wait_timeout(quartered_timing_higher,quartered_timing_lower);
	    return;
	}
	// potentially eval_rc,/set_duty here if we are doing that with our new protocol.
	// .if 0 ; Visualize comparator output on the flag pin.

	// XOR the ACO bit in ACSR with aco_edge_high, true if XOR would be 1, false otherwise.
	// TODO: Clean this up, can probably just do a logical != instead.
	/* aka demagnetization */
	opposite_level = (((ACSR | getByteWithBitSet(ACO)) ^
			   (aco_edge_high ? getByteWithBitSet(ACO) : getByteWithBitCleared(ACO))) > 0U);

	if (opposite_level == HIGH_SIDE_PWM) {
	    // cp xl, xh
	    // adc xl, zh
	    if (quartered_timing_lower < quartered_timing_higher ) {
		++quartered_timing_lower;
	    }
	} else {
	    --quartered_timing_lower;
	    if (quartered_timing_lower == 0) {
		// And then finally done! Return all the way back up through
		// wait_for_edge*
		wait_commutation();
		return;
	    }
	}

    } while(true);  // Check for demagnetization;

}

void wait_for_edge1(byte quartered_timing_lower) {
    wait_for_edge2(quartered_timing_lower,quartered_timing_lower);
}

void wait_for_demag() {
    bool opposite_level;
    do {
	// If we don't have an oct1_pending, go to demag_timeout.
	if (!oct1_pending) {
	    demag_timeout();
	    return;
	}
	// potentially eval_rc,/set_duty here if we are doing that with our new protocol.

	// XOR the ACO bit in ACSR with aco_edge_high, true if XOR would be 1, false otherwise.
	// TODO: Clean this up, can probably just do a logical != instead.
	opposite_level /* aka demagnetization */ = (((ACSR | getByteWithBitSet(ACO)) ^
							  (aco_edge_high ? getByteWithBitSet(ACO) : getByteWithBitCleared(ACO))) > 0U);

    } while(opposite_level != HIGH_SIDE_PWM);  // Check for demagnetization;
    wait_for_edge0();
}

void wait_pwm_running() {
    if ( startup ) {
	wait_startup();
    }
    set_timing_degrees(13U * 256U/120U);
    wait_OCT1_tot(); // Wait for the minimum blanking period;
    set_timing_degrees(42U * 256U/120U); // Set timeout for maximum blanking period.

    wait_for_demag();
}

void wait_pwm_enable() {
    if (pwmSetToNop() ) {
	setPwmToOff();
	redLedOff();
    }
    wait_pwm_running();
}

void set_ocr1a_rel(unsigned short Y) {}

void wait_for_edge() {
    if (power_skip < 1) { // Are we trying to track a maybe running motor?
	wait_pwm_enable();
	return;
    }
    // We might be trying to track a maybe running motor.
    --power_skip;
    if (!startup) {
	wait_for_edge0();
	return;
    }
    unsigned short Y = (0xFFU * 0x100U);
    set_ocr1a_rel(Y);
    // xl = ZC_CHECK_MIN.
    wait_for_edge1(MASKED_ZC_CHECK_MIN);
    return;
}

void wait_for_low() {
    aco_edge_high = false;
    wait_for_edge();
}

void wait_for_high() {
    aco_edge_high = true;
    wait_for_edge();
}

//  See run1 in simonk source.
void run() {}

void updateTiming() {};

void wait_commutation() {
    flagOn();
    updateTiming();
    startup = false;
    wait_OCT1_tot();
    flagOff();

    if (power_skip != 0x00u) {
	power_on = false;
    }
    // On rc_timeout, immediately restart control.
    if ( rc_timeout != 0x00u) {
	// TODO: Risk of stack overflow here eventually if we restart control enough?!
	restartControl();
	return;
    }


}

// Leave running mode and update timing/duty.
void wait_timeout_init() {
    startup = true;
    wait_commutation();
}

void startFromRunning() {
    switchPowerOff();
    initComparator();
    greenLedOff();
    redLedOff();

    // We could just truncate, https://www.learncpp.com/cpp-tutorial/unsigned-integers-and-why-to-avoid-them/
    // as the above link shows is safe, but avoid a compiler warning by using a bit mask to extract the lower 8 bits
    // of PWR_MIN_START.
    const byte sys_control_l = (0xFU /* Lol */ & PWR_MIN_START);
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
    //
    // We also set wait_timeout_init, start_delay, start_modulate,
    // and start_fail to 0x00 in simonK, but don't add them here quite yet...
    rc_timeout = RCP_TOT;
    power_skip = 6U;
    goodies = ENOUGH_GOODIES;
    enablePwmInterrupt();

}

// Also encapsulates wait_for_power_*
void restartControl() {
    switchPowerOff();
    set_duty = false;
    greenLedOn();
    redLedOff();
    // Idle beeping happened here in simonk.
    // dib_l/h set here (although not rc_duty?).
    // YL/rc_duty is set however, which seems to correspond to power?
    // After a bunch of puls input validation yadda yadda, we get to start_from_running
    startFromRunning();
}
