#include "globals.h"
#include "byte_manipulation.h"
#include "timing_degrees.h"
#include "ocr1a.h"
// Also careful, inputs are oh god, 1,2,3, 4, y/temp7.
// Temp4: Degrees
// 1/2/3: local_timing
// y/7: com_time.
// For a quick'n'dirty check that this code did what I think, and what the simonk comment says
// (which hopefully is what this actually does in simonk!!!), I wrote/ran
// https://pastebin.com/eVc0asJa
// returns via y/7.
// Messing around here:
// https://pastebin.com/xW8fGFz5
uint32_t update_timing_add_degrees(uint32_t local_timing,
				   uint32_t local_com_time,
				   uint8_t degree /* temp4 */) {

   local_com_time += get_high(degree * get_low(local_timing));
   local_com_time += degree * get_high(local_timing);
   // TODO: Removing this uint32_t changes the behavior of this function?! Why?
   local_com_time += (((uint32_t)degree) * ((uint32_t)get_byte3(local_timing))) << 8;
   return local_com_time & 0xFFFFFFu;
}


uint32_t set_timing_degrees_slow(const uint8_t degree /* temp4 */) {
    // Loads 24 bits of com_time into y/tmp7, and 24 bits of timing into temp1-3.
    return update_timing_add_degrees(timing, com_timing, degree);

}

// Should this be returning the new com_time perhaps?
// Another scary and tricky and bug prone function?
void set_timing_degrees(const uint8_t degree /* temp4 */) {
    if ( slow_cpu && timing_fast ) {
	// WTF is this? Is this just normal 16x16 multiplication? Need to play around with this,
	// I'm probably seeing the forest for the trees.
	// TODO(bregg): Look at this again.
	const uint16_t timing_low_degree_product =
	    ((uint16_t)get_low(timing)) * ((uint16_t)degree);
	const uint16_t timing_high_degree_product =
	    ((uint16_t)get_high(timing)) * ((uint16_t)degree);
	uint16_t new_com_timing = (com_timing & 0xFFFFu) + get_high(timing_low_degree_product);
	new_com_timing += timing_high_degree_product;
	set_ocr1a_abs_fast(new_com_timing);
    } else {
	uint32_t new_timing = set_timing_degrees_slow(degree);
	set_ocr1a_abs_slow(new_timing);
    }

}
