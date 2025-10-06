//! OVERWRITE
#define IRRECEIVER_PULSE_THRESHOLD_US 450

#define IR_LOGICAL_HIGH_THRESHOLD 150
#define IR_OUTLINER_THRESHOLD 50

#include "../../fun_modules/fun_irReceiver.h"


#define IR_RECEIVER_PIN PD2


void on_handle_irReceiver(u16 *data, u16 len) {

}

void fun_irReceiver_task2(IRReceiver_t* model, void (*handler)(u16*, u8)) {
    static u32 timeRef = 0;
    static u32 timeoutRef = 0;

    if (model->pin == -1) return;

    model->current_pinState = funDigitalRead(model->pin);

    if (model->current_pinState != model->prev_pinState) {
        u32 elapsed = (u32)(micros() - timeRef);
        printf("elapsed: %lu, state: %d\n", (unsigned long)elapsed, model->current_pinState);
        timeRef = micros();
    }

    if ((micros() - timeoutRef) > 3000000) {
        timeoutRef = micros();
        printf("timeout\n");
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
