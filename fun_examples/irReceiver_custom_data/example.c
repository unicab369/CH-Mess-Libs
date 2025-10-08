 //! OVERWRITE
#define IRRECEIVER_PULSE_THRESHOLD_US 450

#define IR_LOGICAL_HIGH_THRESHOLD 150
#define IR_OUTLINER_THRESHOLD 50

#include "../../fun_modules/fun_irReceiver.h"


#define IR_RECEIVER_PIN PD2

// how many lines of log to show when printing long data
#define IR_RECEIVER_LOG_LINE 5

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
	u32 timeRef;
	u32 timeoutRef;
	u8 prev_state;
	u8 current_state;
} IR_Receiver_t;

IR_Receiver_t IRAnalyzer = {
	.pin = IR_RECEIVER_PIN,
	.buf_idx = 0,
	.prev_state = 0,
	.current_state = 0,
};

Cycle_Info_t cycle;

#define MAX_WORDS 200
u16 word_buf[MAX_WORDS] = {0};
volatile u16 word_idx = 0;

void _irReceiver_processBuffer(IR_Receiver_t *model) {
	if (word_idx > 0) {
		printf("\n\nReceived Buffer Len %d\n", word_idx);
		s16 limit = word_idx > MAX_WORDS ? MAX_WORDS : word_idx;
		s16 line_offset = (limit / 8) - IR_RECEIVER_LOG_LINE;

		if (line_offset > 0) {
			printf("... Skipping %d lines ...\n", line_offset);

			for (int i = line_offset * 8; i < limit; i++) {
				printf("0x%04X  ", word_buf[i]);

				// separator
				if (i%8 == 7) printf("\n");
			}
		} else {
			for (int i = 0; i < limit; i++) {
				printf("0x%04X  ", word_buf[i]);

				// separator
				if (i%8 == 7) printf("\n");
			}
		}
	}

	memset(model->buf, 0, sizeof(model->buf));
	model->buf_idx = 0;
	memset(word_buf, 0, sizeof(word_buf));
	word_idx = 0;
}

void fun_irReceiver_task2(IR_Receiver_t* model) {
    if (model->pin == -1) return;
    model->current_state = funDigitalRead(model->pin);
	u32 now = micros();

    if (model->current_state != model->prev_state) {
		//! get time difference
		u16 elapsed = now - model->timeRef;

		if (elapsed < IR_RECEIVER_HIGH_THRESHOLD) {
			//! collect data
			model->buf[model->buf_idx] = elapsed;

			u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - elapsed);
			u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - elapsed);
			int bit = delta_1 < delta_0;

			//! prevent overflow
			if (word_idx >= MAX_WORDS) {
				printf("max words: %d\n", word_idx);
				_irReceiver_processBuffer(&IRAnalyzer);
				model->prev_state = model->current_state;
				return;
			}

			u32 bit_pos = 15 - (model->buf_idx & 0x0F);  	// &0x0F is %16
			if (bit) word_buf[word_idx] |= 1 << bit_pos;		// MSB first (reversed)

			if (bit_pos == 0) {
				word_idx++;
				model->buf_idx = 0;
			} else {
				model->buf_idx++;
			}
		}


		model->timeRef = micros();
		model->timeoutRef = micros();
    }

	//! timeout
    if ((micros() - model->timeoutRef) > 500000) {
        model->timeoutRef = micros();

		//# flush the buffer
		_irReceiver_processBuffer(&IRAnalyzer);
		fun_cycleInfo_flush(&cycle);

		//! Reset values
		model->buf_idx = 0;
		memset(model->buf, 0, sizeof(model->buf));
		word_idx = 0;
		memset(word_buf, 0, sizeof(word_buf));
    }

    model->prev_state = model->current_state;
	fun_cycleInfo_updateWithLimit(&cycle, micros() - now, 50);
}


int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()

	printf("\r\nIR Receiver Test.\r\n");
	Delay_Ms(100);
	funGpioInitAll();

	// IRReceiver_t receiver = {
	// 	.pin = IR_RECEIVER_PIN,
	// };

	funPinMode(IRAnalyzer.pin, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(IRAnalyzer.pin, 1);

	// fun_irReceiver_init(&receiver);
	fun_cycleInfo_clear(&cycle);

	while(1) {
		fun_irReceiver_task2(&IRAnalyzer);
	}
}
