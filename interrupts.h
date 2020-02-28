

#include "globals.h"

#ifndef INTERRUPTS_H
#define INTERRUPTS_H


/************************************/
/* Timer2 Interrpts: PWM Interrupts */
/************************************/

// Equivelent setting ZL to pwm_wdr: in simonk, but we aren't yet using a watchdog.
void setPwmToNop() {
    PWM_STATUS = 0x00;
}

void setPwmToOff() {
    PWM_STATUS = 0x01;
}

bool pwmSetToNop() {
    return PWM_STATUS == 0x00;
}

bool pwmSetToOff() {
    return PWM_STATUS == 0x01;
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
