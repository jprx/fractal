#include <oxide.h>
#include <oxide/logo.h>
#include <io/mouse.h>
#include <oxide/blur.h>
#include <oxide/font.h>
#include <task.h>

// There may be 256 possible ASCII chars, but we only ever support up to this one:
#define HIGHEST_ACTUALLY_DISPLAYABLE_CHAR 126

u32 framebuffer[SCREEN_MAX_H * SCREEN_MAX_W];

u32 SCREEN_W = DEFAULT_SCREEN_W;
u32 SCREEN_H = DEFAULT_SCREEN_H;

bool oxide_should_adjust_resolution = false;
u32 next_screen_w = DEFAULT_SCREEN_W;
u32 next_screen_h = DEFAULT_SCREEN_H;

bool oxide_initialized = false;
extern void __refresh_gpu();

// Returns the length in pixels this string will take when drawn
usize strlen_gui(font_t *f, char *s) {
    if (!f) panic("Called strlen with NULL font");
    const char *c = s;
    usize offset_x = 0;
    while (*c) {
        if (*c == ' ') {
            offset_x += font_get_spacewidth(f);
            c++;
            continue;
        }
        offset_x += font_get_width(f, *c);
        c++;
    }
    return offset_x;
}

void oxide_clear_screen() {
    for (usize y = 0; y < SCREEN_H; y++) {
        for (usize x = 0; x < SCREEN_W; x++) {
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            framebuffer[x+(y*SCREEN_MAX_W)] = 0x00000000;
        }
    }
}

// @TODO: Refactor this to use downsample_nearest_neighbor for shrinking
void oxide_draw_logo(usize imgx, usize imgy, isize scale) {
    rect_t logo_rect = {
        .x = imgx,
        .y = imgy,
        .w = LOGO_WIDTH,
        .h = LOGO_HEIGHT
    };

    oxide_draw_image(
        imgx,
        imgy,
        scale,
        logo_rect,
        logo_graphic
    );
}

// @TODO: Refactor this to use downsample_nearest_neighbor for shrinking
void oxide_draw_image(usize destx, usize desty, isize scale, rect_t img_rect, u32 *img) {
    usize imgx = destx;
    usize imgy = desty;
    usize scaled_width, scaled_height;

    // Scale factor: 0x100 means 100%, 0x80 means 50%, 0x200 means 200%
    scaled_width  = (img_rect.w * scale) / OXIDE_SCALE_FACTOR;
    scaled_height = (img_rect.h * scale) / OXIDE_SCALE_FACTOR;

    if (scale < 2) return;

    for (usize y = 0; y < scaled_height; y++) {
        for (usize x = 0; x < scaled_width; x++) {
            usize screen_x = x + imgx;
            usize screen_y = y + imgy;
            usize screen_idx = screen_x + (screen_y * SCREEN_MAX_W);
            if (screen_x >= SCREEN_W || screen_y >= SCREEN_H) continue;

            usize logo_x = (img_rect.w * x) / scaled_width;
            usize logo_y = (img_rect.h * y) / scaled_height;

            if (logo_x > img_rect.w) continue;
            if (logo_y > img_rect.h) continue;
            if (screen_y > SCREEN_H) continue;
            if (screen_x > SCREEN_W) continue;

            usize logo_idx = logo_x + (logo_y * img_rect.w);
            u32 col = img[logo_idx];
            if (0 == U32_TO_A(col)) continue;

            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            framebuffer[screen_idx] = lerp_color(framebuffer[screen_idx], col, U32_TO_A(col));
        }
    }
}

/*
 * oxide_stretch_into
 * Use nearest neighbor to stretch the source image into the destination.
 *
 * dst: An image buffer to write into.  (dst_w by dst_h) pixels will be written.
 * src: The image buffer to scale from. (src_w by src_h) pixels.
 *
 * dst_real_w is the "real" width of the buffer. For example, if screen is set to
 * render at 1920 by 1080, but the max screen size is 2560 by 1440, dst_w will be 1920,
 * dst_h will be 1080, and dst_real_w will be 2560. The real destination width is used
 * for performing pixel indexing calculations.
 *
 * src will be scaled to (dst_w x dst_h) centered at (0,0) in the destination image buffer.
 */
