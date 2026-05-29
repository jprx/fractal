#ifndef OXIDE_H
#define OXIDE_H

#include <fractal.h>
#include <oxide/font.h>

BEGIN_C_HEADER

// #define DRAW_CURSOR_INTO_FRAMEBUFFER

/**
 * OXIDE
 * Oxide is the graphics / GUI subsystem of Fractal.
 *
 * This header includes all the macros and methods required for
 * kernel services to interact with Oxide.
 */

// This is the resolution required to support Studio Display:
// Studio Display at default resolution is 2560 x 1440
// LG UltraFine 4K at second-largest is 3360 x 1890
// Any larger than that and we crash into the initrd (for ARM/ RISCV),
// as qemu puts the initrd at a fixed distance from the kernel on those archs.
// When making multiboot builds, you should cap resolution at whatever you request from GRUB
// as otherwise edges won't get filled in on wallpaper resets and blur effect will show it.
#define SCREEN_MAX_W ((2560))
#define SCREEN_MAX_H ((1440))

// Wallpapers are 1920 by 1080
#define WALLPAPER_W ((1920))
#define WALLPAPER_H ((1080))

#define DEFAULT_SCREEN_W ((SCREEN_MAX_W))
#define DEFAULT_SCREEN_H ((SCREEN_MAX_H))

// Mandated by virtio standard:
#define CURSOR_W 64
#define CURSOR_H 64
#define CURSOR_ALPHA_THRESH ((100))
extern u32 cursor[CURSOR_W*CURSOR_H];
extern i32 mouse_x, mouse_y;

// This is used to track when the user rescales the window (if ever)
// To prevent Qemu's default resolution of 1280x800 from
// overriding our intended 1920x1080 default
#define VIRTIO_DEFAULT_SCREEN_W ((1280))
#define VIRTIO_DEFAULT_SCREEN_H ((800))

// The current screen width and height
// we support dynamically rescaled screen resolutions (at least for virtio gpus)
extern u32 SCREEN_W, SCREEN_H;

#define MAX_WINDOWS 8

// Set to true if the resolution should be overridden on the next frame
extern bool oxide_should_adjust_resolution;
extern u32 next_screen_w, next_screen_h;

extern usize frame_idx;

// Height of font16 - a few padding pxs
#define TOPBAR_HEIGHT ((28-5))

// Pad bottom of screen by 5 pixels
// windows cannot draw in these last 5 screen rows
#define WINDOW_PADDING_BOTTOM 5

// RGBA mode:
#define OXIDE_BYTES_PER_PIXEL ((4))

// Replace width of space with width of this character:
#define SPACE_REPLACE_CHAR 'M'

// Color used when panic-ing
#define PANIC_BACKGROUND_LERP_COLOR 192

// Various cached framebuffers that image processing routines
// across Oxide will make use of
extern u32 framebuffer[SCREEN_MAX_H * SCREEN_MAX_W];
extern u32 *wallpaper;
extern u32 foreground_blurbuf[SCREEN_MAX_W * SCREEN_MAX_H];
extern u32 background_blurbuf[SCREEN_MAX_W * SCREEN_MAX_H];

#define FRAMEBUFFER_MAX_INDEX ((sizeof(framebuffer) / sizeof(framebuffer[0])))

void oxide_draw_desktop();
void oxide_clear_screen();

#define BUTTON_COLOR_UNPRESSED 0x444444
#define BUTTON_COLOR_HOVER     0xe07f00
#define BUTTON_COLOR_PRESSED   0xe00000

extern bool light_mode;

// Returns the length in pixels this string will take when drawn
usize strlen_gui(font_t *f, char *s);

typedef union __attribute__((packed)) {
    struct {
        u8 r;
        u8 g;
        u8 b;
        u8 a;
    };
    u32 v;
} color_t;

typedef struct {
    i32 x, y, w, h;
} rect_t;

