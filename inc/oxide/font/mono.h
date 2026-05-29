// AUTO-GENERATED WITH: ['./bdf2c.py', 'mono-Thin-48.bdf', '4', 'mono', '../src/oxide/font/mono.c', '../inc/oxide/font/mono.h']
#ifndef FONT_MONO_H
#define FONT_MONO_H
#include <fractal.h>
#include <oxide/font.h>
#ifdef USE_GRAPHICS
extern const u32 font_mono_width[];
extern const u8 font_mono_lut[][28][14];
extern font_t font_mono;
#else // USE_GRAPHICS
extern const u32 font_mono_width[];
extern const u8 font_mono_lut[][1][1];
extern font_t font_mono;
#endif // ! USE_GRAPHICS
#endif // FONT_MONO_H
