#include "ch32fun.h"
#include <stdio.h>

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	u32 time_ref = millis();

	while(1) {

	}
}