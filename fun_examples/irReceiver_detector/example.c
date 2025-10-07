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
#define IR_SENDER_LOGICAL_1_US 440
#define IR_SENDER_LOGICAL_0_US 200

#define IR_MAX_PULSES2 900

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

#define IR_DMA_IN	DMA1_Channel2
#define IR_DMA_IN2  DMA1_Channel5 

// //! For generating the mask/modulus, this must be a power of 2 size.
// uint16_t ir_fallingEdge_buff[IR_MAX_PULSES2];
// uint16_t ir_risingEdge_buff[IR_MAX_PULSES2];

// void fun_irReceiver_init3(IRReceiver_t* model) {
// 	funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
// 	funDigitalWrite(model->pin, 1);
// 	model->ir_started = 0;

// 	// Enable GPIOs
// 	RCC->APB2PCENR |= RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO;
// 	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

// 	//# TIM1_TRIG/TIM1_COM/TIM1_CH4
// 	IR_DMA_IN->MADDR = (u32)ir_fallingEdge_buff;
// 	IR_DMA_IN->PADDR = (u32)&TIM1->CH1CVR; // Input
// 	IR_DMA_IN->CFGR = 
// 		0					|				// PERIPHERAL to MEMORY
// 		0					|				// Low priority.
// 		DMA_CFGR1_MSIZE_0	|				// 16-bit memory
// 		DMA_CFGR1_PSIZE_0	|				// 16-bit peripheral
// 		DMA_CFGR1_MINC		|				// Increase memory.
// 		DMA_CFGR1_CIRC		|				// Circular mode.
// 		0					|				// NO Half-trigger
// 		0					|				// NO Whole-trigger
// 		DMA_CFGR1_EN;						// Enable
// 	IR_DMA_IN->CNTR = IR_MAX_PULSES;


// 	IR_DMA_IN2->MADDR = (u32)ir_risingEdge_buff;
// 	IR_DMA_IN2->PADDR = (u32)&TIM1->CH2CVR; // Input
// 	IR_DMA_IN2->CFGR = 
// 		0                 |                  // PERIPHERAL to MEMORY
// 		0                 |                  // Low priority.
// 		DMA_CFGR1_MSIZE_0 |                  // 16-bit memory
// 		DMA_CFGR1_PSIZE_0 |                  // 16-bit peripheral
// 		DMA_CFGR1_MINC    |                  // Increase memory.
// 		DMA_CFGR1_CIRC    |                  // Circular mode.
// 		0                 |                  // NO Half-trigger
// 		0                 |                  // NO Whole-trigger
// 		DMA_CFGR1_EN;                        // Enable
// 	IR_DMA_IN2->CNTR = IR_MAX_PULSES;


// 	TIM1->PSC = 0x0100;		// set TIM1 clock prescaler divider (Massive prescaler)
// 	TIM1->ATRLR = 0xFFFF;	// set PWM total cycle width

// 	//# Tim 1 input / capture (CC1S = 01) / t1c2 / capture input CC2S = 10)
// 	TIM1->CHCTLR1 = TIM_CC1S_0 | TIM_CC2S_1;

// 	//# Set CC1 and CC2 to have opposite polarities.
// 	TIM1->CCER = TIM_CC1E | TIM_CC2E | TIM_CC2P;


// 	// Setup slave mode for tim1 input to go to t1c2
// 	// 010: encoder mode 2, where the core counter
// 	// increments or decrements the count at the edge of
// 	// TI1FP1, depending on the level of TI2FP2.
// 	TIM1->SMCFGR = TIM_TS_0 | TIM_TS_2 | TIM_SMS_2;

// 	//# initialize counter and start
// 	TIM1->SWEVGR = TIM_UG;
// 	TIM1->CTLR1 = TIM_ARPE | TIM_CEN;

// 	//# Enable DMA for T1CC1 + T1CC2
// 	TIM1->DMAINTENR = TIM_CC1DE | TIM_CC2DE | TIM_UDE;
// }

u16 word_buf[100];

