// CORE FUNCTIONS stolen from ch32fun
// Copyright (c) 2025 UniTheCat
// This is a large modification of the original code

// MIT License
// Copyright (c) 2016 Sandeep Mistry

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

#include "../../fun_modules/fun_base.h"
#include "../util_colors.h"
#include "lib/lib_spi.h"

// #define WSGRB // For SK6805-EC15

// Must be divisble by 4.
#ifndef DMALEDS
	#define DMALEDS 16
#endif

// Note first n LEDs of DMA Buffer are 0's as a "break"
// Need one extra LED at end to leave line high. 
// This must be greater than WS2812B_RESET_PERIOD.
#define WS2812B_RESET_PERIOD 2

#ifdef WSRAW
	#define DMA_BUFFER_LEN (((DMALEDS)/2)*8)
#else
	#define DMA_BUFFER_LEN (((DMALEDS)/2)*6)
#endif

static uint16_t WS2812_DMA_BUF[DMA_BUFFER_LEN];
static volatile int WS2812LEDs;
static volatile int WS2812LED_Place;
static volatile int WS2812BLED_InUse;

uint32_t WS2812BLEDCallback(int ledno);

//! ####################################
//! CORE FUNCTIONS
//! ####################################

// This is the code that updates a portion of the WS2812_DMA_BUF with new data.
// This effectively creates the bitstream that outputs to the LEDs.
static void WS2812FillBuffSec(uint16_t * ptr, int numhalfwords, int tce) {
	const static uint16_t bitquartets[16] = {
		0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
		0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
		0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
		0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110
	};

	int i;
	uint16_t * end = ptr + numhalfwords;
	int ledcount = WS2812LEDs;
	int place = WS2812LED_Place;

	#ifdef WSRAW
		while( place < 0 && ptr != end ) {
			uint32_t * lptr = (uint32_t *)ptr;
			lptr[0] = 0; lptr[1] = 0; lptr[2] = 0; lptr[3] = 0;
			ptr += 8;
			place++;
		}
	#else
		while( place < 0 && ptr != end ) {
			(*ptr++) = 0; (*ptr++) = 0;
			(*ptr++) = 0; (*ptr++) = 0;
			(*ptr++) = 0; (*ptr++) = 0;
			place++;
		}
	#endif

	while( ptr != end ) {
		if( place >= ledcount ) {
			// Optionally, leave line high.
			while( ptr != end )
				(*ptr++) = 0;//0xffff;

			// Only safe to do this when we're on the second leg.
			if( tce ) {
				if( place == ledcount ) {
					// Take the DMA out of circular mode and let it expire.
					DMA1_Channel3->CFGR &= ~DMA_Mode_Circular;
					WS2812BLED_InUse = 0;
				}
				place++;
			}

			break;
		}

		#ifdef WSRAW
			uint32_t ledval32bit = WS2812BLEDCallback( place++ );
			ptr[6] = bitquartets[(ledval32bit>>28)&0xf];
			ptr[7] = bitquartets[(ledval32bit>>24)&0xf];
			ptr[4] = bitquartets[(ledval32bit>>20)&0xf];
			ptr[5] = bitquartets[(ledval32bit>>16)&0xf];
			ptr[2] = bitquartets[(ledval32bit>>12)&0xf];
			ptr[3] = bitquartets[(ledval32bit>>8)&0xf];
			ptr[0] = bitquartets[(ledval32bit>>4)&0xf];
			ptr[1] = bitquartets[(ledval32bit>>0)&0xf];
			ptr += 8;
			i += 8;
		#else
			// Use a LUT to figure out how we should set the SPI line.
			uint32_t ledval24bit = WS2812BLEDCallback( place++ );

			#ifdef WSGRB
				ptr[0] = bitquartets[(ledval24bit>>12)&0xf];
				ptr[1] = bitquartets[(ledval24bit>>8)&0xf];
				ptr[2] = bitquartets[(ledval24bit>>4)&0xf];
				ptr[3] = bitquartets[(ledval24bit>>0)&0xf];
				ptr[4] = bitquartets[(ledval24bit>>20)&0xf];
				ptr[5] = bitquartets[(ledval24bit>>16)&0xf];
			#elif defined( WSRBG )
				ptr[0] = bitquartets[(ledval24bit>>12)&0xf];
				ptr[1] = bitquartets[(ledval24bit>>8)&0xf];
				ptr[2] = bitquartets[(ledval24bit>>4)&0xf];
				ptr[3] = bitquartets[(ledval24bit>>0)&0xf];
				ptr[4] = bitquartets[(ledval24bit>>20)&0xf];
				ptr[5] = bitquartets[(ledval24bit>>16)&0xf];
			#else
				ptr[0] = bitquartets[(ledval24bit>>20)&0xf];
				ptr[1] = bitquartets[(ledval24bit>>16)&0xf];
				ptr[2] = bitquartets[(ledval24bit>>12)&0xf];
				ptr[3] = bitquartets[(ledval24bit>>8)&0xf];
				ptr[4] = bitquartets[(ledval24bit>>4)&0xf];
				ptr[5] = bitquartets[(ledval24bit>>0)&0xf];
			#endif
			
			ptr += 6;
			i += 6;
		#endif
	}

	WS2812LED_Place = place;
}

