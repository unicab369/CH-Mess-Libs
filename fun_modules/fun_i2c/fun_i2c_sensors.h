// MIT License
// Copyright (c) 2025 UniTheCat

#include "ch32fun.h"
#include <stdio.h>

#include "lib/lib_i2c.h"

#define I2C_SENSORS_DEBUG_LOG

#define BUF_MAKE_U16(buff) ((buff[0] << 8) | buff[1])
#define GET_BIT(val, n) ((val >> n) & 1)

i2c_device_t dev_sensor = {
	.clkr = I2C_CLK_100KHZ,
	.type = I2C_ADDR_7BIT,
	.addr = 0x3C,
	.regb = 1,
};

u8 I2C_DATA_BUF[8];

i2c_err_t test_bh1750(u16 *lux) {
	dev_sensor.addr = 0x23;

	if (i2c_ping(dev_sensor.addr) != I2C_OK) {
		printf("BH1750 not found\n");
		return I2C_ERR_BERR;
	}

	//# Power ON
	i2c_err_t ret = i2c_write_raw(&dev_sensor, (u8[]){0x01}, 1);
	if (ret != I2C_OK) { return ret; }

	// Continuous H-Resolution mode 	0x10 1lx resolution 	- 120ms
	// Continuous H-Resolution mode2 	0x11 0l5lx resolution 	- 120ms
	// Continuous L-Resolution mode 	0x13 4lx resolution 	- 16ms
	// One-time H-Resolution mode		0x20 1lx resolution 	- 120ms
	// One-time H-Resolution mode2 		0x21 0.5lx resolution 	- 120ms
	// One-time L-Resolution mode		0x23 4lx resolution 	- 16ms
	ret = i2c_write_raw(&dev_sensor, (u8[]){0x23}, 1);
	if (ret != I2C_OK) { return ret; }
	Delay_Ms(20);	//! REQUIRED

	ret = i2c_read_reg(&dev_sensor, 0x13, I2C_DATA_BUF, 2);
	if (ret != I2C_OK) { return ret; }

	u16 lux_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	*lux = lux_raw * 12 / 10;

	#ifdef I2C_SENSORS_DEBUG_LOG
		printf("\nBH1750: %d lx\n", *lux);
	#endif
}

