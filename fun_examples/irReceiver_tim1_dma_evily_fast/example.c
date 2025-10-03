//! OVERWRITE
// #define IR_LOGICAL_HIGH_THRESHOLD 170
// #define IR_OUTLINER_THRESHOLD 100

// #define IRRECEIVER_PULSE_THRESHOLD_US 460

#include "../../fun_modules/fun_irReceiver.h"

#define IR_RECEIVER_PIN PD2

void on_handle_irReceiver(u16 *data, u16 len) {
	// for (u16 i = 0; i < len; i++) {
	// 	ir_data[len_count++] = data[i];
	// }

	// if (len_count > 30) {
	// 	len_count = 0;
	// 	printf("\nNEC: ");
	// 	for (u16 i = 0; i < 100; i++) {
	// 		if (ir_data[i] == 0) {
	// 			break;
	// 		}
	// 		printf("0x%04X \n", ir_data[i]);
	// 	}
	// 	printf("\r\n");
	// 	len_count = 0;
	// }

	printf("\nNEC: ");
	for (u16 i = 0; i < len; i++) {
		printf("0x%04X ", data[i]);
	}
	printf("\r\n");
}

int main() {
	SystemInit();
	Delay_Ms(100);
	systick_init();			//! REQUIRED for millis()

	printf("\r\nIR Receiver Test.\r\n");
	funGpioInitAll();

	IRReceiver_t receiver = {
		.pin = IR_RECEIVER_PIN,
	};

	fun_irReceiver_init(&receiver);

	while(1) {
		fun_irReceiver_task(&receiver, on_handle_irReceiver);
	}
}
