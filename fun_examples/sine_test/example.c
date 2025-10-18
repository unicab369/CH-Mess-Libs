#include "../../fun_modules/fun_base.h"
#include "../../fun_modules/util_sine.h"
#include "../../fun_modules/util_luts.h"

// Volatile prevents compiler optimization
static volatile u16 result_sink;

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	u32 moment = millis();
	u32 count = 0;

	while(1) {
		moment = micros();
		for (int k = 0; k < 1024/0xFF; k++) {
			for (int i = 0; i < 0xFF; i++) {
				result_sink = sine_8bits(i);
				// printf("%d \n", result_sink);
			}
			// printf("\n\n");
		}
		u32 elapse1 = micros() - moment;

		moment = micros();
		for (int i = 0; i < 1024; i++) {
			result_sink = sine_10bits(i);
			// printf("%d \n", result_sink);
		}
		u32 elapse2 = micros() - moment;

		count = 0;
		moment = micros();
		for (u32 i = 0; i < 0xFFFF; i+=64) {
			result_sink = sine_16bits(i);
			printf("%d \n", result_sink);
			count++;
		}
		u32 elapse3 = micros() - moment;

		printf("\n\n sine_8bits: \t\t %ld us", elapse1);
		printf("\n sine_10bits:	\t %ld us", elapse2);
		printf("\n sine_16bits:	\t %ld us, cycle: %ld", elapse3, count);

		moment = micros();
		for (int k = 0; k < 1024/0xFF; k++) {
			for(int i = 0; i < 0xFF; i++ ) {
				result_sink = LUT_SIN[i & 0xFF];
			}
		}
		printf("\n LUT Lookup: \t\t%ld us\n", micros() - moment);

		Delay_Ms(2000);
	}
}
