// sound and play sound method was borrowed and modified from:
// https://github.com/atomic14/ch32v003-music/tree/main/SimpleSoundFirmware/src

#include "../../fun_modules/fun_base.h"

#define BUZZER_PIN PD0
#define BUZZER_USE_TIM1_PWM

const uint16_t sound_buf[] = {
    23,   499,  476,  522,  476,  544,  476,  544,  476,  544,  499,  544,
    499,  544,  499,  544,  499,  567,  499,  567,  499,  567,  522,  567,
    522,  567,  522,  590,  522,  590,  522,  590,  522,  612,  522,  612,
    544,  590,  544,  635,  544,  612,  544,  635,  567,  612,  567,  635,
    590,  635,  567,  658,  590,  658,  567,  680,  590,  658,  612,  680,
    590,  703,  612,  680,  635,  703,  612,  703,  635,  726,  635,  748,
    635,  748,  658,  748,  658,  748,  680,  771,  680,  794,  680,  794,
    703,  816,  703,  816,  726,  839,  748,  839,  748,  862,  748,  884,
    771,  884,  794,  907,  816,  930,  816,  952,  839,  952,  862,  998,
    884,  998,  907,  1043, 930,  1043, 952,  1088, 998,  1111, 1020, 1156,
    1043, 1179, 1088, 1224, 1134, 1270, 1179, 1338, 1202, 1406, 1270, 1474,
    1338, 1565, 1406, 1655, 1497, 1769, 1633, 1927, 1769, 2109, 1973, 2381,
    2268
};


static inline void SET_OUTPUT(u8 ON) {
	#ifdef BUZZER_USE_TIM1_PWM
		if (ON) {
			TIM1->CCER |=  TIM_CC1NE;
		} else {
			TIM1->CCER &= ~TIM_CC1NE;
		}
	#else
		funDigitalWrite(BUZZER_PIN, ON);
	#endif
}

void gpio_play_sound(u8 pin, u16 *data, u8 len) {
	uint8_t state = 0;

	for (uint16_t i = 0; i < len; i++) {
		uint16_t delay = data[i];
		SET_OUTPUT(state);
		state = !state;
		Delay_Us(delay);
	}

	// Ensure GPIO is low after sound
	SET_OUTPUT(0);	
}

void _buzzer_init(u8 pin) {
	#ifdef BUZZER_USE_TIM1_PWM
		funPinMode(pin, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF);

		// Enable TIM1
		RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
			
		// Reset TIM1 to init all regs
		RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
		RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;
		
		u16 period = 1000;			// PWM period: 48Mhz/38kHz = 1263

		// PWM settings
		TIM1->PSC = 0x0000;			// Prescaler 
		TIM1->ATRLR = period;		// PWM period
		// TIM1->CH1CVR = period/10;	// PWM duty cycle: 10%
		TIM1->SWEVGR |= TIM_UG;		// Reload immediately

		// CH1 Mode is output, PWM1 (CC1S = 00, OC1M = 110)
		TIM1->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1;
		
		TIM1->BDTR |= TIM_MOE;		// Enable TIM1 outputs
		TIM1->CTLR1 |= TIM_CEN;		// Enable TIM1

		TIM1->CCER |= TIM_CC1NP;
	#else
		funPinMode(pin, GPIO_CFGLR_OUT_50Mhz_PP);
	#endif
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	_buzzer_init(BUZZER_PIN);
	u32 time_ref = millis();

	while(1) {
		u32 moment = millis();

		if (moment - time_ref > 3000) {
			time_ref = moment;

			gpio_play_sound(BUZZER_PIN, sound_buf, sizeof(sound_buf)/sizeof(u16));
			printf("IM HERE\r\n");
		}
	}
}