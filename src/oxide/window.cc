#include <fractal.h>
#include <oxide.h>
#include <arch.h>
#include <oxide/window.h>
#include <io/mouse.h>
#include <oxide/logo.h>
#include <version.h>
#include <oxide/compositor.h>
#include <lib/mem.h>
#include <oxide/font.h>

// If the red channel of the border image is less than this,
// we draw a transparent pixel. Otherwise we draw whatever color
// is in the image source.
#define WINDOW_BORDER_TRANSPARENCY_COLORMASK 40

#define ARCH_ICON_WIDTH 194
#define ARCH_ICON_HEIGHT 194
extern u32 arch_icon_x86[ARCH_ICON_HEIGHT*ARCH_ICON_HEIGHT];
extern u32 arch_icon_arm[ARCH_ICON_HEIGHT*ARCH_ICON_HEIGHT];
extern u32 arch_icon_riscv[ARCH_ICON_HEIGHT*ARCH_ICON_HEIGHT];

extern u32 border_topleft[];
extern u32 border_topright[];
extern u32 border_bottomleft[];
extern u32 border_bottomright[];
extern u32 border_topleft_mask[];
extern u32 border_topright_mask[];
extern u32 border_bottomleft_mask[];
extern u32 border_bottomright_mask[];

extern u32 lab_logo[];

extern Compositor global_compositor;

font_t *window_font = &font_mono;

static inline void window_put_background_pixel(i32 x, i32 y, bool is_main_window, i32 darken_numerator, i32 darken_denominator) {
    if (x >= SCREEN_W || y >= SCREEN_H) return;
    usize screen_idx = x + y * SCREEN_MAX_W;
    if (is_main_window) {
        framebuffer[screen_idx] = foreground_blurbuf[screen_idx];
        if (light_mode) {
            framebuffer[screen_idx] = mult_add_andclip_color_nooverflow(framebuffer[screen_idx], 3, 2, 50, 189);
        } else {
            ((color_t *)framebuffer)[screen_idx].r = (darken_numerator * ((color_t *)framebuffer)[screen_idx].r) / darken_denominator;
            ((color_t *)framebuffer)[screen_idx].g = (darken_numerator * ((color_t *)framebuffer)[screen_idx].g) / darken_denominator;
            ((color_t *)framebuffer)[screen_idx].b = (darken_numerator * ((color_t *)framebuffer)[screen_idx].b) / darken_denominator;
        }
    } else {
        framebuffer[screen_idx] = background_blurbuf[screen_idx];
        ((color_t *)framebuffer)[screen_idx].r /= DARK_MODE_WINDOW_DARKEN_FACTOR_INACTIVE;
        ((color_t *)framebuffer)[screen_idx].g /= DARK_MODE_WINDOW_DARKEN_FACTOR_INACTIVE;
        ((color_t *)framebuffer)[screen_idx].b /= DARK_MODE_WINDOW_DARKEN_FACTOR_INACTIVE;
    }
}

static inline void window_put_border_pixel(i32 x, i32 y, bool is_main_window) {
    if (x >= SCREEN_W || y >= SCREEN_H) return;
    usize screen_idx = x + y * SCREEN_MAX_W;
    framebuffer[screen_idx] = WINDOW_BORDER_COLOR;
}

