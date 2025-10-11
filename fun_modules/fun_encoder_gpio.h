#include "ch32fun.h"
#include <stdio.h>

#define ENCODER_TIMEOUT_MS 50       // too big will cause slow outputs
#define ENCODER_DEBOUNCE_TIME_MS 2  // too big will cause bad readings

typedef struct {
    u8 clk_pin;
    u8 dt_pin;
    u8 value;
    u8 last_clk_state;
    u8 last_dt_state;
    u32 last_update_time;
    u32 last_debounce_time;
} Encoder_GPIO_t;


void fun_encoder_gpio_init(Encoder_GPIO_t *model) {
    funPinMode(model->clk_pin, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(model->clk_pin, 1);

    funPinMode(model->dt_pin, GPIO_CFGLR_IN_PUPD);
	funDigitalWrite(model->dt_pin, 1);

    model->last_clk_state = funDigitalRead(model->clk_pin);
    model->last_dt_state = funDigitalRead(model->dt_pin);
}

void fun_encoder_gpio_task(u32 time, Encoder_GPIO_t *model, void (*handler)(u8, int8_t)) {
    //# Ignore changes that happen too quickly (debouncing)
    if (time - model->last_debounce_time < ENCODER_DEBOUNCE_TIME_MS) return;

    u8 clk_state = funDigitalRead(model->clk_pin);
    u8 dt_state = funDigitalRead(model->dt_pin);

    //# STEP 1: Check for no state change
    if (clk_state == model->last_clk_state && dt_state == model->last_dt_state) return;

    //# STEP 2: Handle readings
    if (clk_state != model->last_clk_state) {
        int8_t direction = clk_state == dt_state ? -1 : 1;
        model->value = model->value + direction;

        //# STEP 3:Trigger the callback if enough time has passed since the last update
        if (time - model->last_update_time > ENCODER_TIMEOUT_MS) {
            model->last_update_time = time;
            handler(model->value, direction);
        }
    }

    //# Update state and the debounce time
    model->last_debounce_time = time;
    model->last_clk_state = clk_state;
    model->last_dt_state = dt_state;
}