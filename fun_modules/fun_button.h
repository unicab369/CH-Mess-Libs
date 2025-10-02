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



#include "ch32fun.h"
#include <stdio.h>

#define TICK_DEBOUNCE_DUR     20
#define TICK_CLICK_DUR        160
#define TICK_LONG_PRESS_DUR   4000

#ifndef millis
    #define millis()
#endif

enum {
    BTN_DOWN = 0,
    BTN_UP,
    BTN_DOWN2,
    BUTTON_IDLE
};

typedef enum {
    BTN_SINGLECLICK = 0x01,
    BTN_DOUBLECLICK = 0x02,
    BTN_LONGPRESS = 0x03
} Button_Event_e;


typedef struct {
    uint8_t pin;
    uint8_t btn_state;
    uint32_t debounce_time;
    uint32_t release_time;
    uint32_t press_time;
} Button_t;

void _reset_timers(uint8_t newState, Button_t *model) {
    model->btn_state = newState;
    model->debounce_time = millis();
    model->release_time = millis();
}

void fun_button_setup(Button_t *model) {
    if (model->pin == 0xFF) return; 

    model->btn_state = BUTTON_IDLE;
    model->debounce_time = 0;
    model->release_time = 0;
    model->press_time = 0;

    funPinMode(model->pin, GPIO_CFGLR_IN_PUPD);
    funDigitalWrite(model->pin, 1);
    _reset_timers(BUTTON_IDLE, model);
}

void fun_button_task(uint32_t time, Button_t *model, void (*handler)(int, uint32_t)) {
    if (model->pin == 0xFF) return;
    uint8_t read = funDigitalRead(model->pin);

    // Debounce check
    if (time - model->debounce_time < TICK_DEBOUNCE_DUR) return;
    model->debounce_time = time;

    switch (model->btn_state) {
    case BUTTON_IDLE:
        if (read == 0) {
            model->press_time = time;
            _reset_timers(BTN_DOWN, model);      // First Press  
        }
        break;

    case BTN_DOWN:
        if (read > 0) {
            _reset_timers(BTN_UP, model);        // First Release

        } else {
            // Long press detection
            uint32_t press_duration = time - model->press_time;
            if (press_duration > TICK_LONG_PRESS_DUR) {
            handler(BTN_LONGPRESS, press_duration - TICK_LONG_PRESS_DUR);
            }
        }
        break;

    case BTN_UP: {
        uint32_t release_duration = time - model->release_time;

        if (read == 0 && release_duration < TICK_CLICK_DUR) {
            // Second Press in less than TICK_CLICK_DUR
            _reset_timers(BTN_DOWN2, model);

        } else if (release_duration > TICK_CLICK_DUR) {
            handler(BTN_SINGLECLICK, 0);  // Single Click
            _reset_timers(BUTTON_IDLE, model);
        }
        break;
    }

    case BTN_DOWN2:
        // Second release
        if (read > 0) {
            handler(BTN_DOUBLECLICK, 0);  // Double Click
            _reset_timers(BUTTON_IDLE, model);
        }
        break;
    }
}