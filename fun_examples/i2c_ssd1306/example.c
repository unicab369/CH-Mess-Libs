#include "../../fun_modules/fun_base.h"
#include "../../fun_modules/fun_i2c/lib/lib_i2c.h"
#include "../../fun_modules/fun_i2c/fun_ssd1306.h"
#include "../../fun_modules/fun_encoder_tim2.h"
#include "../../fun_modules/fun_encoder_gpio.h"

char str_output[SSD1306_MAX_STR_LEN];

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

//# Callbacks
void onHandle_Encoder(int8_t position, int8_t direction) {
	printf("pos: %d, direction: %d\n", position, direction);
	// mngI2c_load_encoder(millis(), position, direction);
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

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	//# TIM2: uses PD4(CH1) and PD3(CH2)
	Encoder_t encoder_a;
	// fun_encoder_tim2_init(&encoder_a);

	Encoder_GPIO_t encoder_b = {
		.pinA = PD4,
		.pinB = PD3,
	};
	fun_encoder_gpio_init(&encoder_b);

	u32 time_ref = millis();
	u32 counter = 0;

	if(i2c_init(&dev_ssd1306) != I2C_OK) {
		printf("Err: I2C init failed\n");
	} 
	else {
		if (i2c_ping(0x3C) == I2C_OK) {
			ssd1306_init();
			ssd1306_fill(0x00);

			sprintf(str_output, "Hello Bee %ld", counter);
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

	Str_Config_t str_conf = {
		.FONT = FONT_7x5,
		.WIDTH = 5,
		.HEIGHT = 7,
		.SPACE = 1,
		.scale = 2,
		.color = 0,
	};

	while(1) {
		u32 moment = millis();

		if ((moment - time_ref) > 1000) {
			sprintf(str_output, "Hello %ld", counter++);
			// ssd1306_draw_scaled_text(0, 0, "AAAAAAAAAAAAAAAAAAAAAA", &str_conf);
			ssd1306_draw_scaled_text(0, 0, str_output, &str_conf);

			// test_lines();
			time_ref = millis();
		}

		// fun_encoder_tim2_task(moment, &encoder_a, onHandle_Encoder);
		fun_encoder_gpio_task(moment, &encoder_b, onHandle_Encoder);
	}
}