i2c_err_t test_sht3x(u16 *tempF, u16 *hum) {
	dev_sensor.addr = 0x44;

	if (i2c_ping(dev_sensor.addr) != I2C_OK) {
		printf("SHT3X not found\n");
		return I2C_ERR_BERR;
	}

	// Soft reset - will always return busy
	i2c_err_t ret = i2c_write_raw(&dev_sensor, (u8[]){ 0x30, 0xA2}, 2);
	Delay_Ms(5);	//! REQUIRED
	
	// High repeatability, 1 update per second
	ret = i2c_write_raw(&dev_sensor, (u8[]){0x21, 0x30}, 2);
	if (ret != I2C_OK) { return ret; }
	Delay_Ms(5);	//! REQUIRED

	ret = i2c_read_raw(&dev_sensor, I2C_DATA_BUF, 6);
	if (ret != I2C_OK) { return ret; }

	u16 temp_raw = BUF_MAKE_U16(I2C_DATA_BUF);
	u16 hum_raw = (I2C_DATA_BUF[3] << 8) | I2C_DATA_BUF[4];
	*tempF = (175 * temp_raw) >> 16;		// >> 16 is equivalent to / 65536
	*hum = (100 * hum_raw) >> 16;			// >> 16 is equivalent to / 65536

	#ifdef I2C_SENSORS_DEBUG_LOG
		printf("\nSHT3X temp: %d, hum: %d\n", *tempF, *hum);
	#endif

	return ret;
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

uint16_t ina219_current_divider = 0;
uint16_t ina219_power_multiplier_uW = 0;

u32 max_shunt_mV = 0;

#define R_SHUNT_mOHM 100

// bus_vRange = 1 (32V) or 0 (16V)
// pg_gain = 0 (gain/1 40mV), 1 (gain/2 80mV), 2 (gain/4 160mV), 3 (gain/8 320mV)
void i2c_ina219_setup(u8 bus_vRange, u8 pg_gain) {
	dev_sensor.addr = 0x40;

	if (i2c_ping(dev_sensor.addr) != I2C_OK) {
		printf("INA219 not found\n");
		return;
	}

	// 9bits				= 0x0000	84us
	// 10bits 				= 0x0080	148us 
	// 11bits 				= 0x0100	276us 
	// 12bits 				= 0x0180	532us 
	// 12bits 2 samples 	= 0x0480	1.06ms
	// 12bits 4 samples 	= 0x0500	2.13ms
	// 12bits 8 samples 	= 0x0580	4.26ms	
	// 12bits 16 samples 	= 0x0600	8.51ms	
	// 12bits 32 samples 	= 0x0680	17.02ms	
	// 12bits 64 samples 	= 0x0700	34.05ms	 
	// 12bits 128 samples 	= 0x0780	68.10ms
	uint16_t BUS_RESOLUTION_AVERAGE = 0x0180;

	// Shunt Resolution = Bus_Resolution >> 1
	uint16_t SHUNT_RESOLUTION_AVERAGE = 0x0018;

	// 0x00 = Power Down
	// 0x01 = Shunt Voltage, triggered
	// 0x02 = Bus Voltage, triggered
	// 0x03 = Shunt and Bus Voltage, triggered
	// 0x04 = ADC Off
	// 0x05 = Shunt Voltage, continuous
	// 0x06 = Bus Voltage, continuous
	// 0x07 = Shunt and Bus Voltage, continuous - Default
	uint8_t DEVICE_MODE = 0x07;

	if (pg_gain > 3) pg_gain = 3;

	uint16_t config = (bus_vRange << 13) |
					(pg_gain << 11) |
					BUS_RESOLUTION_AVERAGE |
					SHUNT_RESOLUTION_AVERAGE |
					DEVICE_MODE;
    // printf("INA219 config: 0x%04X\n", config);
    uint8_t config_bytes[2] = {config >> 8, config & 0xFF};

	printf("Config0: 0x%02X, Config1: 0x%02X\n", config_bytes[0], config_bytes[1]);

    i2c_err_t ret;
    ret = i2c_write_reg(&dev_sensor, 0x00, config_bytes, 2);
	// Case 0: PGA ÷1 (40mV max)
	// return;

	ret = i2c_read_reg(&dev_sensor, 0x00, I2C_DATA_BUF, 2);
	printf("read config: 0x%02X, 0x%02X\n", I2C_DATA_BUF[0], I2C_DATA_BUF[1]);

    switch (pg_gain) {
        case 0:	// 40mV
			max_shunt_mV = 40;
            break;
        case 1:	// 80mV
			max_shunt_mV = 80;
            break;
        case 2:	// 160mV
			max_shunt_mV = 160;
            break;
        case 3:	// 320mV - Default
			max_shunt_mV = 320;
            break;
    }

	// Given:
	// Max_Shunt_Current = Max_shunt_V / R_shunt;
	// Current_LSB = Max_Shunt_Current / 2^15 = Max_Shunt_Current / 32768
	// calib = .04096 / (current_LSB * R_shunt)

	// Therefore:
	// calib = .04096 / ((Max_Shunt_Current/32768) * R_shunt)
	// calib = .04096 * 32768 / (Max_Shunt_Current * R_shunt)
	// calib = .04096 * 32768 / (Max_shunt_V)
	// calib = .04096 * 32768 * 10^6 / (max_shunt_mV * 1000)
	// calib = 1342,177,280 / (max_shunt_mV * 1000)
	// Note: shunt_resistor cancel out, the calibration value does not depend on shunt_resistor
	u32 calibration_value = 1342177280 / (max_shunt_mV * 1000);
    uint8_t cal_bytes[2] = {calibration_value >> 8, calibration_value & 0xFF};
    ret = i2c_write_reg(&dev_sensor, 0x05, cal_bytes, 2);

	ret = i2c_read_reg(&dev_sensor, 0x05, I2C_DATA_BUF, 2);
	printf("read cal: 0x%02X, 0x%02X\n", I2C_DATA_BUF[0], I2C_DATA_BUF[1]);

	printf("Calibration: 0x%02X, 0x%02X\n", cal_bytes[0], cal_bytes[1]);
}

void i2c_ina219_reading(
    int16_t *shunt_uV, u16 *bus_mV, u16 *power_uW, int16_t *current_uA
) {
	dev_sensor.addr = 0x40;

	if (i2c_ping(dev_sensor.addr) != I2C_OK) {
		printf("INA219 not found\n");
		return;
	}

	i2c_err_t ret;
	uint8_t buff[2];

	//# Read shunt voltage in uV
	ret = i2c_read_reg(&dev_sensor, 0x01, buff, 2);		
	int16_t raw_shunt = (int16_t)((buff[0] << 8) | buff[1]);
	*shunt_uV = raw_shunt * 10;

	//# Read bus voltage in mV
	ret = i2c_read_reg(&dev_sensor, 0x02, buff, 2);
	u16 raw_bus = (buff[0] << 8) | buff[1];
	*bus_mV = (raw_bus >> 3) * 4;

	// Max_Shunt_Current = Max_shunt_V / R_shunt;
	// Current_LSB = Max_Shunt_Current / 2^15 = Max_shunt_V / (R_shunt * 32768)
	int32_t uCurrent_LSB = 1000000 * max_shunt_mV / (R_SHUNT_mOHM * 32768);

	//# Read power in uW
	ret = i2c_read_reg(&dev_sensor, 0x03, buff, 2);		
	u16 raw_power = (buff[0] << 8) | buff[1];
	// power = raw_value * 20 * Current_LSB
	*power_uW = raw_power * 20 * uCurrent_LSB;

	//# Read current in mA
	ret = i2c_read_reg(&dev_sensor, 0x04, buff, 2);
	int16_t raw_current = (int16_t)((buff[0] << 8) | buff[1]);
	// current = raw_value * current_LSB
	*current_uA = raw_current * uCurrent_LSB;
}
