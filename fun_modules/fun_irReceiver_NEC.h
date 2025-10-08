#include "fun_base.h"
#include "fun_crc.h"

// #define IR_RECEIVER_NEC_USE_DMA

#define IR_DECODE_TIMEOUT_US 100000		// 100ms
#define IR_NEC_BUFFER_LEN 4

//# Debug Mode: 0 = disable, 1 = log level 1, 2 = log level 2
#define IR_RECEIVER_DEBUGLOG 0

//# for DMA
#define IR_NEC_LOGICAL_1_THRESHOLD_TICK 300
#define IR_NEC_START_PULSE_THRESHOLD_TICK 700

//# for GPIO
#define IR_PULSE_BUF_LEN 64

typedef struct {
	int pin;
	u8 ir_started;
	u16 BUFFER[IR_NEC_BUFFER_LEN];	// store the received data

	int16_t counterIdx;			// store pulses count (for gpio) Or tails (for DMA)
	u32 bits_processed;			// number of bits processed
	u32 time_ref;

    // for GPIO ONLY
    u32 pulse_buf[64];
    u8 prev_state, current_state;
} IR_ReceiverNEC_t;


#ifdef IR_RECEIVER_NEC_USE_DMA
    // TIM2_CH1 -> PD2 -> DMA1_CH2
    #define IR_DMA_IN	DMA1_Channel2

    //! ####################################
    //! SETUP FUNCTION USIND DMA
    //! ####################################

    //! For generating the mask/modulus, this must be a power of 2 size.
    #define IR_MAX_PULSES 128
    #define TIM1_BUFFER_LAST_IDX (IR_MAX_PULSES - 1)
    uint16_t ir_ticks_buff[IR_MAX_PULSES];

    //* INIT FUNCTION
    void fun_irReceiver_NEC_init(IR_ReceiverNEC_t* model) {
        printf("IRReceiver DMA init\n");
        funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
        funDigitalWrite(model->pin, 1);
        model->ir_started = 0;

        // Enable GPIOs
        RCC->APB2PCENR |= RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO;
        RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

        //# TIM1_TRIG/TIM1_COM/TIM1_CH4
        IR_DMA_IN->MADDR = (uint32_t)ir_ticks_buff;
        IR_DMA_IN->PADDR = (uint32_t)&TIM1->CH1CVR; // Input
        IR_DMA_IN->CFGR = 
            0					|				// PERIPHERAL to MEMORY
            0					|				// Low priority.
            DMA_CFGR1_MSIZE_0	|				// 16-bit memory
            DMA_CFGR1_PSIZE_0	|				// 16-bit peripheral
            DMA_CFGR1_MINC		|				// Increase memory.
            DMA_CFGR1_CIRC		|				// Circular mode.
            0					|				// NO Half-trigger
            0					|				// NO Whole-trigger
            DMA_CFGR1_EN;						// Enable
        IR_DMA_IN->CNTR = IR_MAX_PULSES;

        TIM1->PSC = 0x00DF;		// set TIM1 clock prescaler divider (Massive prescaler)
        TIM1->ATRLR = 65535;	// set PWM total cycle width

        //# Tim 1 input / capture (CC1S = 01)
        TIM1->CHCTLR1 = TIM_CC1S_0;

        //# Add here CC1P to switch from UP->GOING to DOWN->GOING log times.
        TIM1->CCER = TIM_CC1E; // | TIM_CC1P;
        
        //# initialize counter
        TIM1->SWEVGR = TIM_UG;

        //# Enable TIM1
        TIM1->CTLR1 = TIM_ARPE | TIM_CEN;
        TIM1->DMAINTENR = TIM_CC1DE | TIM_UDE; // Enable DMA for T1CC1
    }


    //! ####################################
    //! RECEIVE FUNCTION USING DMA
    //! ####################################

    //* PROCESS BUFFER FUNCTION
    void _irReceiver_NEC_flush(IR_ReceiverNEC_t* model, void(*handler)(u16*, u8)) {
        handler(model->BUFFER, IR_NEC_BUFFER_LEN);

        //# clear out ir data
        memset(model->BUFFER, 0, IR_NEC_BUFFER_LEN * sizeof(model->BUFFER[0]));
        model->bits_processed = 0;   
    }

    //* TASK FUNCTION
    void fun_irReceiver_NEC_task(IR_ReceiverNEC_t* model, void(*handler)(u16*, u8)) {
        // Must perform modulus here, in case DMA_IN->CNTR == 0.
        int head = (IR_MAX_PULSES - IR_DMA_IN->CNTR) & TIM1_BUFFER_LAST_IDX;

        if( head != model->counterIdx ) {
            u32 time_of_event = ir_ticks_buff[model->counterIdx];
            u8 prev_event_idx = (model->counterIdx-1) & TIM1_BUFFER_LAST_IDX;		// modulus to loop back
            u32 prev_time_of_event = ir_ticks_buff[prev_event_idx];
            u32 elapsed = time_of_event - prev_time_of_event;

            //! Performs modulus to loop back.
            model->counterIdx = (model->counterIdx+1) & TIM1_BUFFER_LAST_IDX;

            //! Timer overflow handling
            if (time_of_event >= prev_time_of_event) {
                elapsed = time_of_event - prev_time_of_event;
            } else {
                // Timer wrapped around (0xFFFF -> 0x0000)
                elapsed = (65536 - prev_time_of_event) + time_of_event;
            }

            if (model->ir_started) {
                //# STEP 2: filter LOW start frame (> 1000 ticks)
                if (elapsed > IR_NEC_START_PULSE_THRESHOLD_TICK) {
                    // printf("\nFrame LOW: %d\n", elapsed);
                    return;
                }
                // printf("elapsed: %d\n", elapsed);

                int bit_pos = 15 - (model->bits_processed & 0x0F);  // bits_processed % 16
                int word_idx = model->bits_processed >> 4;          // bits_processed / 16
                model->bits_processed++;

                //# STEP 3: Collect data, filter for Logical HIGH
                if (elapsed > IR_NEC_LOGICAL_1_THRESHOLD_TICK) {
                    model->BUFFER[word_idx] |= (1 << bit_pos);		// MSB first (reversed)
                }

                //# STEP 4: Complete frame
                if (bit_pos == 0 && word_idx == 3) _irReceiver_NEC_flush(model, handler);
            }

            //# STEP 1: filter HIGH start frame. Expect > 1200 ticks
            else if (elapsed > IR_NEC_START_PULSE_THRESHOLD_TICK) {
                // printf("\nFrame HIGH: %d\n", elapsed);
                model->ir_started = 1;
                model->bits_processed = 0;
            }

            model->time_ref =  micros();
        }

        //# STEP 5: handle timeout
        if ((model->ir_started) && 
            (( micros() - model->time_ref) > IR_DECODE_TIMEOUT_US)
        ) {
            // printf("Timeout %d\n", micros() - model->time_ref);
            _irReceiver_NEC_flush(model, handler);

            model->ir_started = 0;
        }
    }