void fun_irReceiver_task2(IRReceiver_t* model, void (*handler)(u16*, u8)) {
    if (model->pin == -1) return;
    model->current_pinState = funDigitalRead(model->pin);

    if (model->current_pinState != model->prev_pinState) {
		//! get time difference
		u16 elapsed = ROUND_TO_NEAREST_10(micros() - IRAnalyzer.timeRef);

		if (elapsed < IR_RECEIVER_HIGH_THRESHOLD) {
			//! collect data
			IRAnalyzer.buf[IRAnalyzer.buf_idx] = elapsed;

			if (elapsed < IR_RECEIVER_HIGH_THRESHOLD && elapsed > IRAnalyzer.maxValue) {
				IRAnalyzer.maxValue = elapsed;
			}
			if (elapsed < IRAnalyzer.minValue) IRAnalyzer.minValue = elapsed;

			IRAnalyzer.buf_idx++;
		}
		else {
			IRAnalyzer.highthreshold_buf[IRAnalyzer.highthreshold_idx] = elapsed;
			IRAnalyzer.highthreshold_idx++;
		}

		IRAnalyzer.timeRef = micros();
    }

	//! timeout after 3 seconds
    if ((micros() - IRAnalyzer.timeoutRef) > 1000000) {
        IRAnalyzer.timeoutRef = micros();

		if (IRAnalyzer.buf_idx > 0) {
			printf("\n\nState counts: %d\n", IRAnalyzer.buf_idx);
			printf("maxValue: %d, minValue: %d\n\n", IRAnalyzer.maxValue, IRAnalyzer.minValue);

			for (int i = 0; i < IRAnalyzer.highthreshold_idx; i++) {
				// print and clear
				printf("High Thres: %d\n", IRAnalyzer.highthreshold_buf[i]);
				IRAnalyzer.highthreshold_buf[i] = 0;
			}

			//! print the header
			printf("\n%5s %3s | %7s | %3s | %7s | %7s | %7s \n",
					"", "idx", "Value", "Bit", "Delta_1", "Delta_0", "diff");

			for (u16 i = 0; i < IRAnalyzer.buf_idx; i++) {
				u32 value = IRAnalyzer.buf[i];
				u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - value);
				u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - value);
				u16 delta_diff = abs(delta_1 - delta_0);
				int bit = delta_1 < delta_0;

				// find how close the value is to it's 
				const char *bitStr = bit ? "1" : ".";
				const char *maxStr = (value == IRAnalyzer.maxValue) ? "(max)" :
									(value == IRAnalyzer.minValue) ? "(min)" : "";

				//! print out the values
				printf("%5s %3d | %7d | %3s | %7d | %7d | %7d \n",
					maxStr, i, value, bitStr, delta_1, delta_0, delta_diff);

				// separator
				if (i % 8 == 7) printf("\n");
			}

			u16 word = 0x0000;
			printf("\nDecoded words:\n");

			for (u16 i = 0; i < IRAnalyzer.buf_idx; i++) {
				u32 value = IRAnalyzer.buf[i];
				u16 delta_1 = abs(IR_SENDER_LOGICAL_1_US - value);
				u16 delta_0 = abs(IR_SENDER_LOGICAL_0_US - value);
				int bit = delta_1 < delta_0;

				u32 bit_pos = 15 - (i & 0x0F);  // &0x0F is %16
				if (bit) word |= 1 << bit_pos;		// MSB first (reversed)

				//! printout words
				if (bit_pos == 0) {
					printf("[%d] 0x%04X ", i, word);
					word = 0x0000;		// reset word

					// separator
					if ((i+1)%128 == 0) printf("\n");
				}
			}

			printf("\r\n");
		}

		//! Reset values
		IRAnalyzer.maxValue = 0;
        IRAnalyzer.minValue = 0xFFFF;
		IRAnalyzer.buf_idx = 0;
		IRAnalyzer.highthreshold_idx = 0;
		memset(IRAnalyzer.buf, 0, sizeof(IRAnalyzer.buf));
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


	// fun_irReceiver_init3(&receiver);

	// int tail = 0;
	// int frame = 0;

	// while(1) {
	// 	// Must perform modulus here, in case DMA_IN->CNTR == 0.
	// 	int head1 = (IR_MAX_PULSES - IR_DMA_IN->CNTR) & (IR_MAX_PULSES-1);
	// 	int head2 = (IR_MAX_PULSES - IR_DMA_IN2->CNTR) & (IR_MAX_PULSES-1);
	// 	int head = head2;
	// 	// Just in case head
	// 	if( head1 == head2-1 ) head = head1;
	// 	else if( head == head2 || head2 == head1-1 );
	// 	else {
	// 		// Should never ever be possible.
	// 		printf( "Timer DMA Fault\n" );
	// 		continue;
	// 	}

	// 	while( head != tail ) {
	// 		uint32_t time_of_event0 = ir_fallingEdge_buff[tail];
	// 		uint32_t time_of_event1 = ir_risingEdge_buff[tail];
	// 		printf( "A %d, %d\n", (int)time_of_event0, (int)time_of_event1 );

	// 		// Performs modulus to loop back.
	// 		tail = (tail+1)&(IR_MAX_PULSES-1);
	// 	}

	// 	funDigitalWrite( PD3, 1 );
	// 	Delay_Ms(frame & 31);
	// 	funDigitalWrite( PD3, 0 );
	// 	printf( "." );

	// 	frame++;
	// }
}
