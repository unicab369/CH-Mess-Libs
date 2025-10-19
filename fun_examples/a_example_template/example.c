#include "../../fun_modules/fun_base.h"

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	u32 time_ref = millis();

	while(1) {
		u32 moment = millis();

		if (moment - time_ref > 1000) {
			time_ref = moment;
			printf("IM HERE\r\n");
		}
	}
}