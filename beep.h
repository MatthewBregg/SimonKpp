#include "atmel.h"

#ifndef BEEP_H
#define BEEP_H

void beep(const uint8_t frequency) {
    zeroTCNT0();
    //beep 1.
    while ( getTCNT0() < 2*cpu_mhz ) {}
    // Turn off all N side fets.
    allNFetsOff();
    // Turn off all P side fets.
    allPFetsOff();
    for (uint8_t countDown = cpu_mhz; countDown != 0; --countDown) {
	// beep 2.
	zeroTCNT0();
	// beep 3.
	while ( getTCNT0() < frequency ) {}
    }
    return;
}
void beepF1() {
    redLedOn();
    // beep_f1_on
    for ( uint8_t beepDuration = 80U; beepDuration != 0; --beepDuration) {
	BpFetOn();
	AnFetOn();
	beep(200U);
    }
    redLedOff();
}

void beepF2() {
    redLedOn();
    greenLedOn();
    // beep_f2_on
    for ( uint8_t beepDuration = 100U; beepDuration != 0; --beepDuration) {
	CpFetOn();
	BnFetOn();
	beep(180U);
    }
    greenLedOff();
    redLedOff();

}

void beepF3() {
    redLedOn();
    // beep_f2_on
    for ( uint8_t beepDuration = 120U; beepDuration != 0; --beepDuration) {
	ApFetOn();
	CnFetOn();
	beep(160U);
    }
    redLedOff();

}

void beepF4() {
    redLedOn();
    greenLedOn();
    // beep_f2_on
    for ( uint8_t beepDuration = 140U; beepDuration != 0; --beepDuration) {
	CpFetOn();
	AnFetOn();
	beep(140U);
    }
    redLedOff();
    greenLedOff();

}

#endif
