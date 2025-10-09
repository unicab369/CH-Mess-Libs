// MIT License
// Copyright (c) 2025 UniTheCat

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include "fun_base.h"
#include "fun_crc.h"


// #define IR_SENDER_DEBUGLOG 0

#define NEC_LOGIC_1_WIDTH_US 1600
#define NEC_LOGIC_0_WIDTH_US 560

#define IR_USE_TIM1_PWM


// Enable/disable PWM carrier
static inline void PWM_ON(void)  { TIM1->CCER |=  TIM_CC1NE; }
static inline void PWM_OFF(void) { TIM1->CCER &= ~TIM_CC1NE; }

//! ####################################
//! SETUP FUNCTION
//! ####################################

u8 irSender_pin;

u8 fun_irSender_init(u8 pin) {
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
void fun_irSender_sendNEC_blocking(u16* data, u8 len) {
	_IR_carrier_pulse(3000);
	Delay_Us(3000);

	// loop through the data
	for (int k = 0; k < len; k++) {
		// loop through the bits
		for (int i = 15; i >= 0; i--) {
			u8 bit = (data[k] >> i) & 1;		// MSB first
			u32 space = bit ? NEC_LOGIC_1_WIDTH_US : NEC_LOGIC_0_WIDTH_US;
			
			//! NEC data start with a pulse
			//!and then a space that represents the LOGICAL value
			_IR_carrier_pulse(NEC_LOGIC_0_WIDTH_US);
			Delay_Us(space);
		}
	}

	// terminating signals
	_IR_carrier_pulse(NEC_LOGIC_0_WIDTH_US);
}

//* SEND CUSTOM TEST (BLOCKING)
void fun_irSend_CustomTestData() {
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

			//! Custom data only use space as LOGICAL value
			//! the space can happen while the carrier is on or off
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


//! ####################################
//! ASYNC TRANSMIT FUNCTIONS
//! ####################################

#define IR_DATA_BITs_LEN 16

typedef enum {
	IR_Start_Pulse,
	IR_Start_Pulse_Burst,
	IR_Start_Pulse_Space,
	IR_Data_Pulse,
	IR_Idle
} IRSender_State_t;

typedef struct {
	int pin;
	int state;
	int output_state;
	u32 time_ref;
	u16 remaining_data_bits;
	u16 logical_spacing;

	u16 *BUFFER;
	s16 BUFFER_LEN;
	s16 buffer_idx;
} IR_Sender_t;


u32 sending_data = 0x00FFA56D;

void fun_irSender_asyncSend(IR_Sender_t *model) {
	//! start the pulses
	model->state = IR_Start_Pulse;
	model->time_ref = micros();
	model->buffer_idx = 0;
	printf("\nSending Data\r\n");
}

//* NEC TRANSMIT ASYNC. Only works with PWM
void fun_irSender_asyncTask(IR_Sender_t *model) {
	if (model->state == IR_Idle) return;
	u32 elapsed = micros() - model->time_ref;

	switch (model->state) {
		case IR_Data_Pulse:
			//# STEP 4: Handle spacing
			if (model->logical_spacing > 0) {
				if (elapsed > model->logical_spacing) {
					model->logical_spacing = 0;
					model->time_ref = micros();
				}
				break;
			}

			//# STEP 3: send the bit
			if (model->buffer_idx < model->BUFFER_LEN && 
				model->remaining_data_bits > 0
			) {
				// PWM_ON();
				// Delay_Us(NEC_LOGIC_0_WIDTH_US);
				// PWM_OFF();

				if (model->output_state == 0) {
					PWM_ON();
				} else {
					PWM_OFF();
				}
				model->output_state = !model->output_state;

				u16 value = model->BUFFER[model->buffer_idx];
				u8 bit = (value >> (model->remaining_data_bits-1)) & 1;

				// printf("Sending Value: 0x%04X, remaining: %d\n", value, model->remaining_data_bits);

				model->logical_spacing = bit ? NEC_LOGIC_1_WIDTH_US : NEC_LOGIC_0_WIDTH_US;
				model->remaining_data_bits--;
				model->time_ref = micros();		//! REQUIRES new micros() bc carrier pulse is blocking

				if (model->remaining_data_bits == 0) {
					model->remaining_data_bits = IR_DATA_BITs_LEN; // reload remaining data bits
					model->buffer_idx++;
				}
			}
			else {
				//# STEP 4: DONE - terminating signal
				// PWM_ON();
				// Delay_Us(NEC_LOGIC_0_WIDTH_US);
				// PWM_OFF();
				model->state = IR_Idle;
				model->output_state = 0;
			}
			break;

		case IR_Start_Pulse:
			//# STEP 1: Begin start_pulse
			PWM_ON();
			model->state = IR_Start_Pulse_Burst;
			model->time_ref = micros();
			break;
		
		case IR_Start_Pulse_Burst:
			// send the Start_Pulses - 9000us
			if (elapsed > 9000) {
				//# end start_pulse
				PWM_OFF();
				model->state = IR_Start_Pulse_Space;
				model->time_ref = micros();
			}
			break;

		case IR_Start_Pulse_Space:
			//# STEP 2: wait for start_pulse space - 4500us
			if (elapsed > 4500) {
				model->state = IR_Data_Pulse;
				model->time_ref = micros();
				
				model->remaining_data_bits = IR_DATA_BITs_LEN;	// reload remaining data bits
				model->logical_spacing = 0;
				model->buffer_idx = 0;
				model->output_state = 0;
			}
			break;

	}
}
