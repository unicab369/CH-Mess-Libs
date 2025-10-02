// MIT License
// Copyright (c) 2025 UniTheCat

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include "fun_base.h"

typedef enum {
    GpioSensor_State_Idle,
    GpioSensor_State_Triggered,
    GpioSensor_State_Retriggered,
} GpioSensor_State_t;

typedef struct {
    int pin;
    GpioSensor_State_t state;
    u32 timeout_duration_ms;
    u32 timeRef_ms;
} GpioSensor_t;

void fun_gpioSensor_init(GpioSensor_t* model) {
    funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);

    model->state = GpioSensor_State_Idle;
    model->timeRef_ms = millis();

    if (model->timeout_duration_ms < 1) {
        model->timeout_duration_ms = 1000;
    }
}

void fun_update_state(
    GpioSensor_t* model, GpioSensor_State_t state, void (*handler)(int, u32)
) {
    u32 prev_ms = model->timeRef_ms;
    model->state = state;
    model->timeRef_ms = millis();
    handler(model->state, model->timeRef_ms - prev_ms);
}

void fun_gpioSensor_task(GpioSensor_t* model, void (*handler)(int, u32)) {
    if (model->pin < 0) return;
    u32 now = millis();
    int read = funDigitalRead(model->pin);

    switch (model->state) {
        case GpioSensor_State_Idle:
            if (read == 1) {
                fun_update_state(model, GpioSensor_State_Triggered, handler);
            }
            break;
        case GpioSensor_State_Triggered:
        case GpioSensor_State_Retriggered:
            if (read == 0) {
                fun_update_state(model, GpioSensor_State_Idle, handler);

            } else if (now - model->timeRef_ms > model->timeout_duration_ms) {
                fun_update_state(model, GpioSensor_State_Retriggered, handler);
            }
            break;
    }
}