kret_t Window::PaintBox(bool is_main_window) {
    for (usize y = bounds.y + WINDOW_BORDER_IMAGE_WIDTH; y < bounds.y + bounds.h - WINDOW_BORDER_IMAGE_WIDTH; y++) {
        for (usize x = bounds.x + WINDOW_BORDER_IMAGE_WIDTH; x < bounds.x + bounds.w - WINDOW_BORDER_IMAGE_WIDTH; x++) {
            if ((x >= SCREEN_W) || (y >= SCREEN_H)) continue;
            window_put_background_pixel(x,y,is_main_window, active_darken_numerator, active_darken_denominator);
        }
    }

    rect_t border_bounds = {.w=WINDOW_BORDER_IMAGE_WIDTH,.h=WINDOW_BORDER_IMAGE_WIDTH};

    // Top Left
    for (usize iter_y = 0; iter_y < WINDOW_BORDER_IMAGE_WIDTH; iter_y++) {
        for (usize iter_x = 0; iter_x < WINDOW_BORDER_IMAGE_WIDTH; iter_x++) {
            i32 x = iter_x + bounds.x;
            i32 y = iter_y + bounds.y;
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            u32 col = border_topleft_mask[iter_x+((iter_y)*WINDOW_BORDER_IMAGE_WIDTH)];
            if (0xFF != U32_TO_A(col)) continue;
            if (WINDOW_BORDER_TRANSPARENCY_COLORMASK > U32_TO_R(col)) {
                window_put_background_pixel(x,y,is_main_window, active_darken_numerator, active_darken_denominator);
            }
        }
    }
    for (usize iter_y = 0; iter_y < WINDOW_BORDER_IMAGE_WIDTH; iter_y++) {
        for (usize iter_x = 0; iter_x < WINDOW_BORDER_IMAGE_WIDTH; iter_x++) {
            i32 x = iter_x + bounds.x;
            i32 y = iter_y + bounds.y;
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            u32 col = border_topleft[iter_x+((iter_y)*WINDOW_BORDER_IMAGE_WIDTH)];
            if (0 == U32_TO_A(col)) continue;
            usize screen_idx = x + y * SCREEN_MAX_W;
            framebuffer[screen_idx] = lerp_color(framebuffer[screen_idx], WINDOW_BORDER_COLOR, U32_TO_A(col));
        }
    }

    // Bottom Left
    for (usize iter_y = 0; iter_y < WINDOW_BORDER_IMAGE_WIDTH; iter_y++) {
        for (usize iter_x = 0; iter_x < WINDOW_BORDER_IMAGE_WIDTH; iter_x++) {
            i32 x = iter_x + bounds.x;
            i32 y = iter_y + bounds.y + bounds.h - WINDOW_BORDER_IMAGE_WIDTH;
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            u32 col = border_bottomleft_mask[iter_x+((iter_y)*WINDOW_BORDER_IMAGE_WIDTH)];
            if (0xFF != U32_TO_A(col)) continue;
            if (WINDOW_BORDER_TRANSPARENCY_COLORMASK > U32_TO_R(col)) {
                window_put_background_pixel(x,y,is_main_window, active_darken_numerator, active_darken_denominator);
            }
        }
    }
    for (usize iter_y = 0; iter_y < WINDOW_BORDER_IMAGE_WIDTH; iter_y++) {
        for (usize iter_x = 0; iter_x < WINDOW_BORDER_IMAGE_WIDTH; iter_x++) {
            i32 x = iter_x + bounds.x;
            i32 y = iter_y + bounds.y + bounds.h - WINDOW_BORDER_IMAGE_WIDTH;
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            u32 col = border_bottomleft[iter_x+((iter_y)*WINDOW_BORDER_IMAGE_WIDTH)];
            if (0 == U32_TO_A(col)) continue;
            usize screen_idx = x + y * SCREEN_MAX_W;
            framebuffer[screen_idx] = lerp_color(framebuffer[screen_idx], WINDOW_BORDER_COLOR, U32_TO_A(col));
        }
    }

    // Top Right
    for (usize iter_y = 0; iter_y < WINDOW_BORDER_IMAGE_WIDTH; iter_y++) {
        for (usize iter_x = 0; iter_x < WINDOW_BORDER_IMAGE_WIDTH; iter_x++) {
            i32 x = iter_x + bounds.x + bounds.w - WINDOW_BORDER_IMAGE_WIDTH;
            i32 y = iter_y + bounds.y;
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            u32 col = border_topright_mask[iter_x+((iter_y)*WINDOW_BORDER_IMAGE_WIDTH)];
            if (0xFF != U32_TO_A(col)) continue;
            if (WINDOW_BORDER_TRANSPARENCY_COLORMASK > U32_TO_R(col)) {
                window_put_background_pixel(x,y,is_main_window, active_darken_numerator, active_darken_denominator);
            }
        }
    }
    for (usize iter_y = 0; iter_y < WINDOW_BORDER_IMAGE_WIDTH; iter_y++) {
        for (usize iter_x = 0; iter_x < WINDOW_BORDER_IMAGE_WIDTH; iter_x++) {
            i32 x = iter_x + bounds.x + bounds.w - WINDOW_BORDER_IMAGE_WIDTH;
            i32 y = iter_y + bounds.y;
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            u32 col = border_topright[iter_x+((iter_y)*WINDOW_BORDER_IMAGE_WIDTH)];
            if (0 == U32_TO_A(col)) continue;
            usize screen_idx = x + y * SCREEN_MAX_W;
            framebuffer[screen_idx] = lerp_color(framebuffer[screen_idx], WINDOW_BORDER_COLOR, U32_TO_A(col));
        }
    }

    // Bottom Right
    for (usize iter_y = 0; iter_y < WINDOW_BORDER_IMAGE_WIDTH; iter_y++) {
        for (usize iter_x = 0; iter_x < WINDOW_BORDER_IMAGE_WIDTH; iter_x++) {
            i32 x = iter_x + bounds.x + bounds.w - WINDOW_BORDER_IMAGE_WIDTH;
            i32 y = iter_y + bounds.y + bounds.h - WINDOW_BORDER_IMAGE_WIDTH;
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            u32 col = border_bottomright_mask[iter_x+((iter_y)*WINDOW_BORDER_IMAGE_WIDTH)];
            if (0xFF != U32_TO_A(col)) continue;
            if (WINDOW_BORDER_TRANSPARENCY_COLORMASK > U32_TO_R(col)) {
                window_put_background_pixel(x,y,is_main_window, active_darken_numerator, active_darken_denominator);
            }
        }
    }
    for (usize iter_y = 0; iter_y < WINDOW_BORDER_IMAGE_WIDTH; iter_y++) {
        for (usize iter_x = 0; iter_x < WINDOW_BORDER_IMAGE_WIDTH; iter_x++) {
            i32 x = iter_x + bounds.x + bounds.w - WINDOW_BORDER_IMAGE_WIDTH;
            i32 y = iter_y + bounds.y + bounds.h - WINDOW_BORDER_IMAGE_WIDTH;
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            u32 col = border_bottomright[iter_x+((iter_y)*WINDOW_BORDER_IMAGE_WIDTH)];
            if (0 == U32_TO_A(col)) continue;
            usize screen_idx = x + y * SCREEN_MAX_W;
            framebuffer[screen_idx] = lerp_color(framebuffer[screen_idx], WINDOW_BORDER_COLOR, U32_TO_A(col));
        }
    }

    for (usize y = bounds.y + WINDOW_BORDER_IMAGE_WIDTH; y < bounds.y + bounds.h - WINDOW_BORDER_IMAGE_WIDTH; y++) {
        for (usize x = bounds.x; x < bounds.x + WINDOW_BORDER_IMAGE_WIDTH; x++) {
            if (x < bounds.x + 1) window_put_border_pixel(x,y,is_main_window);
            else window_put_background_pixel(x,y,is_main_window, active_darken_numerator, active_darken_denominator);
        }
    }
    for (usize y = bounds.y + WINDOW_BORDER_IMAGE_WIDTH; y < bounds.y + bounds.h - WINDOW_BORDER_IMAGE_WIDTH; y++) {
        for (usize x = bounds.x + bounds.w - WINDOW_BORDER_IMAGE_WIDTH; x < bounds.x + bounds.w; x++) {
            if (x > bounds.x + bounds.w - 2) window_put_border_pixel(x,y,is_main_window);
            else window_put_background_pixel(x,y,is_main_window, active_darken_numerator, active_darken_denominator);
        }
    }
    for (usize y = bounds.y; y < bounds.y + WINDOW_BORDER_IMAGE_WIDTH; y++) {
        for (usize x = bounds.x + WINDOW_BORDER_IMAGE_WIDTH; x < bounds.x + bounds.w - WINDOW_BORDER_IMAGE_WIDTH; x++) {
            if (y < bounds.y + 1) window_put_border_pixel(x,y,is_main_window);
            else window_put_background_pixel(x,y,is_main_window, active_darken_numerator, active_darken_denominator);
        }
    }
    for (usize y = bounds.y + bounds.h - WINDOW_BORDER_IMAGE_WIDTH; y < bounds.y + bounds.h; y++) {
        for (usize x = bounds.x + WINDOW_BORDER_IMAGE_WIDTH; x < bounds.x + bounds.w - WINDOW_BORDER_IMAGE_WIDTH; x++) {
            if (y > bounds.y + bounds.h - 2) window_put_border_pixel(x,y,is_main_window);
            else window_put_background_pixel(x,y,is_main_window, active_darken_numerator, active_darken_denominator);
        }
    }

    return ALL_GOOD;
}

