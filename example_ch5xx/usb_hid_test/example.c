// Simple ADC example to read temperature, battery voltage, and ADC channel 0 (PA4)
#include "ch32fun.h"
#include "ch5xx_usb_hid.h"

#define FUNCONF_UART_PRINTF_BAUD 115200
#define TARGET_UART &R32_UART1_CTRL

// void printf(const char *str) {
// 	uart_send_ch5xx(TARGET_UART, str, strlen(str));
// }

int main(void) {
	SystemInit();
	Delay_Ms(100);
	
	// uart_init_ch5xx(TARGET_UART, FUNCONF_UART_PRINTF_BAUD);
	printf("CH583 USB CDC Demo Started\n");
	USB_DeviceInit();

    while(1) {
        if(Ready) {
            Ready = 0;
            DevHIDReport(0x05, 0x10, 0x20, 0x11);
        }
        Delay_Ms(1000);

        if(Ready) {
            Ready = 0;
            DevHIDReport(0x0A, 0x15, 0x25, 0x22);
        }
        Delay_Ms(1000);

        if(Ready) {
            Ready = 0;
            DevHIDReport(0x0E, 0x1A, 0x2A, 0x44);
        }
        Delay_Ms(1000);

        if(Ready) {
            Ready = 0;
            DevHIDReport(0x10, 0x1E, 0x2E, 0x55);
        }
        Delay_Ms(1000);
    }
}
