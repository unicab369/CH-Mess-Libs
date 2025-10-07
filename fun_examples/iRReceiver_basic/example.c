//! OVERWRITE
#define IRRECEIVER_PULSE_THRESHOLD_US 450

#define IR_LOGICAL_HIGH_THRESHOLD 150
#define IR_OUTLINER_THRESHOLD 50

#include "../../fun_modules/fun_irReceiver.h"


#define IR_RECEIVER_PIN PD2

#define ROUND_TO_NEAREST_10(x)  ((x + 5) / 10 * 10)

void on_handle_irReceiver(u16 *data, u16 len) {

}


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
	u32 maxValue;
	u32 minValue;
	u32 timeRef;
	u32 timeoutRef;
	u32 bits_processed;			// number of bits processed
} IRAnalyzer_t;

IRAnalyzer_t IRAnalyzer = {
	.pin = IR_RECEIVER_PIN,
	.buf_idx = 0,
	.maxValue = 0,
	.minValue = 0xFFFF,
};

#define MAX_WORDS 400
u16 word_buf[MAX_WORDS] = {0};
volatile u16 word_idx = 0;
u32 cycle_time_max = 0;
u16 cycle_time_min = 0xFFFF;
u32 cycle_count = 0;
u32 cycle_count2 = 0;

void _irReceiver_processBuffer() {
	if (word_idx > 0) {
		printf("\n\nReceived %d\n", word_idx);
		s16 limit = word_idx > MAX_WORDS ? MAX_WORDS : word_idx;
		s16 line = limit / 8;
		s16 line_offset = line - 5;

		if (line_offset > 0) {
			for (int i = 0; i < limit; i++) {
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

	IRAnalyzer.buf_idx = 0;
	memset(IRAnalyzer.buf, 0, sizeof(IRAnalyzer.buf));
	word_idx = 0;
	memset(word_buf, 0, sizeof(word_buf));
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

			if (elapsed < IR_RECEIVER_HIGH_THRESHOLD && elapsed > IRAnalyzer.maxValue) {
				IRAnalyzer.maxValue = elapsed;
			}
			if (elapsed < IRAnalyzer.minValue) IRAnalyzer.minValue = elapsed;

			u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - elapsed);
			u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - elapsed);
			int bit = delta_1 < delta_0;

			//! prevent overflow
			if (word_idx >= MAX_WORDS) {
				printf("max words: %d\n", word_idx);
				_irReceiver_processBuffer();
				model->prev_pinState = model->current_pinState;
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

		printf("\ncycle max: %d us, min: %d us, count: %d, count2: %d\n", 
			cycle_time_max, cycle_time_min, cycle_count, cycle_count2);
		cycle_time_max = 0;
		cycle_time_min = 0xFFFF;
		cycle_count = 0;
		cycle_count2 = 0;

		// if (IRAnalyzer.buf_idx > 0) {
		// 	printf("\n\nState counts: %d\n", IRAnalyzer.buf_idx);
		// 	printf("maxValue: %d, minValue: %d\n\n", IRAnalyzer.maxValue, IRAnalyzer.minValue);

		// 	for (int i = 0; i < IRAnalyzer.highthreshold_idx; i++) {
		// 		// print and clear
		// 		printf("High Thres: %d\n", IRAnalyzer.highthreshold_buf[i]);
		// 		IRAnalyzer.highthreshold_buf[i] = 0;
		// 	}

		// 	//! print the header
		// 	printf("\n%5s %3s | %7s | %3s | %7s | %7s | %7s \n",
		// 			"", "idx", "Value", "Bit", "Delta_1", "Delta_0", "diff");

		// 	for (u16 i = 0; i < IRAnalyzer.buf_idx; i++) {
		// 		u32 value = IRAnalyzer.buf[i];
		// 		u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - value);
		// 		u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - value);
		// 		u16 delta_diff = abs(delta_1 - delta_0);
		// 		int bit = delta_1 < delta_0;

		// 		// find how close the value is to it's 
		// 		const char *bitStr = bit ? "1" : ".";
		// 		const char *maxStr = (value == IRAnalyzer.maxValue) ? "(max)" :
		// 							(value == IRAnalyzer.minValue) ? "(min)" : "";

		// 		//! print out the values
		// 		printf("%5s %3d | %7d | %3s | %7d | %7d | %7d \n",
		// 			maxStr, i, value, bitStr, delta_1, delta_0, delta_diff);

		// 		// separator
		// 		if (i % 8 == 7) printf("\n");
		// 	}

		// 	u16 word = 0x0000;
		// 	printf("\nDecoded words:\n");

		// 	for (u16 i = 0; i < IRAnalyzer.buf_idx; i++) {
		// 		u32 value = IRAnalyzer.buf[i];
		// 		u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - value);
		// 		u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - value);
		// 		int bit = delta_1 < delta_0;

		// 		u32 bit_pos = 15 - (i & 0x0F);  // &0x0F is %16
		// 		if (bit) word |= 1 << bit_pos;		// MSB first (reversed)

		// 		//! printout words
		// 		if (bit_pos == 0) {
		// 			printf("[%d] 0x%04X ", i, word);
		// 			word = 0x0000;		// reset word

		// 			// separator
		// 			if ((i+1)%128 == 0) printf("\n");
		// 		}
		// 	}

		// 	printf("\r\n");
		// }

		//! Reset values
		IRAnalyzer.maxValue = 0;
        IRAnalyzer.minValue = 0xFFFF;

		IRAnalyzer.buf_idx = 0;
		memset(IRAnalyzer.buf, 0, sizeof(IRAnalyzer.buf));
		word_idx = 0;
		memset(word_buf, 0, sizeof(word_buf));
    }

    model->prev_pinState = model->current_pinState;

	u32 elapsed2 = micros() - now;
	
	if (elapsed2 > cycle_time_max) {
		cycle_time_max = elapsed2;
	}

	if (elapsed2 < cycle_time_min) {
		cycle_time_min = elapsed2;
	}

	if (elapsed2 < 50) {
		cycle_count2++;
	}

	cycle_count++;
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

	while(1) {
		fun_irReceiver_task2(&receiver, on_handle_irReceiver);
	}
}
