//! OVERWRITE
#define NEC_LOGIC_1_WIDTH_US 550	//! MINIMUM 500us to prevent overlap
#define NEC_LOGIC_0_WIDTH_US 300	//! MINIMUM 250us to properly modulate

#include "../../fun_modules/fun_irSender.h"

#define IR_SENDER_PIN PD0

void _irSender_sendMessages() {
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

	u32 ref = millis();

	_IR_carrier_pulse(2000, 2000);

	u16 data = 0x0000;

	for (int i = 0; i < 150; i++) {
		fun_irSend_CustomData(data);
		data++;
		// Delay_Us(5);
	}

	// terminating signals
	PWM_ON(); Delay_Us(1000); PWM_OFF();

	printf("messages in %d ms\r\n", millis() - ref);
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	

	printf("\r\nIR Sender Test.\r\n");
	u8 mode = fun_irSender_init(IR_SENDER_PIN);
	printf("Mode: %s\r\n", mode ? "PWM" : "Manual GPIO");

	u32 time_ref = millis();

	while(1) {
		if ((millis() - time_ref) > 4000) {
			_irSender_sendMessages();
			// fun_irSender_sendAsync(0x00FF, 0xA56D);
			// fun_irSender_send(0x00F1, 0xA56D, 0x2222);
			time_ref = millis();
		}

		// fun_irSender_task();
	}
}