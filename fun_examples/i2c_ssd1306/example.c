#include "../../fun_modules/fun_base.h"
#include "../../fun_modules/fun_i2c/lib/lib_i2c.h"
#include "../../fun_modules/fun_i2c/fun_ssd1306.h"

char str_output[SSD1306_MAX_STR_LEN];

i2c_device_t dev_ssd1306 = {
	.clkr = I2C_CLK_100KHZ,
	.type = I2C_ADDR_7BIT,
	.addr = 0x3C,				// Default address for SSD1306
	.regb = 1,
};


/* send OLED command byte */
uint8_t SSD1306_CMD(uint8_t cmd) {
	uint8_t pkt[2];
	pkt[0] = 0;
	pkt[1] = cmd;
	return i2c_write_raw(&dev_ssd1306, pkt, 2);
}

/* send OLED data packet (up to 32 bytes) */
uint8_t SSD1306_DATA(uint8_t *data, int sz) {
	uint8_t pkt[33];
	pkt[0] = 0x40;
	memcpy(&pkt[1], data, sz);
	return i2c_write_raw(&dev_ssd1306, pkt, sz+1);
}


void modI2C_display(const char *str, uint8_t line) {
	//! validate device before print
	if (i2c_ping(0x3C) != I2C_OK) return;
	ssd1306_draw_str(str, line, 0);
}

void i2c_scan_callback(const uint8_t addr) {
	if (addr == 0x00 || addr == 0x7F) return; // Skip reserved addresses
	
	static int line = 1;
	sprintf(str_output, "I2C: 0x%02X", addr);
	printf("%s\n", str_output);
	modI2C_display(str_output, line++);
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	u32 time_ref = millis();
	u32 counter = 0;

	if(i2c_init(&dev_ssd1306) != I2C_OK) {
		printf("Err: I2C init failed\n");
	} 
	else {
		if (i2c_ping(0x3C) == I2C_OK) {
			ssd1306_init();

			sprintf(str_output, "Hello Bee %ld", counter);
			ssd1306_draw_str(str_output, 0, 0);
		}

		// Scan the I2C Bus, prints any devices that respond
		printf("- Scanning I2C Bus for Devices -\n");
		i2c_scan(i2c_scan_callback);
		printf("- Done Scanning -\n\n");
	}

	while(1) {
		if ((millis() - time_ref) > 2000) {
			// test_lines();
			// ssd1306_drawAll();

			// sprintf(str_output, "Hello Bee %ld", counter++);
			// ssd1306_draw_str(str_output, 0, 0);
			time_ref = millis();
		}
	}
}