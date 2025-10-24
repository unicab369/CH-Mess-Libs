#include "ch32fun.h"
#include <stdio.h>

#define PIN_BOB PA8
#define PIN_KEVIN PA9
#define SYSTEM_CLOCK_MHZ 60

#define I2C_SDA PB12
#define I2C_SCL PB13

#define I2C_ADDR_WRITE_MASK 0xFE
#define I2C_ADDR_READ_MASK 0x01

#define I2C_TIMEOUT 100000

#define I2C_WAIT_FOR(condition) do { \
    u32 timeout = 100000; \
    while (!(condition)) { \
        if (--timeout == 0) { \
            debug_i2c_registers(); \
            R16_I2C_CTRL1 |= RB_I2C_STOP; \
            break; \
        } \
    } \
} while(0)


void print_bits(u16 val) {
	for (int i = 15; i >= 0; i--) { 
		printf("%d ", (val >> i) & 1);
		if (i == 8) printf("| ");
	}
}

// I2C_CTRL1
// [15] SWRST, [10] ACK_EN, [9] STOP, [8] START, [3] SMBTYPE, [2] Reserved, [1] MSBUS, [0] PE

// I2C_STAR1
// [15] SMBALERT, [14] TIMEOUT, [13] Reserved, [12] PECERR, [11] OVR, [10] AF, [9] ARLO, 
//[8] BEER, [7] TxE, [6] RxNE, [5] Reserved, [4] STOPF, [3] DD10, [2] BTF, [1] ADDR, [0] SB

// I2C_STAR2
// [2] TRA, [1] BUSY, [0] MSL

void debug_i2c_registers(void) {
	printf("CTRL1:  0x%04X \t", R16_I2C_CTRL1);		print_bits(R16_I2C_CTRL1); printf("\r\n");
	printf("CTRL2:  0x%04X \t", R16_I2C_CTRL2);		print_bits(R16_I2C_CTRL2); printf("\r\n");
	printf("STAR1:  0x%04X \t", R16_I2C_STAR1); 	print_bits(R16_I2C_STAR1); printf("\r\n");
	printf("STAR2:  0x%04X \t", R16_I2C_STAR2);		print_bits(R16_I2C_STAR2); printf("\r\n");
	// printf("CKCFGR: 0x%04X \t", R16_I2C_CKCFGR);	print_bits(R16_I2C_CKCFGR); printf("\r\n");
	// printf("RTR:	0x%04X \t", R16_I2C_RTR);		print_bits(R16_I2C_RTR); printf("\r\n");
	// printf("OADDR1: 0x%04X \t", R16_I2C_OADDR1);	print_bits(R16_I2C_OADDR1); printf("\r\n");
	printf("DATAR:  0x%04X\r\n", R16_I2C_DATAR);
	printf("=========================\r\n\n");
}

u8 i2c_get_error() {
	if (R16_I2C_STAR1 & RB_I2C_BERR)	{ R16_I2C_STAR1 &= ~RB_I2C_BERR;	return 1; }		// Bus Error
	if (R16_I2C_STAR1 & RB_I2C_ARLO)	{ R16_I2C_STAR1 &= ~RB_I2C_ARLO;	return 2; }		// Arbitration lost
	if (R16_I2C_STAR1 & RB_I2C_AF)		{ R16_I2C_STAR1 &= ~RB_I2C_AF;		return 3; }		// ACK Failure
	if (R16_I2C_STAR1 & RB_I2C_OVR)		{ R16_I2C_STAR1 &= ~RB_I2C_OVR;		return 4; }		// Overrun
	if (R16_I2C_STAR1 & RB_I2C_PECERR)	{ R16_I2C_STAR1 &= ~RB_I2C_PECERR;	return 5; }		// PEC Error
	return 0;
}