//# interrupt handler
void DMA1_Channel3_IRQHandler( void ) __attribute__((interrupt));
void DMA1_Channel3_IRQHandler( void )  {
	// Backup flags.
	volatile int intfr = DMA1->INTFR;

	do {
		// Clear all possible flags.
		DMA1->INTFCR = DMA1_IT_GL3;

		// Strange note: These are backwards.  DMA1_IT_HT3 should be HALF and
		// DMA1_IT_TC3 should be COMPLETE.  But for some reason, doing this causes
		// LED jitter.  I am henseforth flipping the order.
		if( intfr & DMA1_IT_HT3 ) {
			// Halfwaay (Fill in first part)
			WS2812FillBuffSec( WS2812_DMA_BUF, DMA_BUFFER_LEN / 2, 1 );
		}
		if( intfr & DMA1_IT_TC3 ) {
			// Complete (Fill in second part)
			WS2812FillBuffSec( WS2812_DMA_BUF + DMA_BUFFER_LEN / 2, DMA_BUFFER_LEN / 2, 0 );
		}
		intfr = DMA1->INTFR;
	} while( intfr & DMA1_IT_GL3 );
}


//# DMA init function
static void SPI_DMA_WS2812_init(int leds, DMA_Channel_TypeDef* DMA_Channel) {
	WS2812LEDs = leds;

	// Enable DMA peripheral
	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

	SPI1->CTLR2 = SPI_CTLR2_TXDMAEN;  // Enable Tx buffer DMA
	SPI1->HSCR = 1; // Enable high-speed read mode

	//DMA1_Channel3 is for SPI1TX
	DMA1_Channel3->PADDR = (u32)&SPI1->DATAR;
	DMA1_Channel3->MADDR = (u32)WS2812_DMA_BUF;
	DMA1_Channel3->CNTR  = 0;// sizeof( bufferset )/2; // Number of unique copies.  (Don't start, yet!)
	DMA1_Channel3->CFGR  =
		DMA_M2M_Disable |		 
		DMA_Priority_VeryHigh |
		DMA_MemoryDataSize_HalfWord |
		DMA_PeripheralDataSize_HalfWord |
		DMA_MemoryInc_Enable |
		DMA_Mode_Normal | // OR DMA_Mode_Circular or DMA_Mode_Normal
		DMA_DIR_PeripheralDST |
		DMA_IT_TC | DMA_IT_HT; // Transmission Complete + Half Empty Interrupts. 

	NVIC_EnableIRQ( DMA1_Channel3_IRQn );
	DMA1_Channel3->CFGR |= DMA_CFGR1_EN;
}


