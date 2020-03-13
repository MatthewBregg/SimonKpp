#include "globals.h"

#ifndef OCR1A_H
#define OCR1A_H

inline void wait_OCT1_tot() {
    do {
	// Potentially eval_rc, if the EVAL_RC flag is set.
	// if (eval_rc) { evaluate_rc(); }
    } while(oct1_pending); // Wait for commutation_time,
    // an interrupt will eventually flip this, t1oca_int:.
}



void set_ocr1a_abs_fast(const unsigned short y);
void set_ocr1a_abs_slow(const uint32_t new_timing);
void set_ocr1a_zct();
void set_ocr1a_rel(const uint32_t timing);
void set_ocr1a_rel(unsigned short Y, const uint8_t temp7);
#endif
