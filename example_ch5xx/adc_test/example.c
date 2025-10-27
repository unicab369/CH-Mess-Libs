#include "fun_adc_ch5xx.h"

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	u32 time_ref = millis();
	u16 raw_adc[100];

	while(1) {
		u32 moment = millis();

		if (moment - time_ref > 1000) {
			time_ref = moment;
			// //# get temp reading
			// adc_set_channel(ADC_TEMPERATURE_CHANNEL);
			// adc_set_config(ADC_FREQ_DIV_10, ADC_PGA_GAIN_2, 1);
			// raw_adc = adc_get_singleReading();
			// printf("\nTemperature: %d mC\r\n", adc_to_mCelsius(raw_adc));

			// //# get battery reading
			// adc_set_channel(ADC_VBAT_CHANNEL);
			// adc_set_config(ADC_FREQ_DIV_10, ADC_PGA_GAIN_1_4, 0);
			// raw_adc = adc_get_singleReading();
			// printf("Battery Voltage: %d mV\r\n", adc_to_mV(raw_adc, ADC_PGA_GAIN_1_4));

			// get ch0 reading PA4
			adc_set_channel(0);
			adc_buf_enable(1);
			adc_set_config(ADC_FREQ_DIV_10, ADC_PGA_GAIN_1_4, 0);
			
			//# get single channel reading
			// raw_adc[0] = adc_get_singleReading();
			// printf("PA4: %d mV\r\n", adc_to_mV(raw_adc[0], ADC_PGA_GAIN_1_4));

			//# get dma readings
			adc_dma_start(250, raw_adc, 100, 1);
			adc_dma_wait();
			Delay_Ms(100);
			adc_dma_stop();

			for (int i = 0; i < 100; i++) {
				printf("PA4: %d mV\r\n", adc_to_mV(raw_adc[i], ADC_PGA_GAIN_1_4));
			}
			
		}
	}
}