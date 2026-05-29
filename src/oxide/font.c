#include <fractal.h>
#include <oxide/font.h>

#ifndef USE_GRAPHICS
font_t font_ultralight = {};
font_t font_mono = {};
#else // USE_GRAPHICS
#error "To compile Fractal with Oxide support, you need to provide font data"
#endif // ! USE_GRAPHICS