void oxide_stretch_into(u32 *dst, u32 dst_w, u32 dst_h, u32 dst_real_w, u32 *src, u32 src_w, u32 src_h) {
    for (usize y = 0; y < dst_h; y++) {
        for (usize x = 0; x < dst_w; x++) {
            usize screen_idx = (y*dst_real_w)+x;
            u32 closest_img_x = (x * src_w) / dst_w;
            u32 closest_img_y = (y * src_h) / dst_h;
            dst[screen_idx] = src[(closest_img_y * src_w) + closest_img_x];
        }
    }
}

/*
 * oxide_stretch_wallpaper
 * Stretches the src image to fill the entire framebuffer, at whatever the current screen resolution is.
 */
void oxide_stretch_wallpaper(u32 *src, u32 src_w, u32 src_h) {
    oxide_stretch_into(
        framebuffer,
        SCREEN_W,
        SCREEN_H,
        SCREEN_MAX_W,
        src,
        src_w,
        src_h
    );
}

void oxide_draw_char(font_t *f, char code, usize startx, usize starty, u32 color) {
    if (code > HIGHEST_ACTUALLY_DISPLAYABLE_CHAR) return;
    for (usize y = 0; y < f->char_max_height; y++) {
        for (usize x = 0; x < font_get_width(f, code); x++) {
            u8 lut_element = font_get_bitmask(f, code, y, x);
            if (lut_element < TEXT_MIN_COLOR_VALUE) continue;
            usize screenx = x + startx;
            usize screeny = y + starty;
            if (screenx >= SCREEN_W) continue;
            if (screeny >= SCREEN_H) continue;
            usize screen_idx = screenx + (screeny * SCREEN_MAX_W);
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            framebuffer[screen_idx] = lerp_color(framebuffer[screen_idx], color, lut_element);
        }
    }
}

// Returns true if pressed
bool oxide_draw_hover_button(rect_t button_rect, i32 hover_x, i32 hover_y, char *text, font_t *f) {
    bool is_button_pressed = false;
    if (is_coord_in_box(hover_x, hover_y, button_rect)) {
        if (mouse_currently_pressed) {
            oxide_draw_box_filled(button_rect, BUTTON_COLOR_PRESSED);
            is_button_pressed = true;
        } else {
            oxide_draw_box_filled(button_rect, BUTTON_COLOR_HOVER);
        }
    } else {
        oxide_draw_box_filled(button_rect, BUTTON_COLOR_UNPRESSED);
    }
    draw_string_centeredx(f, text, button_rect.x + button_rect.w / 2, button_rect.y + button_rect.h / 4 + 3);
    return is_button_pressed;
}

bool is_coord_in_box(i32 x, i32 y, rect_t bounds) {
    return (bounds.x < x && bounds.x + bounds.w > x && bounds.y < y && bounds.y + bounds.h > y);
}

void oxide_draw_string(font_t *f, const char *s, i32 x, i32 y) {
    const char *c = s;
    usize offset_x = x;
    u32 col = 0xFFFFFF;
    if (light_mode) col = 0;
    while (*c) {
        if (*c == ' ') {
            offset_x += font_get_spacewidth(f);
            c++;
            continue;
        }
        if (*c == '\n') {
            y += f->char_max_height;
            c++;
            offset_x = x;
            continue;
        }
        oxide_draw_char(f, *c, offset_x, y, col);
        offset_x += font_get_width(f, *c);
        c++;
    }
}

