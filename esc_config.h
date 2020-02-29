#ifndef ESC_CONFIG_H
#define ESC_CONFIG_H


// Pinout ports
#define ANFET_PORT PORTD
#define BNFET_PORT PORTD
#define CNFET_PORT PORTD
#define APFET_PORT PORTD
#define BPFET_PORT PORTB
#define CPFET_PORT PORTB



// Mux config Bools
constexpr bool mux_a_defined = true;
constexpr bool mux_b_defined = true;
constexpr bool mux_c_defined = false;

constexpr byte admux_bitmask_to_enable_mux_a = 0x00U;
constexpr byte admux_bitmask_to_enable_mux_b = 0x01U;
constexpr byte admux_bitmask_to_enable_mux_c = 0x07U;

// Notes: delayMicroseconds is a NOP loop, no interrupts!
// https://electronics.stackexchange.com/questions/84776/arduino-delaymicroseconds
// Millis and Micros() both use timer0, whose frequency we clobbered.
// https://www.best-microcontroller-projects.com/arduino-millis.html
// https://ucexperiment.wordpress.com/2012/03/17/examination-of-the-arduino-micros-function/
// We went from a prescaler of 64 to 8, so multiply the output of millis() * 8 and it should be accurate.
// Micros is overflow count + tcnt0, so will NOT be accurate with that method however!
// We could however, implement our own micros via using the timer0_overflow_count ourself!
constexpr byte cpu_mhz = 16U;

#endif
