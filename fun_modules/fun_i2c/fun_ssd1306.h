// SSD1306 init functions borrowed from
// * Single-File-Header for using SPI OLED
// * 05-05-2023 E. Brombaugh

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
#include "word_font.h"

// enable 0.25 scale for text string
#define SSD1306_TEXT_SCALE_NUMERATOR 5
#define SSD1306_TEXT_SCALE_DENOMINATOR 6
#define SSD1306_TEXT_SCALE

#define SSD1306_128X64

// characteristics of each type
#if !defined (SSD1306_64X32) && !defined (SSD1306_128X32) && \
				!defined (SSD1306_128X64) &&  !defined (SH1107_128x128) && \
				!(defined(SSD1306_W) && defined(SSD1306_H) && defined(SSD1306_OFFSET))
	#error "Please define the SSD1306_WXH resolution used in your application"
#endif

#ifdef SSD1306_64X32
	#define SSD1306_W 64
	#define SSD1306_H 32
	#define SSD1306_OFFSET 32
#endif

#ifdef SSD1306_128X32
	#define SSD1306_W 128
	#define SSD1306_H 32
	#define SSD1306_OFFSET 0
#endif

#ifdef SSD1306_128X64
	#define SSD1306_W 128
	#define SSD1306_H 64
	#define SSD1306_OFFSET 0
#endif

#ifndef SSD1306_MAX_STR_LEN
	#define SSD1306_MAX_STR_LEN 21
#endif

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define SORT_LIMITS(limits) \
    if ((limits)[0] > (limits)[1]) { \
        u8 temp = (limits)[0]; \
        (limits)[0] = (limits)[1]; \
        (limits)[1] = temp; \
    }

#define MIRROR_VALUES(values, max) \
	(values)[0] = max - (values)[0]; \
	(values)[1] = max - (values)[1];

#define CLAMP_VALUE(value, max) \
	if (value > max) value = max;

#define CLAMP_VALUES(value0, value1, max) \
	if (value0 > max) value0 = max; \
	if (value1 > max) value1 = max;



//# INTERFACES
/* send OLED command byte */
u8 SSD1306_CMD(u8 cmd);

/* send OLED data packet (up to 32 bytes) */
u8 SSD1306_DATA(u8 *data, int sz);


//! ####################################
//! MAIN FUNCTIONS
//! ####################################

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2
#define SSD1306_TERMINATE_CMDS 0xFF

const u8 ssd1306_init_array[] = {
	SSD1306_DISPLAYOFF,                    // 0xAE
	SSD1306_SETDISPLAYCLOCKDIV,            // 0xD5
	0x80,                                  // the suggested ratio 0x80
	SSD1306_SETMULTIPLEX,                  // 0xA8

	#ifdef SSD1306_64X32
		0x1F,                                  // for 64-wide displays
	#else
		0x3F,                                  // for 128-wide displays
	#endif

	SSD1306_SETDISPLAYOFFSET,              // 0xD3
	0x00,                                  // no offset
	SSD1306_SETSTARTLINE | 0x0,            // 0x40 | line
	SSD1306_CHARGEPUMP,                    // 0x8D
	0x14,                                  // enable?
	SSD1306_MEMORYMODE,                    // 0x20
	0x00,                                  // 0x0 act like ks0108
	SSD1306_SEGREMAP | 0x1,                // 0xA0 | bit
	SSD1306_COMSCANDEC,
	SSD1306_SETCOMPINS,                    // 0xDA
	0x12,                                  //
	SSD1306_SETCONTRAST,                   // 0x81
	0x8F,
	SSD1306_SETPRECHARGE,                  // 0xd9
	0xF1,
	SSD1306_SETVCOMDETECT,                 // 0xDB
	0x40,
	SSD1306_DISPLAYALLON_RESUME,           // 0xA4
	SSD1306_DISPLAYON,	                   // 0xAF --turn on oled panel
	SSD1306_TERMINATE_CMDS                 // 0xFF --fake command to mark end
};

#define CHUNK_SIZE 32  		// Define your desired chunk size
#define SSD1306_PAGES	   (SSD1306_H / 8) // 8 pages for 64 rows

#define SSD1306_W_LIMIT	SSD1306_W - 1
#define SSD1306_H_LIMIT	SSD1306_H - 1

typedef struct {
	u8 page;
	u8 bitmask;
} Page_Mask_t;

Page_Mask_t page_masks[SSD1306_H];

u8 SSD1306_BUF[SSD1306_PAGES * SSD1306_W] = { 0 };

//# Init function
u8 ssd1306_init() {
	u8 *cmd_list = (u8 *)ssd1306_init_array;

	while(*cmd_list != SSD1306_TERMINATE_CMDS) {
		if(SSD1306_CMD(*cmd_list++))
			return 1;
	}

	// Prerender page masks
	for (u8 y = 0; y < SSD1306_H; y++) {
		page_masks[y].page	   = y >> 3;			 // (y / 8)
		page_masks[y].bitmask	= 1 << (y & 0x07);	// (y % 8)
	}

	return 0;
}

