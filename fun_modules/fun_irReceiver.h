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

#define IR_RECEIVER_TIMEOUT_US 300000	// 300ms

#ifndef ABS
	#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

// #define IR_RECEIVER_DEBUG_LOG

// uncomment to show cycle log
// #define IR_RECEIVER_CYCLE_LOG

//! ####################################
//! RECEIVE FUNCTIONS USING GPIO
//! ####################################

#ifdef IR_RECEIVER_DEBUG_LOG
	u16 DEBUG_BUFFER[100] = {0};

	Debug_Buffer_t debug_buffer = {
		.buf = DEBUG_BUFFER,
		.buf_idx = 0,
		.buf_len = 100
	};
#endif

#define BYTE_BITS_LEN 8
#define IR_NEC_LOGICAL_1_US 1600
#define IR_NEC_LOGICAL_0_US 560
#define IR_NEC_START_SIGNAL_THRESHOLD_US 2200

typedef struct {
	u8 pin;
	u16 BIT_BUFFER[BYTE_BITS_LEN];
	u16 bit_idx;
	u8 prev_state, current_state;
	u32 time_ref, timeout_ref;

	u8 IR_MODE;				// 0 = NEC protocol, 1 = NfS1 protocol
	u8 *RECEIVE_BUF;
	u16 RECEIVE_MAX_LEN;
	u16 byte_idx;

	u16 LOGICAL_1_US;					// LOGICAL_1 in microseconds
	u16 LOGICAL_0_US;					// LOGICAL_0 in microseconds
	u16 START_SIGNAL_THRESHOLD_US;		// START_SIGNAL_THRESHOLD in microseconds
	void (*onHandle_data)(u8*, u16);
	void (*onHandle_string)(const char* str);
} IR_Receiver_t;

Cycle_Info_t ir_cycle;


//* INIT FUNCTION
void fun_irReceiver_init(IR_Receiver_t* model) {
	printf("IRReceiver init\n");
	funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(model->pin, 1);

	if (model-> LOGICAL_1_US < 250 || model->LOGICAL_0_US < 250 
		|| model->START_SIGNAL_THRESHOLD_US < 500
	) {
		model->LOGICAL_1_US = IR_NEC_LOGICAL_1_US;
		model->LOGICAL_0_US = IR_NEC_LOGICAL_0_US;
		model->START_SIGNAL_THRESHOLD_US = IR_NEC_START_SIGNAL_THRESHOLD_US;
	}
	
	//! restart states
	model->bit_idx = 0,
	model->prev_state = 0,
	model->current_state = 0,

	UTIL_cycleInfo_clear(&ir_cycle);		// clear cycle
}

//* PROCESS BUFFER FUNCTION
void _irReceiver_processBuffer(IR_Receiver_t *model) {
	//! make callback
	if (model->byte_idx > 0) {
		if (model->byte_idx > 3 && model->onHandle_string &&
			model->RECEIVE_BUF[0] == 0xFA && 
			model->RECEIVE_BUF[1] == 0xFA &&
			model->RECEIVE_BUF[2] == 0xFA
		) {
			model->onHandle_string((const char*)(model->RECEIVE_BUF + 3));
		}
		else if (model->onHandle_data) {
			model->onHandle_data(model->RECEIVE_BUF, model->byte_idx);
		}
	}
	model->prev_state = model->current_state;

	//! Reset values
	memset(model->BIT_BUFFER, 0, BYTE_BITS_LEN * sizeof(u16));
	memset(model->RECEIVE_BUF, 0, model->RECEIVE_MAX_LEN * sizeof(u8));
	model->bit_idx = 0;
	model->byte_idx = 0;

	//# Debug log
	#ifdef IR_RECEIVER_DEBUG_LOG
		UTIL_DebugBuffer_flush(&debug_buffer);
	#endif
}

//* TASK FUNCTION
void fun_irReceiver_task(IR_Receiver_t* model) {
	if (model->pin == -1) return;
	u8 new_state = funDigitalRead(model->pin);
	u32 moment = micros();

	//! check for state change
	if (new_state != model->prev_state) {
		u16 elapsed = moment - model->time_ref;

		//# STEP 1: Filter out high thresholds
		// NEC protocol uses the PWM's OFF state spacing for LOGICAL value
		u8 valid_signal = (elapsed < model->START_SIGNAL_THRESHOLD_US) && 
						(model->LOGICAL_1_US != IR_NEC_LOGICAL_1_US || !new_state);

		if (valid_signal) {
			//! prevent overflow
			if (model->byte_idx >= model->RECEIVE_MAX_LEN) {
				printf("max byte reached: %d\n", model->byte_idx);
				//# STEP 3: Process Buffer when it's full
				_irReceiver_processBuffer(model);
				return;
			}

			//# STEP 2: collect data
			model->BIT_BUFFER[model->bit_idx] = elapsed;

			//# Debug log
			#ifdef IR_RECEIVER_DEBUG_LOG
				UTIL_DebugBuffer_addValue(&debug_buffer, elapsed);
			#endif

			// Determine bit value
			int bit = ABS(model->LOGICAL_1_US - elapsed) < ABS(model->LOGICAL_0_US - elapsed);
			u8 bit_pos = 7 - (model->bit_idx % 8);
			// Set bit in current byte (MSB first)
			if (bit) model->RECEIVE_BUF[model->byte_idx] |= 1 << bit_pos;

            // Increment and handle byte completion
            if (++model->bit_idx >= 8) {
                model->byte_idx++;        // next byte
                model->bit_idx = 0;   // reset bit counter
            }
		}

		model->time_ref = model->timeout_ref = micros();
	}

	//# STEP 4: Timeout handler
	if ((micros() - model->timeout_ref) > IR_RECEIVER_TIMEOUT_US) {
		model->timeout_ref = micros();

		//! process the buffer
		_irReceiver_processBuffer(model);

		#ifdef IR_RECEIVER_CYCLE_LOG
			UTIL_cycleInfo_flush(&ir_cycle);		// flush cycle
		#endif
	}

	model->prev_state = model->current_state = new_state;
	UTIL_cycleInfo_updateWithLimit(&ir_cycle, micros() - moment, 50);
}