#ifndef ESC_CONFIG_H
#define ESC_CONFIG_H



// Notes: delayMicroseconds is a NOP loop, no interrupts!
// https://electronics.stackexchange.com/questions/84776/arduino-delaymicroseconds
// Millis and Micros() both use timer0, whose frequency we clobbered.
// https://www.best-microcontroller-projects.com/arduino-millis.html
// https://ucexperiment.wordpress.com/2012/03/17/examination-of-the-arduino-micros-function/
// We went from a prescaler of 64 to 8, so multiply the output of millis() * 8 and it should be accurate.
// Micros is overflow count + tcnt0, so will NOT be accurate with that method however!
// We could however, implement our own micros via using the timer0_overflow_count ourself!
constexpr byte cpuMhz = 16U;

#endif