u8 i2c_init(u16 i2c_speed_khz) {
	funPinMode( I2C_SDA, GPIO_CFGLR_IN_PUPD );
	funPinMode( I2C_SCL, GPIO_CFGLR_IN_PUPD );
	funDigitalWrite( I2C_SDA, 1 );
	funDigitalWrite( I2C_SCL, 1 );

	//! REQUIRED: Software reset
	R16_I2C_CTRL1 |= RB_I2C_SWRST;
	R16_I2C_CTRL1 &= ~RB_I2C_SWRST;

	// Clear FREQ bits and set system clock frequency
	R16_I2C_CTRL2 &= ~RB_I2C_FREQ;
	R16_I2C_CTRL2 |= (SYSTEM_CLOCK_MHZ & RB_I2C_FREQ);
	
	// Clear pheriphera
	R16_I2C_CTRL1 &= ~RB_I2C_PE;
	u16 clock_config;
	
	if (i2c_speed_khz <= 100) {
		R16_I2C_RTR = (1 + SYSTEM_CLOCK_MHZ) > 0x3F ? 0x3F : (1 + SYSTEM_CLOCK_MHZ);

		clock_config = (SYSTEM_CLOCK_MHZ * 1000 / (i2c_speed_khz << 1)) & RB_I2C_CCR;
		if (clock_config < 4) clock_config = 4;
		printf("clock_config: %d\r\n", clock_config);
		printf("RTR: %d\r\n", R16_I2C_RTR);
	} 
	else {
		R16_I2C_RTR = 1 + SYSTEM_CLOCK_MHZ * 300 / 1000;

		clock_config = (SYSTEM_CLOCK_MHZ * 1000 / (i2c_speed_khz * 3)) & RB_I2C_CCR;
		if (clock_config == 0) clock_config = 1;
		clock_config |= RB_I2C_F_S;
	}

	// Set CCR value
	R16_I2C_CKCFGR = clock_config;

	// Enable peripheral
	R16_I2C_CTRL1 = RB_I2C_PE;
	R16_I2C_CTRL1 |= RB_I2C_ACK;	// Enable ACK

	return i2c_get_error();
}

u8 i2c_START(u8 address, u8 addr_mask) {
    //# STEP 1: Generate START
    R16_I2C_CTRL1 |= RB_I2C_START;
    
    //# STEP 2: Wait for SB with timeout
    u32 timeout = 100000;
    while (!(R16_I2C_STAR1 & RB_I2C_SB)) {
        if (--timeout == 0) {
            printf("ERROR: SB timeout\r\n");
			debug_i2c_registers();
            R16_I2C_CTRL1 |= RB_I2C_STOP;  	// Clean up
            return 1;
        }
    }
    
    //# STEP 3: Clear SB (READ STAR1 then write DATAR)
    volatile u16 status = R16_I2C_STAR1;  	// Read to clear SB

	// mask: 0xFE = write, 0x01 = read
	if (addr_mask == I2C_ADDR_WRITE_MASK) {
		R16_I2C_DATAR = (address << 1) & addr_mask;
	} else {
		R16_I2C_DATAR = (address << 1) | addr_mask;
	}
    
    //# STEP 4: Wait for ADDR with timeout
    timeout = 100000;
    while (!(R16_I2C_STAR1 & RB_I2C_ADDR)) {
        if (--timeout == 0) {
            printf("ERROR: ADDR timeout\r\n");
			debug_i2c_registers();
            R16_I2C_CTRL1 |= RB_I2C_STOP;
            return 2;
        }

		// // Check for NACK
        // if (R16_I2C_STAR1 & RB_I2C_AF) {
        //     R16_I2C_CTRL1 |= RB_I2C_STOP;
        //     return 3;
        // }
    }

    //# STEP 5: Clear ADDR by reading registers
    status = R16_I2C_STAR1;
    status = R16_I2C_STAR2;

	return i2c_get_error();
}

u8 i2c_ping(u8 address) {
	u8 check = i2c_START(address, I2C_ADDR_WRITE_MASK);
	if (check != 0) return check;

	//# STOP I2C
	R16_I2C_CTRL1 |= RB_I2C_STOP;
	return i2c_get_error();
}

