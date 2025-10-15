#include "ch32fun.h"
#include <stdio.h>

// #define ENCODER_LOG_ENABLE

#define ENCODER_TIMEOUT_MS 50       // too big will cause slow outputs
#define ENCODER_DEBOUNCE_TIME_MS 2  // too big will cause bad readings

typedef struct {
    u8 pinA, pinB;
    int8_t pos;
    int8_t tick_count;
    u8 last_state;
    u32 last_update_time;
    int8_t last_pos;

    // u8 last_pinA_state;
    // u8 last_pinB_state;
} Encoder_GPIO_t;


void fun_encoder_gpio_init(Encoder_GPIO_t *model) {
    funPinMode(model->pinA, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(model->pinA, 1);

    funPinMode(model->pinB, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(model->pinB, 1);

    // model->last_pinA_state = funDigitalRead(model->pinA);
    // model->last_pinB_state = funDigitalRead(model->pinB);
}

// void fun_encoder_gpio_task(u32 time, Encoder_GPIO_t *model, void (*handler)(int8_t, int8_t)) {
//     u8 pinA_state = funDigitalRead(model->pinA);
//     u8 pinB_state = funDigitalRead(model->pinB);

//     //# STEP 1: Check for no state change
//     if (pinA_state == model->last_pinA_state && pinB_state == model->last_pinB_state) return;

//     //# STEP 2: Handle readings
//     if (pinA_state != model->last_pinA_state) {
//         int8_t direction = pinA_state == pinB_state ? -1 : 1;
//         model->pos = model->pos + direction;

//         //# STEP 3:Trigger the callback if enough time has passed since the last update
//         if (time - model->last_update_time > ENCODER_TIMEOUT_MS) {
//             model->last_update_time = time;
//             handler(model->pos, direction);
//         }
//     }

//     //# Update state and the debounce time
//     model->last_pinA_state = pinA_state;
//     model->last_pinB_state = pinB_state;
// }


void fun_encoder_gpio_task(u32 time, Encoder_GPIO_t *model, void (*handler)(int8_t, int8_t)) {
    u8 curr_state = funDigitalRead(model->pinA)<<1 | funDigitalRead(model->pinB);
    u8 combined = model->last_state << 2 | curr_state;
    int8_t compensate = 0;

    switch(combined) {
        case 0b0001: case 0b0111: case 0b1000: case 0b1110: compensate = 1; break;
        case 0b0010: case 0b0100: case 0b1011: case 0b1101: compensate = -1; break;
    }

    if (compensate != 0) {
        model->tick_count += compensate;
        model->last_state = curr_state;

        if (time - model->last_update_time > ENCODER_TIMEOUT_MS 
            && model->pos != model->last_pos
        ) {
            model->last_update_time = time;

            int8_t direction = (model->pos > model->last_pos) ? 1 : -1;
            handler(model->pos, direction);
            model->last_pos = model->pos;

            #ifdef ENCODER_LOG_ENABLE
                printf("pos: %d, direction: %d\n", model->pos, direction);
            #endif
        }
    }

    //reset and increment only when encoder has returned home (detent)
    if (curr_state == 0b11) {
        if ((model->tick_count >= 4)){
            model->tick_count = 0;
            model->pos--;
        }
        if ((model->tick_count <= -4)){
            model->tick_count = 0;
            model->pos++;
        }
    }
}