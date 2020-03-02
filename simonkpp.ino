#include "atmel.h"
#include "globals.h"
#include "commutations.h"
#include "interrupts.h"
#include "beep.h"
// REMEMBER: VARIABLES BEING set/access from an interrupt must be volatile!
// Big TODO: Move into proper .cc/.h files, and INLINE the world. I can use -Winline to make not inlining a warning.


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


uint32_t update_timing_add_degrees(uint32_t local_timing, uint32_t local_com_time, const byte degree /* temp4 */) {
    // I'm probably seeing the forest for the trees.
    // TODO(bregg): Look at this again.
    uint32_t new_com_timing = timing*((uint32_t)degree);
    return ((new_com_timing >> 8) & 0xFFFFFFu) + local_com_time;
}

uint32_t set_timing_degrees_slow(const byte degree /* temp4 */) {
    // Loads 24 bits of com_time into y/tmp7, and 24 bits of timing into temp1-3.
    return update_timing_add_degrees(timing, com_timing, degree);

}



byte get_low(const unsigned short in) {
    return 0xFFu & in;
}

byte get_high(const unsigned short in) {
    return (0xFF00u & in) >> 8;
}

// Should this be returning the new com_time perhaps?
// Another scary and tricky and bug prone function?
void set_timing_degrees(const byte degree /* temp4 */) {
    if ( slow_cpu && timing_fast ) {

	// WTF is this? Is this just normal 16x16 multiplication? Need to play around with this,
	// I'm probably seeing the forest for the trees.
	// TODO(bregg): Look at this again.
	const unsigned short timing_low_degree_product =
	    ((unsigned short)get_low(timing)) * ((unsigned short)degree);
	const unsigned short timing_high_degree_product =
	    ((unsigned short)get_high(timing)) * ((unsigned short)degree);
	unsigned short new_com_timing = (com_timing & 0xFFFFu) + get_high(timing_low_degree_product);
	new_com_timing += timing_high_degree_product;
	set_ocr1a_abs_fast(new_com_timing);
    } else {
	uint32_t new_timing = set_timing_degrees_slow(degree);
	set_ocr1a_abs_slow(new_timing);
    }

}
void wait_OCT1_tot() {
    do {
	// Potentially eval_rc, if the EVAL_RC flag is set.
	// if (eval_rc) { evaluate_rc(); }
    } while(oct1_pending); // Wait for commutation_time,
    // an interrupt will eventually flip this, t1oca_int:.
}

void demag_timeout() {
    pwmSetToNop(); // Stop PWM switching, interrupts will not turn on any fets now!
    pwm_all_off();
    redLedOn();
    // Skip power for the next commutation. Note that this
    // won't decrease a non-zero powerskip because demag
    // checking is skipped when powerskip is non-zero.
    power_skip = 1;
    wait_commutation();
    return;
}

void set_ocr1a_abs_fast(const unsigned short y) {
    const byte ocf1a_mask = getByteWithBitSet(OCF1A);
    cli();
    OCR1A = y;
    TIFR = ocf1a_mask; // Clear any pending OCF1A interrupt.
    const unsigned short tcnt1_in = TCNT1;
    oct1_pending = true;
    ocr1ax = 0x00U;
    sei();
    if (y >= tcnt1_in) {
	return;
    }
    oct1_pending = false;
    return;

}


// Set OCT1_PENDING until the absolute time specified by YL:YH:temp7 passes.
// Returns current TCNT1(L:H:X) value in temp1:temp2:temp3.
//
// tcnt1x may not be updated until many instructions later, even with
// interrupts enabled, because the AVR always executes one non-interrupt
// instruction between interrupts, and several other higher-priority
// interrupts may (have) come up. So, we must save tcnt1x and TIFR with
// interrupts disabled, then do a correction.
//
// New timing = y/temp7.
//
// Potential bug source, this function is REALLY tricky!
// Wait, are we sure that tcnt1x and co should be unsigned actually?
// I might need to look into signed/unsigned subtraction...
void set_ocr1a_abs_slow(const uint32_t new_timing) {
    const byte original_timsk = TIMSK;
    // Temp. disable TOIE1 and OCIE1A
    TIMSK = TIMSK & getByteWithBitCleared(TOIE1) & getByteWithBitCleared(OCIE1A);
    const byte ocf1a_mask = getByteWithBitSet(OCF1A);
    cli();
    OCR1A = 0x0000FFFFu & new_timing;
    TIFR = ocf1a_mask; // Clear any pending OCF1A interrupts.
    const unsigned short tcnt1_in = TCNT1;
    sei();
    oct1_pending = true;

    byte tcnt1x_copy = tcnt1x;
    const byte tifr_orig = TIFR;

    // Question: I can't figure out what the point of cpi temp2, 0x80 is here, so I gave up.
    // OH, it sets the fucking carry for the following possibly adc instruction!!!!!
    const bool carry = (((tcnt1_in) >> 8 & 0x00FFu) < 0x80u);
    if ( carry && ((tifr_orig & getByteWithBitSet(TOV1)) != 0x00U)) {
	++tcnt1x_copy;
    }

    // Lots of tricky 8bit->24bit->32bit conversion here, hopefully nothing is bugged, or too slow.
    const uint32_t tcnt1_combined = ((uint32_t)tcnt1x_copy << 16) | (uint32_t)tcnt1_in;

    ocr1ax = (((new_timing - tcnt1_combined) & 0xFF000000u) >> 16) & 0x000000FFu;

    if (new_timing >= tcnt1_combined) {
	TIMSK = original_timsk;
	return;
    }
    oct1_pending = false;
    TIMSK = original_timsk;
    return;
}

