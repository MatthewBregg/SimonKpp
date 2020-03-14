#include <stdint.h>
#ifndef SET_DUTY_H
#define SET_DUTY_H

void set_new_duty_l(uint16_t rc_duty_copy);
void set_new_duty_set(uint16_t rc_duty_copy, uint16_t new_duty);
void set_new_duty();
void rc_duty_set(uint16_t new_rc_duty);
#endif
