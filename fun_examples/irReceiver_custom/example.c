#include "../../fun_modules/fun_irReceiver.h"

#define IR_RECEIVER_PIN PA1

// how many lines of log to show when printing long data
#define IR_RECEIVER_LINE_LOG 255

#define IR_RECEIVE_MAX_LEN 100
u8 RECEIVE_BUF[IR_RECEIVE_MAX_LEN] = {0};

void onHandle_irData(u8 *data, u16 len) {
	printf("\n\nReceived Buffer Len %d\n", len);
	s16 limit = len > IR_RECEIVE_MAX_LEN ? IR_RECEIVE_MAX_LEN : len;
	s16 line_offset = (limit / 8) - IR_RECEIVER_LINE_LOG;
	u8 start = line_offset > 0 ? line_offset * 8 : 0;

	if (start > 0) printf("... Skipping %d lines ...\n", line_offset);

	for (int i = start; i < limit; i++) {
		printf("0x%02X  ", data[i]);
		if (i%8 == 7) printf("\n");			// separator
	}
}

void onHandle_irString(const char* str) {
	printf("\n\nReceived String: %s\n", str);
	printf("Length: %d\n", strlen(str));
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	printf("\r\nIR Receiver Test.\r\n");

	IR_Receiver_t receiver = {
		.pin = IR_RECEIVER_PIN,
		.RECEIVE_MAX_LEN = IR_RECEIVE_MAX_LEN,
		.RECEIVE_BUF = RECEIVE_BUF,
		//! Default to NEC protocol
		// .IR_LOGICAL_1_US = 0,

		//! NfS protocol
		.LOGICAL_1_US = 550,
		.LOGICAL_0_US = 300,
		.START_SIGNAL_THRESHOLD_US = 1000,
		.onHandle_data = NULL,
		.onHandle_string = onHandle_irString
	};

	fun_irReceiver_init(&receiver);

	while(1) {
		fun_irReceiver_task(&receiver);
	}
}
