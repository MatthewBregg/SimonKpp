#ifndef GLOBALS_H
#define GLOBALS_H

// Constants
constexpr bool HIGH_SIDE_PWM = false;

// PWM Related

// The PWM interrupt uses this byte to determine which action to take.
// TODO: Replace with enum.
volatile byte PWM_STATUS = 0x00;


#endif
