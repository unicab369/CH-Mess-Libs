//! OVERWRITE
// #define IR_LOGICAL_HIGH_THRESHOLD 170
// #define IR_OUTLINER_THRESHOLD 100

// #define IRRECEIVER_PULSE_THRESHOLD_US 460

#include "../../fun_modules/fun_irReceiver.h"

#define IR_RECEIVER_PIN PD2

#define IR_TEST_BUFF_LEN 240
u16 RReceiver_testbuff[IR_TEST_BUFF_LEN];
u16 IRReceiver_testIdx = 0;

void on_handle_irReceiver(u16 *data, u16 len) {
	if (len > 0) {
		for (int i = 0; i < len; i += 1) {
			RReceiver_testbuff[IRReceiver_testIdx] = data[i];
			IRReceiver_testIdx++;
		}
	} else {
		//# timeout

		if (IRReceiver_testIdx > 0) {
			printf("bufIdx: %d\n", IRReceiver_testIdx);
			
			for (u16 i = 0; i < IR_TEST_BUFF_LEN; i++) {
				printf("0x%04X  ", RReceiver_testbuff[i]);
				if (i % 8 == 7) printf("\n");
				RReceiver_testbuff[i] = 0;
			}
			printf("----------\r\n");
		}
		IRReceiver_testIdx = 0;
	}


	// printf("\nNEC: ");
	// for (u16 i = 0; i < len; i++) {
	// 	printf("0x%04X ", data[i]);
	// }
	// printf("\r\n");
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
