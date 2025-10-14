#include "../../fun_modules/fun_base.h"
#include "../../fun_modules/fun_i2c/lib/lib_i2c.h"
// #include "../../fun_modules/fun_i2c/fun_ssd1306.h"
#include "../../fun_modules/fun_encoder_tim2.h"
#include "../../fun_modules/fun_encoder_gpio.h"

#include "ssd1306_menu.h"

//# Callbacks
void onHandle_Encoder(int8_t position, int8_t direction) {
	printf("pos: %d, direction: %d\n", position, direction);
	// mngI2c_load_encoder(millis(), position, direction);
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	//# TIM2: uses PD4(CH1) and PD3(CH2)
	Encoder_t encoder_a;
	// fun_encoder_tim2_init(&encoder_a);

	Encoder_GPIO_t encoder_b = {
		.pinA = PD4,
		.pinB = PD3,
	};
	fun_encoder_gpio_init(&encoder_b);

	u32 time_ref = millis();
	u32 counter = 0;

	ssd1306_menu_init();

	while(1) {
		u32 moment = millis();

		if ((moment - time_ref) > 1000) {
			sprintf(str_output, "Hello %ld", counter++);

			u32 time_ref2 = micros();
			// ssd1306_draw_str(str_output, 0, 0);
			// printf("elapsed: %d us\n", micros() - time_ref2);
			// ssd1306_draw_scaled_text(0, 0, str_output, &DEAULF_STR_CONFIG);
			makeMenu();

			// test_lines();
			time_ref = millis();
		}

		// fun_encoder_tim2_task(moment, &encoder_a, onHandle_Encoder);
		fun_encoder_gpio_task(moment, &encoder_b, onHandle_Encoder);
	}
}

        //    FLASH:        5692 B        16 KB     34.74%
        //      RAM:        1212 B         2 KB     59.18%