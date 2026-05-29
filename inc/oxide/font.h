#ifndef OXIDE_FONT_H
#define OXIDE_FONT_H

#include <fractal.h>

BEGIN_C_HEADER

typedef struct {
    u32 char_max_width;
    u32 char_max_height;
    u32 space_width;
    const u32 *char_width_lut;
    const u8 *lut;
} font_t;

static inline u32 font_get_width(font_t *f, char c) {
    return f->char_width_lut[c];
}

static inline u32 font_get_spacewidth(font_t *f) {
    return f->space_width;
}

static inline u8 font_get_bitmask(font_t *f, char c, u32 y, u32 x) {
    usize idx = (c * (f->char_max_height * f->char_max_width)) +
                (y * f->char_max_width) +
                x;
    return f->lut[idx];
}

#include <oxide/font/mono.h>
#include <oxide/font/ultralight.h>

END_C_HEADER

#endif // OXIDE_FONT_H
