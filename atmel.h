#ifndef ATMEL_H
#define ATMEL_H

void setDefaultRegisterValues() {
    DDRB = 0x06u;
    PORTB = 0x06u;

    DDRC = 0x00u;
    PORTC = 0x30u;

    DDRD = 0x3Eu;
    PORTD = 0x06u;

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
