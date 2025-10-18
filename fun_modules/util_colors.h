#ifndef UTIL_COLORS_H
#define UTIL_COLORS_H

#include "ch32fun.h"
#include "util_luts.h"

typedef union {
    struct {
        uint8_t R, G, B;
    };
    uint32_t packed;
} RGB_t;

#define MAKE_COLOR_FROM32(hex)      (RGB_t){ .R = hex & 0xFF, .G = (hex>>8) & 0xFF, .B = (hex>>16) & 0xFF }
#define MAKE_COLOR_RGB(r, g, b)     (RGB_t){ .R = r, .G = g, .B = b }
#define MAKE_COLOR_WHITE(value)     MAKE_COLOR_RGB(value, value, value)
#define COLOR_BLACK                 MAKE_COLOR_RGB(0x00, 0x00, 0x00)

#define COLOR_RED_HIGH              MAKE_COLOR_RGB(0xFF, 0x00, 0x00)
#define COLOR_RED_MED               MAKE_COLOR_RGB(0x77, 0x00, 0x00)
#define COLOR_RED_LOW               MAKE_COLOR_RGB(0x33, 0x00, 0x00)

#define COLOR_GREEN_HIGH            MAKE_COLOR_RGB(0x00, 0xFF, 0x00)
#define COLOR_GREEN_MED             MAKE_COLOR_RGB(0x00, 0x77, 0x00)
#define COLOR_GREEN_LOW             MAKE_COLOR_RGB(0x00, 0x33, 0x00)

#define COLOR_BLUE_HIGH             MAKE_COLOR_RGB(0x00, 0x00, 0xFF)
#define COLOR_BLUE_MED              MAKE_COLOR_RGB(0x00, 0x00, 0x77)
#define COLOR_BLUE_LOW              MAKE_COLOR_RGB(0x00, 0x00, 0x33)

#define COLOR_MAGENTA_HIGH          MAKE_COLOR_RGB(0xFF, 0x00, 0xFF)
#define COLOR_CYAN_HIGH             MAKE_COLOR_RGB(0x00, 0xFF, 0xFF)
#define COLOR_YELLOW_HIGH           MAKE_COLOR_RGB(0xFF, 0xFF, 0x00)

#define COLOR_EQUAL(a, b)           ((a).packed == (b).packed)


#define DECREMENT_OR_ZERO(value, decr) (((value) > (decr)) ? ((value) - (decr)) : 0)

// MAX of the 2 values
#define DECREMENT_PERCENTAGE(value, perc) \
    ((uint8_t)( \
        ((value) - ((value) * (perc)) / 100) > 0 \
        ? ((value) - ((value) * (perc)) / 100) \
        : 0 \
    ))


#define COLOR_DECREMENT(color_t, perc) \
    MAKE_COLOR_RGB( \
        DECREMENT_PERCENTAGE(color_t.R, perc), \
        DECREMENT_PERCENTAGE(color_t.G, perc), \
        DECREMENT_PERCENTAGE(color_t.B, perc) \
    )

#define COLOR_SET_BRIGHTNESS(color, percent) \
    ({ \
        RGB_t result = color; \
        uint8_t lut_index = (percent * 255) / 100; \
        uint8_t factor = LUT_SIN[lut_index]; \
        result.R = ((uint16_t)result.R * factor) >> 8; \
        result.G = ((uint16_t)result.G * factor) >> 8; \
        result.B = ((uint16_t)result.B * factor) >> 8; \
        result; \
    })


#endif