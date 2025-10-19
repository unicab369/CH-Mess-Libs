#include "../../fun_modules/fun_base.h"

// #define IR_USE_NEC_PROTOCOL
#define IR_USE_TIM1_PWM

#define IR_SENDER_PIN PD0

// Enable/disable PWM carrier
static inline void PWM_ON(void)  { TIM1->CCER |=  TIM_CC1NE; }
static inline void PWM_OFF(void) { TIM1->CCER &= ~TIM_CC1NE; }

//! ####################################
//! SETUP FUNCTION
//! ####################################

u8 irSender_pin;

_irSender_init(u8 pin) {
	irSender_pin = pin;

	#ifdef IR_USE_TIM1_PWM
		funPinMode(irSender_pin, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF);

		// Enable TIM1
		RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
			
		// Reset TIM1 to init all regs
		RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
		RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;
		
		// PWM settings
		TIM1->PSC = 0x0000;			// Prescaler 
		TIM1->ATRLR = 1263;			// PWM period: 48Mhz/38kHz = 1263
		TIM1->CH1CVR = 632;			// PWM duty cycle: 50% = 1263/2
		TIM1->SWEVGR |= TIM_UG;		// Reload immediately

		// CH1 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
		TIM1->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1;
		
		TIM1->BDTR |= TIM_MOE;		// Enable TIM1 outputs
		TIM1->CTLR1 |= TIM_CEN;		// Enable TIM1

		TIM1->CCER |= TIM_CC1NP;
		return 1;
	#else
		funPinMode(irSender_pin, GPIO_CFGLR_OUT_50Mhz_PP);
		return 0;
	#endif
}

//! ####################################
//! TRANSMIT FUNCTIONS
//! ####################################

#define NEC_LOGIC_1_WIDTH_US 1600
#define NEC_LOGIC_0_WIDTH_US 560

#define NfS_LOGIC_1_WIDTH_US 550
#define NfS_LOGIC_0_WIDTH_US 300

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
void _irSender_sendNEC_blocking(u8* data, u8 len) {
	_IR_carrier_pulse(3000);
	Delay_Us(3000);

	// loop through the data
	for (int k = 0; k < len; k++) {
		// loop through the bits
		for (int i = 7; i >= 0; i--) {
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


void _send_Custom_Byte(u8 data) {
	static u8 state = 0;

	// loop through the bits
	for (int i = 7; i >= 0; i--)  {
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
}

//* SEND CUSTOM TEST (BLOCKING)
void _irSend_Custom_TestData() {
	_IR_carrier_pulse(3000);
	Delay_Us(3000);

	u8 data = 0x00;

	// loop through the data
	for (int i = 0; i < 300; i++) {
		_send_Custom_Byte(data);
		data++;
	}

	// terminating signals
	_IR_carrier_pulse(NfS_LOGIC_0_WIDTH_US);
}

void _irSend_Custom_TestString() {
	_IR_carrier_pulse(3000);
	Delay_Us(3000);

	u8 data[] = "ST HELLO WOLRD 12345678901234567890123456890";

	for (int i = 0; i < sizeof(data); i++) {
		_send_Custom_Byte(data[i]);
	}

	// terminating signals
	_IR_carrier_pulse(NfS_LOGIC_0_WIDTH_US);
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	printf("\r\nIR Sender Basic Test\r\n");
	u8 mode = _irSender_init(IR_SENDER_PIN);
	printf("Mode: %s\r\n", mode ? "PWM" : "Manual GPIO");

	u32 moment = millis();

	while(1) {
		if ((millis() - moment) > 3000) {
			u32 ref = millis();

			static u8 data_out[] = { 0x00, 0xFF, 0xAA, 0x11 };

			#ifdef IR_USE_NEC_PROTOCOL
				_irSender_sendNEC_blocking(data_out, 4);	
			#else
				// _irSend_Custom_TestData();
				_irSend_Custom_TestString();
			#endif

			printf("messages in %d ms\r\n", millis() - ref);
			moment = millis();
		}
	}
}