#else
	// low state: average 500us-600us
	// high state: average 1500us-1700us
	// estimated threshold: greater than 1000 for high state
	#define IR_NEC_LOGICAL_1_THRESHOLD_US 1000
    #define IR_NEC_START_PULSE_THRESHOLD_US 3000

    //* INIT FUNCTION
	void fun_irReceiver_NEC_init(IR_ReceiverNEC_t* model) {
		printf("IRReceiver GPIO init\n");
		funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
		funDigitalWrite(model->pin, 1);

		//! restart states
		model->ir_started = 0;
		model->current_state = model->prev_state = 0;
	}

	//! ####################################
	//! RECEIVE FUNCTIONS USING GPIO
	//! ####################################

    //* PROCESS BUFFER FUNCTION
    void _irReceiver_NEC_flush(IR_ReceiverNEC_t* model, void (*handler)(u16*, u8)) {
        // even elements are low signals (logical spacing)
        for (int i = 0; i < model->counterIdx; i += 1) {
            int word_idx = model->bits_processed >> 4;     		// >>4 is /16
            int bit_pos = 15 - (model->bits_processed & 0x0F);  // &0x0F is %16
            model->bits_processed++;

            // printf("puslse buf: %d\n", model->pulse_buf[i]);

            //! collect the data - MSB first (reversed)
            int bit =  model->pulse_buf[i] > IR_NEC_LOGICAL_1_THRESHOLD_US;
            if (bit) model->BUFFER[word_idx] |= 1 << bit_pos;

            //! IMPORTANTE: printf statements introduce delays
            #if IR_RECEIVER_DEBUGLOG == 1
            	const char *bitStr = bit ? "1" : ".";
                printf("%d \t %s \t 0x%04X D%d\n",
                    model->pulse_buf[i], bitStr, model->BUFFER[word_idx], bit_pos);
                // separator
                if (bit_pos % 8 == 0) printf("\n");
            #endif
        }

        //# STEP 4: handle completed packet
        handler(model->BUFFER, IR_NEC_BUFFER_LEN);

        //! clear data
		memset(model->BUFFER, 0, IR_NEC_BUFFER_LEN * sizeof(model->BUFFER[0]));
        model->counterIdx = model->bits_processed = 0;
    }

	//* TASK FUNCTION
	void fun_irReceiver_NEC_task(IR_ReceiverNEC_t* model, void (*handler)(u16*, u8)) {
		if (model->pin == -1) return;
		model->current_state = funDigitalRead(model->pin);

		//# STEP 1: wait for first pulse
		if (!model->ir_started && !model->current_state) {
			model->ir_started = 1;

			//! clear pulses count
			model->counterIdx = 0;
			model->time_ref = micros();
		}

		//# STEP 2: detecting start signals
		else if (model->ir_started == 1) {
            //# State changed
			if (model->current_state != model->prev_state) {
				model->pulse_buf[model->counterIdx] = micros() - model->time_ref;
				model->counterIdx++;
				model->time_ref = micros();

				//! IMPORTANTE: printf statements introduce delays
				//! start pulses: 9ms HIGH, 4.5ms LOW
				if (model->counterIdx == 2) {
					if (model->pulse_buf[0] > IR_NEC_START_PULSE_THRESHOLD_US || 
                        model->pulse_buf[1] > IR_NEC_START_PULSE_THRESHOLD_US
                    ) {
						model->ir_started = 2;	// start handling data pulses
					} else {
						model->ir_started = 0;	// restart
					}

					//! clear data
                    memset(model->BUFFER, 0, IR_NEC_BUFFER_LEN * sizeof(model->BUFFER[0]));
                    model->counterIdx = model->bits_processed = 0;
				}
			}
		}

		//# STEP 3: handling data pulses
		else if (model->ir_started == 2) {
            //# State changed
			if (model->current_state != model->prev_state) {
				if (!model->current_state) {
                    //! only record low signals (logical spacing)
					model->pulse_buf[model->counterIdx] = micros() - model->time_ref;
					model->counterIdx++;
				}
				model->time_ref = micros();
				
				//! enough data to collect
				if (model->counterIdx >= 64) _irReceiver_NEC_flush(model, handler);
			}

			//# STEP 5: handle outout - restart states
			else if (micros() - model->time_ref > IR_DECODE_TIMEOUT_US) {
                _irReceiver_NEC_flush(model, handler);

                //! restart states
                model->ir_started = 0;
                model->current_state = model->prev_state = 0;
			}	
		}

        model->prev_state = model->current_state;
	}

#endif