#include "../../fun_modules/fun_base.h"
#include "../../fun_modules/fun_encoder_tim2.h"
#include "../../fun_modules/fun_encoder_gpio.h"
#include "../../fun_modules/fun_button.h"

#include "i2c_manager.h"

#define BUTTON_PIN PC0

//# Encoder Callback
void onHandle_Encoder(int8_t position, int8_t direction) {
	i2c_menu_move(direction);
}

//# Button Callback
void button_onChanged(Button_Event_e event, u32 time) {
	switch (event) {
		case BTN_SINGLECLICK:
			printf("Single Click\n");
			i2c_menu_handle();
			break;
		case BTN_DOUBLECLICK:
			printf("Double Click\n");
			break;
		case BTN_LONGPRESS:
			printf("Long Press\n"); break;
	}

	// mngI2c_load_buttonState(millis(), event);
}

//! ####################################
//! MAIN FUNCTION
//! ####################################

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	//# Button: uses PC0
	static Button_t button1 = { .pin = BUTTON_PIN };
	fun_button_setup(&button1);

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

	i2c_menu_init();

	while(1) {
		u32 moment = millis();
		
		fun_button_task(moment, &button1, button_onChanged);

		if ((moment - time_ref) > 1000) {
			sprintf(str_output, "Hello %ld", counter++);

			u32 time_ref2 = micros();
			// ssd1306_draw_str(str_output, 0, 0);
			// printf("elapsed: %d us\n", micros() - time_ref2);
			// ssd1306_draw_scaled_text(0, 0, str_output, &DEAULF_STR_CONFIG);

			// test_lines();
			time_ref = millis();
		}

		// fun_encoder_tim2_task(moment, &encoder_a, onHandle_Encoder);
		fun_encoder_gpio_task(moment, &encoder_b, onHandle_Encoder);
	}
}

        //    FLASH:        5692 B        16 KB     34.74%
        //      RAM:        1212 B         2 KB     59.18%