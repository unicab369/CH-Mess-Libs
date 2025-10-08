#define IR_RECEIVER_FAST_MODE

#include "../../fun_modules/fun_irReceiver.h"

#define IR_RECEIVER_PIN PD2

#define IR_RECEIVER_PULSES_LEN 400

Threshold_Buffer_t thresholdBuf = {
	.buf_len = 5,
	.threshold_limit = IR_RECEIVER_HIGH_THRESHOLD,
};

MinMax_Info32_t minMax = {0};
u16 receive_buf[IR_RECEIVER_PULSES_LEN];
u16 receive_buf_idx;

//* PROCESS BUFFER FUNCTION
void _irReceiver_processBuffer2(IR_Receiver_t *model) {
	//! check for valid length
	if (receive_buf_idx > 0) {
		printf("\n\nState changes count: %d\n", receive_buf_idx);
		UTIL_minMax_flush(&minMax);
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
			const char *maxStr = UTIL_minMax_getStr(&minMax, value);

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

	model->prev_state = model->current_state;

	//! Reset values
	UTIL_minMax_clear(&minMax);
	memset(receive_buf, 0, sizeof(receive_buf));
	receive_buf_idx = 0;
	fun_thresholdBuffer_clear(&thresholdBuf);
}

Cycle_Info_t cycle;

//* TASK FUNCTION
void _irReceiver_task(IR_Receiver_t *model) {
	static u32 time_ref, timeout_ref;

    if (model->pin == -1) return;
    model->current_state = funDigitalRead(model->pin);
	u32 moment = micros();

	//! check for state change
    if (model->current_state != model->prev_state) {
		u16 elapsed = moment - time_ref;
		u8 addHighThreshold = fun_thresholdBuffer_addUpperValue(&thresholdBuf, elapsed);

		//# STEP 1: Filter out high thresholds
		if (!addHighThreshold) {
			if (elapsed < IR_RECEIVER_HIGH_THRESHOLD) {
				UTIL_minMax_updateMax(&minMax, elapsed);
			}
			UTIL_minMax_updateMin(&minMax, elapsed);

			//# STEP 2: collect data
			receive_buf[receive_buf_idx] = elapsed;
			receive_buf_idx++;

			//! prevent overflow
			if (receive_buf_idx >= IR_RECEIVER_PULSES_LEN) {
				printf("Max pulses reached: %d\n", receive_buf_idx);

				//# STEP 3: Process Buffer when it's full
				_irReceiver_processBuffer2(&model);
				return;
			}
		}

		time_ref = micros();
		timeout_ref = micros();
    }

	//# STEP 4: Timeout handler
    if ((micros() - timeout_ref) > IR_RECEIVER_TIMEOUT_US) {
        timeout_ref = micros();

		//! process the buffer
		_irReceiver_processBuffer2(&model);
		UTIL_cycleInfo_flush(&cycle);
    }

    model->prev_state = model->current_state;
	UTIL_cycleInfo_updateWithLimit(&cycle, micros() - moment, 50);
}


int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	printf("\r\nIR Receiver Test.\r\n");
	printf("Receiver High Threshold: %d\r\n", IR_RECEIVER_HIGH_THRESHOLD);

	IR_Receiver_t receiver = {
		.pin = IR_RECEIVER_PIN,
	};

	fun_irReceiver_init(&receiver);
	UTIL_cycleInfo_clear(&cycle);

	while(1) {
		_irReceiver_task(&receiver);
	}
}
