#include "esc_config.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////
// Attempt to isolate chip level actions here, with the dream being to swap this file with arm.h, and //
// the code then compiles for arm.  Currently for a fixed esc_config, but will likely be split into	      //
// architecture.h and esc_config.h in the future.							      //
// 												      //
// Examples: chip related constants like Mhz, configuring timers, setting pins.			      //
//												      //
// For the Arduino IDE, I am using https://github.com/carlosefr/atmega.                               //
//												      //
////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef ATMEL_H
#define ATMEL_H

// T0CLK INFO, for TIMER0
// Set to increment twice an us, sets CS01 on.
constexpr byte T0CLK = 1U << CS01; // CLK/8 == 2 MHZ.
// T1CLK INFO, for TIMER1
// Set to increment 16 times a us.
// Also set the ICNC1 noise cancelling bit, and the ICES1 bit to trigger the interrupt on the rising edge.
// http://www.avrbeginners.net/architecture/timers/timers.html
// Look up the ICP pin and such.
// Essentially, on a input capture pin, on a rising/falling pulse, transfer the current value in TCNT1 into
// input capture register (ICR1), which can then be used in the ISR to measure the prior/next pulse.
// EX: Clear TCNT1 on the rising edge, and then measure on the known to be coming falling edge.
constexpr byte T1CLK = 0xC1u;
constexpr byte T2CLK = 1U << CS20; // (CLK/1) 16 MHZ
constexpr byte TIMER_INTERRUPTS_ENABLE =  (1U<<TOIE1) | (1U<<OCIE1A) | (1U<<TOIE2);
constexpr byte UNSIGNED_ZERO = B00000000;


void zeroTCNT0() {
    TCNT0 = 0x00U;
}

// Note: With our current prescaler setting,
// TCNT0 is incremented twice each microsecond.
// If we reset TCNT0, We can use this to EX, loop for 16 us with while(getTCNT0() < 32) { }...
byte getTCNT0() {
    return TCNT0;
}

// We want to do this AFTER beeping!
void enableTimerInterrupts() {
    // Note:  Atmega8 only has TIMSK, while ATMEGA328P and co have TIMSK0/1, which makes
    // some docs/code confusing, as the Atmega328P/Atmega8 are often interchanged/considered the same.
    // Well, this is one of those small differences...
    // https://web.ics.purdue.edu/~jricha14/Timer_Stuff/TIFR.htm
    TIFR = TIMER_INTERRUPTS_ENABLE; // Clear TOIE1, OCIE1A, and TOIE2 flags
    // https://web.ics.purdue.edu/~jricha14/Timer_Stuff/TIMSK.htm
    TIMSK = TIMER_INTERRUPTS_ENABLE; // Enable t1ovfl_int, t1oca_int, t2ovfl_int
}

// Timer2 is used for PWM, enable it.
void enablePwmInterrupt() {
    // Timer2 Control register, start the timer.
    TCCR2 = T2CLK;

}

void setDefaultRegisterValues() {
    ///////////////////////////////////////////////////
    // Set the I/O registers to safe/default values. //
    ///////////////////////////////////////////////////
    DDRB = 0x06u;
    PORTB = 0x06u;

    DDRC = 0x00u;
    PORTC = 0x30u;

    DDRD = 0x3Eu;
    PORTD = 0x06u;

    /////////////////////////////////////////////
    // Set the TIMERS to their default values. //
    /////////////////////////////////////////////
    TCCR0 = T0CLK; // timer0: beep control, delays;
    TCCR1B = T1CLK; // timer1: communtation timing, RC pulse measurement.
    TCCR2 = B00000000; // timer2: PWM, stopped.

}

constexpr byte getByteWithBitSet(byte bitIndex) {
    return B00000001 << bitIndex;
}

constexpr byte getByteWithBitCleared(byte bitIndex) {
    return ~getByteWithBitSet(bitIndex);
}

////////////////////////////////////////////////////////////////////////////////
// ;-- Analog comparator sense macros --------------------------------------- //
// ; We enable and disable the ADC to override ACME when one of the sense     //
// ; pins is AIN1 instead of an ADC pin. In the future, this will allow	      //
// ; reading from the ADC at the same time.				      //
// Analog Comparator Magic: Investigate more later, but for now, just copy    //
// the bits which get SET.                                                    //
////////////////////////////////////////////////////////////////////////////////
void init_comparator() {
    SFIOR = getByteWithBitSet(ACME) | SFIOR; // Set Analog Comparator Multiplexor Enable

    if ( mux_a_defined && mux_b_defined && mux_c_defined) {
	// Disable ADC to make sure ACME works. Only happens if mux_a and mux_b and mux_c are defined...
	ADCSRA = ADCSRA & getByteWithBitCleared(ADEN);
    }
}

void comp_adc_enable() {
    ADCSRA = ADCSRA | getByteWithBitSet(ADEN);
}

void comp_adc_disable() {
    // If any muxes are undefined, this must be ran.
    // If no muxes use the AIN1 pin, then we will never have set ADEN
    // to read AIN1, and this doesn't need to be ran.
    // Otherwise, we are trying to read an ADC 0-7, so make sure
    // the mux is enabled by clearing ADEN, which might have been set
    // during an earlier read involving the AIN1 mux (in afro_nfets, mux_c).
    if ( !mux_a_defined || !mux_b_defined || !mux_c_defined) {
	ADCSRA = ADCSRA & getByteWithBitCleared(ADEN);
    }
}

