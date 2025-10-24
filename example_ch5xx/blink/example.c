#include "ch32fun.h"
#include <stdio.h>

#define PIN_BOB PA8
#define PIN_KEVIN PA9

int main() {
	SystemInit();
	// systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	funPinMode( PIN_BOB,   GPIO_CFGLR_OUT_10Mhz_PP ); // Set PIN_BOB to output
	funPinMode( PIN_KEVIN, GPIO_CFGLR_OUT_10Mhz_PP ); // Set PIN_KEVIN to output

	// u32 time_ref = millis();

	while(1) {
		funDigitalWrite( PIN_BOB,   FUN_HIGH ); // Turn on PIN_BOB
		funDigitalWrite( PIN_KEVIN, FUN_HIGH ); // Turn on PIN_KEVIN
		Delay_Ms( 100 );
		funDigitalWrite( PIN_BOB,   FUN_LOW );  // Turn off PIN_BOB
		funDigitalWrite( PIN_KEVIN, FUN_LOW );  // Turn off PIN_KEVIN
		Delay_Ms( 100 );

		// u32 moment = millis();

		// if (moment - time_ref > 1000) {
		// 	time_ref = moment;
		// 	printf("IM HERE\r\n");

		// 	funDigitalWrite( PIN_BOB,   state ); // Turn on PIN_BOB
		// 	funDigitalWrite( PIN_KEVIN, state ); // Turn on PIN_KEVIN
		// 	state = !state;
		// }
	}
}