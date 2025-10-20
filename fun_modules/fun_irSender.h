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

#define IR_DATA_BITs_LEN 8
#define NEC_LOGICAL_1_US 1600
#define NEC_LOGICAL_0_US 560
#define NEC_START_HI_US 9000
#define NEC_START_LO_US 4500

typedef enum {
	IR_Start_Pulse,
	IR_Start_Pulse_Burst,
	IR_Start_Pulse_Space,
	IR_Data_Pulse,
	IR_Idle
} IRSender_State_t;

typedef struct {
	int pin;					// gpio pin
	int state;					// current state
	int logic_output;			// logic output for custom protocol
	u32 time_ref;
	u16 remaining_data_bits;	// number of bits (of a word) to send
	u16 logical_spacing;		// spacing that represents logic output

	u8 *BUFFER;					// words buffer
	s16 BUFFER_LEN;				// words buffer length
	s16 buffer_idx;				// current buffer index

	u16 LOGICAL_1_US;			// Logical_1 spacing in microseconds
	u16 LOGICAL_0_US;			// Logical_0 spacing in microseconds
	u16 START_HI_US;			// Start_HI spacing in microseconds
	u16 START_LO_US;			// Start_LO spacing in microseconds
} IR_Sender_t;

// Enable/disable PWM carrier
static inline void PWM_ON(void)  { TIM1->CCER |=  TIM_CC1NE; }
static inline void PWM_OFF(void) { TIM1->CCER &= ~TIM_CC1NE; }

//! ####################################
//! SETUP FUNCTION
//! ####################################

u8 fun_irSender_init(IR_Sender_t *model) {
	funPinMode(model->pin, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF);

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
}

//! ####################################
//! ASYNC TRANSMIT FUNCTIONS
//! ####################################

void fun_irSender_asyncSend(IR_Sender_t *model, u8 *data, u16 len) {
	//! start the pulses
	model->state = IR_Start_Pulse;
	model->time_ref = micros();
	model->buffer_idx = 0;
	model->BUFFER = data;
	model->BUFFER_LEN = len;

	if (model->LOGICAL_1_US < 250 || model->LOGICAL_0_US < 250 ||
		model->START_HI_US < 2000 || model->START_LO_US < 2000
	) {
		model->LOGICAL_1_US = NEC_LOGICAL_1_US;
		model->LOGICAL_0_US = NEC_LOGICAL_0_US;
		model->START_HI_US = NEC_START_HI_US;
		model->START_LO_US = NEC_START_LO_US;
	}
}

//* NEC TRANSMIT ASYNC. 
//! NOTE: Only works with PWM
void fun_irSender_asyncTask(IR_Sender_t *model) {
	if (model->state == IR_Idle) return;
	u32 elapsed = micros() - model->time_ref;

	switch (model->state) {
		case IR_Data_Pulse:
			// //# STEP 4: Handle spacing - iffy because of timing
			// if (model->logical_spacing > 0) {
			// 	if (elapsed > model->logical_spacing) {
			// 		model->logical_spacing = 0;
			// 		model->time_ref = micros();
			// 	}
			// 	break;
			// }

			//# STEP 3: send the bit
			if (model->buffer_idx < model->BUFFER_LEN && 
				model->remaining_data_bits > 0
			) {
				u8 value = model->BUFFER[model->buffer_idx];
				// printf("%02X ", value);

				u8 bit = (value >> (model->remaining_data_bits-1)) & 1;
				// printf("Sending Value: 0x%04X, remaining: %d\n", value, model->remaining_data_bits);
				model->remaining_data_bits--;
				model->logical_spacing = bit ? model->LOGICAL_1_US : model->LOGICAL_0_US;

				//! NEC protocol uses the PWM's OFF state spacing for LOGICAL value
				if (model->LOGICAL_1_US == NEC_LOGICAL_1_US) {
					PWM_ON();
					Delay_Us(model->LOGICAL_0_US);
					PWM_OFF();
				}
				//! Custom protocol uses any of the PWM states for LOGICAL value
				else {
					if (model->logic_output == 0) {
						PWM_ON();
					} else {
						PWM_OFF();
					}
					model->logic_output = !model->logic_output;
				}
			
				//! ORDER DOES MATTER. requires new micros() bc carrier pulse is blocking
				model->time_ref = micros();
				// Bypass for STEP 4 - use for debugging only
				Delay_Us(model->logical_spacing);

				if (model->remaining_data_bits == 0) {
					model->remaining_data_bits = IR_DATA_BITs_LEN; // reload remaining data bits
					model->buffer_idx++;
				}
			}
			else {
				//# STEP 4: DONE - terminating signal
				PWM_ON();
				Delay_Us(model->LOGICAL_0_US);
				PWM_OFF();
				model->state = IR_Idle;
			}
			break;

		case IR_Start_Pulse:
			//# STEP 1: Begin start_pulse
			PWM_ON();
			model->state = IR_Start_Pulse_Burst;
			model->time_ref = micros();
			break;
		
		case IR_Start_Pulse_Burst:
			//# end the Start_Pulses - 9000us (for NEC)
			if (elapsed > model->START_HI_US) {
				PWM_OFF();
				model->state = IR_Start_Pulse_Space;
				model->time_ref = micros();
			}
			break;

		case IR_Start_Pulse_Space:
			//# STEP 2: wait for start_pulse space - 4500us (for NEC)
			if (elapsed > model->START_LO_US) {
				model->state = IR_Data_Pulse;
				model->time_ref = micros();
				
				model->remaining_data_bits = IR_DATA_BITs_LEN;	// reload remaining data bits
				model->logical_spacing = 0;
				model->buffer_idx = 0;
				model->logic_output = 0;
			}
			break;
	}
}


//! ####################################
//! BLOCKING TRANSMIT FUNCTIONS
//! ####################################

#define IR_USE_TIM1_PWM

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

void _IR_send_custom_Byte(IR_Sender_t *model, u8 data) {
	static u8 state = 0;

	// loop through the bits
	for (int i = 7; i >= 0; i--)  {
		u8 bit = (data >> i) & 1;        // MSB first
		u32 space = bit ? model->LOGICAL_1_US : model->LOGICAL_0_US;

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

void fun_irSender_blockingTask(IR_Sender_t *model, u8 *data, u16 len) {
	_IR_carrier_pulse(1600);
	Delay_Us(1600);

	for (int i = 0; i < len; i++) {
		_IR_send_custom_Byte(model, data[i]);
	}

	// terminating signals
	_IR_carrier_pulse(model->LOGICAL_0_US);
}