//# tick function
void SPI_DMA_WS2812_tick() {
	// Enter critical section.
	__disable_irq();
	WS2812BLED_InUse = 1;

	DMA1_Channel3->CFGR &= ~DMA_Mode_Circular;
	DMA1_Channel3->CNTR  = 0;
	DMA1_Channel3->MADDR = (uint32_t)WS2812_DMA_BUF;
	
	__enable_irq();
	WS2812LED_Place = -WS2812B_RESET_PERIOD;

	WS2812FillBuffSec( WS2812_DMA_BUF, DMA_BUFFER_LEN, 0 );
	DMA1_Channel3->CNTR = DMA_BUFFER_LEN; // Number of unique uint16_t entries.
	DMA1_Channel3->CFGR |= DMA_Mode_Circular;
}


//! ####################################
//! IMPLEMENTATIONS
//! ####################################

typedef struct {
	u32 frame_duration_ms;	  	// Duration for each frame in ms
	int8_t frame_step;		  	// Step for moving LEDs
	u16 modifier;				// modifier for each frame

	u8 prev_index;			  	// Previous index used
	u8 curr_index;			   	// Last index used
	u32 ref_time;			   	// Last time the move was updated
} WS2812_frame_t;

typedef struct {
	RGB_t* COLOR_BUF;		   	// Array of colors to cycle through
	u8 colors_len;			  	// Number of colors in the array
	u8 index;				   	// Current color index
} animation_color_t;

typedef enum {
	NEO_COLOR_FLASHING = 0x00,   // default
	NEO_COLOR_CHASE = 0x01,
	NEO_SOLO_COLOR_CHASE = 0x02,
	NEO_COLOR_FADE = 0x03,
	NEO_SOLO_COLOR_FADE = 0x04,
	NEO_SOLO_RANDOM_FADE = 0x05,
	NEO_RAINBOW_WAVE = 0x06,
	NEO_RAINBOW_FAST = 0x07,
	NEO_FIRE = 0x08,
	NEO_ICE = 0x09,
} Neo_Event_e;

RGB_t WS2812_BUF[DMALEDS] = {0};

WS2812_frame_t leds_frame = {
	.frame_duration_ms = 0,		// disabled 
	.frame_step = 1,			// Move one LED at a time
	.modifier = 0,

	.curr_index = 0,
	.prev_index = 0,
	.ref_time = 0,
};

RGB_t color_arr[] = {
	COLOR_RED_LOW, COLOR_GREEN_LOW, COLOR_BLUE_LOW
};

animation_color_t color_ani = {
	.COLOR_BUF = color_arr,
	.colors_len = sizeof(color_arr)/sizeof(RGB_t),
	.index = 0,
};


//# NEO_COLOR_FLASHING
void Neo_render_colorFlashing(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
	// Combine frame update and reset check
	if (++frame->modifier >= 100) {
		frame->modifier = 0;
		ani->index = (ani->index + 1) % ani->colors_len;
	}

	RGB_t color = ani->COLOR_BUF[ani->index];
	RGB_t new_color = COLOR_SET_BRIGHTNESS(color, frame->modifier);

	for (int i=0; i < DMALEDS; i++) {
		WS2812_BUF[i] = new_color;
	}
}

//# NEO_COLOR_CHASE
void Neo_render_colorChase(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
	for (int i=0; i < DMALEDS; i++) {
		WS2812_BUF[i] = ani->COLOR_BUF[((i + frame->curr_index) / 3) % ani->colors_len];
	}

	frame->curr_index += frame->frame_step;
}

//# NEO_SOLO_COLOR_CHASE
void Neo_render_soloColorChase(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
	WS2812_BUF[frame->prev_index] = COLOR_BLACK;	   // Turn off previous LED
	WS2812_BUF[frame->curr_index] = ani->COLOR_BUF[ani->index];;

	// update indexes
	frame->prev_index = frame->curr_index;
	frame->curr_index += frame->frame_step;
	
	if (frame->curr_index >= DMALEDS) {
		frame->curr_index %= DMALEDS;
		ani->index = (ani->index + 1) % ani->colors_len;
	}
}

