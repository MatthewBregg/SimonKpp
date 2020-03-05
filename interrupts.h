

#include "globals.h"

#ifndef INTERRUPTS_H
#define INTERRUPTS_H


/************************************/
/* Timer2 Interrpts: PWM Interrupts */
/************************************/

void setPwmToOn() {
    PWM_STATUS = PWM_ON;
}

// Equivelent setting ZL to pwm_wdr: in simonk, but we aren't yet using a watchdog.
void setPwmToNop() {
    PWM_STATUS = PWM_NOP;
}

void setPwmToOff() {
    PWM_STATUS = PWM_OFF;
}

bool pwmSetToNop() {
    return PWM_STATUS == PWM_NOP;
}

bool pwmSetToOff() {
    return PWM_STATUS == PWM_OFF;
}

// Actual PWM interrupt bodies.

// Copying the comment from simonk.
/*****************************************************************************/
/* ; Timer2 overflow interrupt (output PWM) -- the interrupt vector actually */
/* ; "ijmp"s to Z, which should point to one of these entry points.	     */
/* ;									     */
/* ; We try to avoid clobbering (and thus needing to save/restore) flags;    */
/* ; in, out, mov, ldi, cpse, etc. do not modify any flags, while dec does.  */
/* ;									     */
/* ; We used to check the comparator (ACSR) here to help starting, since PWM */
/* ; switching is what introduces noise that affects the comparator result.  */
/* ; However, timing of this is very sensitive to FET characteristics, and   */
/* ; would work well on some boards but not at all on others without waiting */
/* ; another 100-200ns, which was enough to break other boards. So, instead, */
/* ; we do all of the ACSR sampling outside of the interrupt and do digital  */
/* ; filtering. The AVR interrupt overhead also helps to shield the noise.   */
/* ;									     */
/* ; We reload TCNT2 as the very last step so as to reduce PWM dead areas    */
/* ; between the reti and the next interrupt vector execution, which still   */
/* ; takes a good 4 (reti) + 4 (interrupt call) + 2 (ijmp) cycles. We also   */
/* ; try to keep the switch on close to the start of pwm_on and switch off   */
/* ; close to the end of pwm_aff to minimize the power bump at full power.   */
/* ;									     */
/* ; pwm_*_high and pwm_again are called when the particular on/off cycle    */
/* ; is longer than will fit in 8 bits. This is tracked in tcnt2h.	     */
/*****************************************************************************/

void pwm_a_on() {
    if (HIGH_SIDE_PWM) {
	ApFetOn();
    } else {
	AnFetOn();
    }
}
void pwm_b_on() {
    if (HIGH_SIDE_PWM) {
	BpFetOn();
    } else {
	BnFetOn();
    }
}
void pwm_c_on() {
    if (HIGH_SIDE_PWM) {
	CpFetOn();
    } else {
	CnFetOn();
    }
}

void pwm_a_off() {
    if (HIGH_SIDE_PWM) {
	ApFetOff();
    } else {
	AnFetOff();
    }
}
void pwm_b_off() {
    if (HIGH_SIDE_PWM) {
	BpFetOff();
    } else {
	BnFetOff();
    }
}
void pwm_c_off() {
    if (HIGH_SIDE_PWM) {
	CpFetOff();
    } else {
	CnFetOff();
    }
}

void pwm_on_high() {
    --tcnt2h;
    if ( tcnt2h != 0) {
	return;
    }
    setPwmToOn();
    return;
}


void pwm_on_fast_high() {
    pwm_on_high();
    return;
}

void pwm_again() {
    --tcnt2h;
    return;
}

void pwm_on_fast() {
    if (a_fet) {
	pwm_a_on();
    }
    if (b_fet) {
	pwm_b_on();
    }
    if (c_fet) {
	pwm_c_on();
    }
    setPwmToOff();
    // Now reset the '16' bit timer2 to duty
    // H is the high byte
    tcnt2h = ((0xFF00u & duty) >> 8);
    // L is the low byte.
    TCNT2 = (0xFFu & duty);
    return;
}

void pwm_on() {
    pwm_on_fast();
    return;
}



// Disable PWM interrupts and turn off all FETS.
void switchPowerOff() {
    disablePWMInterrupts();
    clearPendingPwmInterrupts();
    setPwmToNop();
    // Switch all fets off.
    // Turn off all pFets
    ApFetOn();
    BpFetOn();
    CpFetOn();
    // Turn off all nFets
    AnFetOn();
    BnFetOn();
    CnFetOn();
}


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
    if ( (tcnt1x & B1111 /* 15U */ ) == 0 ) {
	if ( rc_timeout == 0 ) {
	    // rc_timeout hit, increase the beacon.
	    ++rct_boot;
	    ++rct_beacon;
	}
    }
}

#endif
