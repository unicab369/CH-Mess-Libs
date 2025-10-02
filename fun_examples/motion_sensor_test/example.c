#include "../../fun_modules/fun_gpioSensor.h"



void gpioSensor_handler(int state, u32 elapsed_ms) {
    printf("State: %d, elapsed: %d ms\r\n", state, elapsed_ms);
}

int main() {
    SystemInit();
    systick_init();			//! required for millis()

    Delay_Ms(100);
    funGpioInitAll();
    
    GpioSensor_t gpio_sensor = {
        .pin = PC0,
        .timeout_duration_ms = 8000
    };

    fun_gpioSensor_init(&gpio_sensor);

    u32 timeRef = millis();

    while(1) {
        if ((millis() - timeRef) > 3000) {
            timeRef = millis();
            // printf("IM HERE\r\n");
        }
        
        fun_gpioSensor_task(&gpio_sensor, gpioSensor_handler);
    }
}