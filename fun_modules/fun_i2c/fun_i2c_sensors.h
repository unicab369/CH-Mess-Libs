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
#define GET_BIT(val, n) ((val >> n) & 1)

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


//# INA219 REGISTERS
#define INA219_REG_CONFIG		0x00		// Default Value 0x399F
#define INA219_REG_SHUNT		0x01
#define INA219_REG_BUS			0x02
#define INA219_REG_POWER		0x03		// Default Value 0x000
#define INA219_REG_CURRENT		0x04		// Default Value 0x000
#define INA219_REG_CALIB		0x05		// Default Value 0x000

// Configuration Register
// | 15		| 14	| 13	| 12	| 11	| 10	| 9		| 8		| 7		
// | RST 	|		| BRNG	| GP1	| PG0	| BUS4	| BUS3	| BUS2	| BUS1
// -------------------------------------------------------------------------
// | 6		| 5		| 4		| 3		| 2		| 1		| 0
// | SHNT4	| SHNT3	| SHNT2	| SHNT1	| MODE3	| MODE2	| MODE1

// Volgate Range (BRNG)
// 16V => (0 << 13)
// 32V => (1 << 13) - Default

// Shunt Voltage Range (GP1 GP0)
// +-40mV 	(8x Gain) => (0 << 11)
// +-80mV 	(4x Gain) => (1 << 11)
// +-160mV 	(2x Gain) => (2 << 11)
// +-320mV 	(1x Gain) => (3 << 11) - Default

// Bus ADC Resolution (BUS4 BUS3 BUS2 BUS1)
// 0000 	9-bit		84 us		=> (0 << 7)
// 0001 	10-bit		148 us		=> (1 << 7)
// 0010 	11-bit		276 us		=> (2 << 7)
// 0011 	12-bit		532 us		=> (3 << 7) - Default
// 1000 	12-bit		532 us		=> (8 << 7) - 12-bit 1 sample
// 1001 	12-bitx2	1.06 ms		=> (9 << 7)
// 1010 	12-bitx4	2.13 ms		=> (10 << 7)
// ...
// 1111 	12-bitx128	68.10 ms	=> (15 << 7)	

// Shunt ADC Resolution (SHNT4 SHNT3 SHNT2 SHNT1)
// 0000 	9-bit		84 us		=> (0 << 3)
// 0001 	10-bit		148 us		=> (1 << 3)
// 0010 	11-bit		276 us		=> (2 << 3)
// 0011 	12-bit		532 us		=> (3 << 3) - Default
// 1000 	12-bit		532 us		=> (8 << 3) - 12-bit 1 sample
// 1001 	12-bitx2	1.06 ms		=> (9 << 3)
// 1010 	12-bitx4	2.13 ms		=> (10 << 3)
// ...
// 1111 	12-bitx128	68.10 ms	=> (15 << 3)

// Operating mode
// 000		Power down					=> (0 << 0)
// 001		Shunt voltage, triggered	=> (1 << 0)
// 010		Bus voltage, triggered		=> (2 << 0)
// 011		Shunt and bus, triggered	=> (3 << 0)
// 100		ADC OFF (disabled)			=> (4 << 0)
// 101		Shunt voltage, continuous	=> (5 << 0)
// 110		Bus voltage, continuous		=> (6 << 0)
// 111		Shunt and bus, continuous	=> (7 << 0) - Default

void ina219_config_binary(u16 config) {
    printf("Config: 0x%04X\n", config);
    printf("Binary: ");
    
    // Print binary with bit positions
    for (int i = 15; i >= 0; i--) {
        printf("%d", (config >> i) & 1);
    }
    printf("\n");
    
    // Bit field breakdown
    printf("Bit fields:\n");
    printf("13: BRNG (Bus Range)    = %d\n", GET_BIT(config, 13));
    printf("12-11: PGA (Shunt Gain) = %d%d\n", GET_BIT(config, 12), GET_BIT(config, 11));
    printf("10-7: BADC (Bus ADC)    = %d%d%d%d\n", GET_BIT(config, 10), GET_BIT(config, 9), GET_BIT(config, 8), GET_BIT(config, 7));
    printf("6-3: SADC (Shunt ADC)   = %d%d%d%d\n", GET_BIT(config, 6), GET_BIT(config, 5), GET_BIT(config, 4), GET_BIT(config, 3));
    printf("2-0: MODE (Operating)   = %d%d%d\n",  GET_BIT(config, 2), GET_BIT(config, 1), GET_BIT(config, 0));
}

u8 INA219_CONFIGURED = 0;

