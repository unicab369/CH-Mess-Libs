#include "../../fun_modules/fun_i2c/lib/lib_i2c.h"
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

u8 i2c_ping_display() { return i2c_ping(0x3C) == I2C_OK; }

//! ####################################
//! I2C MENU
//! ####################################


Str_Config_t MENU_STR_CONFIG = {
	.FONT = FONT_7x5,
	.WIDTH = 5,
	.HEIGHT = 7,
	.SPACE = 1,
	.scale_mode = 2,
	.color = 1,
};

char str_output[SSD1306_MAX_STR_LEN];
int8_t menu_selectedIdx = 0;
u8 is_main_menu = 1;

u8 I2C_DEVICES[8] = {0};

void modI2C_display(const char *str, u8 line) {
	//! validate device before print
	if (!i2c_ping_display()) return;
	ssd1306_draw_str(str, line, 0);
}

//# Scan callback
void i2c_scan_callback(const u8 addr) {
	if (addr == 0x00 || addr == 0x7F) return; // Skip reserved addresses
	sprintf(str_output, "I2C: 0x%02X", addr);
	printf("%s\n", str_output);

	if (is_main_menu) return;
	static int scan_line = 0;
	ssd1306_draw_scaled_text(0, scan_line++ * 13, str_output, &MENU_STR_CONFIG);
}

void i2c_start_scan() {
	// Scan the I2C Bus, prints any devices that respond
	printf("\n* Scanning I2Cs *\n");
	i2c_scan(i2c_scan_callback);
	printf("* Done Scanning *\n\n");
}

void i2c_menu_move(int8_t direction) {
	if (is_main_menu == 0) return;
    if (direction == 0) return;
    ssd1306_draw_scaled_text(0, menu_selectedIdx * 13, " ", &MENU_STR_CONFIG);

    if (direction > 0) {
        menu_selectedIdx++;
    }
    else if (direction < 0) {
        menu_selectedIdx--;
    }

    if (menu_selectedIdx < 0) menu_selectedIdx = 4;
    if (menu_selectedIdx > 4) menu_selectedIdx = 0;

    ssd1306_draw_scaled_text(0, menu_selectedIdx * 13, ">", &MENU_STR_CONFIG);
}

void i2c_menu_select(u8 newIdx) {
    if (newIdx > 4) return;
    ssd1306_draw_scaled_text(0, menu_selectedIdx * 13, " ", &MENU_STR_CONFIG);

    menu_selectedIdx = newIdx;
    ssd1306_draw_scaled_text(0, newIdx * 13, ">", &MENU_STR_CONFIG);
}

void i2c_menu_handle() {
	if (is_main_menu) {
		switch (menu_selectedIdx) {
			case 0:
				is_main_menu = 0;
				ssd1306_draw_fill(0x00);
				i2c_start_scan();
				break;
			case 1: break;
			default: break;
		}
	} else {
		switch (menu_selectedIdx) {
			case 0: 
				is_main_menu = 1;
				ssd1306_draw_fill(0x00);
				i2c_menu_main();
				break;
			case 1: break;
			default: break;
		}
	}
}

void i2c_menu_main() {
	ssd1306_render_scaled_txt(0, 0, "  I2C SCAN", &MENU_STR_CONFIG);
    ssd1306_render_scaled_txt(0, 13, "  I2C SENSOR", &MENU_STR_CONFIG);
    ssd1306_render_scaled_txt(0, 26, "  IR RECEIVER", &MENU_STR_CONFIG);
    ssd1306_render_scaled_txt(0, 39, "  OPTION 4", &MENU_STR_CONFIG);
    ssd1306_render_scaled_txt(0, 52, "  12345678901234", &MENU_STR_CONFIG);
    ssd1306_render_scaled_txt(0, 0, ">", &MENU_STR_CONFIG);
	ssd1306_draw_all();
}

void i2c_menu_init() {
    if(i2c_init(&dev_ssd1306) != I2C_OK) {
		printf("Err: I2C init failed\n");
	} 
	else {
		if (i2c_ping_display()) {
			ssd1306_init();
			ssd1306_render_fill(0x00);

            u32 time_ref = millis();

			sprintf(str_output, "Hello Bee");
			ssd1306_draw_str(str_output, 0, 0);
			i2c_menu_main();

			time_ref = millis();
			// ssd1306_draw_test();
			ssd1306_draw_all();
		
			printf("elapsed: %d ms\n", millis() - time_ref);
		}

		i2c_start_scan();
	}
}