// Core functions stolen from ch32fun

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

        #ifdef WSRBG
            ptr[0] = bitquartets[(ledval24bit>>12)&0xf];
            ptr[1] = bitquartets[(ledval24bit>>8)&0xf];
            ptr[2] = bitquartets[(ledval24bit>>20)&0xf];
            ptr[3] = bitquartets[(ledval24bit>>16)&0xf];
            ptr[4] = bitquartets[(ledval24bit>>4)&0xf];
            ptr[5] = bitquartets[(ledval24bit>>0)&0xf];
        #elif defined( WSGRB )
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
    RGB_t color;
    u32 frame_duration;    // Duration for each frame in ms
    int8_t frame_step;          // Step for moving LEDs
    u8 frame_value;        // Value for each frame
    u8 frame_LEN;          // number of LEDs
    u8 is_enabled;
    int8_t cycle_count;
    
    u8 prev_index;         // Previous index used
    u8 ref_index;          // Last index used
    u32 ref_time;          // Last time the move was updated
} WS2812_frame_t;

typedef struct {
    RGB_t* colors;          // Array of colors to cycle through
    u8 num_colors;     // Number of colors in the array
    u8 ref_index;      // Current color index
} animation_color_t;


void animation_step(animation_color_t* ani) {
    ani->ref_index = (ani->ref_index + 1) % ani->num_colors;
}

RGB_t animation_currentColor(animation_color_t* ani) {
    return ani->colors[ani->ref_index];
}

RGB_t animation_colorAt(animation_color_t* ani, u8 steps, u8 index) {
    return ani->colors[(index/steps) % ani->num_colors];
}


typedef enum {
    NEO_COLOR_CHASE = 0x01,
    NEO_SOLO_COLOR_CHASE = 0x02,
    NEO_COLOR_FADE = 0x03,
    NEO_SOLO_COLOR_FADE = 0x04,
    NEO_COLOR_FLASHING = 0x05,
} Neo_Event_e;

Neo_Event_e Neo_Event_list[] = {
    NEO_COLOR_CHASE,
    NEO_SOLO_COLOR_CHASE,
    NEO_COLOR_FADE,
    NEO_SOLO_COLOR_FADE,
    NEO_COLOR_FLASHING
};

RGB_t led_arr[DMALEDS] = {0};

WS2812_frame_t leds_frame = {     
    .frame_duration = 70, 
    .frame_step = 1,            // Move one LED at a time
    .frame_value = 0,
    .is_enabled = 0,

    .ref_index = 0,
    .prev_index = 0,
    .ref_time = 0,
};

RGB_t color_arr[] = {
    COLOR_RED_LOW, COLOR_GREEN_LOW, COLOR_BLUE_LOW
};

animation_color_t color_ani = {
    .colors = color_arr,
    .num_colors = sizeof(color_arr)/sizeof(RGB_t),
    .ref_index = 0,
};

int8_t systick_handleTimeout(u32 *ref_time, u32 duration) {
	u32 now = millis();
	if (now - *ref_time > duration) {
		*ref_time = now;
		return 1;
	}
	return 0;
}

u32 Neo_render_colorChase(WS2812_frame_t* fr, animation_color_t* ani, int ledIdx) {
    if (systick_handleTimeout(&fr->ref_time, fr->frame_duration)) {
        for (int i=0; i < DMALEDS; i++) {
            led_arr[i] = animation_colorAt(ani, 5, i+fr->ref_index);
        }

        fr->ref_index += fr->frame_step;
    }

    return led_arr[ledIdx].packed;
}

u32 Neo_render_soloColorChase(WS2812_frame_t* fr, animation_color_t* ani, int ledIdx) {
    if (systick_handleTimeout(&fr->ref_time, fr->frame_duration)) {
        led_arr[fr->prev_index] = COLOR_BLACK;       // Turn off previous LED
        led_arr[fr->ref_index] = animation_currentColor(ani);

        // set previous index
        fr->prev_index = fr->ref_index;

        // update next index
        u8 next_idx = fr->ref_index + fr->frame_step;
        fr->ref_index = next_idx % DMALEDS;
        
        // animation_step(ani);
        if (next_idx >= DMALEDS) animation_step(ani);
    }

    return led_arr[ledIdx].packed;
}

