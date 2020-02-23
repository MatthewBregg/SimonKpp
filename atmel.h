#ifndef ATMEL_H
#define ATMEL_H

constexpr byte cpuMhz = 16U;

constexpr byte T0CLK = 1U << CS01; // CLK/8 == 2 MHZ.
constexpr byte T1CLK = 0xC1u;
constexpr byte T2CLK = 1U << CS20; // (CLK/2) 16 MHZ
constexpr byte TIMER_INTERRUPTS_ENABLE =  (1U<<TOIE1) | (1U<<OCIE1A) | (1U<<TOIE2);


void zeroTCNT0() {
    TCNT0 = 0x00U;
}

byte getTCNT0() {
    return TCNT0;
}

// We want to do this AFTER beeping!
void enableTimerInterrupts() {
    // https://web.ics.purdue.edu/~jricha14/Timer_Stuff/TIFR.htm
    TIFR = TIMER_INTERRUPTS_ENABLE; // Clear TOIE1, OCIE1A, and TOIE2 flags
    // https://web.ics.purdue.edu/~jricha14/Timer_Stuff/TIMSK.htm
    TIMSK = TIMER_INTERRUPTS_ENABLE; // Enable t1ovfl_int, t1oca_int, t2ovfl_int
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
    TCCR0 = T0CLK; // timer0: beep control, delays
    TCCR1B = T1CLK; // timer1: communtation timing, RC pulse measurement.
    TCCR2 = B00000000; // timer2: PWM, stopped.

}

constexpr getByteWithBitSet(byte bitIndex) {
    return B00000001 << bitIndex;
}

constexpr getByteWithBitCleared(byte bitIndex) {
    return ~getByteWithBitSet(bitIndex);
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

#define ANFET_PORT PORTD
#define BNFET_PORT PORTD
#define CNFET_PORT PORTD
#define APFET_PORT PORTD
#define BPFET_PORT PORTB
#define CPFET_PORT PORTB

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