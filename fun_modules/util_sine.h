// MIT License
// Copyright (c) 2025 UniTheCat

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdint.h>

#define SINE_OFFSET      512      // Center offset

// Very simple sine-like wave perfect for LED animations - range 0-255
static inline u8 sine_8bits(u8 angle) {
    u8 x_mirrored = (angle < 128) ? angle : (255 - angle);
    u16 constant = (x_mirrored * (127 - x_mirrored)) >> 5;
    return (angle < 128) ? (128 + constant) : (127 - constant);
}

// angle range 0-1023
static inline u16 sine_10bits(u16 angle) {
    u16 x = angle & 1023;
    
    // Single expression - compilers optimize this well
    int32_t y = (x < SINE_OFFSET) 
        ? ((x * (SINE_OFFSET - x)) >> 7)
        : -(((x - SINE_OFFSET) * (1024 - x)) >> 7);  // Use 1024-x to avoid temp
    
    return SINE_OFFSET + y;
}

// angle range 0-0xFFFF
static inline u16 sine_16bits(u16 angle) {
    // Single expression
    int32_t value = 32768 + ((angle < 32768) ? 
        ((angle * (32768 - angle)) >> 13) :
        -(((65535 - angle) * (angle - 32767)) >> 13));
    
    if (value < 0) return 0;
    if (value > 65535) return 65535;
    return value;
}
