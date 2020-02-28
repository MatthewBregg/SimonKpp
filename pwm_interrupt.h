#include "globals.h"

#ifndef PWM_INTERRUPT_H
#define PWM_INTERRUPT_H

// Equivelent setting ZL to pwm_wdr: in simonk, but we aren't yet using a watchdog.
void setPwmToNop() {
    PWM_STATUS = 0x00;
}

void setPwmToOff() {
    PWM_STATUS = 0x01;
}

bool pwmSetToNop() {
    return PWM_STATUS == 0x00;
}

bool pwmSetToOff() {
    return PWM_STATUS == 0x01;
}


// Disable PWM interrupts and turn off all FETS.
void switchPowerOff() {
    disablePWMInterrupts();
    clearPendingPwmInterrupts();
    setPwmToNop();
    // Switch all fets off.
    // Turn off all pFets
    ApFetOn();
    BpFetOn();
    CpFetOn();
    // Turn off all nFets
    AnFetOn();
    BnFetOn();
    CnFetOn();
}




#endif
