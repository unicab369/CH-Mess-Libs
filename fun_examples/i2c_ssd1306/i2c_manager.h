#include "../../fun_modules/fun_i2c/lib/lib_i2c.h"
#include "../../fun_modules/fun_i2c/fun_ssd1306.h"
#include "../../fun_modules/fun_i2c/fun_i2c_sensors.h"

#define I2C_MENU_LINE_SPACING 12

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
//! MENU FUNCTIONS
//! ####################################

Str_Config_t MENU_STR_CONFIG = {
	.FONT = FONT_7x5,
	.WIDTH = 5,
	.HEIGHT = 7,
	.SPACE = 1,
	.scale_mode = 3,
	.color = 1,
};

// char str_output[SSD1306_MAX_STR_LEN] = { 0 };
int8_t menu_selectedIdx = 0;
u8 is_main_menu = 1;

#define I2C_MAX_DEVICES_COUNT 8
u8 I2C_DEVICES[I2C_MAX_DEVICES_COUNT] = {0};
u8 I2C_DEVICES_COUNT = 0;

void modI2C_display(const char *str, u8 line) {
	//! validate device before print
	if (!i2c_ping_display()) return;
	ssd1306_draw_str(str, line, 0);
}

//# Scan callback
void i2c_scan_callback(const u8 addr) {
	if (addr == 0x00 || addr == 0x7F) return; // Skip reserved addresses
    char local_buf[32] = {0};
    sprintf(local_buf, "I2C: 0x%02X", addr);
	printf("%s\n", local_buf);

	if (is_main_menu) return;
	ssd1306_draw_scaled_text(0, I2C_DEVICES_COUNT++ * I2C_MENU_LINE_SPACING, 
								local_buf, &MENU_STR_CONFIG);
}

void i2c_start_scan() {
	// Scan the I2C Bus, prints any devices that respond
	printf("\n* Scanning I2Cs *\n");
	i2c_scan(i2c_scan_callback);
	printf("* Done Scanning *\n\n");
}

void i2c_menu_updateCursor() {
	ssd1306_draw_scaled_text(0, menu_selectedIdx * I2C_MENU_LINE_SPACING, ">", &MENU_STR_CONFIG);
}

void i2c_menu_move(int8_t direction) {
	if (is_main_menu == 0) return;
	if (direction == 0) return;
	ssd1306_draw_scaled_text(0, menu_selectedIdx * I2C_MENU_LINE_SPACING, " ", &MENU_STR_CONFIG);

	if (direction > 0) {
		menu_selectedIdx++;
	}
	else if (direction < 0) {
		menu_selectedIdx--;
	}

	if (menu_selectedIdx < 0) menu_selectedIdx = 4;
	if (menu_selectedIdx > 4) menu_selectedIdx = 0;

	i2c_menu_updateCursor();
}

void i2c_menu_select(u8 newIdx) {
	if (newIdx > 4) return;
	ssd1306_draw_scaled_text(0, menu_selectedIdx * I2C_MENU_LINE_SPACING, " ", &MENU_STR_CONFIG);

	menu_selectedIdx = newIdx;
	i2c_menu_updateCursor();
}

void i2c_menu_render_text_at(u8 line, const char *str) {
	ssd1306_render_scaled_txt(0, line * I2C_MENU_LINE_SPACING, str, &MENU_STR_CONFIG);
}

void i2c_menu_main() {
	printf("show main menu\n");
	ssd1306_draw_fill(0x00);
	i2c_menu_render_text_at(0, "  I2C SENSORS");
	i2c_menu_render_text_at(1, "  I2C SCAN");
	i2c_menu_render_text_at(2, "  IR SENDER");
	i2c_menu_render_text_at(3, "  IR RECEIVER");
	i2c_menu_render_text_at(4, "  PERFORMANCE");
	ssd1306_draw_all();

	i2c_menu_updateCursor();
}

//! ####################################
//! MENU INIT
//! ####################################

void i2c_menu_init() {
	if(i2c_init(&dev_ssd1306) != I2C_OK) {
		printf("Err: I2C init failed\n");
	} 
	else {
		if (i2c_ping_display()) {
			ssd1306_init();
			ssd1306_render_fill(0x00);

			u32 time_ref = millis();

			// sprintf(str_output, "Hello Bee");
			// ssd1306_draw_str(str_output, 0, 0);
			i2c_menu_main();

			time_ref = millis();
			// ssd1306_draw_test();
			ssd1306_draw_all();
		
			printf("elapsed: %ld ms\n", millis() - time_ref);
		}

		Delay_Ms(100);
		i2c_start_scan();
	}
}

u8 irCount = 0;

void i2c_menu_handle() {
	if (is_main_menu) {
		switch (menu_selectedIdx) {
			case 0: case 1: case 2: case 3:
				is_main_menu = 0;
				break;
			default: break;
		}
	} else {
		switch (menu_selectedIdx) {
			case 0: case 1: case 2: case 3:
				is_main_menu = 1;
				i2c_menu_main();
				break;
			default: break;
		}
	}
}

char IR_Receive_Str[SSD1306_MAX_STR_LEN] = { 0 };

u8 i2c_menu_period_tick() {
	if (is_main_menu) return 0;
	static char str_output[SSD1306_MAX_STR_LEN] = { 0 };

	// clear screen
	ssd1306_render_fill(0x00);

	switch (menu_selectedIdx) {
		case 0: {
			u16 lux;
			test_bh1750(&lux);
			sprintf(str_output, "LUX %d", lux);
			i2c_menu_render_text_at(0, str_output);

			u16 tempF, hum;
			test_sht3x(&tempF, &hum);
			sprintf(str_output, "TEMP %d, HUM %d", tempF, hum);
			i2c_menu_render_text_at(1, str_output);

			u16 shunt_mV, bus_mV, current_uA, power_uW;
			test_ina219(&shunt_mV, &bus_mV, &current_uA, &power_uW);
			sprintf(str_output, "SHUNT %d uV", shunt_mV);
			i2c_menu_render_text_at(2, str_output);

			sprintf(str_output, "BUS %d mV", bus_mV);
			i2c_menu_render_text_at(3, str_output);

			sprintf(str_output, "%d uA, %d uW", current_uA, power_uW);
			i2c_menu_render_text_at(4, str_output);

			ssd1306_draw_all();
			return 1;			
		}
		case 1: {
			I2C_DEVICES_COUNT = 0;
			ssd1306_draw_all();
			i2c_start_scan();
			return 1;
		}
		case 2: {
			sprintf(str_output, "IR Sending %d", irCount++);
			i2c_menu_render_text_at(0, str_output);
			ssd1306_draw_all();
			return 1;
		}
		case 3: {
			sprintf(str_output, "IR Receiving %d", irCount++);
			i2c_menu_render_text_at(0, str_output);

			// get the ir string
			// sprintf(str_output, "%s", IR_Receive_Str);
			i2c_menu_render_text_at(1, IR_Receive_Str);
			memset(IR_Receive_Str, 0, sizeof(IR_Receive_Str));

			ssd1306_draw_all();
			return 1;
		}
	}
}