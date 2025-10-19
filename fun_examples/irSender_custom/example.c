#include "../../fun_modules/fun_irSender.h"

#define IR_SENDER_PIN PD0

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	printf("\r\nIR Sender Test 1.\r\n");
	u8 mode = fun_irSender_init(IR_SENDER_PIN);
	printf("Mode: %s\r\n", mode ? "PWM" : "Manual GPIO");

	u32 time_ref = millis();

	IR_Sender_t irSender = {
		//! NEC protocol
		// .LOGICAL_1_US = 1600,
		// .LOGICAL_0_US = 560,

		//! NfS protocol
		.LOGICAL_1_US = 550,
		.LOGICAL_0_US = 300,
	};

	u8 data_out[] = { 0x00, 0xFF, 0xAA, 0x11, 0x22, 0x33, 0x44 };

	while(1) {
		if ((millis() - time_ref) > 2000) {
			// u64 combined = combine_64(address, command, extra, 0x00);
			// u16 crc = crc16_ccitt_compute(combined);

			// u64 dataout = 0;
			// u64 combined2 = combine_64(address, command, extra, crc);
			// u8 check = crc16_ccitt_check64(combined2, &dataout);

			// #if IR_SENDER_DEBUGLOG > 0
			// 	printf("check crc 0x%04X: %d\r\n", crc, check);
			// 	printf("Data: 0x%08X%08X\n", 
			// 		(uint32_t)(combined2 >> 32), 
			// 		(uint32_t)(combined2 & 0xFFFFFFFF));
			// #endif

			printf("Sendding message\n");
			fun_irSender_asyncSend(&irSender, data_out, sizeof(data_out));

			time_ref = millis();
		}

		fun_irSender_asyncTask(&irSender);
	}
}