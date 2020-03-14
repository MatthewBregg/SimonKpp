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
#include "set_duty.h"
#include "control.h"

// REMEMBER: VARIABLES BEING set/access from an interrupt must be volatile!
// Big TODO: Move into proper .cc/.h files, and INLINE the world. I can use -Winline to make not inlining a warning.


// Afro NFet ESCs have an external 16mhz oscillator.
// We are disregarding the bootloader, and will be using USBASP.
// For laziness/convenience, we will be using https://github.com/carlosefr/atmega to flash.
//
// This does not set fuses, we must do so manually if it's a new chip.
//    With external oscillator: avrdude -U lfuse:w:0x3f:m -U hfuse:w:0xca:m

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
