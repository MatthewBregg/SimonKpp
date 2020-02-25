#include "atmel.h"
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

volatile bool oct1_pending = false;
volatile byte ocr1ax = 0; // third byte of OCR1A.
volatile byte tcnt1x = 0; // third byte of TCNT1.
volatile byte rc_timeout = 0;
volatile byte rct_boot = 0; // Counter which increments while rc_timeout is 0 to jump to boot loader (TODO delete?)
volatile byte rct_beacon = 0; // Counter which increments while rc_timeout is 0 to disarm and beep occasionally (TODO delete?)

///////////////////////////////////////////////////////////////////////////////////////
// Interrupt Documentation							     //
//    http://ee-classes.usc.edu/ee459/library/documents/avr_intr_vectors/	     //
//    https://arduino.stackexchange.com/questions/32721/interrupts-on-the-arduino-ng //
// BE VERY CAREFUL ABOUT USING RETURN STATEMENTS in interrupts, see                  //
// https://www.avrfreaks.net/forum/return-statement-isr                              //
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Be sure to check interrupt names for the ATMEGA8 here 							     //
// https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html#gad28590624d422cdf30d626e0a506255f	     //
// The LEDs can be used to verify an interrupt works				      				     //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
