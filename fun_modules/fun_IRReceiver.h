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


//# Comment this line out to use the GPIO input ortherwise use the DMA input
// #define IR_RECEIVER_USE_TIM1


//# Debug Mode: 0 = disable, 1 = log level 1, 2 = log level 2
#define IR_RECEIVER_DEBUGLOG 1


#ifndef IR_LOGICAL_HIGH_THRESHOLD
	#define IR_LOGICAL_HIGH_THRESHOLD 300
#endif

#ifndef IR_OUTLINER_THRESHOLD
	#define IR_OUTLINER_THRESHOLD 150
#endif

//! For generating the mask/modulus, this must be a power of 2 size.
#define IR_MAX_PULSES 128
#define TIM1_BUFFER_LAST_IDX (IR_MAX_PULSES - 1)
#define IR_DECODE_TIMEOUT_US 100000		// 100ms
#define IRRECEIVER_BUFFER_SIZE 4
#define IRRECEIVER_BUFFER_BITS_COUNT (IRRECEIVER_BUFFER_SIZE * 2 * 8)

typedef struct {
	int pin;
	int ir_started;
	u16 ir_data[IRRECEIVER_BUFFER_SIZE];	// store the received data
	
	int prev_pinState;			// for gpio
	int current_pinState;		// for gpio

	int16_t counterIdx;			// store pulses count (for gpio) Or tails (for DMA)
	u32 bits_processed;			// number of bits processed
	u32 timeRef;
	u16 pulse_buf[IR_MAX_PULSES];
} IRReceiver_t;


#ifdef IR_RECEIVER_USE_TIM1
	// TIM2_CH1 -> PD2 -> DMA1_CH2
	#define IR_DMA_IN	DMA1_Channel2

	//! ####################################
	//! SETUP FUNCTION
	//! ####################################

	//! For generating the mask/modulus, this must be a power of 2 size.
	uint16_t ir_ticks_buff[IR_MAX_PULSES];

	void fun_irReceiver_init(IRReceiver_t* model) {
		funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
		funDigitalWrite(model->pin, 1);
		model->ir_started = 0;

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
		IR_DMA_IN->CNTR = IR_MAX_PULSES;

		TIM1->PSC = 0x00DF;		// set TIM1 clock prescaler divider (Massive prescaler)
		TIM1->ATRLR = 65535;	// set PWM total cycle width

		//# Tim 1 input / capture (CC1S = 01)
		TIM1->CHCTLR1 = TIM_CC1S_0;

		//# Add here CC1P to switch from UP->GOING to DOWN->GOING log times.
		TIM1->CCER = TIM_CC1E; // | TIM_CC1P;
		
		//# initialize counter
		TIM1->SWEVGR = TIM_UG;

		//# Enable TIM1
		TIM1->CTLR1 = TIM_ARPE | TIM_CEN;
		TIM1->DMAINTENR = TIM_CC1DE | TIM_UDE; // Enable DMA for T1CC1
	}


	//! ####################################
	//! RECEIVE FUNCTION USING DMA
	//! ####################################

	u16 outliner = 0;
	
	void fun_irReceiver_task(IRReceiver_t* model, void(*handler)(u16*, u8)) {
		// Must perform modulus here, in case DMA_IN->CNTR == 0.
		int head = (IR_MAX_PULSES - IR_DMA_IN->CNTR) & TIM1_BUFFER_LAST_IDX;

		if( head != model->counterIdx ) {
			u32 time_of_event = ir_ticks_buff[model->counterIdx];
			u8 prev_event_idx = (model->counterIdx-1) & TIM1_BUFFER_LAST_IDX;		// modulus to loop back
			u32 prev_time_of_event = ir_ticks_buff[prev_event_idx];
			u32 elapsed = time_of_event - prev_time_of_event;

			//! Performs modulus to loop back.
			model->counterIdx = (model->counterIdx+1) & TIM1_BUFFER_LAST_IDX;

			//# Timer overflow handling
			if (time_of_event >= prev_time_of_event) {
				elapsed = time_of_event - prev_time_of_event;
			} else {
				// Timer wrapped around (0xFFFF -> 0x0000)
				elapsed = (65536 - prev_time_of_event) + time_of_event;
			}

			if (model->ir_started) {
				//# STEP 2: filter LOW start frame (> 1000 ticks)
				if (elapsed > 700) {
					#if IR_RECEIVER_DEBUGLOG > 2
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
					// #if IR_RECEIVER_DEBUGLOG > 0
					// 	printf("\n*** outliner: %d\n", elapsed);
					// #endif
					outliner = elapsed;
					return;
				}

				int bit_pos = 15 - (model->bits_processed & 0x0F);  // bits_processed % 16
				int word_idx = model->bits_processed >> 4;          // bits_processed / 16
				model->bits_processed++;

				//# STEP 3: Decode here, filter for Logical HIGH
				if (elapsed > IR_LOGICAL_HIGH_THRESHOLD) {
					model->ir_data[word_idx] |= (1 << bit_pos);		// MSB first (reversed)
				}

				model->timeRef =  micros();
				
				// # Logging
				//! IMPORTANTE: printf statements introduce delays
				#if IR_RECEIVER_DEBUGLOG > 0
					// const char *bit = elapsed > IR_LOGICAL_HIGH_THRESHOLD ? "1" : ".";

					// printf("%d 0x%04X %s - D%d",
					// 	elapsed,		// round to nearest 10
					// 	model->ir_data[word_idx],
					// 	bit, bit_pos);
					// 	printf(" \t [%d]%ld, [%d]%ld", model->counterIdx, time_of_event, 
					// 								prev_event_idx, prev_time_of_event);
					// printf("\n");
					// // separator
					// if (bit_pos % 8 == 0) printf("\n");
				#endif

				
				//! enough data to collect
				if (bit_pos == 0 && word_idx == 3) {
					#if IR_RECEIVER_DEBUGLOG > 1
						// printf("\nbits processed: %d\n", model->bits_processed);
						u64 combined = combine_64(model->ir_data[0], model->ir_data[1],
												model->ir_data[2], model->ir_data[3]);
						u8 check = crc16_ccitt_check64(combined, &combined);
						printf("check: %d\n", check);
					#endif

					//# STEP 4: Complete frame
					handler(model->ir_data, IRRECEIVER_BUFFER_SIZE);

					//# clear out ir data
					model->bits_processed = 0;
					memset(model->ir_data, 0, IRRECEIVER_BUFFER_SIZE * sizeof(model->ir_data[0]));
				}
			}

			//# STEP 1: filter HIGH start frame. Expect > 1200 ticks
			else if (elapsed > 500) {
				#if IR_RECEIVER_DEBUGLOG > 2
					printf("Frame HIGH: %d\n", elapsed);
				#endif
				model->ir_started = 1;
				model->bits_processed = 0;
				model->timeRef =  micros();
				outliner = 0;
			}
		}

		//# STEP 5: handle timeout
		if ((model->ir_started) && 
			(( micros() - model->timeRef) > IR_DECODE_TIMEOUT_US)
		) {
			// printf("Timeout %d\n", micros() - model->timeRef);
			handler(NULL, 0);

			//# clear out ir data
			model->ir_started = 0;
			memset(model->ir_data, 0, IRRECEIVER_BUFFER_SIZE * sizeof(model->ir_data[0]));
		}
	}