u8 i2c_writeData(u8 address, u8 *data, u8 len, u8 stop_when_done) {
	uint32_t timeout;
	u8 check = i2c_START(address, I2C_ADDR_WRITE_MASK);
	if (check != 0) return check;

	for (u8 i = 0; i < len; i++) {
		timeout = 100000;

		while (!(R16_I2C_STAR1 & RB_I2C_TxE)) {
			if(--timeout < 1) {
				printf("ERROR: TXE timeout\r\n");
				debug_i2c_registers();
				R16_I2C_CTRL1 |= RB_I2C_STOP;
				return 4;
			}
		}
		R16_I2C_DATAR = data[i];
	}

	// Wait for BTF (Byte Transfer Finished) - all data shifted out
	timeout = 100000;
	while (!(R16_I2C_STAR1 & RB_I2C_BTF)) {
		if(--timeout < 1) {
			printf("ERROR: BTF timeout\r\n");
			debug_i2c_registers();
			R16_I2C_CTRL1 |= RB_I2C_STOP;
			return 5; // BTF timeout
		}
	}

	// Clear BTF: Read STAR1
	volatile u16 status = R16_I2C_STAR1;

	if (stop_when_done == 1) {
		//# STOP I2C
		R16_I2C_CTRL1 |= RB_I2C_STOP;
	}

	return i2c_get_error();
}


u8 i2c_readData(u8 address, u8 *data, u8 len) {
	uint32_t timeout;
	u8 check = i2c_START(address, I2C_ADDR_READ_MASK);
	if (check != 0) return check;

	volatile uint16_t status;

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
			if(--timeout < 1) {
				printf("ERROR: RXNE timeout\r\n");
				debug_i2c_registers();
				R16_I2C_CTRL1 |= RB_I2C_STOP;
				return 6; // RX timeout
			}
		}

		// Read data byte
		data[i] = R16_I2C_DATAR;

		// Check for BTF and clear if needed
		if (R16_I2C_STAR1 & RB_I2C_BTF) {
			status = R16_I2C_STAR1;
			data[i] = R16_I2C_DATAR; // Read again to clear BTF
		}
	}

	//# STOP I2C
	R16_I2C_CTRL1 |= RB_I2C_STOP;
	return i2c_get_error();

	// Re-enable ACK for future operations
	// R16_I2C_CTRL1 |= RB_I2C_ACK;
}

#define BUF_MAKE_U16(buff) ((buff[0] << 8) | buff[1])

void read_bh1750() {
	u8 address = 0x23;

	//# power on
	u8 ret = i2c_writeData(address, (u8[]){0x01}, 1, 1);
	printf("I2C powerON: %d\r\n", ret);

	Delay_Ms(50);
	
	//# set resolution
	ret = i2c_writeData(address, (u8[]){0x23}, 1, 1);
	printf("I2C config: %d\r\n", ret);

	//# request reading
	ret = i2c_writeData(address, (u8[]){0x13}, 1, 1);
	printf("I2C request: %d\r\n", ret);

	//# parse reading
	u8 data[2];
	ret = i2c_readData(address, data, 2);
	printf("I2C reading: %d\r\n", ret);

	u16 lux_raw = BUF_MAKE_U16(data);
	u16 lux = lux_raw * 12 / 10;
	printf("lux: %d\r\n", lux);
}

int main() {
	SystemInit();
	// systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	funPinMode( PIN_BOB,   GPIO_CFGLR_OUT_10Mhz_PP ); // Set PIN_BOB to output
	funPinMode( PIN_KEVIN, GPIO_CFGLR_OUT_10Mhz_PP ); // Set PIN_KEVIN to output

	u8 err = i2c_init(100);
	printf("\n*****I2C init: %d\r\n", err);

	u8 status = i2c_ping(0x23);
	printf("I2C status: %d\r\n", status);

	Delay_Ms(50);

	// u8 data[2];
	// status = i2c_readData(0x23, data, 2);
	// printf("I2C status2: %d\r\n", status);
	// printf("I2C data: %d\r\n", BUF_MAKE_U16(data));

	u8 state = 0;
	
	// u32 time_ref = millis();

	while(1) {
		funDigitalWrite( PIN_BOB,   FUN_HIGH ); // Turn on PIN_BOB
		funDigitalWrite( PIN_KEVIN, FUN_HIGH ); // Turn on PIN_KEVIN
		Delay_Ms( 500 );
		funDigitalWrite( PIN_BOB,   FUN_LOW );  // Turn off PIN_BOB
		funDigitalWrite( PIN_KEVIN, FUN_LOW );  // Turn off PIN_KEVIN
		Delay_Ms( 500 );

		read_bh1750();


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