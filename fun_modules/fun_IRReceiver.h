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

// #define IR_RECEIVER_DEBUGLOG_ENABLE

// #define IR_RECEIVER_USE_TIM1

//! For generating the mask/modulus, this must be a power of 2 size.
#define IRREMOTE_MAX_PULSES 128
#define TIM1_BUFFER_LAST_IDX (IRREMOTE_MAX_PULSES - 1)
#define IRRECEIVER_BUFFER_SIZE 4

typedef enum {
	IR_Started,
	IR_Idle,
	IR_Parsing
} IRReceiver_State_t;

typedef struct {
	int pin;
	IRReceiver_State_t state;
	u16 ir_data[IRRECEIVER_BUFFER_SIZE];	// store the received data
	
	int prev_pinState;			// for gpio
	int current_pinState;		// for gpio

	int16_t counter;			// store pulses count (for gpio) Or tails (for DMA)
	u8 bits_processed;			// number of bits processed
	u32 timeRef, timeout_duration_us;
	u32 pulse_buf[IRREMOTE_MAX_PULSES];
} IRReceiver_t;


#ifdef IR_RECEIVER_USE_TIM1
	// TIM2_CH1 -> PD2 -> DMA1_CH2
	#define IR_DMA_IN	DMA1_Channel2

	//! ####################################
	//! SETUP FUNCTION
	//! ####################################

	//! For generating the mask/modulus, this must be a power of 2 size.
	uint16_t ir_ticks_buff[IRREMOTE_MAX_PULSES];

	void fun_irReceiver_init(IRReceiver_t* model) {
		model->timeout_duration_us = 100000; 	// 100ms
		model->state = IR_Idle;
		model->counter = 0;

		funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
		funDigitalWrite(model->pin, 1);

		// Enable GPIOs
		RCC->APB2PCENR |= RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO;
		RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

		//# TIM1_TRIG/TIM1_COM/TIM1_CH4
		IR_DMA_IN->MADDR = (uint32_t)ir_ticks_buff;
		IR_DMA_IN->PADDR = (uint32_t)&TIM1->CH1CVR; // Input
		IR_DMA_IN->CFGR = 
			0					|				// PERIPHERAL to MEMORY
			0					|				// Low priority.
			DMA_CFGR1_MSIZE_0	|				// 16-bit memory
			DMA_CFGR1_PSIZE_0	|				// 16-bit peripheral
			DMA_CFGR1_MINC		|				// Increase memory.
			DMA_CFGR1_CIRC		|				// Circular mode.
			0					|				// NO Half-trigger
			0					|				// NO Whole-trigger
			DMA_CFGR1_EN;						// Enable
		IR_DMA_IN->CNTR = IRREMOTE_MAX_PULSES;

		TIM1->PSC = 0x00ff;		// set TIM1 clock prescaler divider (Massive prescaler)
		TIM1->ATRLR = 65535;	// set PWM total cycle width

		//# Tim 1 input / capture (CC1S = 01)
		TIM1->CHCTLR1 = TIM_CC1S_0;

		//# Add here CC1P to switch from UP->GOING to DOWN->GOING log times.
		TIM1->CCER = TIM_CC1E | TIM_CC1P;
		
		//# initialize counter
		TIM1->SWEVGR = TIM_UG;

		//# Enable TIM1
		TIM1->CTLR1 = TIM_ARPE | TIM_CEN;
		TIM1->DMAINTENR = TIM_CC1DE | TIM_UDE; // Enable DMA for T1CC1
	}


	//! ####################################
	//! RECEIVE FUNCTION USING DMA
	//! ####################################

	#define IR_LOGICAL_HIGH_THRESHOLD 300

	void fun_irReceiver_task(IRReceiver_t* model, void(*handler)(u16, u16)) {
			// Must perform modulus here, in case DMA_IN->CNTR == 0.
		int head = (IRREMOTE_MAX_PULSES - IR_DMA_IN->CNTR) & TIM1_BUFFER_LAST_IDX;
		u32 now = micros();

		while( head != model->counter ) {
			u32 time_of_event = ir_ticks_buff[model->counter];
			u8 prev_event_idx = (model->counter-1) & TIM1_BUFFER_LAST_IDX;		// modulus to loop back
			u32 prev_time_of_event = ir_ticks_buff[prev_event_idx];
			u32 elapsed = time_of_event - prev_time_of_event;

			// Performs modulus to loop back.
			model->counter = (model->counter+1) & TIM1_BUFFER_LAST_IDX;

			//! decode here
			if (model->state == IR_Started) {
				//# filter 2nd start frame (> 1000 ticks)
				if (elapsed > 1000) continue;
				int bit_pos = 15 - (model->bits_processed & 0x0F);  // bits_processed % 16
				int word_idx = model->bits_processed >> 4;          // bits_processed / 16
				model->bits_processed++;

				//# filter for Logical HIGH
				// logical HIGH (~200 ticks), logical LOW (~100 ticks)
				if (elapsed > IR_LOGICAL_HIGH_THRESHOLD) {
					model->ir_data[word_idx] |= (1 << bit_pos);		// MSB first (reversed)
				}

				#ifdef IR_RECEIVER_DEBUGLOG_ENABLE
					printf("%d (%d) - 0x%04X \t [%d]%ld, [%d]%ld D%d\n",
						elapsed, elapsed > IR_LOGICAL_HIGH_THRESHOLD,
						model->ir_data[word_idx],
						model->counter, time_of_event, prev_event_idx, prev_time_of_event, bit_pos);
					if (bit_pos % 8 == 0) printf("\n");
				#endif
			}

			//# 1st start frame. Expect > 1200 ticks
			else if (elapsed > 500) {
				model->state = IR_Started;
				model->timeRef = now;
			}
		}

		// handle timeout
		if ((model->state == IR_Started) && 
			((now - model->timeRef) > model->timeout_duration_us)
		) {
			if (model->bits_processed > 0) {
				#ifdef IR_RECEIVER_DEBUGLOG_ENABLE
					// printf("\nbits processed: %d\n", model->bits_processed);
					printf( "0x%04X 0x%04X 0x%04X 0x%04X\n",
						model->ir_data[0], model->ir_data[1], model->ir_data[2], model->ir_data[3] );

					u64 combined = combine_64(model->ir_data[0], model->ir_data[1],
											model->ir_data[2], model->ir_data[3]);
					u8 check = crc16_ccitt_check64(combined, &combined);
					printf("check: %d\n", check);
				#endif

				printf( "0x%04X 0x%04X 0x%04X 0x%04X\n",
						model->ir_data[0], model->ir_data[1], model->ir_data[2], model->ir_data[3] );
				handler(model->ir_data[0], model->ir_data[1]);
			}

			//# clear out ir data
			memset(model->ir_data, 0, IRRECEIVER_BUFFER_SIZE * sizeof(model->ir_data[0]));
			model->state = IR_Idle;
			model->bits_processed = 0;
		}
	}

