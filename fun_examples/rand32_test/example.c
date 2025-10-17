#include "../../fun_modules/fun_base.h"
#include "../../fun_modules/util_rand32.h"

#include "../../fun_modules/util_colors.h"

// Volatile prevents compiler optimization
static volatile uint8_t result_sink;

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	u32 moment = millis();

	while(1) {
		moment = micros();
		for (int i = 0; i < 1000; i++) {
			result_sink = rand_make_byte();
		}
		u32 elapse1 = micros() - moment;

		moment = micros();
		for (int i = 0; i < 10000; i++) {
			result_sink = rand_make_byte();
		}
		u32 elapse2 = micros() - moment;

		moment = micros();
		for (int i = 0; i < 100000; i++) {
			result_sink = rand_make_byte();
		}
		u32 elapse3 = micros() - moment;

		printf("\n1k rand: %ld, 10k rand: %ld, 100k rand: %ld (us)\n",
			elapse1, elapse2, elapse3);

		moment = micros();
		for(int i = 0; i < 100000; i++ ) {
			result_sink = LUT_RANDS[i & 0xFF];
		}
		printf("LUT 100k Lookup time %ld\n", micros() - moment);

		Delay_Ms(2000);
	}
}