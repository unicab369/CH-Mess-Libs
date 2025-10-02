#include "../../fun_modules/fun_irReceiver.h"

#define IR_RECEIVER_PIN PD2

void on_handle_irReceiver(u16 address, u16 command) {
	printf("NEC: 0x%04X 0x%04X\r\n", address, command);
	printf("------------------------\r\n");
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
