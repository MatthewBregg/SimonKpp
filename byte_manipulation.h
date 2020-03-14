#include <stdint.h>

#ifndef BYTE_MANIPULATION_H
#define BYTE_MANIPULATION_H
// get_low/get_high/etc with macros/union from
// https://www.avrfreaks.net/forum/c-programming-how-split-int16-bits-2-char8bit,
// which appears to generate much more efficient code.

/// Super efficient, but scary byte macros
// Sensative to endian ordering, but mutable and very efficient!
#define LowB(x) (*((uint8_t*)  &(x)+0))

#define HighB(x) (*((uint8_t*) &(x)+1))


#define ThirdByteB(x) (*((uint8_t*) &(x)+2))

inline constexpr uint8_t getByteWithBitSet(uint8_t bitIndex) {
    return 0b00000001 << bitIndex;
}

inline constexpr uint8_t getByteWithBitCleared(uint8_t bitIndex) {
    return ~getByteWithBitSet(bitIndex);
}


inline uint8_t get_low(const uint16_t in) {
    return 0xFFu & in;
}

inline uint8_t get_high(const uint16_t in) {
    return (0xFF00u & in) >> 8;
}

inline uint8_t get_byte3(const uint32_t in) {
    return (0xFF0000u & in) >> 16;
}


#endif
