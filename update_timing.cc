#include "update_timing.h"

#include "globals.h"
#include "byte_manipulation.h"
#include "timing_degrees.h"
#include "ocr1a.h"


// Time for the dragon: UPDATE TIMING.
void update_timing() {
    cli(); // Disable interrupts
    // LOADING TIME:
    // Load TCNT1L -> temp1.
    // Load TCNT1H into temp2
    // Load tcnt1x -> temp3.
    // Load TIFR into temp4
    uint16_t tcnt1_copy = TCNT1;
    uint8_t tcnt1x_copy = tcnt1x;
    uint8_t tifr_copy = TIFR;
    // We've loaded our values without worrying about interrupts,
    // now re-enable interrupts.
    sei();
    // Is TCNT1h set? Then tcnt1x is right, no need to increment.
    // Otherwise, if TOV1 was/is pending, increment our copy of tcnt1x.
    if ( 0x80u > get_high(tcnt1_copy) && ((tifr_copy & TOV1) != 0x00u)) {
	++tcnt1x_copy;
    }
    // Calculate the timing from the last two zero_crossings.
    // Yl/h/temp7 is now last_tcnt1.
    //
    // Take a copy of last_tcnt1 before we clobber it.
    const uint32_t last_tcnt1_copy = last_tcnt1;
    // Clobber our original last_tcnt1.
    last_tcnt1 = (((uint32_t)tcnt1x_copy) << 16) | (tcnt1_copy);

    // temp5/6/4 is now last2_tcnt1.
    const uint32_t last2_tcnt1_copy = last2_tcnt1;
    // Clobber our original last2_tcnt1 with our original last_tcnt1.
    last2_tcnt1 = last_tcnt1_copy;

    ////////////////////////////////////////////////////////////////////////////
    // ; Cancel DC bias by starting our timing from the average of the	  //
    // ; last two zero-crossings. Commutation phases always alternate.	  //
    // ; Next start = (cur(c) - last2(a)) / 2 + last(b)			  //
    // ; -> start=(c-b+(c-a)/2)/2+b						  //
    // ;									  //
    // ;                  (c - a)						  //
    // ;         (c - b + -------)						  //
    // ;                     2						  //
    // ; start = ----------------- + b					  //
    // ;                 2							  //
    ////////////////////////////////////////////////////////////////////////////

    // TLDR: Subtract our full 24bit copy of tcnt1x/TCNT1H/TCNT1L - last2_tcnt1_copy.
    uint32_t tcnt1_and_x_copy =  ((((uint32_t)tcnt1x_copy) << 16) | (tcnt1_copy)) - last2_tcnt1_copy;

    if ( tcnt1_and_x_copy < (TIMING_MAX * cpu_mhz/2) ) {
	// We've reached timing_max, divide sys_control by 2 and go to update_timing1.
	tcnt1_and_x_copy = (TIMING_MAX * cpu_mhz/2);
	sys_control /= 2;
	update_timing1(tcnt1_and_x_copy, last_tcnt1_copy);
	return;
    }
    // Otherwise repeat the above check with our governor.
    // service_governor:
    if ( tcnt1_and_x_copy < safety_governor  ) {
	// We've reached out safety governer, divide sys_control by 2 and go to update_timing1.
	tcnt1_and_x_copy = safety_governor;
	sys_control /= 2;
    }
    update_timing1(tcnt1_and_x_copy, last_tcnt1_copy);
    return;
}


////////////////////////////////////////////////////////////////////////////
// ; Calculate a hopefully sane duty cycle limit from this timing,	  //
// ; to prevent excessive current if high duty is requested at low	  //
// ; speed. This is the best we can do without a current sensor.	  //
// ; The actual current peak will depend on motor KV and voltage,	  //
// ; so this is just an approximation. This is calculated smoothly	  //
// ; with a (very slow) software divide only if timing permits.		  //
////////////////////////////////////////////////////////////////////////////

// Current Timing_period = temp1/2/3.
// Last_tcnt1_copy = yl/yh/temp7.
// xl/xh new_duty
void update_timing4(uint16_t new_duty, uint32_t current_timing_period, uint32_t unused) {
    current_timing_period &= 0xFFFFFF;
    timing_duty = new_duty;
    // Set timing_l/h/x.
    timing = current_timing_period;

    current_timing_period = current_timing_period >> 1;

    uint32_t last_tcnt1_copy = last_tcnt1;
    // This clobbers registers y/7.
    // Get and then store the start of the next commutation.
    com_timing = update_timing_add_degrees(current_timing_period,
						    last_tcnt1_copy,
						    (30 - MOTOR_ADVANCE) * 256 / 60.0);

    // Will 240 fit in 15 bits?
    if ( 0x0010 > ((current_timing_period >> 8) & 0xFFFF))  {
	timing_fast = true;
	set_ocr1a_abs_fast(com_timing); // Set timer for the next commutation
    } else  {
	timing_fast = false;
	set_ocr1a_abs_slow(com_timing);
    }
    // EVal rc?
    set_new_duty();
    return;

};


// Current Timing_period = temp1/2/3.
// Last_tcnt1_copy = yl/yh/temp7.
void update_timing1(const uint32_t current_timing_period, const uint32_t last_tcnt1_copy) {
    // XL/XH = MAX_POWER
    uint16_t new_duty = MAX_POWER;
    if ( SLOW_CPU && ((current_timing_period >> 8) & 0x00FFFF) < (TIMING_RANGE3 * cpu_mhz/2)) {
	update_timing4(new_duty, current_timing_period, last_tcnt1_copy);  // Fast timing: no duty limit
	return;
    }
    // TODO: Look into this more, after all this is quite tricky!!
    // Hope this division works!!!!
    // Implement simple fixed point division.
    // new_duty = MAX_POWER * (TIMING_RANGE3 * cpu_mhz/2)/  current_timing_period);;

    uint32_t undivided_value = (MAX_POWER * (TIMING_RANGE3 * cpu_mhz / 2) / 256.0);
    uint8_t counter = 33;
    bool carry = false;
    uint32_t temp456 = 0x00u;;
    // Is this shift going to set the carry?
    carry = undivided_value & 0x800000u;
    undivided_value = undivided_value << 1;
    --counter;
    do {
	// We don't use this carry.
	// bool carry_next = temp456 & 0x800000u;
	temp456 = temp456 << 1;
	if (carry) {
	    ++temp456;
	}
	// We don't use this carry.
	// carry = carry_next;

	carry = current_timing_period > temp456;
	if ( ! carry ) {
	    // Set what the carry will be from subtraction here.
	    if ( current_timing_period > temp456 ) {
		carry = true;
	    }
	    // subtraction here.
	    temp456-=current_timing_period;
	}
	// Is our shift about to set the carry?
	bool carry_next = undivided_value & 0x800000u;
	undivided_value = undivided_value << 1;
	// If we went into this with the OG carry set, add the carry on now.
	if ( carry ) {
	    ++undivided_value;
	}
	// Now set the carry to if our shift would have set it.
	carry = carry_next;
	// This finished rolling xl/xh/timing_duty_l.
	--counter;
    } while(counter != 0);

    // DONE DIVISION!
    // Ones complement the output.
    new_duty = ~undivided_value;
    new_duty = new_duty & 0x0000FFFFu;

    if ( PWR_MAX_RPM1 > new_duty ) {
	new_duty = PWR_MAX_RPM1;
    }

    update_timing4(new_duty, current_timing_period, last_tcnt1_copy);
    return;
}
