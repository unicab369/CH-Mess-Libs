#include "../../fun_modules/fun_base.h"
// #include "../../fun_modules/fun_encoder_tim2.h"
#include "../../fun_modules/fun_encoder_gpio.h"
#include "../../fun_modules/fun_button.h"
// #include "../../fun_modules/fun_irSender.h"
// #include "../../fun_modules/fun_irReceiver.h"
// #include "../../fun_modules/fun_ws2812_gpio.h"

// #include "i2c_manager.h"

#define BUTTON_PIN PC0
#define IR_SENDER_PIN PD0
#define IR_RECEIVER_PIN PA1

//# Encoder Callback
void onHandle_Encoder(int8_t position, int8_t direction) {
	i2c_menu_move(direction);
}

//# Button Callback
void onHandle_Button(Button_Event_e event, u32 time) {
	switch (event) {
		case BTN_SINGLECLICK:
			printf("Single Click\n");
			// i2c_menu_handle();
			break;
		case BTN_DOUBLECLICK:
			printf("Double Click\n");
			break;
		case BTN_LONGPRESS:
			printf("Long Press\n"); break;
	}

	// mngI2c_load_buttonState(millis(), event);
}


#define IR_RECEIVER_LINE_LOG 255
#define MAX_WORDS_LEN 200
u16 WORD_BUFFER[MAX_WORDS_LEN] = {0};

//# IR Receiver Callback
void onHandle_irReceiver(u16 *words, u16 len) {
	printf("\n\nReceived Buffer Len %d\n", len);
	s16 limit = len > MAX_WORDS_LEN ? MAX_WORDS_LEN : len;
	s16 line_offset = (limit / 8) - IR_RECEIVER_LINE_LOG;
	u8 start = line_offset > 0 ? line_offset * 8 : 0;

	if (start > 0) printf("... Skipping %d lines ...\n", line_offset);

	for (int i = start; i < limit; i++) {
		printf("0x%04X  ", words[i]);
		if (i%8 == 7) printf("\n");			// separator
	}
}

//! ####################################
//! MAIN FUNCTION
//! ####################################

void _IR_carrier_pulse(u32 duration_us) {
	// the pulse has duration the same as NEC_LOGIC_0_WIDTH_US
	//# Start CH1N output
	PWM_ON();
	Delay_Us(duration_us);

	//# Stop CH1N output
	PWM_OFF();
}

#define NfS_LOGIC_1_WIDTH_US 550
#define NfS_LOGIC_0_WIDTH_US 300

void _irSend_CustomTestData() {
	static u8 state = 0;
	_IR_carrier_pulse(3000);
	Delay_Us(3000);

	u16 data = 0x0000;

	// loop through the data
	for (int i = 0; i < 150; i++) {
		// loop through the bits
		for (int i = 15; i >= 0; i--)  {
			u8 bit = (data >> i) & 1;        // MSB first
			u32 space = bit ? NfS_LOGIC_1_WIDTH_US : NfS_LOGIC_0_WIDTH_US;

			//! Custom protocol does not need a signal pulse
			//! it uses any of the GPIO states spacing LOGICAL value
			if (state == 0) {
				_IR_carrier_pulse(space);
			} else {
				Delay_Us(space);
			}
			
			state = !state;
		}

		data++;
	}

	// terminating signals
	_IR_carrier_pulse(NfS_LOGIC_0_WIDTH_US);
}


int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	//# Button: uses PC0
	static Button_t button1 = { .pin = BUTTON_PIN };
	fun_button_setup(&button1);

	//# WS28126: uses PD5
	// fun_gpio_ws2812_init(PD5);

	//# TIM2: uses PD4(CH1) and PD3(CH2)
	// Encoder_t encoder_a;
	// fun_encoder_tim2_init(&encoder_a);

	Encoder_GPIO_t encoder_b = {
		.pinA = PD4,
		.pinB = PD3,
	};
	fun_encoder_gpio_init(&encoder_b);
	// i2c_menu_init();

	// //# IR Sender
	// IR_Sender_t irSender = {
	// 	// .IR_MODE = 0			// NEC protocol
	// 	.IR_MODE = 1			// NfS1 protocol
	// };

	// u8 mode = fun_irSender_init(IR_SENDER_PIN);
	// static u16 data_out[] = { 0x0000, 0xFFFF, 0xAAAA, 0x1111, 0x2222, 0x3333, 0x4444 };
	// irSender.BUFFER = data_out;
	// irSender.BUFFER_LEN = 7;

	// //# IR Receiver
	// IR_Receiver_t receiver = {
	// 	.pin = IR_RECEIVER_PIN,
	// 	.WORD_BUFFER_LEN = MAX_WORDS_LEN,
	// 	.WORD_BUFFER = WORD_BUFFER,
	// 	// .IR_MODE = 0				// NEC protocol
	// 	.IR_MODE = 1				// NfS protocol
	// };

	// fun_irReceiver_init(&receiver);

	//# start loop
	u32 time_ref = millis();
	u32 counter = 0;

	while(1) {
		u32 moment = millis();
		
		fun_button_task(moment, &button1, onHandle_Button);

		// if ((moment - time_ref) > 1000) {
		// 	sprintf(str_output, "Hello %ld", counter++);

		// 	u32 time_ref2 = micros();
		// 	// ssd1306_draw_str(str_output, 0, 0);
		// 	// printf("elapsed: %d us\n", micros() - time_ref2);
		// 	// ssd1306_draw_scaled_text(0, 0, str_output, &DEAULF_STR_CONFIG);

		// 	// test_lines();
		// 	u8 check = i2c_menu_period_tick();
			
		// 	if (check) {
		// 		switch (menu_selectedIdx) {
		// 			case 2: {
		// 				printf("Sending Ir message\n");
		// 				// fun_irSender_asyncSend(&irSender);
		// 				// _irSend_CustomTestData();
		// 				break;
		// 			}
		// 		}
		// 	}

		// 	time_ref = millis();
		// }

		// if (!is_main_menu) {
		// 	switch (menu_selectedIdx) {
		// 		case 2:
		// 			// fun_irSender_asyncTask(&irSender);
		// 			break;
		// 		case 3:
		// 			// fun_irReceiver_task(&receiver, onHandle_irReceiver);
		// 			break;
		// 		default: break;
		// 	}
		// }

		// // fun_encoder_tim2_task(moment, &encoder_a, onHandle_Encoder);
		// fun_encoder_gpio_task(moment, &encoder_b, onHandle_Encoder);
	}
}

        //    FLASH:        5692 B        16 KB     34.74%
        //      RAM:        1212 B         2 KB     59.18%