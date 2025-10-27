#include "../../modules_ch5xx/fun_rtc.h"


int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();

	rtc_date_t date = {2025, 10, 25};
	rtc_time_t time = {8, 30, 5};
	fun_rtc_setDateTime(date, time);

	u32 time_ref = millis();

	while(1) {
		u32 moment = millis();

		if (moment - time_ref > 1000) {
			time_ref = moment;

			// fun_rtc_printRaw();
			fun_rtc_getDateTime(&date, &time);
			
			printf("%04d-%02d-%02d %02d:%02d:%02d.%03d\r\n",
				date.year, date.month, date.day,
				time.hr, time.min, time.sec, time.ms);
		}
	}
}