//# Set window
void ssd1306_setwindow(u8 start_page, u8 end_page, u8 start_column, u8 end_column) {
	SSD1306_CMD(SSD1306_COLUMNADDR);
	SSD1306_CMD(start_column);   		// Column start address (0 = reset)
	SSD1306_CMD(end_column); 			// Column end address (127 = reset)
	
	SSD1306_CMD(SSD1306_PAGEADDR);
	SSD1306_CMD(start_page); 			// Page start address (0 = reset)
	SSD1306_CMD(end_page); 				// Page end address
}

//# Draw page string
// this method fill the rest of the line with spaces
void ssd1306_draw_pageStr(const char *str, u8 page, u8 column, u8 fill_space) {
	ssd1306_setwindow(page, page, 0, SSD1306_W_LIMIT); // Set the window to the current page
	u8 CHAR_WIDTH = 5;

	for (int i=0; i < SSD1306_MAX_STR_LEN; i++) {
		if (*str) {
			u8 char_index = *str - 32; // Adjust for ASCII offset
			SSD1306_DATA(&FONT_7x5[char_index * CHAR_WIDTH], CHAR_WIDTH); // Send font data
			str++;
		}
		else if (fill_space) {
			// fill the remaining space
			SSD1306_DATA(&FONT_7x5[0], CHAR_WIDTH);
		}
	}

	// fill the remaining space for none fulled size char
	// without this, if the remaining space is less than the char width it doesn't fill that space
	if (fill_space) {
		for (int i=0; i < SSD1306_W - (CHAR_WIDTH * SSD1306_MAX_STR_LEN); i++) {
			SSD1306_DATA(&FONT_7x5[0], 1);
		}
	}
}

//# Draw string
void ssd1306_draw_str(const char *str, u8 page, u8 column) {
	ssd1306_draw_pageStr(str, page, column, 0);
}

//# Draw area
void ssd1306_draw_area(
	u8 start_page, u8 end_page, u8 col_start, u8 col_end
) {
	ssd1306_setwindow(start_page, end_page, col_start, col_end-1);

    for (u8 page = start_page; page <= end_page; page++) {
        // Calculate the base address for this page in the 1D array
        u8* page_base = &SSD1306_BUF[page * SSD1306_W + col_start];
        u16 page_data_length = col_end - col_start;
        
        // Send page data in chunks
        for (u16 chunk = 0; chunk < page_data_length; chunk += CHUNK_SIZE) {
            u16 chunk_end = chunk + CHUNK_SIZE;
            if (chunk_end > page_data_length) chunk_end = page_data_length;
            SSD1306_DATA(&page_base[chunk], chunk_end - chunk);
        }
    }
}

//# Draw the entire frame
void ssd1306_draw_all() {
	ssd1306_draw_area(0, 7, 0, SSD1306_W);
}

void ssd1306_render_fill(u8 value) {
	memset(SSD1306_BUF, value, sizeof(SSD1306_BUF));
}

void ssd1306_draw_fill(u8 value) {
	ssd1306_render_fill(value);
	ssd1306_draw_all();
}

//# render pixel
void render_pixel(u8 x, u8 y) {
	if (x >= SSD1306_W || y >= SSD1306_H) return; // Skip if out of bounds
	Page_Mask_t mask = page_masks[y];
	SSD1306_BUF[mask.page * SSD1306_W + x] |= mask.bitmask;
}

void render_pixel_erase(u8 x, u8 y) {
	if (x >= SSD1306_W || y >= SSD1306_H) return; // Skip if out of bounds
	Page_Mask_t mask = page_masks[y];
	SSD1306_BUF[mask.page * SSD1306_W + x] &= ~mask.bitmask;
}


//! ####################################
//! CUSTOM STRING FUNCTIONS
//! ####################################

typedef struct {
	u8 *FONT;
	u8 WIDTH, HEIGHT;
	u8 SPACE;
	u8 scale_mode;
	u8 color;
} Str_Config_t;

typedef struct {
	u8 max_x;
	u8 max_y;
} SSD1306_Text_Bounds_t;


