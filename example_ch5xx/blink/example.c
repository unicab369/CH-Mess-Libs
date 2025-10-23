#include "ch32fun.h"
#include <stdio.h>

#define PIN_BOB PA8
#define PIN_KEVIN PA9
#define SYSTEM_CLOCK_MHZ 60

#define I2C_SDA PB12
#define I2C_SCL PB13

void i2c_init() {
	R16_I2C_CTRL2 = (SYSTEM_CLOCK_MHZ & RB_I2C_FREQ);
	// R16_I2C_CKCFGR	leave alone
	R16_I2C_RTR = 35;

	R16_I2C_CTRL1 = RB_I2C_PE;
}

u8 i2c_check(u8 address) {
	R16_I2C_CTRL1 |= RB_I2C_START;

	// wait for start bit
	u32 timeout = 100000; // Add timeout to prevent hanging
	while (!(R16_I2C_STAR1 & RB_I2C_SB)) {
		if(--timeout == 1) {
            printf("ERROR: START timeout\r\n");
            return 1;
        }
	}
	printf("IM HERE 1\r\n");

	//! Read to clear SB
	volatile u16 status = R16_I2C_STAR1;
	R16_I2C_DATAR = address << 1;

	// wait for Address bit
	timeout = 100000;
	while (!(R16_I2C_STAR1 & RB_I2C_ADDR)) {
		if(--timeout == 0) {
            printf("ERROR: ADDR timeout\r\n");
            R16_I2C_CTRL1 |= RB_I2C_STOP;
            return 2;
        }
        // Check for NACK (device not present)
        if (R16_I2C_STAR1 & RB_I2C_AF) {
            printf("Device 0x%02X: NOT PRESENT (NACK)\r\n", address);
            R16_I2C_CTRL1 |= RB_I2C_STOP;
            return 3;
        }
	}
	printf("IM HERE 2\r\n");

	// *** CLEAR ADDR FLAG: Read STAR1 then read STAR2 ***
    status = R16_I2C_STAR1;  // First read
    status = R16_I2C_STAR2;  // Second read clears ADDR

	return 0;
}

u8 i2c_ping(u8 address) {
	u8 check = i2c_check(address);
	R16_I2C_CTRL1 |= RB_I2C_STOP;
	return check;
}

u8 i2c_writeData(u8 address, u8 *data, u8 len) {
	uint32_t timeout;
	u8 check = i2c_check(address);

	if (check == 0) {
		for (u8 i = 0; i < len; i++) {
			timeout = 100000;

			while (!(R16_I2C_STAR1 & RB_I2C_TxE)) {
				if(--timeout == 0) {
					printf("ERROR: TXE timeout\r\n");
					R16_I2C_CTRL1 |= RB_I2C_STOP;
					return 4;
				}
			}
			R16_I2C_DATAR = data[i];
		}
	}

	    // Wait for BTF (Byte Transfer Finished) - all data shifted out
    timeout = 100000;
    while (!(R16_I2C_STAR1 & RB_I2C_BTF)) {
        if(--timeout == 0) {
            R16_I2C_CTRL1 |= RB_I2C_STOP;
            return 5; // BTF timeout
        }
    }

	// Clear BTF: Read STAR1 then write DATAR
    volatile u16 status = R16_I2C_STAR1;
    R16_I2C_DATAR = 0x00; // Dummy write to clear BTF

	R16_I2C_CTRL1 |= RB_I2C_STOP;
	return check;
}

u8 i2c_readData(u8 address, u8 *data, u8 len) {
	uint32_t timeout;
	u8 check = i2c_check(address);

	if (check == 0) {
		// Configure ACK for multi-byte read
		if(len > 1) {
			// Enable ACK for all bytes except last
			R16_I2C_CTRL1 |= RB_I2C_ACK;
		} else {
			// Single byte - disable ACK (send NACK after first byte)
			R16_I2C_CTRL1 &= ~RB_I2C_ACK;
		}

		// Read data bytes
		for(u8 i = 0; i < len; i++) {
			if(i == len - 1) {
				// Last byte - disable ACK to send NACK
				R16_I2C_CTRL1 &= ~RB_I2C_ACK;
			}

			// Wait for RxNE (Receive Data Register Not Empty)
			timeout = 100000;
			while (!(R16_I2C_STAR1 & RB_I2C_RxNE)) {
				if(--timeout == 0) {
					R16_I2C_CTRL1 |= RB_I2C_STOP;
					return 6; // RX timeout
				}
			}

			// Check for BTF (data not read before new data)
			if(R16_I2C_STAR1 & RB_I2C_BTF) {
				// Clear BTF: Read STAR1 then read DATAR
				volatile u16 status = R16_I2C_STAR1;
				data[i] = R16_I2C_DATAR; // This read clears BTF
			} else {
				// Normal read
				data[i] = R16_I2C_DATAR; // Clears RxNE
			}
		}
	}

	// Generate STOP condition
    R16_I2C_CTRL1 |= RB_I2C_STOP;
    
    // Re-enable ACK for future operations
    R16_I2C_CTRL1 |= RB_I2C_ACK;

	return check;
}

int main() {
	SystemInit();
	// systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	funPinMode( PIN_BOB,   GPIO_CFGLR_OUT_10Mhz_PP ); // Set PIN_BOB to output
	funPinMode( PIN_KEVIN, GPIO_CFGLR_OUT_10Mhz_PP ); // Set PIN_KEVIN to output

	i2c_init();
	u8 i2c_status = i2c_ping(0x23);
	printf("I2C status: %d\r\n", i2c_status);
	
	u8 state = 0;
	
	// u32 time_ref = millis();

	while(1) {
		funDigitalWrite( PIN_BOB,   FUN_HIGH ); // Turn on PIN_BOB
		funDigitalWrite( PIN_KEVIN, FUN_HIGH ); // Turn on PIN_KEVIN
		Delay_Ms( 200 );
		funDigitalWrite( PIN_BOB,   FUN_LOW );  // Turn off PIN_BOB
		funDigitalWrite( PIN_KEVIN, FUN_LOW );  // Turn off PIN_KEVIN
		Delay_Ms( 200 );

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