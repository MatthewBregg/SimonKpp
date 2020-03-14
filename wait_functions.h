#include <stdint.h>
#include "globals.h"

#ifndef WAIT_FUNCTIONS_H
#define WAIT_FUNCTIONS_H


void wait_for_edge1(uint8_t quartered_timing_lower);
void wait_for_edge();
void wait_for_edge2(uint8_t quartered_timing_higher, uint8_t quartered_timing_lower);
void wait_for_edge0();
void wait_pwm_enable();
void wait_pwm_running();
void wait_timeout(uint8_t quartered_timing_higher, uint8_t quartered_timing_lower);
void wait_commutation();

inline void wait_for_low() {
    aco_edge_high = false;
    wait_for_edge();
}

inline void wait_for_high() {
    aco_edge_high = true;
    wait_for_edge();
}

// Leave running mode and update timing/duty.
inline void wait_timeout_init() {
    startup = true;
    wait_commutation();
    return;
}


#endif
