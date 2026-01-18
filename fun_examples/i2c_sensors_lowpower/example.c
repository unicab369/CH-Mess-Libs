#include "../../fun_modules/fun_base.h"
#include "../../fun_modules/fun_i2c/fun_i2c_sensors.h"
#include "../../fun_modules/fun_clockfreq.h"
#include "../../fun_modules/fun_irSender.h"


#define IR_SENDER_PIN PD0

i2c_device_t dev_i2c = {
	.clkr = I2C_CLK_100KHZ,
	.type = I2C_ADDR_7BIT,
	.addr = 0x23,				// Default address for BH1750
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

void sleep_prepare() {
	// Set all GPIOs to input pull up to reduce power
	funGpioInitAll();

	GPIOA->CFGLR = (GPIO_CNF_IN_PUPD<<(4*2)) | (GPIO_CNF_IN_PUPD<<(4*1));
	GPIOA->BSHR = GPIO_BSHR_BS2 | GPIO_BSHR_BR1;

	GPIOC->CFGLR = (GPIO_CNF_IN_PUPD<<(4*7)) | (GPIO_CNF_IN_PUPD<<(4*6)) |
					(GPIO_CNF_IN_PUPD<<(4*5)) | (GPIO_CNF_IN_PUPD<<(4*4)) |
					(GPIO_CNF_IN_PUPD<<(4*3)) | (GPIO_CNF_IN_PUPD<<(4*2)) |
					(GPIO_CNF_IN_PUPD<<(4*1)) | (GPIO_CNF_IN_PUPD<<(4*0));
	GPIOC->BSHR = GPIO_BSHR_BS7 | GPIO_BSHR_BS6 |
					GPIO_BSHR_BS5 | GPIO_BSHR_BS4 |
					GPIO_BSHR_BS3 | GPIO_BSHR_BS2 |
					GPIO_BSHR_BS1 | GPIO_BSHR_BS0;
				
	GPIOD->CFGLR = (GPIO_CNF_IN_PUPD<<(4*7)) | (GPIO_CNF_IN_PUPD<<(4*6)) |
					(GPIO_CNF_IN_PUPD<<(4*5)) | (GPIO_CNF_IN_PUPD<<(4*4)) |
					(GPIO_CNF_IN_PUPD<<(4*3)) |(GPIO_CNF_IN_PUPD<<(4*2)) |
					(GPIO_CNF_IN_PUPD<<(4*0));
	GPIOD->BSHR = GPIO_BSHR_BS7 | GPIO_BSHR_BS6 |
					GPIO_BSHR_BS5 | GPIO_BSHR_BS4 |
					GPIO_BSHR_BS3 | GPIO_BSHR_BS2 |
					GPIO_BSHR_BS0;
}

void sleep_init() {
	// enable power interface module clock
	RCC->APB1PCENR |= RCC_APB1Periph_PWR;

	// enable low speed oscillator (LSI)
	RCC->RSTSCKR |= RCC_LSION;
	while ((RCC->RSTSCKR & RCC_LSIRDY) == 0) {}

	// enable AutoWakeUp event
	EXTI->EVENR |= EXTI_Line9;
	EXTI->FTENR |= EXTI_Line9;

	// t = AWUWR * AWUPSC / fLSI; fLSI = 128000; AWUWR = 1 to 63; AWUPSC = prescaler_1 to prescaler_4096
	// configure AWU prescaler
	PWR->AWUPSC |= PWR_AWU_Prescaler_10240;

	// configure AWU window comparison value
	PWR->AWUWR &= ~0x3f;
	PWR->AWUWR |= 50;		// 1 to 63

	// enable AWU
	PWR->AWUCSR |= (1 << 1);

	// select standby on power-down
	PWR->CTLR |= PWR_CTLR_PDDS;

	// peripheral interrupt controller send to deep sleep
	PFIC->SCTLR |= (1 << 2);
}

// #define SLEEP_MODE_ENABLE

static u8 str_output[40] = { 0 };

IR_Sender_t irSender = {
	.pin = IR_SENDER_PIN,

	//! NfS protocol
	.LOGICAL_1_US = 550,
	.LOGICAL_0_US = 300,
	.START_HI_US = 2500,
	.START_LO_US = 2500
};


#ifdef SLEEP_MODE_ENABLE
int main() {
	SystemInit();
	
	// This delay gives us some time to reprogram the device. 
	// Otherwise if the device enters standby mode we can't 
	// program it any more.
	Delay_Ms(4000);

	//! NOTE: DO NOT CALL BEFORE Delay, it overwrite the 
	sleep_prepare();
	sleep_init();
	uint16_t counter = 0;

	while(1) {
		__WFE();
		// restore clock to full speed
		SystemInit();

		funGpioInitAll();
		funPinMode(PC0, GPIO_CFGLR_OUT_10Mhz_PP);
		funDigitalWrite(PC0, 1);
		Delay_Ms(1);

		//# IR Sender
		fun_irSender_init(&irSender);
		memset(str_output, 0, 40);

		//# I2C
		if(i2c_init(&dev_i2c) == I2C_OK) {
			u16 lux;
			test_bh1750(&lux);

			u16 tempF, hum;
			test_sht3x(&tempF, &hum);

			u16 shunt_mV, bus_mV, current_uA, power_uW;
			test_ina219(&shunt_mV, &bus_mV, &current_uA, &power_uW);

			sprintf(str_output, "SE1 %d %d %d %d %d %d %d",
					lux, tempF, hum, shunt_mV, bus_mV, current_uA, power_uW);
		} else {
			memcpy(str_output, "ERR I2C init failed", 40);
		}

		fun_irSender_blockingTask(&irSender, str_output, 24);
		funDigitalWrite(PC0, 0);

		// printf("\r\nawake, %u\r\n", counter++);
	}
}

#else

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	funPinMode(PC0, GPIO_CFGLR_OUT_10Mhz_PP);
	funDigitalWrite(PC0, 1);

	fun_clockfreq_set(CLOCK_48Mhz);

	u32 time_ref = millis();

	if(i2c_init(&dev_i2c) != I2C_OK) {
		printf("Err: I2C init failed\n");
	} else {
		i2c_start_scan();

		// u16 lux;
		// test_bh1750(&lux);
		// sprintf(str_output, "LUX %d", lux);
		// printf("%s\n", str_output);

		// i2c_ina219_setup(0, 3);
	}

	while(1) {
		u32 moment = millis();

		if (moment - time_ref > 2000) {
			time_ref = moment;

			// i2c_ina219_setup(0, 3);

			// u16 lux;
			// test_bh1750(&lux);
			// sprintf(str_output, "LUX %d", lux);
			// printf("%s\n", str_output);

			// u16 tempF, hum;
			// test_sht3x(&tempF, &hum);
			// sprintf(str_output, "TEMP %d, HUM %d", tempF, hum);
			// printf("%s\n", str_output);

			// int16_t shunt_uV, current_uA;
			// u16 bus_mV, power_uW;

			// i2c_ina219_reading(&shunt_uV, &bus_mV, &power_uW, &current_uA);
			// sprintf(str_output, "\nShunt %d uV, bus %d mV", shunt_uV, bus_mV);
			// printf("%s\n", str_output);

			// sprintf(str_output, "%d uW, %d uA", power_uW, current_uA);
			// printf("%s\n", str_output);	

			printf("\n");
		}
	}
}

#endif		// SLEEP_MODE_ENABLE