#else


	//! ####################################
	//! RECEIVE FUNCTIONS USING GPIO
	//! ####################################

	// low state: average 500us-600us
	// high state: average 1500us-1700us
	// estimated threshold: greater than 1000 for high state
	#define IRREMOTE_PULSE_THRESHOLD_US 1000

	void fun_irReceiver_init(IRReceiver_t* model) {
		model->timeout_duration_us = 100000; 	// 100ms
		model->state = IR_Idle;
		model->prev_pinState = 0;
		model->counter = 0;

		funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
		funDigitalWrite(model->pin, 1);
	}

	u32 IR_durations[IRREMOTE_MAX_PULSES];

	//# Decode IR signal
	void _irReceiver_decode(IRReceiver_t* model, void (*handler)(u16, u16)) {
		if (model->counter < 1) return;

		//# the first 2 pulses are start pulses
		#ifdef IR_RECEIVER_DEBUGLOG_ENABLE
			// start pulses: 9ms HIGH, 4.5ms LOW
			printf("\nPulses: %d\n", model->counter);
			printf("start pulses (us): %ld %ld\n", IR_durations[0], IR_durations[1]);

			u8 len_count = 0;
			for (int i = 2; i < model->counter; i++) {
				u16 rounded = 10 * (IR_durations[i] / 10);
				printf((i%2 == 0) ? "\n%ld " : "-%ld ", rounded);
				len_count++;
				if (len_count%16 == 0) printf("\n"); // line seperator
			}
		#endif

		for (int i = 3; i < model->counter; i += 2) {
			if (model->bits_processed >= 16*IRRECEIVER_BUFFER_SIZE) break;

			int word_idx = model->bits_processed >> 4;       // (>>4) is (/16)
			int bit_pos = model->bits_processed & 0x0F;  	// (&0x0F) is (%16)
			model->bits_processed++;

			if (IR_durations[i] > IRREMOTE_PULSE_THRESHOLD_US) {
				// MSB first (reversed)
				model->ir_data[word_idx] |= (1 << (15 - bit_pos));
			}
		}

		handler(model->ir_data[0], model->ir_data[1]);

		// #ifdef IR_RECEIVER_DEBUGLOG_ENABLE
			printf("MSB First: ");
			for (int i = 0; i < IRRECEIVER_BUFFER_SIZE; i++) {
				printf("0x%04X ", model->ir_data[i]);
			}
			printf("\n");
		// #endif
	}

	void _irReceiver_update_PulseCount(IRReceiver_t* model, u8 pulse_count) {
		model->counter = pulse_count;
		model->timeRef = micros();
		model->prev_pinState = model->current_pinState;
	}

	//# Receive IR task
	void fun_irReceiver_task(IRReceiver_t* model, void (*handler)(u16, u16)) {
		if (model->pin == -1) return;
		model->current_pinState = funDigitalRead(model->pin);

		if ((model->state == IR_Idle) && !model->current_pinState) {
			model->state = IR_Started;
			_irReceiver_update_PulseCount(model, 0);
		}
		else if ((model->state == IR_Started) && (model->current_pinState != model->prev_pinState)) {
			model->state = IR_Parsing;
			// State changed - record duration
			IR_durations[model->counter] = micros() - model->timeRef;
			_irReceiver_update_PulseCount(model, model->counter + 1);
		}
		else if ((model->state == IR_Parsing) && (model->current_pinState != model->prev_pinState)) {
			// State changed - record duration
			IR_durations[model->counter] = micros() - model->timeRef;
			_irReceiver_update_PulseCount(model, model->counter + 1);      
		}
		else if (
			((model->state == IR_Parsing) || (model->state == IR_Started))
			&& (micros() - model->timeRef > model->timeout_duration_us)
		) {
			model->state = IR_Idle;
			//! decode here
			_irReceiver_decode(model, handler);
			_irReceiver_update_PulseCount(model, 0);
			model->bits_processed = 0;
		}
	}
#endif