//* Clear text bounds
void _clear_text_bounds(u8 x, u8 y, SSD1306_Text_Bounds_t *bounds, u8 fill) {    
    u8 x_count = bounds->max_x - x + 1;
	u8 max_y = bounds->max_y;
    
    // Use page_masks to build combined bitmasks per page
    for (u8 page = 0; page < SSD1306_PAGES; page++) {
        u8 output_bitmask = 0;
        u8 page_START = page * 8;
        u8 page_END = page_START + 7;
        
        // Only process pages that overlap with our Y range
        if (max_y < page_START || y > page_END) continue;
        
        // Combine all bitmasks for Y coordinates in this page's range
        u8 start_y = (y > page_START) ? y : page_START;
        u8 end_y = (max_y < page_END) ? max_y : page_END;
        
        for (u8 py = start_y; py <= end_y; py++) {
            output_bitmask |= page_masks[py].bitmask;
        }
        
        // Apply to all X coordinates in range
        u8 *dest = &SSD1306_BUF[page * SSD1306_W + x];

		if (fill) {
            for (u8 i = 0; i < x_count; i++) dest[i] |= output_bitmask;
        } else {
            for (u8 i = 0; i < x_count; i++) dest[i] &= ~output_bitmask;
        }
    }
}

//* Calculate text bounds
SSD1306_Text_Bounds_t _calc_text_bounds(
	u8 char_count, u8 x, u8 y, Str_Config_t *config
) {
	SSD1306_Text_Bounds_t bounds = {0};
    if (!config || char_count < 1) return bounds;

	u16 scaled_width, scaled_height;

	#ifdef SSD1306_TEXT_SCALE
		scaled_width = config->WIDTH * 
			(config->scale_mode + SSD1306_TEXT_SCALE_NUMERATOR) / SSD1306_TEXT_SCALE_DENOMINATOR;
		scaled_height = config->HEIGHT *
			(config->scale_mode + SSD1306_TEXT_SCALE_NUMERATOR) / SSD1306_TEXT_SCALE_DENOMINATOR;
	#else
		// Calculate total width: (chars * width * scale) + (spaces between chars)
		scaled_width = config->WIDTH * config->scale_mode;
		scaled_height = config->HEIGHT * config->scale_mode;
	#endif

    u8 width = (char_count * scaled_width) + ((char_count - 1) * config->SPACE);
    u8 height = scaled_height;
    
    // Calculate maximum bounds
    bounds.max_x = x + width;
    bounds.max_y = y + height - 1;  // -1 because coordinates are 0-indexed
    
    // Clamp to screen bounds
	CLAMP_VALUE(bounds.max_x, SSD1306_W_LIMIT);
	CLAMP_VALUE(bounds.max_y, SSD1306_H_LIMIT);
    
	return bounds;
}

//# Render string with size
void ssd1306_render_scaled_txt(
	u8 x, u8 y, const char *str, Str_Config_t *config
) {
	// Early exit if starting position is off-screen
    if (x >= SSD1306_W || y >= SSD1306_H) return;

	#ifdef SSD1306_TEXT_SCALE
		u8 char_width_scaled = config->WIDTH *
			(config->scale_mode + SSD1306_TEXT_SCALE_NUMERATOR) / SSD1306_TEXT_SCALE_DENOMINATOR;
	#else
		u8 char_width_scaled = config->WIDTH * config->scale_mode;
    #endif

	u8 space_offset = char_width_scaled + config->SPACE;
	void (*pixel_func)(u8 x, u8 y) = config->color ? render_pixel : render_pixel_erase;

    while (*str) {
        u8 c = *str++;

		//# Efficient spacebar handling - skip rendering entirely
        if (c == ' ') {
            x += space_offset;
            if (x >= SSD1306_W) break;
            continue;
        }

        if (c < 32 || c > 127) c = '?';
        u8 char_index = c - 32;
        u16 char_pos = char_index * config->WIDTH;

		//# loop by Width and Height
        for (u8 col = 0; col < config->WIDTH; col++) {
            u8 glyph = config->FONT[char_pos + col];

            for (u8 row = 0; row < config->HEIGHT; row++) {
                if (glyph & (1 << row)) {
					//# draw the glyth bit - expand by scale

					#ifdef SSD1306_TEXT_SCALE
						// Calculate start and end positions using the full scaling formula
						u8 factor = config->scale_mode + SSD1306_TEXT_SCALE_NUMERATOR;
						u8 start_col = (col * factor) / SSD1306_TEXT_SCALE_DENOMINATOR;
						u8 end_col = ((col + 1) * factor) / SSD1306_TEXT_SCALE_DENOMINATOR;
						u8 start_row = (row * factor) / SSD1306_TEXT_SCALE_DENOMINATOR;
						u8 end_row = ((row + 1) * factor) / SSD1306_TEXT_SCALE_DENOMINATOR;
						
						// Fill the rectangular block
						for (u8 tc = start_col; tc < end_col; tc++) {
							for (u8 tr = start_row; tr < end_row; tr++) {
								pixel_func(x + tc, y + tr);
							}
						}
					#else
						u8 col_value = col * config->scale_mode;
						u8 row_value = row * config->scale_mode;

						for (u8 dx = 0; dx < config->scale_mode; dx++) {
							for (u8 dy = 0; dy < config->scale_mode; dy++) {
								pixel_func(x + dx + col_value, y + dy + row_value);
							}
						}
					#endif
                }
            }
        }

		//# update x and early exit for next char
        x += space_offset;
		if (x >= SSD1306_W) break;
    }
}

