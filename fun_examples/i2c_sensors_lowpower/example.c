#include "../../fun_modules/fun_base.h"
#include "../../fun_modules/fun_i2c/fun_i2c_sensors.h"
#include "../../fun_modules/fun_clockfreq.h"

i2c_device_t dev_i2c = {
	.clkr = I2C_CLK_100KHZ,
	.type = I2C_ADDR_7BIT,
	.addr = 0x3C,				// Default address for SSD1306
	.regb = 1,
};



//# Scan callback
void i2c_scan_callback(const u8 addr) {
	if (addr == 0x00 || addr == 0x7F) return; // Skip reserved addresses
    char local_buf[32] = {0};
    sprintf(local_buf, "I2C: 0x%02X", addr);
	printf("%s\n", local_buf);
}

void i2c_start_scan() {
	// Scan the I2C Bus, prints any devices that respond
	printf("\n* Scanning I2Cs *\n");
	i2c_scan(i2c_scan_callback);
	printf("* Done Scanning *\n\n");
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	fun_clockfreq_set(CLOCK_48Mhz);

	u32 time_ref = millis();

	if(i2c_init(&dev_i2c) != I2C_OK) {
		printf("Err: I2C init failed\n");
	} else {
		i2c_start_scan();
	}

	static char str_output[21] = { 0 };

	while(1) {
		u32 moment = millis();

		if (moment - time_ref > 1000) {
			time_ref = moment;

			u16 lux;
			test_bh1750(&lux);
			sprintf(str_output, "LUX %d", lux);
			printf("%s\n", str_output);

			u16 tempF, hum;
			test_sht3x(&tempF, &hum);
			sprintf(str_output, "TEMP %d, HUM %d", tempF, hum);
			printf("%s\n", str_output);

			u16 shunt_mV, bus_mV, current_uA, power_uW;
			test_ina219(&shunt_mV, &bus_mV, &current_uA, &power_uW);
			sprintf(str_output, "SHUNT %d uV", shunt_mV);
			printf("%s\n", str_output);

			sprintf(str_output, "BUS %d mV", bus_mV);
			printf("%s\n", str_output);

			sprintf(str_output, "%d uA, %d uW", current_uA, power_uW);
			printf("%s\n", str_output);	

			printf("\n");
		}
	}
}
