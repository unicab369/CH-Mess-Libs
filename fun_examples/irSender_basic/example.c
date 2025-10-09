#include "../../fun_modules/fun_irSender.h"

#define IR_SENDER_PIN PD0

#define IR_TEST_CUSTOM_PROTOCOL

#define NEC_LOGIC_1_WIDTH_US 1600
#define NEC_LOGIC_0_WIDTH_US 560

//! ####################################
//! TRANSMIT FUNCTIONS
//! ####################################

// carrier frequency = 38kHz
// period = 1/38000 = 26.5µs
// half period = 26.5µs / 2 = ~13µs
#define IR_CARRIER_HALF_PERIOD_US 13

//* CARRIER PULSE (BLOCKING)
void _IR_carrier_pulse(u32 duration_us) {
	// the pulse has duration the same as NEC_LOGIC_0_WIDTH_US
	#ifdef IR_USE_TIM1_PWM
		//# Start CH1N output
		PWM_ON();
		Delay_Us(duration_us);

		//# Stop CH1N output
		PWM_OFF();
	#else
		u16 carrier_cycles = duration_us / (IR_CARRIER_HALF_PERIOD_US * 2);

		for(u32 i = 0; i < carrier_cycles; i++) {
			funDigitalWrite(irSender_pin, 1);  	// Set high
			Delay_Us( IR_CARRIER_HALF_PERIOD_US );
			funDigitalWrite(irSender_pin, 0);   // Set low
			Delay_Us( IR_CARRIER_HALF_PERIOD_US );
		}

		// Ensure pin is low during space
		funDigitalWrite(irSender_pin, 0);
	#endif
}

//* SEND NEC (BLOCKING)
void _irSender_sendNEC_blocking(u16* data, u8 len) {
	_IR_carrier_pulse(3000);
	Delay_Us(3000);

	// loop through the data
	for (int k = 0; k < len; k++) {
		// loop through the bits
		for (int i = 15; i >= 0; i--) {
			u8 bit = (data[k] >> i) & 1;		// MSB first
			u32 space = bit ? NEC_LOGIC_1_WIDTH_US : NEC_LOGIC_0_WIDTH_US;
			
			//! NEC protocol start with a pulse
			//! and use the PWM's OFF state spacing for LOGICAL value
			_IR_carrier_pulse(NEC_LOGIC_0_WIDTH_US);
			Delay_Us(space);
		}
	}

	// terminating signals
	_IR_carrier_pulse(NEC_LOGIC_0_WIDTH_US);
}

//* SEND CUSTOM TEST (BLOCKING)
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
			u32 space = bit ? NEC_LOGIC_1_WIDTH_US : NEC_LOGIC_0_WIDTH_US;

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
	_IR_carrier_pulse(NEC_LOGIC_0_WIDTH_US);
}


int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	printf("\r\nIR Sender Basic Test\r\n");
	u8 mode = fun_irSender_init(IR_SENDER_PIN);
	printf("Mode: %s\r\n", mode ? "PWM" : "Manual GPIO");

	IR_Sender_t irSender = {0};
	u32 time_ref = millis();

	while(1) {
		if ((millis() - time_ref) > 3000) {
			u32 ref = millis();

			static u16 data_out[] = { 0x0000, 0xFFFF, 0xAAAA, 0x1111 };

			#ifdef IR_TEST_CUSTOM_PROTOCOL
				_irSend_CustomTestData();
			#else
				_irSender_sendNEC_blocking(data_out, 4);
			#endif

			printf("messages in %d ms\r\n", millis() - ref);
			time_ref = millis();
		}
	}
}