void set_ocr1a_zct_slow() {
    // Loads 24 bits of com_time into y/tmp7, and 24 bits of timing into temp1-3.
    set_ocr1a_abs_slow(timing*2 + com_timing);
    return;
}

// Should his be returning the new com time perhaps?
void set_ocr1a_zct() {
    if ( slow_cpu && timing_fast ) {
	unsigned short y = ((unsigned short)com_timing) + 2*((unsigned short)timing);
	set_ocr1a_abs_fast(y);
    } else {
	set_ocr1a_zct_slow();
    }
}


void wait_timeout(byte quartered_timing_higher, byte quartered_timing_lower) {}

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

void set_ocr1a_rel(unsigned short Y, const byte temp7) {
    // Compensate for timer increment durring in-add-out
    // TODO(bregg): Uhoh, this sounds very, very implementation/ASM specific, lol.
    // Might be a pain with compiled code, will port as is for now, and see about a better solution later.
    // Perhaps check assembly, although that will be a PITA.
    // See https://forum.arduino.cc/index.php?topic=50169.0
    Y+=7; // Manually counted, held at 7 for now!
    // Isn't OCF1A constant? Can I just combine these?
    // Oh, I need to load OCF1A into a register before I can out it, which is why temp4 is used.
    // Leave for now, as we want to load OCF1A into the register first anyway, although I don't think the compiler
    // needs to respect that wish, so might need to get tweaked anyway based on code generated.
    // TLDR; I'll need to comback to this.
    const byte ocf1a_bitmask = getByteWithBitSet(OCF1A); // A
    cli(); // B
    Y+=TCNT1; // C // Registers 0xF7/ and 0xFF?
    OCR1A = Y; // D
    TIFR = ocf1a_bitmask; // clear any pending interrupts, ideally, this should be 7 cycles from the earlier TCNT1 read.
    ocr1ax = temp7; // E
    oct1_pending = true; // F
    sei(); // G
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

void run6() {
    // IF last commutation timed out and power is off, return to restart control
    if (!power_on && goodies == 0) {
	// TODO: Risk of stack overflow here eventually if we restart control enough?!
	// Eventually does it make sense to just make restartControl a loop perhaps?
	restart_control();
	return;
    }

}

//  See run1 in simonk source.
void run_reverse() {
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
    run6();

}

void update_timing() {};

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
    if ( rc_timeout != 0x00u) {
	// TODO: Risk of stack overflow here eventually if we restart control enough?!
	restart_control();
	return;
    }


}

// Leave running mode and update timing/duty.
void wait_timeout_init() {
    startup = true;
    wait_commutation();
}

void start_from_running() {
    switchPowerOff();
    init_comparator();
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
void restart_control() {
    switchPowerOff();
    set_duty = false;
    greenLedOn();
    redLedOff();
    // Idle beeping happened here in simonk.
    // dib_l/h set here (although not rc_duty?).
    // YL/rc_duty is set however, which seems to correspond to power?
    // After a bunch of puls input validation yadda yadda, we get to start_from_running
    start_from_running();
}




 ////////////////////////
 // Wait For Edge Here //
 ////////////////////////
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
    set_ocr1a_rel(Y, 0x00U);
    // xl = ZC_CHECK_MIN.
    wait_for_edge1(MASKED_ZC_CHECK_MIN);
    return;
}
 ////////////////////////
 // Wait For Edge Here //
 ////////////////////////