//# NEO_COLOR_FADE
void Neo_render_colorFade(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
	// Fade all LEDs slightly
	for (int i = 0; i < DMALEDS; i++) {
		u8 diff = frame->curr_index - i;
		WS2812_BUF[i] = COLOR_DECREMENT(ani->COLOR_BUF[ani->index], diff*49);	   // Triangular diff growth
	}

	// update indexes
	frame->curr_index += frame->frame_step;

	if (frame->curr_index >= DMALEDS) {
		frame->curr_index %= DMALEDS;
		ani->index = (ani->index + 1) % ani->colors_len;
	}
}

//# NEO_SOLO_COLOR_FADE
void Neo_render_soloColorFade(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
	frame->modifier += 3;
	RGB_t color = ani->COLOR_BUF[ani->index];;
	WS2812_BUF[frame->curr_index] = COLOR_SET_BRIGHTNESS(color, frame->modifier);

	if (frame->modifier >= 100) {
		frame->modifier = 0;

		u8 next_idx = frame->curr_index + frame->frame_step;
		frame->curr_index = next_idx % DMALEDS;

		if (next_idx >= DMALEDS) {
			ani->index = (ani->index + 1) % ani->colors_len;
		}
	}
}

//# NEO_SOLO_RANDOM_FADE
void Neo_render_soloRandomFade(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
	frame->modifier += 3;
	RGB_t color = ani->COLOR_BUF[ani->index];
	WS2812_BUF[frame->curr_index] = COLOR_SET_BRIGHTNESS(color, frame->modifier);

	if (frame->modifier >= 100) {
		frame->modifier = 0;

        // Branchless different random index
        u8 next_idx = (frame->curr_index + 1 + (rand_make_u32() % (DMALEDS - 1))) % DMALEDS;
        frame->curr_index = next_idx;
	}
}

//# NEO_RAINBOW_WAVE
void Neo_render_rainbow_wave(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
    for (int i = 0; i < DMALEDS; i++) {
        // Moving rainbow with smooth sine transitions
        u8 base_hue = (frame->curr_index * 4 + i * 8) & 0xFF;
        
        u8 r = sine_8bits(base_hue);
        u8 g = sine_8bits((base_hue + 85) & 0xFF);   // 120° offset
        u8 b = sine_8bits((base_hue + 170) & 0xFF);  // 240° offset
        
        WS2812_BUF[i] = MAKE_COLOR_RGB(r, g, b);
    }
    
    frame->curr_index += frame->frame_step;
}

//# NEO_RAINBOW_FAST
void Neo_render_rainbow_fast(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
    // Adjustable parameters
    static const u8 SPEED = 3;           // Movement speed (1-20)
    static const u8 WIDTH = 200;         // Rainbow width (1-255)
    static const u8 divider = 2;
	
    for (int i = 0; i < DMALEDS; i++) {
        // Calculate hue with adjustable speed and width
        u8 hue = ((i * WIDTH / DMALEDS) + frame->curr_index * SPEED) & 0xFF;
        u8 r, g, b;
        
        // Rainbow color wheel
        if (hue < 85) {
            r = 255 - hue * 3;
            g = hue * 3;
            b = 0;
        } else if (hue < 170) {
            hue -= 85;
            r = 0;
            g = 255 - hue * 3;
            b = hue * 3;
        } else {
            hue -= 170;
            r = hue * 3;
            g = 0;
            b = 255 - hue * 3;
        }
		
        WS2812_BUF[i] = MAKE_COLOR_RGB(r/divider, g/divider, b/divider);
    }
    
    frame->curr_index += frame->frame_step;
}

//# NEO_FIRE - Stolen from ch32fun ws2812 example
uint16_t WS2812_PHASE_BUF[DMALEDS];

u8 sawtooth_hue(u8 index) {
	index &= 0xFF;
	
	// Continuous sawtooth: 0-255 repeating
	return (index * 255) / 255;  // Simple linear ramp
}

u8 generate_hue_value(u8 index) {
	index &= 0xFF;
	
	if (index < 42) return (index * 6);		   // 0-252 in steps of 6
	if (index < 170) return 0xFF;				 // Plateau
	if (index < 212) return ((211 - index) * 6);  // 252-0 in steps of 6
	return 0x00;								  // Bottom
}

