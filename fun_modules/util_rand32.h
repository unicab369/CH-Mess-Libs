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

// Xorshift32 state variable
static u32 rand_state = 0x33845789;
static u32 rand_current;

// Initialize the generator with a seed
static inline void rand_xorshift32_seed(u32 seed) {
    // Seed cannot be 0
    rand_state = seed ? seed : 1;
}

// Generate next random number
static inline u32 rand_xorshift32(u32 *state) {
    u32 x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

static inline u8 rand_make_byte(void) {
    static u8 *byte_ptr = (u8*)&rand_current;
    static u8 *end_ptr = (u8*)&rand_current + 4;
    
    if (byte_ptr >= end_ptr) {
        rand_current = rand_xorshift32(&rand_state);
        byte_ptr = (u8*)&rand_current;
    }
    
    return *byte_ptr++;
}

static inline u32 rand_make_u32() {
    return rand_xorshift32(&rand_state);
}