// MIT License
// Copyright (c) 2025 UniTheCat

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include "ch32fun.h"
#include <stdint.h>
#include <stdio.h>

#include "fonts/font_5x7.h"

#ifndef DELAY_US
	#define DELAY_US(x)
#endif

//# LCD1602 Pinouts
//# Note: Use D7-D4 for 4bits mode, D7-D0 for 8bits mode
// 1 - K (LED Cathode)	
// 2 - A (LED Anode - 3.3Vdc)
// 3 - D7 (MSB)
// 4 - D6
// 5 - D5
// 6 - D4
// 7 - D3
// 8 - D2
// 9 - D1
// 10 - D0
// 11 - E	(1 = Enable)
// 12 - RW  (GND for Write)
// 13 - RS	(0 = Cmd, 1 = Data)
// 14 - VDD (5.5Vdc)
// 15 - VSS (GND)

//! ####################################
//! TRANSFER FUNCTIONS
//! ####################################

// D0, D1, D2, D3, D4, D5, D6, D7 (MSB)
int LCD1602_GPIOS[8] = {-1};

u8 LCD1602_EN_PIN = -1;
u8 LCD1602_RS_PIN = -1;
u8 LCD1602_MODE_8BITS = 0;

// Set RS: 0 = command, 1 = data
void LCD1602_COMMAND_MODE() { funDigitalWrite(LCD1602_RS_PIN, 0); }
void LCD1602_DATA_MODE() 	{ funDigitalWrite(LCD1602_RS_PIN, 1); }

void LCD1602_COMMIT() {
	// Pulse Enable
	if (LCD1602_EN_PIN != -1) funDigitalWrite(LCD1602_EN_PIN, 1);
	DELAY_US(50);
	if (LCD1602_EN_PIN != -1) funDigitalWrite(LCD1602_EN_PIN, 0);
	DELAY_US(50);
}

void lcd1602_write4(u8 nimble) {
	funDigitalWrite(LCD1602_GPIOS[7], (nimble >> 3) & 1);
	funDigitalWrite(LCD1602_GPIOS[6], (nimble >> 2) & 1);
	funDigitalWrite(LCD1602_GPIOS[5], (nimble >> 1) & 1);
	funDigitalWrite(LCD1602_GPIOS[4], (nimble >> 0) & 1);
	LCD1602_COMMIT();
}

void lcd1602_write8(u8 data) {
	if (LCD1602_MODE_8BITS) {
		funDigitalWrite(LCD1602_GPIOS[7], (data >> 7) & 1);
		funDigitalWrite(LCD1602_GPIOS[6], (data >> 6) & 1);
		funDigitalWrite(LCD1602_GPIOS[5], (data >> 5) & 1);
		funDigitalWrite(LCD1602_GPIOS[4], (data >> 4) & 1);

		funDigitalWrite(LCD1602_GPIOS[3], (data >> 3) & 1);
		funDigitalWrite(LCD1602_GPIOS[2], (data >> 2) & 1);
		funDigitalWrite(LCD1602_GPIOS[1], (data >> 1) & 1);
		funDigitalWrite(LCD1602_GPIOS[0], (data >> 0) & 1);
		LCD1602_COMMIT();
	} else {
		u8 output = data >> 4;
		lcd1602_write4(output);
		output = data & 0x0F;
		lcd1602_write4(output);
	}
}


//! ####################################
//! CONTROL FUNCTIONS
//! ####################################

//# DDRAM AD SET
#define LCD1602_DDRAM_ADSET			0b10000000
#define LCD1602_CGRAM_ADSET			0b01000000
#define LCD1602_LINE2_ADSET			0b0100

//# 0x01: Clear display
void fun_lcd1602_clear() 	{ lcd1602_write8(0x01); DELAY_US(2000); }

//# 0x02: Return home
void fun_lcd1602_home() 	{ lcd1602_write8(0x02); DELAY_US(2000); }

