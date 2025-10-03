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

//# Debug Mode: 0 = disable, 1 = log level 1, 2 = log level 2
#define IR_RECEIVER_DEBUGLOG 2

//# Comment this line out to use the GPIO input ortherwise use the DMA input
#define IR_RECEIVER_USE_TIM1

//! For generating the mask/modulus, this must be a power of 2 size.
#define IRREMOTE_MAX_PULSES 128
#define TIM1_BUFFER_LAST_IDX (IRREMOTE_MAX_PULSES - 1)
#define IRRECEIVER_BUFFER_SIZE 4
#define IRRECEIVER_DECODE_TIMEOUT_US 100000		// 100ms

typedef struct {
	int pin;
	int ir_started;
	u16 ir_data[IRRECEIVER_BUFFER_SIZE];	// store the received data
	
	int prev_pinState;			// for gpio
	int current_pinState;		// for gpio

	int16_t counter;			// store pulses count (for gpio) Or tails (for DMA)
	u8 bits_processed;			// number of bits processed
	u32 timeRef;
	u32 pulse_buf[IRREMOTE_MAX_PULSES];
} IRReceiver_t;


void _irReciever_Restart(IRReceiver_t* model) {
	model->ir_started = 0;
	model->bits_processed = 0;
	model->counter = 0;
	model->prev_pinState = 0;
	memset(model->ir_data, 0, IRRECEIVER_BUFFER_SIZE * sizeof(model->ir_data[0]));
}

