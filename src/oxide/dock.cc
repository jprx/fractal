#include <fractal.h>
#include <oxide.h>
#include <io/mouse.h>
#include <oxide/window.h>
#include <oxide/compositor.h>
#include <oxide/font.h>

#define DOCK_NUM_ICONS 5
#define DOCK_ICON_W 100
#define DOCK_ICON_H 100
#define DOCK_ICON_PADDING 50

extern Compositor global_compositor;

#define ARCH_ICON_WIDTH 194
#define ARCH_ICON_HEIGHT 194
extern u32 arch_icon_x86[ARCH_ICON_HEIGHT*ARCH_ICON_HEIGHT];

extern u32 icon_fractal[DOCK_ICON_W*DOCK_ICON_H];
extern u32 icon_console[DOCK_ICON_W*DOCK_ICON_H];
extern u32 icon_prefs[DOCK_ICON_W*DOCK_ICON_H];
extern u32 icon_info[DOCK_ICON_W*DOCK_ICON_H];
extern u32 icon_textedit[DOCK_ICON_W*DOCK_ICON_H];

u32 *icon_lut[] = {
    icon_fractal,
    icon_console,
    icon_prefs,
    icon_info,
    icon_textedit
};

char *app_tooltips[] = {
    "Fractal",
    "Console",
    "System Info",
    "Preferences",
    "Text Editor"
};

kret_t Dock::Paint(i32 hover_x, i32 hover_y, bool is_main_window) {
    rect_t icons[DOCK_NUM_ICONS];
    bounds.w = DOCK_NUM_ICONS * DOCK_ICON_W + (DOCK_NUM_ICONS - 1) * DOCK_ICON_PADDING + 2 * WINDOW_INNER_PADDING;
    bounds.x = SCREEN_W / 2 - bounds.w / 2;
    bounds.y = SCREEN_H - DOCK_HEIGHT - 5;

    for (usize i = 0; i < DOCK_NUM_ICONS; i++) {
        icons[i] = {
            .x = (i32)((DOCK_ICON_W + DOCK_ICON_PADDING) * i) + bounds.x + WINDOW_INNER_PADDING,
            .y = bounds.y + WINDOW_INNER_PADDING,
            .w = DOCK_ICON_W, .h = DOCK_ICON_H
        };
    };

    PaintBox(is_main_window);

    for (usize idx = 0; idx < DOCK_NUM_ICONS; idx++) {
        oxide_draw_image(
            icons[idx].x,
            icons[idx].y,
            OXIDE_SCALE_FACTOR,
            icons[idx],
            icon_lut[idx]
        );

        if (is_coord_in_box(hover_x, hover_y, icons[idx])) {
            usize textlen = strlen_gui(window_font, app_tooltips[idx]) + 15;
            for (usize y = icons[idx].y - 60; y < icons[idx].y - 60 + font_mono.char_max_height - 6; y++) {
                for (usize x_offset = 0; x_offset < textlen; x_offset++) {
                    usize x = icons[idx].x + (DOCK_ICON_W / 2) - (textlen / 2) + x_offset;
                    usize screen_idx = x+(y*SCREEN_MAX_W);
                    if (x >= SCREEN_W || y >= SCREEN_H) continue;
                    framebuffer[screen_idx] = foreground_blurbuf[screen_idx];
                }
            }
            draw_string_centeredx(window_font, app_tooltips[idx], icons[idx].x + icons[idx].w / 2, icons[idx].y - 60);
        }
    }

    return ALL_GOOD;
}

kret_t Dock::MouseClick(i32 click_x, i32 click_y, bool pressed) {
    rect_t icons[DOCK_NUM_ICONS];
    for (usize i = 0; i < DOCK_NUM_ICONS; i++) {
        icons[i] = {
            .x = (i32)((DOCK_ICON_W + DOCK_ICON_PADDING) * i) + bounds.x + WINDOW_INNER_PADDING,
            .y = bounds.y + WINDOW_INNER_PADDING,
            .w = DOCK_ICON_W, .h = DOCK_ICON_H
        };
    };
    if (pressed) {
        for (usize idx = 0; idx < DOCK_NUM_ICONS; idx++) {
            if (is_coord_in_box(
                click_x, click_y,
                icons[idx]
            )) {
                global_compositor.LaunchWindow(idx);
            }
        }
    }
    return ALL_GOOD;
}
