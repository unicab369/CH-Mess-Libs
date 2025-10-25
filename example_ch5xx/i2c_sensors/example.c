#include "ch32fun.h"
#include <stdio.h>

#include "lib_i2c_ch5xx.h" 

#define PIN_BOB PA8
#define PIN_KEVIN PA9

void read_bh1750() {
	u8 address = 0x23;

	//# power on
	u8 ret = i2c_writeData(address, (u8[]){0x01}, 1);
	if (ret != 0) {
		printf("\nERROR: I2C powerON 0x%02X\r\n", ret);
		i2c_debug_print();
		return;
	}

	//# set resolution
	ret = i2c_writeData(address, (u8[]){0x23}, 1);
	if (ret != 0) {
		printf("\nERROR: I2C resolution 0x%02X\r\n", ret);
		i2c_debug_print();
		return;
	}

	//# request reading
	ret = i2c_writeData(address, (u8[]){0x13}, 1);
	if (ret != 0) {
		printf("\nERROR: I2C request 0x%02X\r\n", ret);
		i2c_debug_print();
		return;
	}

	//# parse reading
	u8 data[2];
	ret = i2c_readData(address, data, 2);
	if (ret != 0) {
		printf("\nERROR: I2C reading 0x%02X\r\n", ret);
		i2c_debug_print();
		return;
	}

	u16 lux_raw = BUF_MAKE_U16(data);
	u16 lux = lux_raw * 12 / 10;
	printf("lux: %d\r\n", lux);
}

void read_sht3x() {
	u8 addres = 0x44;

	//# soft reset
	u8 ret = i2c_writeData(addres, (u8[]){0x30, 0xA2}, 2);
	// this command will alwasy be busy, don't check for error
	Delay_Ms(15);	//! REQUIRED

	//# config
	ret = i2c_writeData(addres, (u8[]){0x21, 0x30}, 2);
	if (ret != 0) {
		printf("\nERROR: I2C Config 0x%02X\r\n", ret);
		i2c_debug_print();
		return;
	}
	Delay_Ms(15);	//! REQUIRED

	//# parse reading
	u8 data[6];
	ret = i2c_readData(addres, data, 6);
	if (ret != 0) {
		printf("\nERROR: I2C reading 0x%02X\r\n", ret);
		i2c_debug_print();
		return;
	}

	u16 temp_raw = BUF_MAKE_U16(data);
	u16 hum_raw = (data[3] << 8) | data[4];
	u16 temp = (175 * temp_raw) >> 16;		// >> 16 is equivalent to / 65536
	u16 hum = (100 * hum_raw) >> 16;		// >> 16 is equivalent to / 65536
	printf("temp: %d, hum: %d\r\n", temp, hum);
}

int main() {
	SystemInit();
	// systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	funPinMode( PIN_BOB,   GPIO_CFGLR_OUT_10Mhz_PP ); // Set PIN_BOB to output
	funPinMode( PIN_KEVIN, GPIO_CFGLR_OUT_10Mhz_PP ); // Set PIN_KEVIN to output

	u8 err = i2c_init(100);
	printf("\nI2C init: %d\r\n", err);

	u8 status = i2c_ping(0x23);
	printf("I2C ping: %d\r\n", status);

	u8 state = 0;
	// u32 time_ref = millis();

	while(1) {
		funDigitalWrite( PIN_BOB,   FUN_HIGH ); // Turn on PIN_BOB
		funDigitalWrite( PIN_KEVIN, FUN_HIGH ); // Turn on PIN_KEVIN
		Delay_Ms( 500 );
		funDigitalWrite( PIN_BOB,   FUN_LOW );  // Turn off PIN_BOB
		funDigitalWrite( PIN_KEVIN, FUN_LOW );  // Turn off PIN_KEVIN
		Delay_Ms( 500 );

		printf("\r\n");
		read_bh1750();
		read_sht3x();

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