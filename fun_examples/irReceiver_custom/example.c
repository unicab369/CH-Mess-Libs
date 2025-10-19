#include "../../fun_modules/fun_irReceiver.h"

#define IR_RECEIVER_PIN PA1

// how many lines of log to show when printing long data
#define IR_RECEIVER_LINE_LOG 255

#define RECEIVE_BUFFER_LEN 200
u8 RECEIVE_BUF[RECEIVE_BUFFER_LEN] = {0};

void onHandle_irReceiver(u8 *data, u16 len) {
	printf("\n\nReceived Buffer Len %d\n", len);
	s16 limit = len > RECEIVE_BUFFER_LEN ? RECEIVE_BUFFER_LEN : len;
	s16 line_offset = (limit / 8) - IR_RECEIVER_LINE_LOG;
	u8 start = line_offset > 0 ? line_offset * 8 : 0;

	if (start > 0) printf("... Skipping %d lines ...\n", line_offset);

	for (int i = start; i < limit; i++) {
		printf("0x%02X  ", data[i]);
		if (i%8 == 7) printf("\n");			// separator
	}
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	printf("\r\nIR Receiver Test.\r\n");

	IR_Receiver_t receiver = {
		.pin = IR_RECEIVER_PIN,
		.RECEIVE_BUF_LEN = RECEIVE_BUFFER_LEN,
		.RECEIVE_BUF = RECEIVE_BUF,
		// .IR_MODE = 0				// NEC protocol
		.IR_MODE = 1				// NfS protocol
	};

	fun_irReceiver_init(&receiver);

	while(1) {
		fun_irReceiver_task(&receiver, onHandle_irReceiver);
	}
}
