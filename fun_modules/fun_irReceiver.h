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

#define IR_RECEIVER_BITBUFFER_LEN 16

typedef struct {
	u8 pin;
	u16 BIT_BUFFER[IR_RECEIVER_BITBUFFER_LEN];
	u16 bit_buf_idx;
	u8 prev_state, current_state;
	u32 time_ref, timeout_ref;

	u8 IR_MODE;				// 0 = NEC protocol, 1 = NfS1 protocol
	u16 WORD_BUFFER_LEN;
	u16 *WORD_BUFFER;
	u16 word_idx;
} IR_Receiver_t;

Cycle_Info_t ir_cycle;

static u16 IR_START_SIGNAL_THRESHOLD_US = 2200;
static u16 IR_LOGICAL_1_US = 1600;
static u16 IR_LOGICAL_0_US = 560;

//* INIT FUNCTION
void fun_irReceiver_init(IR_Receiver_t* model) {
	printf("IRReceiver init\n");
	funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(model->pin, 1);

	switch (model->IR_MODE) {
		case 0:
			IR_START_SIGNAL_THRESHOLD_US = 2200;
			IR_LOGICAL_1_US = 1600;
			IR_LOGICAL_0_US = 560;
			break;
		case 1:
			IR_START_SIGNAL_THRESHOLD_US = 800;
			IR_LOGICAL_1_US = 550;
			IR_LOGICAL_0_US = 300;
			break;
	}

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
	memset(model->BIT_BUFFER, 0, IR_RECEIVER_BITBUFFER_LEN * sizeof(u16));
	model->bit_buf_idx = 0;
	memset(model->WORD_BUFFER, 0, model->WORD_BUFFER_LEN * sizeof(u16));
	model->word_idx = 0;

	//# Debug log
	#ifdef IR_RECEIVER_DEBUG_LOG
		UTIL_DebugBuffer_flush(&debug_buffer);
	#endif
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
		u8 filter_signal = elapsed < IR_START_SIGNAL_THRESHOLD_US;

		// NEC protocol uses the PWM's OFF state spacing for LOGICAL value
		if (model->IR_MODE == 0) {
			filter_signal = filter_signal && !model->current_state;
		}

		if (filter_signal) {
			//! prevent overflow
			if (model->word_idx >= model->WORD_BUFFER_LEN) {
				printf("max words: %d\n", model->word_idx);

				//# STEP 3: Process Buffer when it's full
				_irReceiver_processBuffer(model, handler);
				return;
			}

			//# STEP 2: collect data
			model->BIT_BUFFER[model->bit_buf_idx] = elapsed;

			//# Debug log
			#ifdef IR_RECEIVER_DEBUG_LOG
				UTIL_DebugBuffer_addValue(&debug_buffer, elapsed);
			#endif

			// handle bit shifting
			u16 delta_1 = ABS(IR_LOGICAL_1_US - elapsed);
			u16 delta_0 = ABS(IR_LOGICAL_0_US - elapsed);
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