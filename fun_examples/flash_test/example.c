#include "../../fun_modules/fun_base.h"
#include "nvs.h"

#define NVS_SIGNATURE 0xBEEF

typedef struct {
    u32 value1;
    u32 value2;
    u16 signature;
} MyStruct_t;


static volatile MyStruct_t myStruct = {
    .value1 = 111,
    .value2 = 222,
};

int main() {
	SystemInit();
	systick_init();			//! REQUIRED for millis()
	Delay_Ms(100);
	funGpioInitAll();
	
	NVS_Init();
    NVS_Load(&myStruct, 0, sizeof(myStruct));
	printf("\nValue1: %d, Value2: %d", myStruct.value1, myStruct.value2);

	myStruct.value1++;
	myStruct.value2++;

	NVS_Save(&myStruct, sizeof(myStruct));
	NVS_Load(&myStruct, 0, sizeof(myStruct));
	printf("\nValue1: %d, Value2: %d", myStruct.value1, myStruct.value2);

	u32 time_ref = millis();

	while(1) {
		u32 moment = millis();

		if (moment - time_ref > 1000) {
			time_ref = moment;
			printf("IM HERE\r\n");
		}
	}
}