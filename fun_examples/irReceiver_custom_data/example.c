 //! OVERWRITE
#define IRRECEIVER_PULSE_THRESHOLD_US 450

#define IR_LOGICAL_HIGH_THRESHOLD 150
#define IR_OUTLINER_THRESHOLD 50

#include "../../fun_modules/fun_irReceiver.h"


#define IR_RECEIVER_PIN PD2

#define ROUND_TO_NEAREST_10(x)  ((x + 5) / 10 * 10)

void on_handle_irReceiver(u16 *data, u16 len) {

}

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
	u32 highthreshold_buf[5];
	u8 highthreshold_idx;
	u32 timeRef;
	u32 timeoutRef;
	u32 bits_processed;			// number of bits processed
} IRAnalyzer_t;

IRAnalyzer_t IRAnalyzer = {
	.pin = IR_RECEIVER_PIN,
	.buf_idx = 0,
};

Cycle_Info_t cycle;

#define MAX_WORDS 200
u16 word_buf[MAX_WORDS] = {0};
volatile u16 word_idx = 0;

void _irReceiver_processBuffer() {
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

	IRAnalyzer.highthreshold_idx = 0;

	memset(IRAnalyzer.buf, 0, sizeof(IRAnalyzer.buf));
	IRAnalyzer.buf_idx = 0;
	memset(word_buf, 0, sizeof(word_buf));
	word_idx = 0;
}

void fun_irReceiver_task2(IRReceiver_t* model, void (*handler)(u16*, u8)) {
    if (model->pin == -1) return;
    model->current_pinState = funDigitalRead(model->pin);
	u32 now = micros();

    if (model->current_pinState != model->prev_pinState) {
		//! get time difference
		u16 elapsed = now - IRAnalyzer.timeRef;

		if (elapsed < IR_RECEIVER_HIGH_THRESHOLD) {
			//! collect data
			IRAnalyzer.buf[IRAnalyzer.buf_idx] = elapsed;

			u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - elapsed);
			u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - elapsed);
			int bit = delta_1 < delta_0;

			//! prevent overflow
			if (word_idx >= MAX_WORDS) {
				printf("max words: %d\n", word_idx);
				_irReceiver_processBuffer();
				model->prev_pinState = model->current_pinState;
				return;
			}

			u32 bit_pos = 15 - (IRAnalyzer.buf_idx & 0x0F);  	// &0x0F is %16
			if (bit) word_buf[word_idx] |= 1 << bit_pos;		// MSB first (reversed)

			if (bit_pos == 0) {
				word_idx++;
				IRAnalyzer.buf_idx = 0;
			} else {
				IRAnalyzer.buf_idx++;
			}
		}
		else {
			IRAnalyzer.highthreshold_buf[IRAnalyzer.highthreshold_idx] = elapsed;
			IRAnalyzer.highthreshold_idx++;
		}

		IRAnalyzer.timeRef = micros();
		IRAnalyzer.timeoutRef = micros();
    }

	//! timeout
    if ((micros() - IRAnalyzer.timeoutRef) > 500000) {
        IRAnalyzer.timeoutRef = micros();

		_irReceiver_processBuffer();
		fun_cycleInfo_flush(&cycle);

		//! Reset values
		IRAnalyzer.buf_idx = 0;
		memset(IRAnalyzer.buf, 0, sizeof(IRAnalyzer.buf));
		word_idx = 0;
		memset(word_buf, 0, sizeof(word_buf));
    }

    model->prev_pinState = model->current_pinState;

	fun_cycleInfo_updateWithLimit(&cycle, micros() - now, 50);
}


int main() {
	SystemInit();
	Delay_Ms(100);
	systick_init();			//! REQUIRED for millis()

	printf("\r\nIR Receiver Test.\r\n");
	funGpioInitAll();

	IRReceiver_t receiver = {
		.pin = IR_RECEIVER_PIN,
	};

	fun_irReceiver_init(&receiver);

	fun_cycleInfo_clear(&cycle);

	while(1) {
		fun_irReceiver_task2(&receiver, on_handle_irReceiver);
	}
}