// #define SSD1306_DRAW_TEXT_LOG

//# Draw string with size
void ssd1306_draw_scaled_text(
	u8 x, u8 y, const char *str, Str_Config_t *config
) {
	// SSD1306_Text_Bounds_t bounds = _calc_text_bound05(strlen(str), x, y, config);
	SSD1306_Text_Bounds_t bounds = _calc_text_bounds(strlen(str), x, y, config);
	// printf("mx: %d, my: %d, page: %d\n", bounds.max_x, bounds.max_y, bounds.max_page);

	//# clear text bounds
	// Pre-fill the entire text area background
	u32 moment = micros();
	_clear_text_bounds(x, y, &bounds, !(config->color));
	u32 clear_elapsed = micros() - moment;
	
	//# render text
	moment = micros();
	ssd1306_render_scaled_txt(x, y, str, config);
	u32 render_elapsed = micros() - moment;
	
	// //# draw text
	moment = micros();
	u8 min_page = y>>3;
	u8 max_page = bounds.max_y>>3;
	ssd1306_draw_area(min_page, max_page, x, bounds.max_x);
	u32 draw_elapsed = micros() - moment;

	#ifdef SSD1306_DRAW_TEXT_LOG
		printf("\nclear (%d), render (%d), draw (%d) in us\n", 
				clear_elapsed, render_elapsed, draw_elapsed);
	#endif
}


//! ####################################
//! LINE FUNCTIONS
//! ####################################

//# render_fastHorLine
void render_fastHorLine(u8 y, u8 x0, u8 x1) {
	if (y > SSD1306_H_LIMIT) return;
	
	// Clamp x-coordinates
	CLAMP_VALUES(x0, x1, SSD1306_W_LIMIT);

	Page_Mask_t mask = page_masks[y];
	u8* row_start = &SSD1306_BUF[mask.page * SSD1306_W];

	for (u8 x = x0; x <= x1; x++) {
		row_start[x] |= mask.bitmask;
	}
}

//# render_fastHorLine erase
void render_fastHorLine_erase(u8 y, u8 x0, u8 x1) {
    if (y > SSD1306_H_LIMIT) return;
    
	// Clamp x-coordinates
	CLAMP_VALUES(x0, x1, SSD1306_W_LIMIT);

    Page_Mask_t mask = page_masks[y];
    u8* row = &SSD1306_BUF[mask.page * SSD1306_W];
    
    for (u8 x = x0; x <= x1; x++) {
        row[x] &= ~mask.bitmask;  // CLEAR pixels instead of setting them
    }
}

//# render horizontal line
void render_horLine(u8 y, u8 x_limits[2], u8 thickness, u8 mirror) {
	// Validate coordinates
	if (y > SSD1306_H_LIMIT) return;

	// Clamp to display bounds
	CLAMP_VALUES(x_limits[0], x_limits[1], SSD1306_W_LIMIT);
	
	// Handle mirroring
	if (mirror) {
		MIRROR_VALUES(x_limits, SSD1306_W_LIMIT);
	}

	// Ensure x1 <= x2 (swap if needed)
	SORT_LIMITS(x_limits);

	// Handle thickness
	u8 y_end  = y + thickness - 1;
	CLAMP_VALUE(y_end, SSD1306_H_LIMIT);
	if (y_end < y) return;  // Skip if thickness is 0 or overflowed

	// Draw thick line
	u8 width = x_limits[1] - x_limits[0] + 1;

	for (u8 y_pos = y; y_pos <= y_end ; y_pos++) {
		Page_Mask_t mask = page_masks[y_pos];
		u8* row_start = &SSD1306_BUF[mask.page * SSD1306_W + x_limits[0]];

        for (u8 i = 0; i < width; i++) {
            row_start[i] |= mask.bitmask;
        }
	}
}

//# render vertical line
void render_verLine(u8 x, u8 y_limits[2], u8 thickness, u8 mirror) {
	// Validate coordinates
	if (x > SSD1306_W_LIMIT) return;

	// Clamp to display bounds
	CLAMP_VALUES(y_limits[0], y_limits[1], SSD1306_H_LIMIT);

	// Handle mirroring
	if (mirror) {
		MIRROR_VALUES(y_limits, SSD1306_H_LIMIT);
	}

	// Ensure y1 <= y2 (swap if needed)
	SORT_LIMITS(y_limits);

	// Handle thickness
	u8 x_end = x + thickness - 1;
	CLAMP_VALUE(x_end, SSD1306_W_LIMIT);
	if (x_end < x) return;  // Skip if thickness causes overflow

	//# Optimized: save 500-700 us
	u8 x_len = x_end - x + 1;  // Prerender length

	for (u8 y_pos = y_limits[0]; y_pos <= y_limits[1]; y_pos++) {
		Page_Mask_t mask = page_masks[y_pos];
		u8* row_start = &SSD1306_BUF[mask.page * SSD1306_W + x];

		for (u8 i = 0; i < x_len; i++) {
			row_start[i] |= mask.bitmask;  					// Sequential access
		}
	}
}

