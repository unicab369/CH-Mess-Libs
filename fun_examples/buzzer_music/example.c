// stream and play music method was borrowed and modified from:
// https://github.com/atomic14/ch32v003-music/tree/main/SimpleSoundFirmware/src

#include "../../fun_modules/fun_base.h"
#include "sounds.h"

#define BUZZER_PIN PD0


void play_music(
	const NoteCmd *midi_cmds, int midi_cmds_len,
	int max_len_us, int pitch_shift
) {
	int total_elapsed = 0;

	// Iterate through all note commands
	for (int i = 0; i < midi_cmds_len; i++) {
		NoteCmd n = midi_cmds[i];
		// 0 period_us indicates a rest
		if (n.period_us == 0) {
			int rest_duration = n.duration_us / pitch_shift;
			Delay_Us(rest_duration);
			total_elapsed += rest_duration;
			continue;
		}

		// Play the note by toggling the pin at the specified frequency
		int elapsed = 0;
		int period_us = n.period_us / pitch_shift;

		while (elapsed < n.duration_us) {
			funDigitalWrite(BUZZER_PIN, 1);
			// Delay_Us(20);
			Delay_Us(period_us/10); 				// Wait for half the period
			funDigitalWrite(BUZZER_PIN, 0);

			// Delay_Us(n.period_us - 20);    		// Wait for the other half
			Delay_Us(period_us - period_us/10); 	// Wait for half the period
			elapsed += period_us;    				// Track how long we've been playing
		}

		total_elapsed += elapsed;

		// Stop if we've exceeded the maximum playback time
		if (total_elapsed > max_len_us) break;
	}
}

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	funPinMode(BUZZER_PIN, GPIO_CFGLR_OUT_10Mhz_PP);
	u32 time_ref = millis();

	while(1) {
		u32 moment = millis();

		if (moment - time_ref > 1000) {
			time_ref = moment;

			play_music(leadline_stream_0, sizeof(leadline_stream_0)/sizeof(NoteCmd), 30E6, 2);
			printf("IM HERE\r\n");
		}
	}
}