//# Send data
void fun_lcd1602_printStr(char *str, u8 row) {

    u8 address = LCD1602_DDRAM_ADSET;  // 0x80
    if (row == 1) address |= 0x40;     // Add line 2 offset
	LCD1602_COMMAND_MODE();
	lcd1602_write8(address);

	LCD1602_DATA_MODE();
	while(*str) {
		char c = *str++;
		lcd1602_write8(c);
	}
}


//! ####################################
//! INIT FUNCTIONS
//! ####################################

//# FUNCTION SET: (1, DL_bitMode, N_Line, Font)
#define LCD1602_FUNCTION_SET		0b100000
#define LCD1602_DL_8BITS_MODE		0x010000	// DL: 1 = 8 bit mode, 0 = 4 bit mode (default)
#define LCD1602_NLINE_2LINES		0b001000	// N_Line: 1 = 2 lines, 0 = 1 line (default)
#define LCD1602_FONT_5X10			0b000100	// Font: 1 = 5x10, 0 = 5x7 (default)

//# DISPLAY SWITCH: (1, Display_ON, Cursor_ON, Blink_ON)
#define LCD1602_DISPLAY_SWITCH		0b1000
#define LCD1602_DISPLAY_ON			0b0100		// default off
#define LCD1602_CURSOR_ON			0b0010		// default off
#define LCD1602_BLINK_ON			0b0001		// default off

//# INPUT SET: (1, Cursor_Dir, Shift)
#define LCD1602_INPUT_SET			0b0100
#define LCD1602_CURSOR_INCREMENT	0b0010		// default off
#define LCD1602_SHIFT				0b0001		// default off


//! REQUIRED: LoadGPIOs first!
u8 fun_lcd1602_loadGPIOs(int *pin) {
	u8 gpio_count = 0;

	for (int i = 0; i < 8; i++) {
		LCD1602_GPIOS[i] = pin[i];

		if (LCD1602_GPIOS[i] == -1) continue;
		funPinMode(LCD1602_GPIOS[i], GPIO_CFGLR_OUT_50Mhz_PP);
		gpio_count++;
	}

	LCD1602_MODE_8BITS = (gpio_count == 8);
	return LCD1602_MODE_8BITS;
}

void fun_lcd1602_init(u8 enPin,u8 rsPin) {
	LCD1602_EN_PIN = enPin;
	LCD1602_RS_PIN = rsPin;
	funPinMode(LCD1602_EN_PIN, GPIO_CFGLR_OUT_50Mhz_PP);
	funPinMode(LCD1602_RS_PIN, GPIO_CFGLR_OUT_50Mhz_PP);

	LCD1602_COMMAND_MODE();

	// Step 1: Function set (3 times)
	lcd1602_write4(0b0011); DELAY_US(5000);
	lcd1602_write4(0b0011); DELAY_US(1000);
	lcd1602_write4(0b0011); DELAY_US(1000);
	lcd1602_write4(0b0010); // Set 4-bit mode
	DELAY_US(1000);

	lcd1602_write8(LCD1602_FUNCTION_SET | LCD1602_NLINE_2LINES);
	// lcd1602_write8(0x28);  // Command
	// Function Set: 4-bit, 2 lines, 5x8 font
	// lcd1602_write8(LCD1602_FUNCTION_SET | LCD1602_NLINE_2LINES);
	DELAY_US(1000);

	// Input Set: Increment cursor, no shift
	lcd1602_write8(LCD1602_INPUT_SET | LCD1602_CURSOR_INCREMENT);
	DELAY_US(1000);

	// Display Switch: Display ON, cursor OFF, blink OFF
	lcd1602_write8(LCD1602_DISPLAY_SWITCH | LCD1602_DISPLAY_ON);  // Command
	DELAY_US(1000);

	fun_lcd1602_clear();
	DELAY_US(1000);
}