// NOTE: CONNECT WS2812's to PC6
//#define WSRBG //For WS2816C's.
#define WSGRB // For SK6805-EC15
#define DMALEDS 8

#include "../../fun_modules/fun_spi/fun_ws2812_spi.h"
#include "../../fun_modules/util_sine.h"
#include "../../fun_modules/util_rand32.h"

uint16_t phases[DMALEDS];

u8 sawtooth_hue(u8 index) {
    index &= 0xFF;
    
    // Continuous sawtooth: 0-255 repeating
    return (index * 255) / 255;  // Simple linear ramp
}

u8 generate_hue_value(u8 index) {
    index &= 0xFF;
    
    if (index < 42) return (index * 6);           // 0-252 in steps of 6
    if (index < 170) return 0xFF;                 // Plateau
    if (index < 212) return ((211 - index) * 6);  // 252-0 in steps of 6
    return 0x00;                                  // Bottom
}

// uint32_t WS2812BLEDCallback( int ledno ) {
// 	u8 sin_value = sine_8bits(phases[ledno] >> 8);
// 	u8 intensity = sin_value >> 3;		// scale down sin_value/8

// 	u8 rChannel = generate_hue_value(intensity + 30);
// 	u8 gChannel = generate_hue_value((intensity + 0)) >> 1;
// 	u8 bChannel = generate_hue_value(intensity + 190) >> 1;

// 	uint32_t fire =	rChannel | (u16)(gChannel << 8) | (u16)(bChannel << 16);
// 	uint32_t ice = (sin_value>>1) | ((sin_value>>1)<<8) | 0x7f0000;

// 	return fire;
// }

#define SPI_DC_PIN		PD2
#define SPI_RST_PIN		PC3

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	printf("TESTING \n");

	SPI_init(SPI_RST_PIN, SPI_DC_PIN);
	SPI_DMA_WS2812_init(DMALEDS, DMA1_Channel3);
	Neo_loadCommand(0x00);

	for(int k = 0; k < DMALEDS; k++ ) phases[k] = k<<8;

	u32 moment = micros();
	int counter = 0;
	
	while(1) {
		// // Wait for LEDs to totally finish.
		// Delay_Ms( 12 );

		// if (WS2812BLED_InUse) { continue; }

		// moment = micros();
		// for (int k = 0; k < DMALEDS; k++ ) {
		// 	// u8 rand = LUT_make_rand(k);
		// 	u8 rand = rand_make_byte() * 2;
		// 	// printf("%d \n", rand);
		// 	phases[k] += ((rand << 2) + (rand << 1)) >> 1;
		// }
		// // printf("%d \n", micros() - moment);
		

		// SPI_DMA_WS2812_tick();


		// if (millis() - moment > 7000) {
		// 	Neo_loadCommand(counter++);
		// 	moment = millis();
		// }

		Neo_task(millis());
	}
}

