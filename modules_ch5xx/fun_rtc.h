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

#include "../fun_modules/fun_base.h"

//! ####################################
//! CORE FUNCTION
//! ####################################

void rtc_print_regs() {
	UTIL_PRINT_REG8(R8_RTC_FLAG_CTRL, "FLAG_CTRL");
	UTIL_PRINT_REG8(R8_RTC_MODE_CTRL, "MODE_CTRL");
	UTIL_PRINT_REG32(R32_RTC_TRIG, "TRIG");
	UTIL_PRINT_REG16(R16_RTC_CNT_32K, "CNT_32K");
	UTIL_PRINT_REG16(R16_RTC_CNT_2S, "CNT_2SS");
	UTIL_PRINT_REG32(R32_RTC_CNT_DAY, "CNT_DAY");
}

void fun_rtc_printRaw() {
    printf("\n32k (%d), 2s (%d), day (%ld)\r\n", R16_RTC_CNT_32K, R16_RTC_CNT_2S, R32_RTC_CNT_DAY);
}

void fun_rtc_setData(u32 day_counter, u32 time_counter) {
    SYS_SAFE_ACCESS(
		// set day counter
		R32_RTC_TRIG = day_counter;
		R8_RTC_MODE_CTRL |= RB_RTC_LOAD_HI;

		// set time counter
		R32_RTC_TRIG = time_counter;
		R8_RTC_MODE_CTRL |= RB_RTC_LOAD_LO;
	);

    fun_rtc_printRaw();
}


//! ####################################
//! SET CALENDAR FUNCTIONS
//! ####################################

#define IS_LEAP_YEAR(year) ((((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0))

// Array of days in each month (non-leap year)
const u8 DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

typedef struct {
    u16 year;
    u8 month;
    u8 day;
} rtc_date_t;

typedef struct {
    u8 hour;
    u8 min;
    u8 sec;
    u16 millis;
} rtc_time_t;


u32 calculate_day_of_year(u16 year, u8 month, u8 day) {
    u32 day_of_year = 0;
    
    // Add days from January to month-1
    for (u8 m = 0; m < month - 1; m++) {
        day_of_year += DAYS_IN_MONTH[m];
    }
    
    // Add extra day for February if it's a leap year
    if (month > 2 && IS_LEAP_YEAR(year)) {
        day_of_year += 1;
    }
    
    // Add days in the current month
    day_of_year += day;

    return day_of_year - 1;     // subtract 1 to make it 0-based
}

// the R32_RTC_CNT_DAY is only 14 bits.
//Therefore it can only store max 16383 calendar days ~ 44 years
// We will calculate the days counter from the year since 2020

void fun_rtc_setDateTime(rtc_date_t date, rtc_time_t time) {
    // Clip year since 2020
    u32 days_counter = 0;
    if (date.year < 2020) date.year = 2020;
    if (date.year > 2064) date.year = 2064;

    // Add days for complete years from 2020 to year-1
    for (u16 y = 2020; y < date.year; y++) {
        days_counter += IS_LEAP_YEAR(y) ? 366 : 365;
    }

    // Add days in the current year (0-based)
    days_counter += calculate_day_of_year(date.year, date.month, date.day);    
    fun_rtc_setData(days_counter, 0);
}


//! ####################################
//! GET CALENDAR FUNCTIONS
//! ####################################

void fun_rtc_getDateTime(rtc_date_t *date, rtc_time_t *time) {
    // Read all registers once
    u32 days_counter = R32_RTC_CNT_DAY;
    u32 seconds_2s = R16_RTC_CNT_2S;
    u32 ticks_32k = R16_RTC_CNT_32K;

    // fun_rtc_printRaw();
    
    // Calculate date from DAY counter only
    if (date != NULL) {
        u16 year = 2020;
        u8 month, day;

        // 1. Find the year
        u32 days_remaining = days_counter;
        while (days_remaining >= (IS_LEAP_YEAR(year) ? 366 : 365)) {
            days_remaining -= IS_LEAP_YEAR(year) ? 366 : 365;
            year++;
        }
        
        // 2. Find the month
        u8 days_in_feb = IS_LEAP_YEAR(year) ? 29 : 28;
        month = 1;
        while (month <= 12) {
            u8 days_in_current_month = (month == 2) ? days_in_feb : DAYS_IN_MONTH[month - 1];
            
            if (days_remaining < days_in_current_month) break;
            days_remaining -= days_in_current_month;
            month++;
        }
        
        // 3. Set the date structure
        date->year = year;
        date->month = month;
        date->day = days_remaining + 1;  // Add 1 because days_remaining is 0-based
        
        // printf("\nDate: %04u-%02u-%02u", date->year, date->month, date->day);
        // printf("\t from day counter: %lu", days_counter);
        // printf("\n");
    }
    
    // Calculate time from 2S and 32K counters only
    if (time != NULL) {
        // Combine counters: each 2S count = 2 seconds = 65536 ticks
        u32 total_ticks = (seconds_2s * 65536) + ticks_32k;
        
        // One division gives us both seconds and remaining ticks
        u32 total_seconds = total_ticks / 32768;
        u32 remaining_ticks = total_ticks % 32768;
        
        // Convert seconds to h:m:s
        time->hour = total_seconds / 3600;
        time->min = (total_seconds % 3600) / 60;
        time->sec = total_seconds % 60;
        
        // ms = ticks * 1000 / 32768 ≈ tick * 125 / 4096
        // time->millis = remaining_ticks >> 5;  // Fast approximation (÷32)
        time->millis = remaining_ticks * 125 / 4096;
        
        // printf("Time: %02u:%02u:%02u.%03u", time->hour, time->min, time->sec, time->millis);
        // printf("\t from 2s: %lu, 32k: %lu", seconds_2s, ticks_32k);
        // printf("\n");
    }
}