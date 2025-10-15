// MIT License
// Copyright (c) 2025 UniTheCat

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include "ch32fun.h"
#include <stdio.h>

#include "lib/lib_i2c.h"

// #define I2C_SENSORS_DEBUG_LOG

#define BUF_MAKE_U16(buff) ((buff[0] << 8) | buff[1])

i2c_device_t dev_sensor = {
	.clkr = I2C_CLK_100KHZ,
	.type = I2C_ADDR_7BIT,
	.addr = 0x3C,
	.regb = 1,
};

u8 I2C_DATA_BUF[8];

void test_bh1750(u16 *lux) {
	dev_sensor.addr = 0x23;

	if (i2c_ping(dev_sensor.addr) != I2C_OK) {
		printf("BH1750 not found\n");
		return;
	}

	i2c_err_t ret = i2c_write_raw(&dev_sensor, (u8[]){0x01}, 1);

	// One-time H-resolution mode
	ret = i2c_read_reg(&dev_sensor, 0x20, I2C_DATA_BUF, 2);

	u16 lux_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*lux = lux_raw * 12 / 10;

	#ifdef I2C_SENSORS_DEBUG_LOG
		printf("\nBH1750: %d lx\n", *lux);
	#endif
}

void test_sht3x(u16 *tempF, u16 *hum) {
	dev_sensor.addr = 0x44;

	if (i2c_ping(dev_sensor.addr) != I2C_OK) {
		printf("SHT3X not found\n");
		return;
	}

	// Soft reset
	i2c_err_t ret = i2c_write_reg(&dev_sensor, 0x30, (u8[]){0xA2}, 1);
	// ret = i2c_write_raw(&dev_sensor, (u8[]){0x30, 0xA2}, 2);
	Delay_Ms(20);	//! REQUIRED
	
	// High repeatability, 1 update per second
	ret = i2c_write_raw(&dev_sensor, (u8[]){0x21, 0x30}, 2);
	Delay_Ms(20);	//! REQUIRED

	ret = i2c_read_raw(&dev_sensor, I2C_DATA_BUF, 6);
	u16 temp_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	u16 hum_raw = (I2C_DATA_BUF[3] << 8) | I2C_DATA_BUF[4];
	*tempF = (175 * temp_raw) >> 16;		// >> 16 is equivalent to / 65536
	*hum = (100 * hum_raw) >> 16;			// >> 16 is equivalent to / 65536

	#ifdef I2C_SENSORS_DEBUG_LOG
		printf("\nSHT3X temp: %d, hum: %d\n", *tempF, *hum);
	#endif
}

// Calculate calibration value using integer math
u16 ina219_calculate_calibration(u32 shunt_resistor_mohm) {
	// Formula: Cal = 0.04096 / (Current_LSB * R_shunt)
	// Using fixed-point: 0.04096 * 1000000 = 40960
	// Current_LSB = 1mA = 0.001 (we'll use 1mA precision)
	
	const u32 CAL_NUMERATOR = 40960;  // 0.04096 * 1000000
	const u32 CURRENT_LSB_UA = 1000;  // 1mA in uA
	
	// Cal = (40960 * 1000) / (CURRENT_LSB_UA * R_shunt_mohm)
	u32 denominator = CURRENT_LSB_UA * shunt_resistor_mohm;
	u32 cal_value = (CAL_NUMERATOR * 1000) / denominator;
	
	return (u16)cal_value;
}


void test_ina219(u16 *shunt_mV, u16 *bus_mV, u16 *current_mA, u16 *power_mW) {
	dev_sensor.addr = 0x40;

	if (i2c_ping(dev_sensor.addr) != I2C_OK) {
		printf("INA219 not found\n");
		return;
	}

	u16 powerLSB = 20;		// 2mW per bit

	//# Configure INA219 32V 1A Range
	// write 0x399F
	i2c_err_t ret = i2c_write_reg(&dev_sensor, 0x00, (u8[]){0x39, 0x9F}, 2);

	i2c_read_reg(&dev_sensor, 0x00, I2C_DATA_BUF, 2);
	u16 config_read = BUF_MAKE_U16(I2C_DATA_BUF);
	printf("\nConfig register: 0x%04X\n", config_read);

	//# Calibrate for 1A range (assuming 0.1Ohm shunt)
	u16 cal = 4096;	 // 0x1000
	u8 cal_bytes[2] = {cal >> 8, cal & 0xFF};
	i2c_write_reg(&dev_sensor, 0x05, cal_bytes, 2);
	Delay_Ms(10);

	i2c_read_reg(&dev_sensor, 0x05, I2C_DATA_BUF, 2);
	u16 cal_read = BUF_MAKE_U16(I2C_DATA_BUF);
	printf("Calibration register: 0x%04X\n", cal_read);

	//# Read shunt voltage
	ret = i2c_read_reg(&dev_sensor, 0x01, I2C_DATA_BUF, 2);
	u16 shunt_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*shunt_mV = shunt_raw / 100;	// in mV

	//# Read bus voltage
	ret = i2c_read_reg(&dev_sensor, 0x02, I2C_DATA_BUF, 2);
	u16 bus_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*bus_mV = (bus_raw >> 3) * 4;	// in mV

	//# Read power
	ret = i2c_read_reg(&dev_sensor, 0x03, I2C_DATA_BUF, 2);
	u16 power_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*power_mW = power_raw * powerLSB;	// in mW

	//# Read current
	ret = i2c_read_reg(&dev_sensor, 0x04, I2C_DATA_BUF, 2);
	u16 current_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*current_mA = current_raw;			// in mA

	#ifdef I2C_SENSORS_DEBUG_LOG
		printf("\nINA219 Shunt: %dmV, Bus: %dmV, Current: %dmA\n", 
				*shunt_mV, *shunt_mV, *current_mA);
	#endif
}
