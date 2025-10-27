
#include "../../modules_ch5xx/fun_pwm_ch5xx.h"

u8 adc_channel = 4;

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	// 8-bit, cycle_sel = 1
	pwm_config(8, 1);

	// channel 4, clock div = 4, polarity 0 = active high
	pwm_init(adc_channel, 4, 0);

	u8 percent = 0;
	u32 time_ref = millis();

	while(1) {
		u32 moment = millis();

		if (moment - time_ref > 10) {
			time_ref = moment;
			pwm_set_duty_cycle(adc_channel, percent);
			if (++percent > 100) percent = 0;
		}
	}
}