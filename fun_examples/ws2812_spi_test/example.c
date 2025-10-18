// NOTE: CONNECT WS2812's to PC6
//#define WSRBG //For WS2816C's.
#define WSGRB // For SK6805-EC15
#define DMALEDS 8

#include "../../fun_modules/util_sine.h"
#include "../../fun_modules/util_rand32.h"
#include "../../fun_modules/fun_spi/fun_ws2812_spi.h"

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
	Neo_loadCommand(NEO_RAINBOW_FAST);

	u32 moment = micros();
	int counter = 0;
	
	while(1) {
		if (millis() - moment > 7000) {
			Neo_loadCommand(counter++);
			moment = millis();
		}

		Neo_task(millis());
	}
}