#ifdef IR_RECEIVER_USE_TIM1
	// TIM2_CH1 -> PD2 -> DMA1_CH2
	#define IR_DMA_IN	DMA1_Channel2

	//! ####################################
	//! SETUP FUNCTION
	//! ####################################

	//! For generating the mask/modulus, this must be a power of 2 size.
	uint16_t ir_ticks_buff[IRREMOTE_MAX_PULSES];

	void fun_irReceiver_init(IRReceiver_t* model) {
		funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
		funDigitalWrite(model->pin, 1);
		_irReciever_Restart(model);

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

	#define IR_LOGICAL_HIGH_THRESHOLD 170
	#define IR_OUTLINER_THRESHOLD 100

	// #define IR_LOGICAL_HIGH_THRESHOLD 300
	// #define IR_OUTLINER_THRESHOLD 150

	u16 outliner = 0;
	
	void fun_irReceiver_task(IRReceiver_t* model, void(*handler)(u16*, u8)) {
			// Must perform modulus here, in case DMA_IN->CNTR == 0.
		int head = (IRREMOTE_MAX_PULSES - IR_DMA_IN->CNTR) & TIM1_BUFFER_LAST_IDX;

		if( head != model->counter ) {
			u32 time_of_event = ir_ticks_buff[model->counter];
			u8 prev_event_idx = (model->counter-1) & TIM1_BUFFER_LAST_IDX;		// modulus to loop back
			u32 prev_time_of_event = ir_ticks_buff[prev_event_idx];
			u32 elapsed = time_of_event - prev_time_of_event;

			//! Performs modulus to loop back.
			model->counter = (model->counter+1) & TIM1_BUFFER_LAST_IDX;

			//! Timer overflow handling
			if (time_of_event >= prev_time_of_event) {
				elapsed = time_of_event - prev_time_of_event;
			} else {
				// Timer wrapped around (0xFFFF -> 0x0000)
				elapsed = (65536 - prev_time_of_event) + time_of_event;
			}

			//! decode here
			if (model->ir_started) {
				//! filter LOW start frame (> 1000 ticks)
				if (elapsed > 700) {
					#if IR_RECEIVER_DEBUGLOG > 0
						printf("Frame LOW: %d\n", elapsed);
					#endif
					return;
				}

				//! handle outliner (SUPER hacky)
				if (outliner > 0) {
					elapsed += outliner;	// add prev outliner
					outliner = 0;
				}
				//! look for outliner
				if (elapsed < IR_OUTLINER_THRESHOLD) {
					#if IR_RECEIVER_DEBUGLOG > 0
						printf("\n*** outliner: %d\n", elapsed);
					#endif
					outliner = elapsed;
					return;
				}

				int bit_pos = 15 - (model->bits_processed & 0x0F);  // bits_processed % 16
				int word_idx = model->bits_processed >> 4;          // bits_processed / 16
				model->bits_processed++;

				//# filter for Logical HIGH
				// logical HIGH (~200 ticks), logical LOW (~100 ticks)
				if (elapsed > IR_LOGICAL_HIGH_THRESHOLD) {
					model->ir_data[word_idx] |= (1 << bit_pos);		// MSB first (reversed)
				}

				#if IR_RECEIVER_DEBUGLOG > 0
					const char *bit = elapsed > IR_LOGICAL_HIGH_THRESHOLD ? "1" : ".";

					printf("%d 0x%04X %s - D%d",
						elapsed,		// round to nearest 10
						model->ir_data[word_idx],
						bit, bit_pos);

					#if IR_RECEIVER_DEBUGLOG > 1
						printf(" \t [%d]%ld, [%d]%ld", model->counter, time_of_event, 
													prev_event_idx, prev_time_of_event);
					#endif

					printf("\n");
					// separator
					if (bit_pos % 8 == 0) printf("\n");
				#endif

				model->timeRef =  micros();
			}

			//! filter HIGH start frame. Expect > 1200 ticks
			else if (elapsed > 500) {
				#if IR_RECEIVER_DEBUGLOG > 0
					printf("Frame HIGH: %d\n", elapsed);
				#endif
				model->ir_started = 1;
				model->bits_processed = 0;
				model->timeRef =  micros();
				outliner = 0;
			}
		}

		// handle timeout
		if ((model->ir_started) && 
			(( micros() - model->timeRef) >IRRECEIVER_DECODE_TIMEOUT_US)
		) {
			if (model->bits_processed > 0) {
				#if IR_RECEIVER_DEBUGLOG > 1
					// printf("\nbits processed: %d\n", model->bits_processed);
					u64 combined = combine_64(model->ir_data[0], model->ir_data[1],
											model->ir_data[2], model->ir_data[3]);
					u8 check = crc16_ccitt_check64(combined, &combined);
					printf("check: %d\n", check);
				#endif

				handler(model->ir_data, IRRECEIVER_BUFFER_SIZE);
			}

			//# clear out ir data
			memset(model->ir_data, 0, IRRECEIVER_BUFFER_SIZE * sizeof(model->ir_data[0]));
			model->ir_started = 0;
			model->timeRef =  micros();
		}
	}

#else


	//! ####################################
	//! RECEIVE FUNCTIONS USING GPIO
	//! ####################################

	#define IRRECEIVER_PULSE_THRESHOLD_US 460

	// low state: average 500us-600us
	// high state: average 1500us-1700us
	// estimated threshold: greater than 1000 for high state
	// #define IRRECEIVER_PULSE_THRESHOLD_US 1000

	void fun_irReceiver_init(IRReceiver_t* model) {
		funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
		funDigitalWrite(model->pin, 1);
		_irReciever_Restart(model);
	}

	// u32 IR_durations[IRREMOTE_MAX_PULSES];

	//# Decode IR signal
	void _irReceiver_DECODE(IRReceiver_t* model, void (*handler)(u16*, u8)) {
		if (model->counter < 1) return;

		//# Logs
		#if IR_RECEIVER_DEBUGLOG > 1
			printf("\nPulses: %d\n", model->counter);
			printf("start pulses (us): %ld %ld\n", model->pulse_buf[0], model->pulse_buf[1]);

			// u8 len_count = 0;
			// for (int i = 2; i < model->counter; i++) {
			// 	u16 rounded = 10 * (model->pulse_buf[i] / 10);
			// 	printf((i%2 == 0) ? "\n%ld " : "-%ld ", rounded);
			// 	len_count++;
			// 	if (len_count%16 == 0) printf("\n"); // line seperator
			// }
		#endif

		//! start pulses: 9ms HIGH, 4.5ms LOW
		if (model->pulse_buf[0] < 9000 || model->pulse_buf[1] < 4000) {
			_irReciever_Restart(model);
			return;
		}

		//! start from index 3 to skip start phases
		for (int i = 3; i < model->counter; i += 2) {
			if (model->bits_processed >= 16*IRRECEIVER_BUFFER_SIZE) break;

			int word_idx = model->bits_processed >> 4;     			// (>>4) is (/16)
			int bit_pos = 15 - (model->bits_processed & 0x0F);  	// (&0x0F) is (%16)
			model->bits_processed++;
			int bit =  model->pulse_buf[i] > IRRECEIVER_PULSE_THRESHOLD_US;

			//! collect the data
			if (bit) {
				model->ir_data[word_idx] |= 1 << bit_pos;		// MSB first (reversed)
			}

			const char *bitStr = bit ? "1" : ".";
			printf("%d \t %s \t 0x%04X D%d\n",
				model->pulse_buf[i], bitStr, model->ir_data[word_idx], bit_pos);
			// separator
			if (bit_pos % 8 == 0) printf("\n");
		}

		handler(model->ir_data, IRRECEIVER_BUFFER_SIZE);
	}

	void _irReceiver_update_PulseCount(IRReceiver_t* model, u8 pulse_count) {
		model->counter = pulse_count;
		model->timeRef = micros();
		model->prev_pinState = model->current_pinState;
	}

	//# Receive IR task
	void fun_irReceiver_task(IRReceiver_t* model, void (*handler)(u16*, u8)) {
		if (model->pin == -1) return;
		model->current_pinState = funDigitalRead(model->pin);

		//! wait for first pulse
		if (!model->ir_started && !model->current_pinState) {
			model->ir_started = 1;
			_irReceiver_update_PulseCount(model, 0);

		} else if (model->ir_started) {
			u32 elapsed = micros() - model->timeRef;

			if (model->current_pinState != model->prev_pinState) {
				//! State changed - record duration
				model->pulse_buf[model->counter] = elapsed;
				_irReceiver_update_PulseCount(model, model->counter + 1);

			} else if (elapsed > IRRECEIVER_DECODE_TIMEOUT_US) {
				//! decode here after timeout
				_irReceiver_DECODE(model, handler);
				_irReceiver_update_PulseCount(model, 0);

				//! restart
				_irReciever_Restart(model);
			}
		}
	}
#endif