#else

	#define IR_RECEIVER_TIMEOUT_US 300000	// 300ms

	// uncomment to show cycle log
	// #define IR_RECEIVER_CYCLE_LOG

	// #define IR_RECEIVER_HIGH_THRESHOLD 2000
	// #define IR_SENDER_LOGICAL_1_US 1600
	// #define IR_SENDER_LOGICAL_0_US 560

	#define IR_RECEIVER_HIGH_THRESHOLD 800
	#define IR_SENDER_LOGICAL_1_US 550
	#define IR_SENDER_LOGICAL_0_US 300

	//! ####################################
	//! RECEIVE FUNCTIONS USING GPIO
	//! ####################################

	typedef struct {
		u8 pin;
		u16 BIT_BUFFER[16];
		u16 bit_buf_idx;
		u8 prev_state, current_state;
		u32 time_ref, timeout_ref;

		u16 WORD_BUFFER_LEN;
		u16 *WORD_BUFFER;
		u16 word_idx;
	} IR_Receiver_t;

	Cycle_Info_t ir_cycle;

	//* INIT FUNCTION
	void fun_irReceiver_init(IR_Receiver_t* model) {
		printf("IRReceiver init\n");
		funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
		funDigitalWrite(model->pin, 1);

		//! restart states
		model->bit_buf_idx = 0,
		model->prev_state = 0,
		model->current_state = 0,

		UTIL_cycleInfo_clear(&ir_cycle);		// clear cycle
	}

	//* PROCESS BUFFER FUNCTION
	void _irReceiver_processBuffer(IR_Receiver_t *model, void (*handler)(u16*, u16)) {
		//! make callback
		if (model->word_idx > 0) handler(model->WORD_BUFFER, model->word_idx);
		model->prev_state = model->current_state;

		//! Reset values
		memset(model->BIT_BUFFER, 0, sizeof(model->BIT_BUFFER));
		model->bit_buf_idx = 0;
		memset(model->WORD_BUFFER, 0, sizeof(model->WORD_BUFFER));
		model->word_idx = 0;
	}

	//* TASK FUNCTION
	void fun_irReceiver_task(IR_Receiver_t* model, void (*handler)(u16*, u16)) {
		if (model->pin == -1) return;
		model->current_state = funDigitalRead(model->pin);
		u32 moment = micros();

		//! check for state change
		if (model->current_state != model->prev_state) {
			u16 elapsed = moment - model->time_ref;

			//# STEP 1: Filter out high thresholds
			if (elapsed < IR_RECEIVER_HIGH_THRESHOLD) {
				//! prevent overflow
				if (model->word_idx >= model->WORD_BUFFER_LEN) {
					printf("max words: %d\n", model->word_idx);

					//# STEP 3: Process Buffer when it's full
					_irReceiver_processBuffer(model, handler);
					return;
				}

				//# STEP 2: collect data
				model->BIT_BUFFER[model->bit_buf_idx] = elapsed;

				// handle bit shifting
				u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - elapsed);
				u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - elapsed);
				int bit = delta_1 < delta_0;

				u32 bit_pos = 15 - (model->bit_buf_idx & 0x0F);  // &0x0F is %16
				// MSB first (reversed)
				if (bit) model->WORD_BUFFER[model->word_idx] |= 1 << bit_pos;

				if (bit_pos == 0) {
					model->word_idx++;			// next word
					model->bit_buf_idx = 0;		// reset the buffer
				} else {
					model->bit_buf_idx++;		// next bit
				}
			}

			model->time_ref = micros();
			model->timeout_ref = micros();
		}

		//# STEP 4: Timeout handler
		if ((micros() - model->timeout_ref) > IR_RECEIVER_TIMEOUT_US) {
			model->timeout_ref = micros();

			//! process the buffer
			_irReceiver_processBuffer(model, handler);

			#ifdef IR_RECEIVER_CYCLE_LOG
				UTIL_cycleInfo_flush(&ir_cycle);		// flush cycle
			#endif
		}

		model->prev_state = model->current_state;
		UTIL_cycleInfo_updateWithLimit(&ir_cycle, micros() - moment, 50);
	}
#endif