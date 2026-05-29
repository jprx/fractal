#include <fractal.h>
#include <oxide.h>
#include <io/mouse.h>
#include <oxide/window.h>
#include <oxide/compositor.h>
#include <version.h>

extern Window *top_window;
char barbuf[128];

kret_t TopBar::Paint(i32 hover_x, i32 hover_y, bool is_main_window) {
    bounds.w = SCREEN_W;
    bounds.h = TOPBAR_HEIGHT;
    bounds.x = 0;
    bounds.y = 0;

    for (usize y = bounds.y; y < bounds.y + bounds.h; y++) {
        for (usize x = bounds.x; x < bounds.x + bounds.w; x++) {
            if (x >= SCREEN_W || y >= SCREEN_H) continue;
            usize screen_idx = x + y * SCREEN_MAX_W;
            framebuffer[screen_idx] = foreground_blurbuf[screen_idx];
        }
    }

    if (NULL != top_window) {
        snprintf(barbuf, "FRACTAL %s", top_window->window_name);
    } else {
        snprintf(barbuf, "FRACTAL");
    }
    oxide_draw_string(window_font, barbuf, 15, 0);

    snprintf(barbuf, (char *)fractal_version_builder);
    usize rhs_padding = strlen_gui(window_font, barbuf);
    oxide_draw_string(window_font, barbuf, SCREEN_W - rhs_padding - 15, 0);

    return ALL_GOOD;
}