// Set the ADC to compare against phase x.
void set_comp_phase_a() {
    if (mux_a_defined) {
	// Set comparator multiplexer to phase a.
	ADMUX = admux_bitmask_to_enable_mux_a;
	// If we disabled the mux to read AIN1 (in afro_nfet, mux_c is on ain1),
	// then disable the ADC here, which re-enables the mux.
	comp_adc_disable();
    } else {
	comp_adc_enable();
    }
}

void set_comp_phase_b() {
    if (mux_b_defined) {
	// Set comparator multiplexer to phase a.
	ADMUX = admux_bitmask_to_enable_mux_b;
	// If we disabled the mux to read AIN1 (in afro_nfet, mux_c is on ain1),
	// then disable the ADC here, which re-enables the mux.
	comp_adc_disable();
    } else {
	comp_adc_enable();
    }
}

// Mux C is `undefined` and set to AIN1 on afros_nfe, so we enable the ADC here!
void set_comp_phase_c() {
    if (mux_c_defined) {
	// Set comparator multiplexer to phase a.
	ADMUX = admux_bitmask_to_enable_mux_c;
	// If we disabled the mux to read AIN1 (in afro_nfet, mux_c is on ain1),
	// then disable the ADC here, which re-enables the mux.
	comp_adc_disable();
    } else {
	comp_adc_enable();
    }
}

/***************************/
/* Motor Driving Functions */
/***************************/
constexpr byte AnFetIdx = 3;
constexpr byte BnFetIdx = 4;
constexpr byte CnFetIdx = 5;
constexpr byte ApFetIdx = 2;
constexpr byte BpFetIdx = 2;
constexpr byte CpFetIdx = 1;


// In high_side PWM mode, add code to make these be XpFET_PORT.
#define PWM_A_PORT_IN ANFET_PORT
#define PWM_B_PORT_IN BNFET_PORT
#define PWM_C_PORT_IN CNFET_PORT


// xNFets are active high.
void AnFetOn() {
    ANFET_PORT = ANFET_PORT | getByteWithBitSet(AnFetIdx);
}
void BnFetOn() {
    BNFET_PORT = BNFET_PORT | getByteWithBitSet(BnFetIdx);
}
void CnFetOn() {
    CNFET_PORT = CNFET_PORT | getByteWithBitSet(CnFetIdx);
}

void AnFetOff() {
    ANFET_PORT = ANFET_PORT & getByteWithBitCleared(AnFetIdx);
}
void BnFetOff() {
    BNFET_PORT = BNFET_PORT & getByteWithBitCleared(BnFetIdx);
}
void CnFetOff() {
    CNFET_PORT = CNFET_PORT & getByteWithBitCleared(CnFetIdx);
}

// xPFets are active low.
void ApFetOff() {
    APFET_PORT = APFET_PORT | getByteWithBitSet(ApFetIdx);
}
void BpFetOff() {
    BPFET_PORT = BPFET_PORT | getByteWithBitSet(BpFetIdx);
}
void CpFetOff() {
    CPFET_PORT = CPFET_PORT | getByteWithBitSet(CpFetIdx);
}

void ApFetOn() {
    APFET_PORT = APFET_PORT & getByteWithBitCleared(ApFetIdx);
}
void BpFetOn() {
    BPFET_PORT = BPFET_PORT & getByteWithBitCleared(BpFetIdx);
}
void CpFetOn() {
    CPFET_PORT = CPFET_PORT & getByteWithBitCleared(CpFetIdx);
}

void allPFetsOff() {
    ApFetOff();
    BpFetOff();
    CpFetOff();
}

void allNFetsOff() {
    AnFetOff();
    BnFetOff();
    CnFetOff();
}

// Motor debug output, unimplemented ATM.
void flagOn() {}
void flagOff() {}

void sync_on() {}
void sync_off() {}

// Disable PWM, clear PWM interrupts, stop PWM switching

void disablePWMInterrupts() {

    TCCR2 = UNSIGNED_ZERO; // Disable PWM interrupts;
}
void clearPendingPwmInterrupts() {
    TIFR = getByteWithBitSet(TOV2); // Clear pending PWM interrupts
}


/************************************************************/
/* LED IO FUNCTIONS 					     */
/* The LEDs are active low!				     */
/* SimonK sets PORTC to leave GRN_LED and RED_LED at 0, and */
/* then flips DDRC to output or not output.		     */
/************************************************************/
constexpr byte GRN_LED_INDEX = 2;
constexpr byte RED_LED_INDEX = 3;
void greenLedOn() {
    // Set pinmode to output.
    DDRC = DDRC | getByteWithBitSet(GRN_LED_INDEX);
    // Set output mode to LOW, not needed as long as this BIT in PORTC
    // is not otherwise clobbered, so comment out.
    // PORTC = PORTC & getByteWithBitCleared(GRN_LED_INDEX);
}
void greenLedOff() {
    // Set pinmode to high impedence.
    DDRC = DDRC & getByteWithBitCleared(GRN_LED_INDEX);
    // Set output mode to high, not needed as we just used DDRC to make
    // this pin be high impedence.
    // PORTC = PORTC | getByteWithBitSet(GRN_LED_INDEX);
}

void redLedOn() {
    // Set pinmode to output.
    DDRC = DDRC | getByteWithBitSet(RED_LED_INDEX);
    // Set output mode to LOW, not needed as long as this BIT in PORTC
    // is not otherwise clobbered, so comment out.
    // PORTC = PORTC & getByteWithBitCleared(RED_LED_INDEX);
}
void redLedOff() {
    // Set pinmode to high impedence.
    DDRC = DDRC & getByteWithBitCleared(RED_LED_INDEX);
    // Set output mode to high, not needed as we just used DDRC to make
    // this pin be high impedence.
    // PORTC = PORTC | getByteWithBitSet(RED_LED_INDEX);
}


#endif