//# render line (Bresenham's algorithm)
void render_line(u8 point_a[2], u8 point_b[2], u8 thickness) {
	u8 x0 = point_a[0], y0 = point_a[1];
    u8 x1 = point_b[0], y1 = point_b[1];

	// Clamp coordinates to display bounds
	CLAMP_VALUES(x0, x1, SSD1306_W_LIMIT);
	CLAMP_VALUES(y0, y1, SSD1306_H_LIMIT);

	// Bresenham's line algorithm
	s16 dx = ABS(x1 - x0);
	s16 dy = -ABS(y1 - y0);
	s16 sx = x0 < x1 ? 1 : -1;
	s16 sy = y0 < y1 ? 1 : -1;
	s16 err = dx + dy;
	s16 e2;

	while (1) {
		// Draw the pixel(s)
		if (thickness == 1) {
			// Fast path for single-pixel
			if (x0 < SSD1306_W && y0 < SSD1306_H) {
				Page_Mask_t mask = page_masks[y0];
				SSD1306_BUF[mask.page * SSD1306_W + x0] |= mask.bitmask;
			}
		} else {
			// Calculate bounds (branchless min/max)
			u8 x_start = x0;
			u8 x_end = x0 + thickness - 1;
			u8 y_start = y0;
			u8 y_end = y0 + thickness - 1;
			
			CLAMP_VALUE(x_end, SSD1306_W_LIMIT);
			CLAMP_VALUE(y_end, SSD1306_H_LIMIT);
			u8 width = x_end - x_start + 1;

			// Optimized row filling
			for (u8 y = y_start; y <= y_end; y++) {
				Page_Mask_t mask = page_masks[y];
				u8* row = &SSD1306_BUF[mask.page * SSD1306_W + x_start];
				u8 pattern = mask.bitmask;

				// Optimized filling based on width
				if (width <= 4) {
					// Fully unrolled for common cases
					if (width > 0) row[0] |= pattern;
					if (width > 1) row[1] |= pattern;
					if (width > 2) row[2] |= pattern;
					if (width > 3) row[3] |= pattern;
				} else {
					// For larger widths, use memset-style optimization
					for (u8 i = 0; i < width; i++) {
						row[i] |= pattern;
					}
				}
			}
		}

		// Bresenham Advance
		if (x0 == x1 && y0 == y1) break;
		e2 = err << 1; // e2 = 2*err via bit shift
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}


//! ####################################
//! POLYGON FUNCTIONS
//! ####################################

//# render poligon
void render_poly(u8 pts[][2], u8 num_pts, u8 thickness) {
    if (num_pts < 3) return;  // Need at least 3 points for a polygon

	for (u8 i = 0; i < num_pts - 1; i++) {
		render_line(pts[i], pts[i+1], thickness);
	}

    // Draw closing line - directly use array rows
    render_line(pts[num_pts-1], pts[0], thickness);
}

//# render filled polygon
void render_solid_poly(u8 pts[][2], u8 num_pts) {
    // Find bounding box
    u8 y_min = SSD1306_H, y_max = 0;

    for (u8 i = 0; i < num_pts; i++) {
        if (pts[i][1] < y_min) y_min = pts[i][1];
        if (pts[i][1] > y_max) y_max = pts[i][1];
    }
    
    for (u8 y = y_min; y <= y_max; y++) {
        u8 intersections[8];
        u8 count = 0;
        
        // Simple intersection detection
        for (u8 i = 0, j = num_pts-1; i < num_pts; j = i++) {
            u8 y1 = pts[i][1], y2 = pts[j][1];
            
            if ((y1 <= y && y < y2) || (y2 <= y && y < y1)) {
                u8 x1 = pts[i][0], x2 = pts[j][0];
                // Simple interpolation (good enough for small polygons)
                u8 x = x1 + ((x2 - x1) * (y - y1)) / (y2 - y1 + 1);
                intersections[count++] = x;
            }
        }
        
        // Simple bubble sort
        for (u8 i = 0; i < count-1; i++) {
            for (u8 j = i+1; j < count; j++) {
                if (intersections[i] > intersections[j]) {
                    u8 temp = intersections[i];
                    intersections[i] = intersections[j];
                    intersections[j] = temp;
                }
            }
        }
        
        // Fill between pairs
        for (u8 i = 0; i+1 < count; i += 2) {
            render_fastHorLine(y, intersections[i], intersections[i+1]);
        }
    }
}

//# render rectangle
void render_rect(u8 p0[2], u8 area[2], u8 fill) {
	u8 x = p0[0], y = p0[1];
	u8 w = area[0], h = area[1];

	// validate and Clamp to display bounds
	if (x >= SSD1306_W || y >= SSD1306_H) return;
	u8 x_end = (x + w < SSD1306_W) ? (x + w) : SSD1306_W_LIMIT;
	u8 y_end = (y + h < SSD1306_H) ? (y + h) : SSD1306_H_LIMIT;

	// Draw rectangle with optional fill
	u8 x_limit[] = { x, x_end };

	if (fill) {
		// Filled rectangle using horizontal lines (faster for row-major displays)
		for (u8 y_pos = y; y_pos <= y_end; y_pos++) {
			render_horLine(y_pos, x_limit, 1, 0);
		}
	} else {
		u8 y_limit[] = { y + 1, y_end - 1 };

		// Outline only
		render_horLine(y, x_limit, 1, 0);	 	// Top edge
		render_horLine(y_end, x_limit, 1, 0);	// Bottom edge
		render_verLine(x, y_limit, 1, 0); 		// Left edge
		render_verLine(x_end, y_limit, 1, 0); 	// Right edge
	}
}


//! ####################################
//! CIRCLE FUNCTIONS
//! ####################################

const u8 SIN_LUT[] = {
	0, 4, 9, 13, 18, 22, 27, 31, 36, 40, 44, 49, 53, 58, 62, 66,
	71, 75, 79, 83, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 131,
	135, 139, 142, 146, 149, 153, 156, 159, 163, 166, 169, 172, 175, 178, 181, 183,
	186, 189, 191, 194, 196, 198, 201, 203, 205, 207, 209, 211, 212, 214, 216, 217,
	219, 220, 221, 223, 224, 225, 226, 227, 228, 228, 229, 230, 230, 231, 231, 231,
	232, 232, 232, 232, 232, 232, 232, 231, 231, 231, 230, 230, 229, 228, 228, 227,
	226, 225, 224, 223, 221, 220, 219, 217, 216, 214, 212, 211, 209, 207, 205, 203,
	201, 198, 196, 194, 191, 189, 186, 183, 181, 178, 175, 172, 169, 166, 163, 159,
	156, 153, 149, 146, 142, 139, 135, 131, 128, 124, 120, 116, 112, 108, 104, 100,
	96, 92, 88, 83, 79, 75, 71, 66, 62, 58, 53, 49, 44, 40, 36, 31,
	27, 22, 18, 13, 9, 4, 0
};

// Cosine table (just offset by 90 degrees from sine)
#define COS_LUT(angle) SIN_LUT[(angle + 90) % 360]
#define M_PI 3.14

//# render circle (Bresenham's algorithm)
void render_circle(
	u8 point[2], u8 radius, u8 fill
) {
	// Validate center coordinates
	u8 px = point[0], py = point[1];
	if (px >= SSD1306_W || py >= SSD1306_H) return;

	s16 x = -radius;
	s16 y = 0;
	s16 err = 2 - 2 * radius;
	s16 e2;

	do {
		// Calculate endpoints with clamping
		u8 x_start 	= px + x;
		u8 x_end   	= px - x;
		u8 y_top   	= py - y;
		u8 y_bottom = py + y;

		if (fill) {
			// Draw filled horizontal lines (top and bottom halves)
			render_fastHorLine(y_top, x_start, x_end);	 	// Top half
			render_fastHorLine(y_bottom, x_start, x_end);  	// Bottom half
		} else {
			u8 xy_start 	= px + y;
			u8 xy_end   	= px - y;
			u8 yx_start 	= py + x;
			u8 yx_end   	= py - x;

			// Draw all 8 symmetric points (using prerenderd page_masks)
			render_pixel(x_end		, y_bottom); 	// Octant 1
			render_pixel(x_start	, y_bottom); 	// Octant 2
			render_pixel(x_start	, y_top); 		// Octant 3
			render_pixel(x_end		, y_top); 		// Octant 4
			render_pixel(xy_end		, yx_start); 	// Octant 5
			render_pixel(xy_start	, yx_start); 	// Octant 6
			render_pixel(xy_start	, yx_end); 		// Octant 7
			render_pixel(xy_end		, yx_end); 		// Octant 8
		}

		// Update Bresenham error
		e2 = err;
		if (e2 <= y) {
			err += ++y * 2 + 1;
			if (-x == y && e2 <= x) e2 = 0;
		}
		if (e2 > x) err += ++x * 2 + 1;
	} while (x <= 0);
}

//# render ring (Bresenham's algorithm)
void _render_circle_with_func(u8 point[2], u8 radius, void (*func)(u8 y, u8 x_start, u8 x_end)) {
	// Validate center coordinates
	u8 px = point[0], py = point[1];
    if (px >= SSD1306_W || py >= SSD1306_H) return;

    s16 x = -radius;
    s16 y = 0;
    s16 err = 2 - 2 * radius;
    s16 e2;

    do {
        u8 x_start  = px + x;
        u8 x_end    = px - x;
        u8 y_top    = py - y;
        u8 y_bottom = py + y;

        // handle callbacks
		func(y_top, x_start, x_end);		// Top half
		func(y_bottom, x_start, x_end);		// Bottom half

        e2 = err;
        if (e2 <= y) {
            err += ++y * 2 + 1;
            if (-x == y && e2 <= x) e2 = 0;
        }
        if (e2 > x) err += ++x * 2 + 1;
    } while (x <= 0);
}

void render_ring(u8 point[2], u8 radius, u8 thickness) {
    if ((point[0] >= SSD1306_W) | (point[1] >= SSD1306_H) | (radius == 0)) return;
    
    // Draw filled outer circle
    render_circle(point, radius, 1);
    
    // Erase the inner circle
	u8 inner_radius = (thickness >= radius) ? 0 : (radius - thickness);

    if (inner_radius > 0) {
		_render_circle_with_func(point, inner_radius, render_fastHorLine_erase);
    }
}

//# render pie
// Helper function to get circle point using SIN_LUT
static void get_circle_point(
	u8 center[2], u8 radius, u16 angle, 
	u16 *x, u16 *y
) {
	u8 quadrant = angle / 90;
	u8 reduced_angle = angle % 90;
	u8 sin_val = SIN_LUT[reduced_angle];
	u8 cos_val = SIN_LUT[90 - reduced_angle];
	
	switch (quadrant) {
		case 0: *x = center[0] + (radius * cos_val) / 255;
				*y = center[1] - (radius * sin_val) / 255;
				break;
		case 1: *x = center[0] - (radius * sin_val) / 255;
				*y = center[1] - (radius * cos_val) / 255;
				break;
		case 2: *x = center[0] - (radius * cos_val) / 255;
				*y = center[1] + (radius * sin_val) / 255;
				break;
		case 3: *x = center[0] + (radius * sin_val) / 255;
				*y = center[1] + (radius * cos_val) / 255;
				break;
	}
}

// Simplified pie rendering
void render_pie(u8 center[2], u8 radius, u16 start_angle, u16 end_angle) {	
    // Handle full circle case
    if (start_angle == end_angle) {
        render_circle(center, radius, 1);  // Fill entire circle
        return;
    }
    
    // Normalize angles and determine direction
    u16 angle = start_angle % 360;
    u16 target_angle = end_angle % 360;
    
    // Ensure we always draw in positive direction
    if (target_angle < angle) target_angle += 360;
    u8 step = 1;
    
    // Draw radial lines
    while (angle <= target_angle) {
        u8 x, y;
        get_circle_point(center, radius, angle, &x, &y);
        
        // Draw line from center to edge
        u8 p0[] = { center[0], center[1] };
        u8 p1[] = { x, y };
        render_line(p0, p1, 1);
        
        angle += step;
    }
}

//! ####################################
//! TEST FUNCTIONS
//! ####################################

void test_polys() {
	int y = 0;

	//# rectangles
	for (int8_t i = 0; i<4; i++) {
		u8 should_fill = i > 1 ? 1 : 0;
		render_rect((u8[]){ 84, y }, (u8[]){ 15, 5 }, should_fill);
		y += 7;
	}

	//# zigzag
	u8 zigzag[][2] = {
		{ 60, 8 },
		{ 50, 15 },
		{ 80, 8 },
		{ 70, 0 },
		{ 70, 20 }
	};

	u8 pt_count = sizeof(zigzag)/sizeof(zigzag[0]);
	render_solid_poly(zigzag, pt_count);

	u8 zigzag2[5][2];
	memcpy(zigzag2, zigzag, sizeof(zigzag));  // Fast copy

	for (int i = 0; i < sizeof(zigzag)/sizeof(zigzag[0]); i++) {
		zigzag2[i][1] += 24;  // Add 20 to each x-coordinate
	}
	render_poly(zigzag2, pt_count, 1);

	//# star
	// Concave polygon: Star (22px tall)
	u8 star[][2] = {
		{12 , 0},  // Top point
		{16 , 8}, // Right upper
		{24 , 8}, // Right outer
		{18 , 14}, // Right inner
		{22 , 22}, // Bottom right
		{12 , 16}, // Bottom center
		{2  , 22},  // Bottom left
		{6  , 14},  // Left inner
		{0  , 8},  // Left outer
		{8  , 8}   // Left upper
	};

	pt_count = sizeof(star)/sizeof(star[0]);
	render_solid_poly(star, pt_count);

	// Shift star right by 25px
	for (int i = 0; i < pt_count; i++) {
		star[i][0] += 25;  // Add 20 to each x-coordinate
	}
	render_poly(star, pt_count, 1);

	//# quad
	// Convex polygon: Quad (12px tall)
	static u8 quad[][2] = {
		{6  , 24},  // Bottom-left (aligned with star's left)
		{18 , 24}, // Bottom-right (centered)
		{22 , 34}, // Top-right (matches star width - unchanged)
		{2  , 34}   // Top-left (aligned - unchanged)
	};

	pt_count = sizeof(quad)/sizeof(quad[0]);
	render_solid_poly(quad, pt_count);

	// Shift quad right by 25px
	u8 quad2[4][2];
	memcpy(quad2, quad, sizeof(quad));  // Fast copy

	for (int i = 0; i < pt_count; i++) {
		quad2[i][0] += 25;  // Add 20 to each x-coordinate
	}
	render_poly(quad2, pt_count, 1);

	//# hourglass
	// Self-intersecting: Hourglass (12px tall)
	static u8 hourglass[][2] = {
		{6  , 38},   // Top-left (aligned with quad's bottom-left)
		{18 , 38},  // Top-right (aligned with quad's bottom-right)
		{6  , 52},   // Bottom-left
		{18 , 52}   // Bottom-right
	};

	pt_count = sizeof(hourglass)/sizeof(hourglass[0]);
	render_solid_poly(hourglass, pt_count);

	// Shift hourglass right by 25px
	u8 hourglass2[4][2];
	memcpy(hourglass2, hourglass, sizeof(hourglass));  // Fast copy

	for (int i = 0; i < pt_count; i++) {
		hourglass2[i][0] += 25;  // Add 20 to each x-coordinate
	}
	render_poly(hourglass2, pt_count, 1);
}


u8 myvalues[16] = { 30, 50, 60, 40, 20, 50, 30, 10, 35, 10, 20, 30, 40, 50, 60, 20 };
int y = 0;

// #define SSD1306_TEST_LINES
#define SSD1306_TEST_POLYGONS
#define SSD1306_TEST_CIRCLES

void ssd1306_draw_test() {
	#ifdef SSD1306_TEST_LINES
		//# Test hor lines
		for(int8_t i = 0; i<sizeof(myvalues); i++) {
			u8 limit[] = { 0, myvalues[i] };
			render_horLine(y, limit, 2, 0);
			render_horLine(y, limit, 2, 1);
			y += 4;
		}

		y = 0;
		//# Test ver lines
		for(int8_t i = 0; i<sizeof(myvalues); i++) {
			u8 limit[] = { 0, myvalues[i] };
			render_verLine(y, limit, 1, 0);
			render_verLine(y, limit, 1, 1);
			y += 4;
		}

		// //# Test lines
		// for(u8 x=0; x<SSD1306_W; x+=16) {
		// 	u8 point_a0[] = { x, 0 };
		// 	u8 point_a1[] = { SSD1306_W, y };
		// 	render_line(point_a0, point_a1, 2);

		// 	u8 point_b0[] = { SSD1306_W-x, SSD1306_H };
		// 	u8 point_b1[] = { 0, SSD1306_H-y };
		// 	render_line(point_b0, point_b1, 2);

		// 	y+= SSD1306_H/8;
		// }
	#endif

	#ifdef SSD1306_TEST_CIRCLES
		y = 0;
		//# Test circles
		for (int8_t i = 0; i<4; i++) {
			u8 should_fill = i > 1 ? 1 : 0;
			render_circle((u8[]){ 110, y }, 5, should_fill);

			if (i > 1) {
				render_ring((u8[]){ 90, y + 12 }, 7, 3);
			}
			
			y += 14;
		}

		render_pie((u8[]){ 60, 55 }, 12, 0, 180);
	#endif

	// ssd1306_draw_str("sz 8x8 testing", 0, 0);
	// ssd1306_draw_str("testing 22222234fdafadfafa", 1, 0);
	// ssd1306_draw_str("testing 33334fdafadfafa", 2, 0);
	// ssd1306_draw_str("testing 44444fdafadfafa", 3, 0);
	// ssd1306_draw_str("testing 55554fdafadfafa", 4, 0);
	// ssd1306_draw_str("testing 66664fdafadfafa", 5, 0);
	// ssd1306_draw_str("testing 77774fdafadfafa", 6, 0);
	// ssd1306_draw_str("testing 88884fdafadfafa", 7, 0);

	#ifdef SSD1306_TEST_POLYGONS
		test_polys();
	#endif

	ssd1306_draw_all();
}
