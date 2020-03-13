#ifndef TIMING_DEGREES_H
#define TIMING_DEGREES_H

// TODO REMOVE HACK
void set_new_duty();
void set_timing_degrees(const uint8_t degree /* temp4 */);
uint32_t set_timing_degrees_slow(const uint8_t degree /* temp4 */);
uint32_t update_timing_add_degrees(uint32_t local_timing,
				   uint32_t local_com_time,
				   uint8_t degree /* temp4 */);

#endif