u32 Neo_render_colorFade(WS2812_frame_t* fr, animation_color_t* ani, int ledIdx) {
    if (systick_handleTimeout(&fr->ref_time, fr->frame_duration)) {
        // Fade all LEDs slightly
        for (int i = 0; i < DMALEDS; i++) {
            u8 diff = fr->ref_index - i;
            RGB_t color = animation_currentColor(ani);
            led_arr[i] = COLOR_DECREMENT(color, diff*49);       // Triangular diff growth
        }

        u8 next_increment = fr->ref_index + fr->frame_step;
        fr->ref_index = next_increment % DMALEDS;

        if (next_increment >= DMALEDS) {
            animation_step(ani);
        }
    }
    
    return led_arr[ledIdx].packed;
}

u32 Neo_render_soloColorFade(WS2812_frame_t* fr, animation_color_t* ani, int ledIdx) {
    if (systick_handleTimeout(&fr->ref_time, fr->frame_duration)) {
        fr->frame_value += 3;
        RGB_t color = animation_currentColor(ani);
        led_arr[fr->ref_index] = COLOR_SET_BRIGHTNESS(color, fr->frame_value);

        if (fr->frame_value >= 100) {
            fr->frame_value = 0;

            u8 next_idx = fr->ref_index + fr->frame_step;
            fr->ref_index = next_idx % DMALEDS;

            if (next_idx >= DMALEDS) {
                animation_step(ani);
            }
        }
    }

    return led_arr[ledIdx].packed;
}

u32 Neo_render_colorFlashing(WS2812_frame_t* fr, animation_color_t* ani, int ledIdx) {
    if (systick_handleTimeout(&fr->ref_time, fr->frame_duration)) {
        fr->frame_value += 1;
        RGB_t color = animation_currentColor(ani);

        for (int i=0; i < DMALEDS; i++) {
            led_arr[i] = COLOR_SET_BRIGHTNESS(color, fr->frame_value);
        }

        if (fr->frame_value >= 100) {
            fr->frame_value = 0;

            animation_step(ani);
        }
    }

    return led_arr[ledIdx].packed;
}

u8 Neo_LedCmd = 0x61;

void Neo_loadCommand(u8 cmd) {
    printf("Neo_loadCommand: %02X\n", cmd);

    Neo_LedCmd = cmd;
    leds_frame.is_enabled = 1;
    leds_frame.ref_index = 0;
    leds_frame.ref_time = millis();

    color_ani.ref_index = 0;
    memset(led_arr, 0, sizeof(led_arr));
}

// u32 WS2812BLEDCallback(int ledIdx){
//     leds_frame.frame_duration = 70;

//     switch (Neo_LedCmd) {
//         case NEO_COLOR_CHASE:
//             return Neo_render_colorChase(&leds_frame, &color_ani, ledIdx);
//             break;
//         case NEO_SOLO_COLOR_CHASE:
//             return Neo_render_soloColorChase(&leds_frame, &color_ani, ledIdx);
//             break;
//         case NEO_COLOR_FADE:
//             return Neo_render_colorFade(&leds_frame, &color_ani, ledIdx);
//             break;
//         case NEO_SOLO_COLOR_FADE:
//             leds_frame.frame_duration = 10;
//             return Neo_render_soloColorFade(&leds_frame, &color_ani, ledIdx);
//             break;
//         case NEO_COLOR_FLASHING:
//             leds_frame.frame_duration = 10;
//             return Neo_render_colorFlashing(&leds_frame, &color_ani, ledIdx);
//             break;
//         default:
//             return Neo_render_colorFlashing(&leds_frame, &color_ani, ledIdx);
//     }
// }

u32 neo_timeRef = 0;

void Neo_task(u32 time) {
    if (time - neo_timeRef < 10) return;
    neo_timeRef = time;
    
    if (WS2812BLED_InUse || leds_frame.is_enabled == 0) return;

    SPI_DMA_WS2812_tick(DMALEDS);
}