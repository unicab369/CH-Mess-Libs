#include "ch32fun.h"
#include <stdio.h>


int main() {
    SystemInit();
    systick_init();			//! required for millis()

    funGpioInitAll();
    Delay_Ms(100);


    while(1) {

    }
}