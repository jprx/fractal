// AUTO-GENERATED WITH: ['./bdf2c.py', 'UltraLight-256.bdf', '4', 'ultralight', '../src/oxide/font/ultralight.c', '../inc/oxide/font/ultralight.h']
#ifndef FONT_ULTRALIGHT_H
#define FONT_ULTRALIGHT_H
#include <fractal.h>
#include <oxide/font.h>
#ifdef USE_GRAPHICS
extern const u32 font_ultralight_width[];
extern const u8 font_ultralight_lut[][101][65];
extern font_t font_ultralight;
#else // USE_GRAPHICS
extern const u32 font_ultralight_width[];
extern const u8 font_ultralight_lut[][1][1];
extern font_t font_ultralight;
#endif // ! USE_GRAPHICS
#endif // FONT_ULTRALIGHT_H
