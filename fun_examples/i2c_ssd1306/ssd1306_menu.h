#include "../../fun_modules/fun_i2c/fun_ssd1306.h"


i2c_device_t dev_ssd1306 = {
	.clkr = I2C_CLK_100KHZ,
	.type = I2C_ADDR_7BIT,
	.addr = 0x3C,				// Default address for SSD1306
	.regb = 1,
};


//# SSD1306 INTERFACES
/* send OLED command byte */
u8 SSD1306_CMD(u8 cmd) {
	u8 pkt[2];
	pkt[0] = 0;
	pkt[1] = cmd;
	return i2c_write_raw(&dev_ssd1306, pkt, 2);
}

/* send OLED data packet (up to 32 bytes) */
u8 SSD1306_DATA(u8 *data, int sz) {
	u8 pkt[33];
	pkt[0] = 0x40;
	memcpy(&pkt[1], data, sz);
	return i2c_write_raw(&dev_ssd1306, pkt, sz+1);
}

Str_Config_t DEAULF_STR_CONFIG = {
	.FONT = FONT_7x5,
	.WIDTH = 5,
	.HEIGHT = 7,
	.SPACE = 1,
	.scale = 2,
	.color = 1,
};


char str_output[SSD1306_MAX_STR_LEN];


void makeMenu(void) {
	ssd1306_render_scaled_txt(0, 0, "123456789012345678901", &DEAULF_STR_CONFIG, render_pixel);

	ssd1306_draw_all();
}

void modI2C_display(const char *str, u8 line) {
	//! validate device before print
	if (i2c_ping(0x3C) != I2C_OK) return;
	ssd1306_draw_str(str, line, 0);
}

void i2c_scan_callback(const u8 addr) {
	if (addr == 0x00 || addr == 0x7F) return; // Skip reserved addresses
	
	static int line = 1;
	sprintf(str_output, "I2C: 0x%02X", addr);
	printf("%s\n", str_output);
	modI2C_display(str_output, line++);
}

void ssd1306_menu_init() {
    if(i2c_init(&dev_ssd1306) != I2C_OK) {
		printf("Err: I2C init failed\n");
	} 
	else {
		if (i2c_ping(0x3C) == I2C_OK) {
			ssd1306_init();
			ssd1306_render_fill(0x00);

            u32 time_ref = millis();

			sprintf(str_output, "Hello Bee");
			ssd1306_draw_str(str_output, 0, 0);

			time_ref = millis();
			// ssd1306_draw_test();
			ssd1306_draw_all();
		
			printf("elapsed: %d ms\n", millis() - time_ref);
		}

		// Scan the I2C Bus, prints any devices that respond
		printf("- Scanning I2Cs -\n");
		i2c_scan(i2c_scan_callback);
		printf("- Done Scanning -\n\n");
	}
}