void ina219_configure(u16 max_current_ma, u16 shunt_resistor_mohm) {
	if (INA219_CONFIGURED == 1) return;

	i2c_err_t ret;
	//# Configure INA219
	u16 config = (0 << 13) |	// 16V range
				 (0 << 11) |	// +-40 mV
				 (3 << 7)  |	// 12-bit Bus Resolution
				 (3 << 3)  |	// 12-bit Shunt Resolution
				 (7 << 0);		// Mode: Shunt and Bus Continous
	u8 config_bytes[2] = {config >> 8, config & 0xFF};
	// ina219_config_binary(config);

	ret = i2c_write_reg(&dev_sensor, 0x00, config_bytes, 2);

	i2c_read_reg(&dev_sensor, INA219_REG_CONFIG, I2C_DATA_BUF, 2);
	u16 config_read = BUF_MAKE_U16(I2C_DATA_BUF);
	// printf("config: 0x%04X\n", config);
	// printf("\nConfig register: 0x%04X\n", config_read);


	//# Calibrate for 1A range (assuming R_shunt = 0.1 Ohm = 100 mOhm)
	// calib = .04096 / (current_LSB * R_shunt)
	// current_LSB = max_current/32768
	// => calib = .04096 / ((max_current/32768) * R_shunt)
	// => calib = .04096 * 32768 / (max_current * R_shunt)
	// => calib = .04096 * 32768 * 10^6 / (max_current_ma * R_shunt_mohm)
	// => calib = 1342,177,280 / (max_current_ma * R_shunt_mohm)
	// if max_current_ma = 1000 mA, R_shunt_mohm = 100 mOhm, calib = 13421

	u32 cal = 1342177280 / (max_current_ma * shunt_resistor_mohm);
	u8 cal_bytes[2] = {cal >> 8, cal & 0xFF};
	i2c_write_reg(&dev_sensor, INA219_REG_CALIB, cal_bytes, 2);
	// Delay_Ms(1);

	i2c_read_reg(&dev_sensor, INA219_REG_CALIB, I2C_DATA_BUF, 2);
	u16 cal_read = BUF_MAKE_U16(I2C_DATA_BUF);

	// printf("\nCalibration value: %ld\n", cal);
	// printf("Calibration register: 0x%04X\n", cal_read);

	INA219_CONFIGURED = 1;
}

void test_ina219(u16 *shunt_uV, u16 *bus_mV, u16 *current_uA, u16 *power_uW) {
	dev_sensor.addr = 0x40;

	if (i2c_ping(dev_sensor.addr) != I2C_OK) {
		printf("INA219 not found\n");
		return;
	}

	i2c_err_t ret;

	//# configure and calibrate
	// max_cal <= 0xFFFF, solve for usuable max_current & shunt
	// max_current_mA * shunt_mOhm >= 1342,177,280/0XFFFF = 20480
	// max_current_mA * shunt_mOhm has to be greater than 20480 for a valid calibration value
	// if shunt_mOhm = 100 mOhm, max_current_mA has to be at least 200 (300 recommended bc of rounding errors)
	u16 MAX_CURRENT_MA = 300;
	ina219_configure(MAX_CURRENT_MA, 140);

	//# Read shunt voltage
	ret = i2c_read_reg(&dev_sensor, INA219_REG_SHUNT, I2C_DATA_BUF, 2);
	u16 shunt_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*shunt_uV = shunt_raw * 10;	

	//# Read bus voltage
	ret = i2c_read_reg(&dev_sensor, INA219_REG_BUS, I2C_DATA_BUF, 2);
	u16 bus_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*bus_mV = (bus_raw >> 3) * 4;

	//# Read power
	ret = i2c_read_reg(&dev_sensor, INA219_REG_POWER, I2C_DATA_BUF, 2);
	// power_LSB = 20 * current_LSB = 20 * max_current/32768
	u16 power_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*power_uW = 20 * power_raw * 1000 * MAX_CURRENT_MA / 32768;

	//# Read current
	ret = i2c_read_reg(&dev_sensor, INA219_REG_CURRENT, I2C_DATA_BUF, 2);
	// current_LSB = max_current/32768
	// current_mA = current_raw * current_LSB
	u16 current_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*current_uA = current_raw * 1000 * MAX_CURRENT_MA / 32768;

	#ifdef I2C_SENSORS_DEBUG_LOG
		printf("\nINA219 Shunt: %dmV, Bus: %dmV, Current: %dmA\n", 
				*shunt_mV, *shunt_mV, *current_uA);
	#endif
}
