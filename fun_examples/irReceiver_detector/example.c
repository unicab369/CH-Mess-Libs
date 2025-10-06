//! OVERWRITE
#define IRRECEIVER_PULSE_THRESHOLD_US 450

#define IR_LOGICAL_HIGH_THRESHOLD 150
#define IR_OUTLINER_THRESHOLD 50

#include "../../fun_modules/fun_irReceiver.h"


#define IR_RECEIVER_PIN PD2

#define ROUND_TO_NEAREST_10(x)  ((x + 5) / 10 * 10)

void on_handle_irReceiver(u16 *data, u16 len) {

}


#define IR_RECEIVER_HIGH_THRESHOLD 800
#define IR_SENDER_LOGICAL_1_US 440
#define IR_SENDER_LOGICAL_0_US 200

u32 IRReceive_buf[50];
u16 IRReceive_idx = 0;
u32 maxValue = 0;
u32 minValue = 0xFFFF;


void fun_irReceiver_task2(IRReceiver_t* model, void (*handler)(u16*, u8)) {
    static u32 timeRef = 0;
    static u32 timeoutRef = 0;

    if (model->pin == -1) return;
    model->current_pinState = funDigitalRead(model->pin);

    if (model->current_pinState != model->prev_pinState) {
		//! get time difference
		u16 elapsed = ROUND_TO_NEAREST_10(micros() - timeRef);

		if (elapsed < IR_RECEIVER_HIGH_THRESHOLD) {
			IRReceive_buf[IRReceive_idx] = elapsed;

			if (elapsed < IR_RECEIVER_HIGH_THRESHOLD && elapsed > maxValue) {
				maxValue = elapsed;
			}
			if (elapsed < minValue) minValue = elapsed;

			IRReceive_idx++;
		}

		timeRef = micros();
    }

	//! timeout for 3 seconds
    if ((micros() - timeoutRef) > 3000000) {
        timeoutRef = micros();

		if (IRReceive_idx > 0) {
			printf("\n\nState counts: %d\n", IRReceive_idx);
			printf("maxValue: %d, minValue: %d\n\n", maxValue, minValue);

			//! print the header
			printf("%5s %3s | %8s | %3s | %8s | %8s | %8s\n",
					"", "idx", "Value", "Bit", "Delta_1", "Delta_0", "diff");

			for (u16 i = 0; i < IRReceive_idx; i++) {
				// int bit = IRReceive_buf[i] > 270;

				u32 value = IRReceive_buf[i];
				u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - value);
				u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - value);
				u16 delta_diff = abs(delta_1 - delta_0);
				int bit = delta_1 < delta_0;

				// find how close the value is to it's 
				const char *bitStr = bit ? "1" : ".";
				const char *maxStr = (value == maxValue) ? "(max)" :
									(value == minValue) ? "(min)" : "";
				
				//! print out the values
				printf("%5s %3d | %8d | %3s | %8d | %8d | %8d\n",
					maxStr, i, value, bitStr, delta_1, delta_0, delta_diff);
				if (i % 8 == 7) printf("\n");
			}
			printf("\r\n");
		}

		//! Reset values
		maxValue = 0;
        minValue = 0xFFFF;
		IRReceive_idx = 0;
		memset(IRReceive_buf, 0, sizeof(IRReceive_buf));
    }

    model->prev_pinState = model->current_pinState;
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
