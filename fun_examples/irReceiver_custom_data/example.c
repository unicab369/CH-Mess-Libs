#include "../../fun_modules/fun_irReceiver.h"

#define IR_RECEIVER_PIN PD2

// uncomment to show cycle log
// #define IR_RECEIVER_CYCLE_LOG

// how many lines of log to show when printing long data
#define IR_RECEIVER_LINE_LOG 5

// #define IR_RECEIVER_HIGH_THRESHOLD 2000
// #define IR_SENDER_LOGICAL_1_US 1600
// #define IR_SENDER_LOGICAL_0_US 560

#define IR_RECEIVER_HIGH_THRESHOLD 800
#define IR_SENDER_LOGICAL_1_US 550
#define IR_SENDER_LOGICAL_0_US 300

#define IR_MAX_PULSES2 16

typedef struct {
	u8 pin;
	u16 buf[IR_MAX_PULSES2];
	u16 buf_idx;
	u32 time_ref, timeout_ref;
	u8 prev_state, current_state;
} IR_Receiver_t;

Cycle_Info_t cycle;

#define MAX_WORDS 200
u16 word_buf[MAX_WORDS] = {0};
volatile u16 word_idx = 0;

void _irReceiver_processBuffer(IR_Receiver_t *model) {
	if (word_idx > 0) {
		printf("\n\nReceived Buffer Len %d\n", word_idx);
		s16 limit = word_idx > MAX_WORDS ? MAX_WORDS : word_idx;
		s16 line_offset = (limit / 8) - IR_RECEIVER_LINE_LOG;
		u8 start = line_offset > 0 ? line_offset * 8 : 0;

		if (start > 0) printf("... Skipping %d lines ...\n", line_offset);

		for (int i = start; i < limit; i++) {
			printf("0x%04X  ", word_buf[i]);
			if (i%8 == 7) printf("\n");			// separator
		}
	}

	model->prev_state = model->current_state;

	//! Reset values
	memset(model->buf, 0, sizeof(model->buf));
	model->buf_idx = 0;
	memset(word_buf, 0, sizeof(word_buf));
	word_idx = 0;
}

void fun_irReceiver_task2(IR_Receiver_t* model) {
    if (model->pin == -1) return;
    model->current_state = funDigitalRead(model->pin);
	u32 moment = micros();

	//! check for state change
    if (model->current_state != model->prev_state) {
		u16 elapsed = moment - model->time_ref;

		if (elapsed < IR_RECEIVER_HIGH_THRESHOLD) {
			//# STEP 2: collect data
			model->buf[model->buf_idx] = elapsed;

			//! prevent overflow
			if (word_idx >= MAX_WORDS) {
				printf("max words: %d\n", word_idx);

				//# STEP 3: Process Buffer when it's full
				_irReceiver_processBuffer(model);
				return;
			}

			// handle bit shifting
			u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - elapsed);
			u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - elapsed);
			int bit = delta_1 < delta_0;

			u32 bit_pos = 15 - (model->buf_idx & 0x0F);  		// &0x0F is %16
			if (bit) word_buf[word_idx] |= 1 << bit_pos;		// MSB first (reversed)

			if (bit_pos == 0) {
				word_idx++;				// next word
				model->buf_idx = 0;		// reset the buffer
			} else {
				model->buf_idx++;		// next bit
			}
		}

		model->time_ref = micros();
		model->timeout_ref = micros();
    }

	//# STEP 4: Timeout handler
    if ((micros() - model->timeout_ref) > 500000) {
        model->timeout_ref = micros();

		//# flush the buffer
		_irReceiver_processBuffer(model);
		fun_cycleInfo_flush(&cycle);
    }

    model->prev_state = model->current_state;
	fun_cycleInfo_updateWithLimit(&cycle, micros() - moment, 50);
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()

	printf("\r\nIR Receiver Test.\r\n");
	Delay_Ms(100);
	funGpioInitAll();

	IR_Receiver_t receiver = {
		.pin = IR_RECEIVER_PIN,
		.buf_idx = 0,
		.prev_state = 0,
		.current_state = 0,
	};

	funPinMode(receiver.pin, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(receiver.pin, 1);

	fun_cycleInfo_clear(&cycle);

	while(1) {
		fun_irReceiver_task2(&receiver);
	}
}