void Neo_render_fire(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
	for (int i = 0; i < DMALEDS; i++ ) {
		// u8 rand = LUT_make_rand(i);
		u8 rand = rand_make_byte() * 2;
		WS2812_PHASE_BUF[i] += ((rand << 2) + (rand << 1)) >> 1;

		u8 sin_value = sine_8bits(WS2812_PHASE_BUF[i] >> 8);
		u8 intensity = sin_value >> 3;		// scale down sin_value/8

		u8 rChannel = generate_hue_value(intensity + 30);
		u8 gChannel = generate_hue_value((intensity + 0)) >> 1;
		u8 bChannel = generate_hue_value(intensity + 190) >> 1;

		u32 fire =	rChannel | (u16)(gChannel << 8) | (u16)(bChannel << 16);
		WS2812_BUF[i] = MAKE_COLOR_FROM32(fire);
	}
}

//# NEO_ICE - Stolen from ch32fun ws2812 example
void Neo_render_ice(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) {
	for (int i = 0; i < DMALEDS; i++ ) {
		u8 rand = LUT_make_rand(i);
		// u8 rand = rand_make_byte() * 2;
		WS2812_PHASE_BUF[i] += ((rand << 2) + (rand << 1)) >> 1;

		u8 sin_value = sine_8bits(WS2812_PHASE_BUF[i] >> 8);
		u32 ice = (sin_value>>1) | ((sin_value>>1)<<8) | 0x7f0000;
		WS2812_BUF[i] = MAKE_COLOR_FROM32(ice);
	}
}


void (*onNeo_handler)(WS2812_frame_t* frame, animation_color_t* ani, int ledIdx) = Neo_render_colorFlashing;

void Neo_loadCommand(u8 cmd) {
	cmd = cmd % 10;
	printf("Neo_loadCommand: %02X\n", cmd);

	leds_frame.curr_index = 0;
	leds_frame.ref_time = millis();
	color_ani.index = 0;
	memset(WS2812_BUF, 0, sizeof(WS2812_BUF));

	leds_frame.frame_duration_ms = 10;

	for(int k = 0; k < DMALEDS; k++ ) WS2812_PHASE_BUF[k] = k<<8;

	switch (cmd) {
		case NEO_COLOR_CHASE:
			leds_frame.frame_duration_ms = 150;
			onNeo_handler = Neo_render_colorChase; break;
		case NEO_SOLO_COLOR_CHASE:
			leds_frame.frame_duration_ms = 70;
			onNeo_handler = Neo_render_soloColorChase; break;
		case NEO_COLOR_FADE:
			leds_frame.frame_duration_ms = 70;
			onNeo_handler = Neo_render_colorFade; break;
		case NEO_SOLO_COLOR_FADE:
			onNeo_handler = Neo_render_soloColorFade; break;
		case NEO_SOLO_RANDOM_FADE:
			leds_frame.frame_duration_ms = 15;
			onNeo_handler = Neo_render_soloRandomFade; break;
		case NEO_RAINBOW_WAVE:
			leds_frame.frame_duration_ms = 30;
			onNeo_handler = Neo_render_rainbow_wave; break;
		case NEO_RAINBOW_FAST:
			leds_frame.frame_duration_ms = 30;
			onNeo_handler = Neo_render_rainbow_fast; break;
		case NEO_FIRE:
			onNeo_handler = Neo_render_fire; break;
		case NEO_ICE:
			onNeo_handler = Neo_render_ice; break;
		default:
			onNeo_handler = Neo_render_colorFlashing; break;
	}
}

u32 WS2812BLEDCallback(int ledIdx){
	u32 moment = millis();
	if (moment - leds_frame.ref_time > leds_frame.frame_duration_ms) {
		leds_frame.ref_time = moment;
		onNeo_handler(&leds_frame, &color_ani, ledIdx);
	}
	
	return WS2812_BUF[ledIdx].packed;
}

u32 neo_timeRef = 0;

void Neo_task(u32 time) {
	if (WS2812BLED_InUse || leds_frame.frame_duration_ms < 1) return;
	if (time - neo_timeRef < 12) return;
	neo_timeRef = time;

	SPI_DMA_WS2812_tick(DMALEDS);
}