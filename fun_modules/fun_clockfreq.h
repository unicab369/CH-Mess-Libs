// modified by uniTheCat
// source: https://github.com/bitbank2/ch32v003fun/blob/5691ce6b331c8f3bb198c0238ac34b787944eeb1/extralibs/ch32v_hal.inl#L102

// written by Larry Bank
// bitbank@pobox.com
//
// Copyright 2023 BitBank Software, Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ch32fun.h"
#include <stdio.h>

typedef enum {
	CLOCK_48Mhz,		// 6.75 mA
	CLOCK_24Mhz,		// about the same as 48Mhz ???
	CLOCK_12Mhz,		// 4.75 mA
	CLOCK_8Mhz,			// 4.25 mA
	CLOCK_6Mhz,			// 3.75 mA
	CLOCK_4P8Mhz,		// 3.45 mA
	CLOCK_4Mhz,			// 3.25 mA
	CLOCK_3P4Mhz,		// 3.10 mA
	CLOCK_3Mhz,			// 3.00 mA
	CLOCK_1P5Mhz,		// 2.65 mA
	CLOCK_750Khz,		// 2.50 mA
	CLOCK_375Khz,		//! 1.90 mA NOTE: Bricks device. unbrick with .\minichlink.exe -u
	CLOCK_185Khz,
} Clock_Frequency_t;

void fun_clockfreq_set(Clock_Frequency_t freq) {
	uint32_t u32Div = 0;
	uint32_t SystemCoreClock = 48000000;

	switch(freq) {
		case CLOCK_48Mhz:
			break;
		case CLOCK_24Mhz:
			SystemCoreClock = 24000000; u32Div = RCC_HPRE_DIV1; break;
		case CLOCK_12Mhz:
			SystemCoreClock = 12000000; u32Div = RCC_HPRE_DIV2; break;
		case CLOCK_8Mhz:
			SystemCoreClock = 8000000; u32Div = RCC_HPRE_DIV3; break;
		case CLOCK_6Mhz:
			SystemCoreClock = 6000000; u32Div = RCC_HPRE_DIV4; break;
		case CLOCK_4P8Mhz:
			SystemCoreClock = 4800000; u32Div = RCC_HPRE_DIV5; break;
		case CLOCK_4Mhz:
			SystemCoreClock = 4000000; u32Div = RCC_HPRE_DIV6; break;
		case CLOCK_3P4Mhz:
			SystemCoreClock = 3428571; u32Div = RCC_HPRE_DIV7; break;
		case CLOCK_3Mhz:
			SystemCoreClock = 3000000; u32Div = RCC_HPRE_DIV8; break;
		case CLOCK_1P5Mhz:
			SystemCoreClock = 1500000; u32Div = RCC_HPRE_DIV16; break;
		case CLOCK_750Khz:
			SystemCoreClock = 750000; u32Div = RCC_HPRE_DIV32; break;
		case CLOCK_375Khz:
			SystemCoreClock = 375000; u32Div = RCC_HPRE_DIV64; break;
		case CLOCK_185Khz:
			SystemCoreClock = 187500; u32Div = RCC_HPRE_DIV128; break;
	}
	
	switch (SystemCoreClock) {
		case 48000000: // special case - needs PLL
			/* Flash 0 wait state */
			FLASH->ACTLR &= (uint32_t)((uint32_t)~FLASH_ACTLR_LATENCY);
			FLASH->ACTLR |= (uint32_t)FLASH_ACTLR_LATENCY_1;

			/* HCLK = SYSCLK = APB1 */
			RCC->CFGR0 |= (uint32_t)RCC_HPRE_DIV1;

			/* PLL configuration: PLLCLK = HSI * 2 = 48 MHz */
			RCC->CFGR0 &= (uint32_t)((uint32_t)~(RCC_PLLSRC));
			RCC->CFGR0 |= (uint32_t)(RCC_PLLSRC_HSI_Mul2);

			/* Enable PLL */
			RCC->CTLR |= RCC_PLLON;
			/* Wait till PLL is ready */
			while((RCC->CTLR & RCC_PLLRDY) == 0) { }

			/* Select PLL as system clock source */
			RCC->CFGR0 &= (uint32_t)((uint32_t)~(RCC_SW));
			RCC->CFGR0 |= (uint32_t)RCC_SW_PLL;
			/* Wait till PLL is used as system clock source */
			while ((RCC->CFGR0 & (uint32_t)RCC_SWS) != (uint32_t)0x08) {}
			break;

		default: // simpler - just use the RC clock with a divider
			/* Flash 0 wait state */
			FLASH->ACTLR &= (uint32_t)((uint32_t)~FLASH_ACTLR_LATENCY);
			FLASH->ACTLR |= (SystemCoreClock >= 24000000) ? (uint32_t)FLASH_ACTLR_LATENCY_1 : (uint32_t)FLASH_ACTLR_LATENCY_0;

			/* HCLK = SYSCLK = APB1 */
			RCC->CFGR0 |= u32Div;
			break;
	} // switch on clock
}
