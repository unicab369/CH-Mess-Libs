 //! OVERWRITE
#include "../../fun_modules/fun_irReceiver.h"

#define IR_RECEIVER_PIN PD2

// #define IR_RECEIVER_HIGH_THRESHOLD 2000
// #define IR_SENDER_LOGICAL_1_US 1600
// #define IR_SENDER_LOGICAL_0_US 560

#define IR_RECEIVER_HIGH_THRESHOLD 800
#define IR_SENDER_LOGICAL_1_US 550
#define IR_SENDER_LOGICAL_0_US 300

#define IR_MAX_PULSES2 400

Threshold_Buffer_t thresholdBuf = {
	.buf_len = 5,
	.threshold_limit = IR_RECEIVER_HIGH_THRESHOLD,
};

MinMax_Info32_t minMax = {0};
u16 receive_buf[IR_MAX_PULSES2];
u16 receive_buf_idx;

void _irReceiver_processBuffer(IRReceiver_t* model) {
	//! check for valid length
	if (receive_buf_idx > 0) {
		printf("\n\nState changes count: %d\n", receive_buf_idx);
		fun_minMax32_flush(&minMax);
		fun_thresholdBuffer_flush(&thresholdBuf);

		//! print the header
		printf("\n%5s %3s | %7s | %3s | %7s | %7s | %7s \n",
				"", "idx", "Value", "Bit", "Delta_1", "Delta_0", "diff");

		u16 word = 0x0000;

		for (u16 i = 0; i < receive_buf_idx; i++) {
			u32 value = receive_buf[i];
			u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - value);
			u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - value);
			u16 delta_diff = abs(delta_1 - delta_0);
			int bit = delta_1 < delta_0;

			// find how close the value is to it's 
			const char *bitStr = bit ? "1" : ".";
			const char *maxStr = fun_minMax32_getStr(&minMax, value);

			//! print out the values
			printf("%5s %3d | %7d | %3s | %7d | %7d | %7d \n",
				maxStr, i, value, bitStr, delta_1, delta_0, delta_diff);

			//! printout words
			u32 bit_pos = 15 - (i & 0x0F);  	// &0x0F is %16
			if (bit) word |= 1 << bit_pos;		// MSB first (reversed)

			if (bit_pos == 0) {
				printf("0x%04X ", word);
				word = 0x0000;		// reset word
			}

			// separator
			if (i % 8 == 7) printf("\n");
		}

		printf("\r\n");
	}

	model->prev_pinState = model->current_pinState;

	//! Reset values
	fun_minMax32_clear(&minMax);
	memset(receive_buf, 0, sizeof(receive_buf));
	receive_buf_idx = 0;
	fun_thresholdBuffer_clear(&thresholdBuf);
}

Cycle_Info_t cycle;

void _irReceiver_task(IRReceiver_t* model) {
	static u32 time_ref, timeout_ref;

    if (model->pin == -1) return;
    model->current_pinState = funDigitalRead(model->pin);
	u32 moment = micros();

	//! chec for state change
    if (model->current_pinState != model->prev_pinState) {
		u16 elapsed = moment - time_ref;
		u8 addHighThreshold = fun_thresholdBuffer_addUpperValue(&thresholdBuf, elapsed);

		//# STEP 1: Filter out high thresholds
		if (!addHighThreshold) {
			if (elapsed < IR_RECEIVER_HIGH_THRESHOLD) {
				fun_minMax32_updateMax(&minMax, elapsed);
			}
			fun_minMax32_updateMin(&minMax, elapsed);

			//! collect data
			receive_buf[receive_buf_idx] = elapsed;
			receive_buf_idx++;

			//! prevent overflow
			if (receive_buf_idx >= IR_MAX_PULSES2) {
				printf("Max pulses reached: %d\n", receive_buf_idx);

				//# STEP 2: Process Buffer when it's full
				_irReceiver_processBuffer(&model);
				return;
			}
		}

		time_ref = micros();
		timeout_ref = micros();
    }

	//# STEP 3: Timeout handler
    if ((micros() - timeout_ref) > 500000) {
        timeout_ref = micros();

		//# flush the buffer
		_irReceiver_processBuffer(&model);
		fun_cycleInfo_flush(&cycle);

		//! Reset values
		fun_minMax32_clear(&minMax);
		memset(receive_buf, 0, sizeof(receive_buf));
		receive_buf_idx = 0;
		fun_thresholdBuffer_clear(&thresholdBuf);
    }

    model->prev_pinState = model->current_pinState;

	fun_cycleInfo_updateWithLimit(&cycle, micros() - moment, 50);
}


int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()

	printf("\r\nIR Receiver Test.\r\n");
	Delay_Ms(100);
	funGpioInitAll();

	IRReceiver_t receiver = {
		.pin = IR_RECEIVER_PIN,
	};

	fun_irReceiver_init(&receiver);
	fun_cycleInfo_clear(&cycle);

	while(1) {
		_irReceiver_task(&receiver);
	}
}