void oxide_draw_box(rect_t bounds, u32 col) {
    for (i32 y = bounds.y; y < bounds.y + bounds.h; y++) {
        for (i32 x = bounds.x - 1; x < bounds.x + 1; x++) {
            if (y >= SCREEN_H || x >= SCREEN_W) continue;
            usize screen_idx = x + y * SCREEN_MAX_W;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            framebuffer[screen_idx] = col;
        }
    }
    for (i32 y = bounds.y; y < bounds.y + bounds.h; y++) {
        for (i32 x = bounds.x - 1; x < bounds.x + 1; x++) {
            if (y >= SCREEN_H || (x+bounds.w) >= SCREEN_W) continue;
            usize screen_idx = (x + bounds.w) + y * SCREEN_MAX_W;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            framebuffer[screen_idx] = col;
        }
    }
    for (i32 x = bounds.x; x < bounds.x + bounds.w; x++) {
        for (i32 y = bounds.y - 1; y < bounds.y + 1; y++) {
            if (y >= SCREEN_H || x >= SCREEN_W) continue;
            usize screen_idx = x + y * SCREEN_MAX_W;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            framebuffer[screen_idx] = col;
        }
    }
    for (i32 x = bounds.x; x < bounds.x + bounds.w; x++) {
        for (i32 y = bounds.y - 1; y < bounds.y + 1; y++) {
            if ((y+bounds.h) >= SCREEN_H || x >= SCREEN_W) continue;
            usize screen_idx = x + (y+bounds.h) * SCREEN_MAX_W;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            framebuffer[screen_idx] = col;
        }
    }
}

void oxide_draw_box_filled(rect_t bounds, u32 col) {
    for (usize y = bounds.y; y < bounds.y + bounds.h; y++) {
        for (usize x = bounds.x; x < bounds.x + bounds.w; x++) {
            if (y >= SCREEN_H || x >= SCREEN_W) continue;
            usize screen_idx = x + y * SCREEN_MAX_W;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            framebuffer[screen_idx] = col;
        }
    }
}

void draw_string_centeredx(font_t *f, char *s, i32 x, i32 y) {
    if (!s) return;
    usize strlen = strlen_gui(f, s);
    oxide_draw_string(f, s, x - (strlen / 2), y);
}

void oxide_draw_background() {
    for (usize y = 0; y < SCREEN_H; y++) {
        for (usize x = 0; x < SCREEN_W; x++) {
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            usize screen_idx = x + (y * SCREEN_MAX_W);
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            framebuffer[x+(y*SCREEN_MAX_W)] = wallpaper[x+(y*SCREEN_MAX_W)];
        }
    }
}

void oxide_fade(isize amount) {
    if (amount > LERP_MAX) amount = LERP_MAX;
    if (amount < LERP_MIN) amount = LERP_MIN;
    for (usize y = 0; y < SCREEN_H; y++) {
        for (usize x = 0; x < SCREEN_W; x++) {
            usize screen_idx = x + (y * SCREEN_MAX_W);
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            framebuffer[screen_idx] = lerp_color(0, framebuffer[screen_idx], amount);
        }
    }
}

void oxide_panic(char *reason) {
    if (!oxide_initialized) return;
    char buf[512];
    for (usize y = 0; y < SCREEN_H; y++) {
        for (usize x = 0; x < SCREEN_W; x++) {
            usize screen_idx = x + (y * SCREEN_MAX_W);
            framebuffer[screen_idx] = lerp_color(0, background_blurbuf[screen_idx], PANIC_BACKGROUND_LERP_COLOR);
        }
    }
    oxide_draw_string(&font_ultralight, "Kernel Panic", SCREEN_W / 2 - strlen_gui(&font_ultralight, "Kernel Panic") / 2, SCREEN_H / 2 - font_ultralight.char_max_height / 2);
    snprintf(buf, "Task Name: %s", current_task()->t_name);
    draw_string_centeredx(&font_mono, buf, SCREEN_W / 2, SCREEN_H / 2 + 28);
    if (reason) {
        draw_string_centeredx(&font_mono, reason, SCREEN_W / 2, SCREEN_H / 2 + 50);
    }
    __refresh_gpu();
}
