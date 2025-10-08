#include "../../fun_modules/fun_irReceiverNEC.h"

#define IR_RECEIVER_PIN PD2

void onHandle_irReceiver_NEC(u16* words, u8 len) {
	printf("\nreceived NEC data\n");

	for (int i = 0; i < len; i++) {
		printf("0x%04X  ", words[i]);
	}
	printf("\n");
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	IR_ReceiverNEC_t receiver = {
		.pin = IR_RECEIVER_PIN,
	};

	fun_irReceiver_NEC_init(&receiver);
	
	u32 time_ref = millis();

	while(1) {
		fun_irReceiver_NEC_task(&receiver, onHandle_irReceiver_NEC);
	}
}