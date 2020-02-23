#include "atmel.h"
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
    greenLedOn();
    redLedOff();
    delay(2000);
    greenLedOff();
    redLedOn();
    delay(2000);
}
void beep(const byte period) {
    zeroTCNT0();
    //beep 1.
    while ( getTCNT0() < 2*cpuMhz ) {}
    // Turn off all N side fets.
    AnFetOff();
    BnFetOff();
    CnFetOff();
    // Turn off all P side fets.
    ApFetOff();
    BpFetOff();
    CpFetOff();
    for (byte countDown = cpuMhz; countDown != 0; --countDown) {
	// beep 2.
	zeroTCNT0();
	// beep 3.
	while ( getTCNT0() < 200U ) {}
    }
    return;
}
void beepF1Impl() {
    for ( byte countDown = 80U; countDown != 0; --countDown) {
	BpFetOn();
	AnFetOn();
	beep(200U);
    }
}
void beepF1() {
    redLedOn();
    beepF1Impl();
    redLedOff();
}

void loop() {
    // put your main code here, to run repeatedly:
    beepF1();
    delay(2000);
    return;
}
