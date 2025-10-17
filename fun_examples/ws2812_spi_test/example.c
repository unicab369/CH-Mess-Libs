// NOTE: CONNECT WS2812's to PC6
//#define WSRBG //For WS2816C's.
#define WSGRB // For SK6805-EC15
#define DMALEDS 16

#include "../../fun_modules/util_colors.h"
#include "../../fun_modules/fun_spi/fun_ws2812_spi.h"

static inline uint32_t FastMultiply( uint32_t big_num, uint32_t small_num ) 
	__attribute__((section(".srodata")));

static inline uint32_t FastMultiply( uint32_t big_num, uint32_t small_num ) {
	// The CH32V003 is an EC core, so no hardware multiply. GCC's way multiply
	// is slow, so I wrote this.
	//
	// This basically does this:
	//	return small_num * big_num;
	//
	// Note: This does NOT check for zero to begin with, though this still
	// produces the correct results, it is a little weird that even if
	// small_num is zero it executes once.
	//
	// Additionally note, instead of the if( m&1 ) you can do the following:
	//  ret += multiplciant & neg(multiplicand & 1).
	//
	// BUT! Shockingly! That is slower than an extra branch! The CH32V003
	//  can branch unbelievably fast.
	//
	// This is functionally equivelent and much faster.
	//
	// Perf numbers, with small_num set to 180V.
	//  No multiply:         21.3% CPU Usage
	//  Assembly below:      42.4% CPU Usage  (1608 bytes for whole program)
	//  C version:           41.4% CPU Usage  (1600 bytes for whole program)
	//  Using GCC (__mulsi3) 65.4% CPU Usage  (1652 bytes for whole program)
	//
	// The multiply can be done manually:
	uint32_t ret = 0;
	uint32_t multiplicand = small_num;
	uint32_t mutliplicant = big_num;
	do
	{
		if( multiplicand & 1 )
			ret += mutliplicant;
		mutliplicant<<=1;
		multiplicand>>=1;
	} while( multiplicand );
	return ret;

	// Which is equivelent to the following assembly (If you were curious)
/*
	uint32_t ret = 0;
	asm volatile( "\n\
		.option   rvc;\n\
	1:	andi t0, %[small], 1\n\
		beqz t0, 2f\n\
		add %[ret], %[ret], %[big]\n\
	2:	srli %[small], %[small], 1\n\
		slli %[big], %[big], 1\n\
		bnez %[small], 1b\n\
	" :
		[ret]"=&r"(ret), [big]"+&r"(big_num), [small]"+&r"(small_num) : :
		"t0" );
	return ret;
*/
}

// static const unsigned char huetable[64] = {
//     0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120,					// 16
//     128, 136, 144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248,		// 32
//     255, 248, 240, 232, 224, 216, 208, 200, 192, 184, 176, 168, 160, 152, 144, 136,		// 48
//     128, 120, 112, 104, 96, 88, 80, 72, 64, 56, 48, 40, 32, 24, 16, 8					// 64
// };


static const unsigned char huetable[] = {
	0x00, 0x06, 0x0c, 0x12, 0x18, 0x1e, 0x24, 0x2a, 0x30, 0x36, 0x3c, 0x42, 0x48, 0x4e, 0x54, 0x5a, 
	0x60, 0x66, 0x6c, 0x72, 0x78, 0x7e, 0x84, 0x8a, 0x90, 0x96, 0x9c, 0xa2, 0xa8, 0xae, 0xb4, 0xba, 
	0xc0, 0xc6, 0xcc, 0xd2, 0xd8, 0xde, 0xe4, 0xea, 0xf0, 0xf6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xf9, 0xf3, 0xed, 0xe7, 0xe1, 0xdb, 0xd5, 0xcf, 0xc9, 0xc3, 0xbd, 0xb7, 0xb1, 0xab, 0xa5, 
	0x9f, 0x99, 0x93, 0x8d, 0x87, 0x81, 0x7b, 0x75, 0x6f, 0x69, 0x63, 0x5d, 0x57, 0x51, 0x4b, 0x45, 
	0x3f, 0x39, 0x33, 0x2d, 0x27, 0x21, 0x1b, 0x15, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

uint16_t phases[DMALEDS];

uint32_t WS2812BLEDCallback( int ledno ) {
	// get the high byte of 16-bit phase and use as index for Sin LUT (Lookup Table)
	u8 sin_idx = (phases[ledno]) >> 8;
	u8 sin_value = LUT_SIN[sin_idx];
	u8 intensity = sin_value >> 3;		// scale down sin_value/8

	uint32_t fire =	(huetable[(intensity + 30 ) & 0xff]) |					// R channel
					((huetable[(intensity + 0)] >> 1) << 8) |				// G channel
					((huetable[(intensity + 190) & 0xff] >> 1) << 16);		// B channel
	uint32_t ice = (sin_value>>1) | ((sin_value>>1)<<8) | 0x7f0000;

	return fire;
}

#define SPI_DC_PIN		PD2
#define SPI_RST_PIN		PC3

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	printf("TESTING \n");

	SPI_init(SPI_RST_PIN, SPI_DC_PIN);
	SPI_DMA_WS2812_init(DMA1_Channel3);
	// Neo_loadCommand(0x01);

	for(int k = 0; k < DMALEDS; k++ ) phases[k] = k<<8;

	while(1) {
		// Wait for LEDs to totally finish.
		Delay_Ms( 12 );

		if (WS2812BLEDInUse) { continue; }

		for(int k = 0; k < DMALEDS; k++ ) {
			u8 rand = LUT_make_rand(k);
			phases[k] += ((rand << 2) + (rand << 1)) >> 1;
		}

		SPI_DMA_WS2812_tick( DMALEDS );

		// Neo_task(moment);
	}
}

