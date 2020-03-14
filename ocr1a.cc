#include "ocr1a.h"
#include "globals.h"
#include "byte_manipulation.h"


void set_ocr1a_abs_fast(const uint16_t y) {
    const uint8_t ocf1a_mask = getByteWithBitSet(OCF1A);
    cli();
    OCR1A = y;
    TIFR = ocf1a_mask; // Clear any pending OCF1A interrupt.
    const uint16_t tcnt1_in = TCNT1;
    oct1_pending = true;
    ocr1ax = 0x00U;
    sei();
    if (y >= tcnt1_in) {
	return;
    }
    oct1_pending = false;
    return;

}


// Set OCT1_PENDING until the absolute time specified by YL:YH:temp7 passes.
// Returns current TCNT1(L:H:X) value in temp1:temp2:temp3.
//
// tcnt1x may not be updated until many instructions later, even with
// interrupts enabled, because the AVR always executes one non-interrupt
// instruction between interrupts, and several other higher-priority
// interrupts may (have) come up. So, we must save tcnt1x and TIFR with
// interrupts disabled, then do a correction.
//
// New timing = y/temp7.
//
// Potential bug source, this function is REALLY tricky!
// Wait, are we sure that tcnt1x and co should be unsigned actually?
// I might need to look into signed/unsigned subtraction...
void set_ocr1a_abs_slow(const uint32_t new_timing) {
    const uint8_t original_timsk = TIMSK;
    // Temp. disable TOIE1 and OCIE1A
    TIMSK = TIMSK & getByteWithBitCleared(TOIE1) & getByteWithBitCleared(OCIE1A);
    const uint8_t ocf1a_mask = getByteWithBitSet(OCF1A);
    cli();
    OCR1A = 0x0000FFFFu & new_timing;
    TIFR = ocf1a_mask; // Clear any pending OCF1A interrupts.
    const uint16_t tcnt1_in = TCNT1;
    sei();
    oct1_pending = true;

    uint8_t tcnt1x_copy = tcnt1x;
    const uint8_t tifr_orig = TIFR;

    // Question: I can't figure out what the point of cpi temp2, 0x80 is here, so I gave up.
    // OH, it sets the fucking carry for the following possibly adc instruction!!!!!
    const bool carry = (((tcnt1_in) >> 8 & 0x00FFu) < 0x80u);
    if ( carry && ((tifr_orig & getByteWithBitSet(TOV1)) != 0x00U)) {
	++tcnt1x_copy;
    }

    // Lots of tricky 8bit->24bit->32bit conversion here, hopefully nothing is bugged, or too slow.
    const uint32_t tcnt1_combined = ((uint32_t)tcnt1x_copy << 16) | (uint32_t)tcnt1_in;

    // TODO: Wrap in sei/cli?
    ocr1ax = (((new_timing - tcnt1_combined) & 0xFF000000u) >> 16) & 0x000000FFu;

    if (new_timing >= tcnt1_combined) {
	TIMSK = original_timsk;
	return;
    }
    oct1_pending = false;
    TIMSK = original_timsk;
    return;
}

void set_ocr1a_zct_slow() {
    // Loads 24 bits of com_time into y/tmp7, and 24 bits of timing into temp1-3.
    uint32_t y = com_timing;
    y += timing;
    y += timing;
    y &= 0xFFFFFF;
    set_ocr1a_abs_slow(y);
    return;
}

// Should his be returning the new com time perhaps?
void set_ocr1a_zct() {
    if ( slow_cpu && timing_fast ) {
        uint16_t y = com_timing;
        y += timing;
        y += timing;
	set_ocr1a_abs_fast(y);
    } else {
	set_ocr1a_zct_slow();
    }
}




void set_ocr1a_rel(uint16_t Y, const uint8_t temp7) {
    // Compensate for timer increment durring in-add-out
    // TODO(bregg): Uhoh, this sounds very, very implementation/ASM specific, lol.
    // Might be a pain with compiled code, will port as is for now, and see about a better solution later.
    // Perhaps check assembly, although that will be a PITA.
    // See https://forum.arduino.cc/index.php?topic=50169.0
    Y+=7; // Manually counted, held at 7 for now!
    // Isn't OCF1A constant? Can I just combine these?
    // Oh, I need to load OCF1A into a register before I can out it, which is why temp4 is used.
    // Leave for now, as we want to load OCF1A into the register first anyway, although I don't think the compiler
    // needs to respect that wish, so might need to get tweaked anyway based on code generated.
    // TLDR; I'll need to comback to this.
    const uint8_t ocf1a_bitmask = getByteWithBitSet(OCF1A); // A
    cli(); // B
    Y+=TCNT1; // C // Registers 0xF7/ and 0xFF?
    OCR1A = Y; // D
    TIFR = ocf1a_bitmask; // clear any pending interrupts, ideally, this should be 7 cycles from the earlier TCNT1 read.
    ocr1ax = temp7; // E
    oct1_pending = true; // F
    sei(); // G
    return;
}

void set_ocr1a_rel(const uint32_t timing) {
    const uint8_t upper = (timing >> 16) & 0xFFu;
    const uint16_t lower = (timing & 0xFFFFu);
    set_ocr1a_rel(lower, upper);
}
