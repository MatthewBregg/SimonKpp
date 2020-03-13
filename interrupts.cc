#include "globals.h"
#include "atmel.h"
#include "interrupts.h"
/********************************************/
/* Timer1 Interrupts: Commutation timing.   */
/* Timer0 Interrupts: Beep control, delays. */
/********************************************/

///////////////////////////////////////////////////////////////////////////////////////
// Interrupt Documentation							     //
//    http://ee-classes.usc.edu/ee459/library/documents/avr_intr_vectors/	     //
//    https://arduino.stackexchange.com/questions/32721/interrupts-on-the-arduino-ng //
// BE VERY CAREFUL ABOUT USING RETURN STATEMENTS in interrupts, see                  //
// https://www.avrfreaks.net/forum/return-statement-isr                              //
///////////////////////////////////////////////////////////////////////////////////////


//Be sure to check interrupt names for the ATMEGA8 here.
// https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html#gad28590624d422cdf30d626e0a506255f
// The LEDs can be used to verify an interrupt works.

// timer1 output compare interrupt
ISR(TIMER1_COMPA_vect) {
    if (ocr1ax < 1) {
	oct1_pending = false; // Passed OCT1A.
    }
    --ocr1ax;
}

// timer1 overflow interrupt (happens every 4096µs)
ISR(TIMER1_OVF_vect) {
    ++tcnt1x;
    if ( (tcnt1x & 0b1111 /* 15U */ ) == 0 ) {
	if ( rc_timeout == 0 ) {
	    // rc_timeout hit, increase the beacon.
	    ++rct_boot;
	    ++rct_beacon;
	}
    }
}

ISR(TIMER2_OVF_vect) {
    switch(PWM_STATUS) {
    case PWM_OFF:
	pwm_off();
	break;
    case PWM_ON:
	pwm_on();
	break;
    case PWM_ON_FAST:
	pwm_on_fast();
	break;
    case PWM_ON_FAST_HIGH:
	pwm_on_fast_high();
	break;
    case PWM_ON_HIGH:
	pwm_on_high();
	break;
    case PWM_NOP:
	break;
    }
}