#define U8_TO_COLOR(c) ((c | c << 8 | c << 16 | c << 24))
#define PACK_COLOR(r,g,b) (((r) | (g) << 8 | (b) << 16 | (0x0FF) << 24))
#define COLOR_IGNORE_ALPHA(c) ((c & 0x00FFFFFF))

// Any color values smaller than this are not rendered
#define TEXT_MIN_COLOR_VALUE 0x10

#define U32_TO_R(u) ((u & 0x0FF))
#define U32_TO_G(u) ((u >> 8) & 0x0FF)
#define U32_TO_B(u) ((u >> 16) & 0x0FF)
#define U32_TO_A(u) ((u >> 24) & 0x0FF)

#define LERP_MIN 0
#define LERP_HALF 127
#define LERP_MAX 255

#define U8_MAX 255

// Min and Max for each color channel
#define CHANNEL_MIN 0
#define CHANNEL_MAX 255

static inline u8 lerp(u8 a, u8 b, u8 amount) {
    u32 x = (((u32)b * amount) + ((LERP_MAX - amount) * (u32)a));
    x = (x + LERP_HALF) / LERP_MAX;
    if (x > LERP_MAX) return LERP_MAX;
    return (u8)x;
}

static inline u32 lerp_color(u32 a, u32 b, u8 amount) {
    u64 r_a = U32_TO_R(a);
    u64 r_b = U32_TO_R(b);
    u64 g_a = U32_TO_G(a);
    u64 g_b = U32_TO_G(b);
    u64 b_a = U32_TO_B(a);
    u64 b_b = U32_TO_B(b);

    u8 r_lerp = lerp(r_a, r_b, amount);
    u8 g_lerp = lerp(g_a, g_b, amount);
    u8 b_lerp = lerp(b_a, b_b, amount);

    return PACK_COLOR(r_lerp, g_lerp, b_lerp);
}

// Don't need a version of this for division because division tends -> 0, can't overflow
static inline u32 mult_add_andclip_color_nooverflow(u32 c, i32 mult_numerator, i32 mult_denominator, i32 add_amount, u8 clip_level) {
    u64 r = U32_TO_R(c) + add_amount;
    u64 g = U32_TO_G(c) + add_amount;
    u64 b = U32_TO_B(c) + add_amount;

    r = (r * mult_numerator) / mult_denominator;
    g = (g * mult_numerator) / mult_denominator;
    b = (b * mult_numerator) / mult_denominator;

    if (r > clip_level) r = clip_level;
    if (g > clip_level) g = clip_level;
    if (b > clip_level) b = clip_level;

    return PACK_COLOR(r, g, b);
}

void oxide_draw_string(font_t *f, const char *s, i32 x, i32 y);
void oxide_draw_logo(usize imgx, usize imgy, isize scale);
void oxide_draw_char(font_t *f, char code, usize startx, usize starty, u32 color);
void oxide_draw_box(rect_t bounds, u32 col);
void oxide_draw_image(usize destx, usize desty, isize scale, rect_t img_rect, u32 *img);
void oxide_stretch_wallpaper(u32 *img, u32 img_w, u32 img_h);
void oxide_stretch_into(u32 *dst, u32 dst_w, u32 dst_h, u32 dst_real_w, u32 *src, u32 src_w, u32 src_h);
bool is_coord_in_box(i32 x, i32 y, rect_t bounds);
void oxide_draw_box_filled(rect_t bounds, u32 col);
void draw_string_centeredx(font_t *f, char *s, i32 x, i32 y);
void oxide_fade(isize amount);
bool oxide_draw_hover_button(rect_t button_rect, i32 hover_x, i32 hover_y, char *text, font_t *f); // return true if button pressed

void oxide_panic(char *reason);

// Fixed point scaling factor for all Oxide image processing routines
#define OXIDE_SCALE_FACTOR 0x100

// Returns the framebuffer linear address of this pixel
// Converts screen-space coordinates to memory offset
static inline usize get_screen_idx(u32 x, u32 y) {
    return (y * SCREEN_MAX_W) + x;
}

END_C_HEADER

#endif // OXIDE_H
