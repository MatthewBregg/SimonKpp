#include "wait_functions.h"
#include "ocr1a.h"
#include "timing_degrees.h"
#include <avr/io.h>
#include "control.h"
#include "atmel.h"
#include "interrupts.h"
#include "update_timing.h"
#include "commutations.h"

void demag_timeout() {
    setPwmToNop(); // Stop PWM switching, interrupts will not turn on any fets now!
    pwm_all_off();
    redLedOn();
    // Skip power for the next commutation. Note that this
    // won't decrease a non-zero powerskip because demag
    // checking is skipped when powerskip is non-zero.
    power_skip = 1;
    wait_commutation();
    return;
}

void wait_for_demag() {
    do {
	// If we don't have an oct1_pending, go to demag_timeout.
	if (!(flag_1 & byte_index_on(OCT1_PENDING_IDX))) {
	    demag_timeout();
	    return;
	}
	// potentially eval_rc,/set_duty here if we are doing that with our new protocol.
    } while((aco_edge_high != (bool(ACSR & getByteWithBitSet(ACO)))) != HIGH_SIDE_PWM);  // Check for demagnetization;
    wait_for_edge0();
}


void wait_for_edge_fast(uint8_t quartered_timing_lower) {
    set_ocr1a_zct();
    wait_for_edge1(quartered_timing_lower);
}
void wait_for_edge_fast_min() {
    wait_for_edge_fast(ZC_CHECK_MIN);
}
void wait_for_edge_below_max(uint8_t quartered_timing_lower) {
    if ( quartered_timing_lower < ZC_CHECK_FAST ) {
	wait_for_edge_fast(quartered_timing_lower);
	return;
    }
    set_timing_degrees(24*256/120.0);
    wait_for_edge1(quartered_timing_lower);
}

void wait_for_edge0() {
    // We take the two high  bytes of timing, and then left shift twice!
    uint16_t quartered_timing =  ((timing >> 8) & 0xFFFF) >> 2;
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

void wait_for_edge1(uint8_t quartered_timing_lower) {
    wait_for_edge2(quartered_timing_lower,quartered_timing_lower);
}

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
    uint16_t Y = (0xFFU * 0x100U);
    set_ocr1a_rel(Y, 0x00U);
    // xl = ZC_CHECK_MIN.
    wait_for_edge1(MASKED_ZC_CHECK_MIN);
    return;
}


void wait_for_edge2(uint8_t quartered_timing_higher, uint8_t quartered_timing_lower) {
    bool opposite_level;
    do {
	// If OCT1_pending, we need to go to wait_timeout.
	if (!(flag_1 & byte_index_on(OCT1_PENDING_IDX))) {
	    wait_timeout(quartered_timing_higher,quartered_timing_lower);
	    return;
	}
	// potentially eval_rc,/set_duty here if we are doing that with our new protocol.
	// .if 0 ; Visualize comparator output on the flag pin.

	opposite_level = (aco_edge_high != (bool(ACSR & getByteWithBitSet(ACO))));

	if (opposite_level != HIGH_SIDE_PWM) {
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

void wait_startup() {
    uint32_t new_timing = START_DELAY_US * ((uint32_t) cpu_mhz);
    if ( goodies >= 2 ) {
	const uint8_t degrees = start_delay;
	constexpr uint32_t start_destep_micros_clock_cycles = START_DSTEP_US * ((uint32_t) cpu_mhz) * 0x100u;
	new_timing = update_timing_add_degrees(start_destep_micros_clock_cycles, new_timing, degrees);
    }
    set_ocr1a_rel(new_timing);
    wait_OCT1_tot();
    const uint32_t timeout_cycles = TIMEOUT_START * ((uint32_t) cpu_mhz);
    set_ocr1a_rel(timeout_cycles);

   // Powered startup: Use a fixed (long) ZC check count until goodies reaches
   // ENOUGH_GOODIES and the STARTUP flag is cleared.
    wait_for_edge1(0xFFu * (cpu_mhz/16));
}






void wait_pwm_running() {
    if ( startup ) {
	wait_startup();
    }
    set_timing_degrees(13U * 256U/120.0);
    wait_OCT1_tot(); // Wait for the minimum blanking period;
    set_timing_degrees(42U * 256U/120.0); // Set timeout for maximum blanking period.

    wait_for_demag();
}

void wait_pwm_enable() {
    if (isPwmSetToNop() ) {
	setPwmToOff();
	redLedOff();
    }
    wait_pwm_running();
}

void wait_commutation() {
    flagOn();
    update_timing();
    startup = false;
    wait_OCT1_tot();
    flagOff();

    if (power_skip != 0x00u) {
	power_on = false;
    }
    // On rc_timeout, immediately restart control.
    if ( rc_timeout == 0x00u) {
	// TODO: Risk of stack overflow here eventually if we restart control enough?!
	restart_control();
	return;
    }


}


void wait_timeout_start() {
    goodies = 0x00u; // Clear good commutation count.

    // Increase the start (blanking) delay unless we are running.
    if (startup) {
	start_delay += START_DELAY_INC;
    }
    wait_timeout_init();
    return;
}

void wait_timeout_run() {
    redLedOn();
    wait_timeout_start();
}

void wait_timeout1(uint8_t quartered_timing_higher, uint8_t quartered_timing_lower) {
    set_ocr1a_zct();
    wait_for_edge2(quartered_timing_higher,quartered_timing_lower);
    return;
}

// We need a separate wait_timeout init FYI!
void wait_timeout(uint8_t quartered_timing_higher, uint8_t quartered_timing_lower) {
    if (startup) {
	wait_timeout_start();
	return;
    }
    if ( quartered_timing_higher < ZC_CHECK_FAST ) {
	wait_timeout_run();
	return;
    }
    quartered_timing_higher = (ZC_CHECK_FAST - 1);
    // Limit back tracking
    if ( quartered_timing_lower >= quartered_timing_higher ) {
	wait_timeout1(quartered_timing_higher,quartered_timing_lower);
	return;
    }
    // Intentionally passing higher both times.
    wait_timeout1(quartered_timing_higher,quartered_timing_higher); // Clip current distance from crossing.
    return;
}
