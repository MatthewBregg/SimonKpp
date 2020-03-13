#include <stdint.h>

#ifndef UPDATE_TIMING_H
#define UPDATE_TIMING_H

void update_timing1(const uint32_t current_timing_period, const uint32_t last_tcnt1_copy);

void update_timing();
#endif