kret_t Window::Paint(i32 hover_x, i32 hover_y, bool is_main_window) {
    // Keep window 1 pixel away from the edge, for the
    // width of the window border
    if (bounds.x < 1) bounds.x = 1;
    if (bounds.y < TOPBAR_HEIGHT) bounds.y = TOPBAR_HEIGHT;
    if (bounds.x + bounds.w > SCREEN_W) {
        bounds.x = SCREEN_W - bounds.w - 1;
    }
    if (bounds.y + bounds.h + WINDOW_PADDING_BOTTOM > SCREEN_H) {
        bounds.y = SCREEN_H - bounds.h - WINDOW_PADDING_BOTTOM - 1;
    }
    xbutton_bounds = {
        .x = bounds.x + 2 * WINDOW_BORDER_RADUS - XBUTTON_W,
        .y = bounds.y + 2 * WINDOW_BORDER_RADUS - XBUTTON_H,
        .w = XBUTTON_W, .h = XBUTTON_H
    };

    PaintBox(is_main_window);

    usize window_namelen = strlen_gui(window_font, window_name);
    usize window_centered_y = (bounds.w / 2) - (window_namelen / 2);
    oxide_draw_string(window_font, window_name, bounds.x + window_centered_y, bounds.y + WINDOW_TITLE_PADDING);

    DrawInnerContent(hover_x, hover_y, is_main_window);

    if (!is_coord_in_box(hover_x, hover_y, xbutton_bounds)) {
        oxide_draw_image(xbutton_bounds.x, xbutton_bounds.y, OXIDE_SCALE_FACTOR, xbutton_bounds, xbutton);
    } else {
        oxide_draw_image(xbutton_bounds.x, xbutton_bounds.y, OXIDE_SCALE_FACTOR, xbutton_bounds, xbutton_hover);
    }

    return ALL_GOOD;
}

