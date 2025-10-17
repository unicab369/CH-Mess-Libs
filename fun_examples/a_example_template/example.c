#include "ch32fun.h"
#include <stdio.h>

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	u32 moment = millis();

	while(1) {

	}
}