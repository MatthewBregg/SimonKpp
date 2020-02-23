#include "atmel.h"

// Afro NFet ESCs have an external 16mhz oscillator.
// We are disregarding the bootloader, and will be using USBASP.
// For laziness/convenience, we will be using https://github.com/carlosefr/atmega to flash.
//
// This does not set fuses, we must do so manually if it's a new chip.
//    With external oscillator: avrdude -U lfuse:w:0x3f:m -U hfuse:w:0xca:m
void setup() {
  // put your setup code here, to run once:
    setDefaultRegisterValues();
}
void loop() {
    // put your main code here, to run repeatedly:
    greenLedOn();
    redLedOff();
    delay(2000);
    greenLedOff();
    redLedOn();
    delay(2000);
    return;
}
