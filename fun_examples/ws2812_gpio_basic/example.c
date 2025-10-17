// Stolen from Chuck

#include "../../fun_modules/fun_base.h"

#define PIXELS 7
#define COLORS (PIXELS * 3)
#define BUFLEN (COLORS * 24)

uint8_t buf[BUFLEN];

/* green pixels */
uint8_t pixel[COLORS] = {
	0x0, 0x0, 0x0,	// Black
	0x0, 0x0, 0x0,	// Blue
	0x0, 0x0, 0x0,	// Red
	0x0, 0x0, 0x0,	// Magenta
	0x0, 0x0, 0x0,	// Green
	0x0, 0x0, 0x0,	// Cyan
	0x0, 0x0, 0x0,	// Yellow
	0x0, 0x0, 0x0,	// White
};

/* This just points at the GPIO Port C output data register */
uint32_t *port_c = (uint32_t *)(0x4001110C);

/* Convert the pixel data into a buffer. 
 * This iterates over the 3 color components (G, R, B), and then the 8 
 * bits of those colors, sending them MSB to LSB, one after the other. 
 * PC1 is our data path so 0x2 is on, 0x0 is off, PC2 is our trigger 
 * indicator so 0x4 is we'er xmitting, 0x0 is we aren't.
 */
void build_buffer() {
	for (int i = 0, ndx = 0; i < COLORS; i++) {
		uint8_t c = pixel[i];	// color
		
		for (int bit = 0; bit < 8; bit++) {
			buf[ndx++] = 6;
			buf[ndx++] = 4 + ((c & 0x80) >> 6);
			buf[ndx++] = 4;
			c = c << 1;
		}
	}
}

void update_pixels() {
	*port_c = 0x0;
	/* generate the 50uS 'reset' signal by holding input low */
	// spinloop delay for 50 uS
	for (int i = 0; i < 500; i++) {
		asm("nop");
	}

	for (int i = 0; i < BUFLEN; i++) {
		*port_c = buf[i];
		// So full speed is 288nS per bit time. Need 400ns
		asm(" nop ");
		asm(" nop ");
		asm(" nop ");
		asm(" nop ");
		asm(" nop ");
		asm(" nop ");
		/* With 6 NOPs we are at 406.5nS which seems good enough */
	}
}

void set_pixel(int n, uint8_t r, uint8_t g, uint8_t b) {
	if ((n < 0) || (n >= PIXELS)) {
		return; // bad pixel number
	}

	pixel[n*3] = g;
	pixel[n*3+1] = r;
	pixel[n*3+2] = b;
}

int main() {
	int step = 0;
	int direction = 1;
	SystemInit();

	/* Set up the GPIO pins */
    funGpioInitAll();
    funPinMode(PC1, GPIO_CFGLR_OUT_50Mhz_PP);
    funPinMode(PC2, GPIO_CFGLR_OUT_50Mhz_PP);
	
	while (1) {
		switch (step) {
			case 0:
				if (direction == -1) {
					direction = 1;
				}
				set_pixel(0, 0x0, 0, 0x80);
				break;
			case 7:
				if (direction == 1) {
					direction = -1;
				}
				set_pixel(7, 0x80, 0, 0);
				break;
			default:
				set_pixel(step - direction, 0, 0, 0);
				if (direction == 1) {
					set_pixel(step, 0, 0, 0x80);
				} else {
					set_pixel(step, 0x80, 0, 0);
				}
				break;
		}
		
		step = step + direction;
		build_buffer();
		update_pixels();
		Delay_Ms(100);
	}
}