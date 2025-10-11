#include "ch32fun.h"
#include <stdio.h>

typedef struct {
	u16 INITIAL_timer_count;		// initial count
	u16 LAST_timer_count;			// previous count
	u16 encoder_count;

	u32 debounce_timeRef;			// debounce ref
	u32 debounce_timeout_ms;
} Encoder_t;


//# Timer 2 pin mappings by AFIO->PCFR1
/*	00	AFIO_PCFR1_TIM2_REMAP_NOREMAP
		D4		T2CH1ETR
		D3		T2CH2
		C0		T2CH3
		D7		T2CH4  		//? requires disabling nRST in opt
	01	AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP1
		C5		T2CH1ETR_	//! SPI-SCK
		C2		T2CH2_		//! I2C-SDA
		D2		T2CH3_
		C1		T2CH4_		//! I2C-SCL
	10	AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP2
		C1		T2CH1ETR_	//! I2C-SCL
		D3		T2CH2		
		C0		T2CH3
		D7		T2CH4  		//? requires disabling nRST in opt
	11	AFIO_PCFR1_TIM2_REMAP_FULLREMAP
		C1		T2CH1ETR_	//! I2C-SCL
		C7		T2CH2_		//! SPI-MISO
		D6		T2CH3_		//! UART_TX
		D5		T2CH4_		//! UART_RX
*/

//! Expected funGpioInitAll() before init
void fun_encoder_tim2_init(Encoder_t *model) {
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	//! TIM2 remap mode
	AFIO->PCFR1 |= AFIO_PCFR1_TIM2_REMAP_NOREMAP;

	funPinMode(PD3, GPIO_CFGLR_IN_PUPD);
	funPinMode(PD4, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(PD3, 1);			// pull up
	funDigitalWrite(PD4, 1);			// pull up

	//! Reset TIM2 to init all regs
	RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;
	
	// set TIM2 clock prescaler If you want to reduce the resolution of the encoder
	// Prescaler: divide by 10 => 4.8MHz
	// TIM2->PSC = 0x0005;

	// set a automatic reload if you want the counter to wrap earlier than 0xffff
	//TIM2->ATRLR = 0xffff;

	// SMCFGR: set encoder mode SMS=011b
	TIM2->SMCFGR |= TIM_EncoderMode_TI12;

	// set count to about mid-scale to avoid wrap-around
	TIM2->CNT = 0x8fff;

	TIM2->SWEVGR |= TIM_UG;			// initialize timer
	TIM2->CTLR1 |= TIM_CEN;			// TIM2 Counter Enable

	model->INITIAL_timer_count = TIM2->CNT;
	model->LAST_timer_count = TIM2->CNT;
	model->encoder_count = 0;
	model->debounce_timeout_ms = 200;
};

void fun_encoder_tim2_task(u32 time, Encoder_t *model, void (*handler)(u8, int8_t)) {
	if (time - model->debounce_timeRef < model->debounce_timeout_ms) { return; }
	model->debounce_timeRef = time;

	u16 NEW_timer_count = TIM2->CNT;

	if (NEW_timer_count != model->LAST_timer_count) {
		u8 relative_pos = NEW_timer_count - model->INITIAL_timer_count;
		int8_t direction = (model->LAST_timer_count < NEW_timer_count) ? 1 : -1;
		// u8 delta		= NEW_timer_count - model->LAST_timer_count;
		model->encoder_count += direction;

		handler(model->encoder_count, direction);
		model->LAST_timer_count = NEW_timer_count;
	}
}