kret_t Window::Resize(i32 new_w, i32 new_h) {
    if (new_w < min_bounds.w) new_w = min_bounds.w;
    if (new_h < min_bounds.h) new_h = min_bounds.h;
    if (new_w > SCREEN_W - 4) new_w = SCREEN_W - 4;
    if (new_h > SCREEN_H - 18 - TOPBAR_HEIGHT) new_h = SCREEN_H - 18 - TOPBAR_HEIGHT;
    bounds.w = new_w;
    bounds.h = new_h;
    return ALL_GOOD;
}

bool Window::IsClosing() {
    return is_closing;
}

kret_t Window::DrawInnerContent(i32 hover_x, i32 hover_y, bool is_top) { return ALL_GOOD; }
kret_t Window::Keypress(char c) { return ALL_GOOD; }
kret_t Window::MouseClick(i32 click_x, i32 click_y, bool pressed) {
    if (pressed && is_coord_in_box(click_x, click_y, xbutton_bounds)) {
        OnClose();
        is_closing = true;
    }
    return OnClick(click_x, click_y, pressed);
}
kret_t Window::OnClick(i32 x, i32 y, bool p) {return ALL_GOOD;}
void Window::OnClose() {}

kret_t SystemWindow::DrawInnerContent(i32 hover_x, i32 hover_y, bool is_top) {
    char sbuf[128];
    snprintf(sbuf, "Kernel: %s", KERNEL_NAME);
    oxide_draw_string(window_font, sbuf, bounds.x + WINDOW_INNER_PADDING, bounds.y + (27 * font_mono.char_max_height / 3));
    snprintf(sbuf, "Graphics: %s", WINDOW_MANAGER_NAME);
    oxide_draw_string(window_font, sbuf, bounds.x + WINDOW_INNER_PADDING, bounds.y + (29 * font_mono.char_max_height / 3));
    snprintf(sbuf, "Variant: %s", KERNEL_VARIANT);
    oxide_draw_string(window_font, sbuf, bounds.x + WINDOW_INNER_PADDING, bounds.y + (31 * font_mono.char_max_height / 3));
    snprintf(sbuf, "CPU: %s", ARCH_STRING);
    oxide_draw_string(window_font, sbuf, bounds.x + WINDOW_INNER_PADDING, bounds.y + (33 * font_mono.char_max_height / 3));
    snprintf(sbuf, "By: %s", fractal_version_builder);
    oxide_draw_string(window_font, sbuf, bounds.x + WINDOW_INNER_PADDING, bounds.y + (35 * font_mono.char_max_height / 3));
    snprintf(sbuf, "Build: %s", fractal_version_build_date);
    oxide_draw_string(window_font, sbuf, bounds.x + WINDOW_INNER_PADDING, bounds.y + (37 * font_mono.char_max_height / 3));
    snprintf(sbuf, "Resolution: %d x %d", SCREEN_W, SCREEN_H);
    oxide_draw_string(window_font, sbuf, bounds.x + WINDOW_INNER_PADDING, bounds.y + (39 * font_mono.char_max_height / 3));

    snprintf(sbuf, "Made with <3 in Cambridge, MA", fractal_version_build_date);
    draw_string_centeredx(window_font, sbuf, bounds.x + bounds.w / 2, bounds.y + bounds.h - font_mono.char_max_height);

    oxide_draw_logo(
        bounds.x + ( bounds.w / 2) - (LOGO_WIDTH / 2),
        bounds.y + bounds.h - (471 / 4) + 8 - 25,
        OXIDE_SCALE_FACTOR
    );

    rect_t lab_icon_rect = {.x=0,.y=0,.w=130,.h=59};
    oxide_draw_image(
        bounds.x + (bounds.w / 6) - ((1042 / 16) - 2) + 14,
        bounds.y + bounds.h - (471 / 4) + 8,
        OXIDE_SCALE_FACTOR,
        lab_icon_rect,
        lab_logo
    );

    rect_t school_logo_rect={.x=0,.y=0,.w=130,.h=59};
    extern u32 school_logo[];
    oxide_draw_image(
        bounds.x + (5 * bounds.w / 6) - ((1042 / 16) - 2),
        bounds.y + bounds.h - (471 / 4) + 8,
        10 * OXIDE_SCALE_FACTOR / 11,
        school_logo_rect,
        school_logo
    );

    rect_t arch_icon_rect = {.w=ARCH_ICON_WIDTH,.h=ARCH_ICON_HEIGHT};
    oxide_draw_image(
        bounds.x + (bounds.w / 2) - ((ARCH_ICON_WIDTH / 2)),
        bounds.y + (4 * font_mono.char_max_height / 3),
        OXIDE_SCALE_FACTOR,
        arch_icon_rect,
#ifdef ARCH_X86
        arch_icon_x86
#endif
#ifdef ARCH_ARM
        arch_icon_arm
#endif
#ifdef ARCH_RISCV
        arch_icon_riscv
#endif
    );

    return ALL_GOOD;
}

SystemWindow::SystemWindow() {
    bounds.x = WINDOW_INIT_X;
    bounds.y = WINDOW_INIT_Y;
    bounds.w = 450;
    bounds.h = 560;
    min_bounds.w = bounds.w;
    min_bounds.h = bounds.h;
    active_darken_numerator = DARK_MODE_WINDOW_DEFAULT_DARKEN_NUMERATOR;
    active_darken_denominator = DARK_MODE_WINDOW_DEFAULT_DARKEN_DENOMINATOR;
    strncpy(window_name, "System Info", WINDOW_NAME_MAX);
}
