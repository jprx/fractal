#include <fractal.h>
#include <oxide.h>
#include <io/mouse.h>
#include <oxide/window.h>
#include <oxide/compositor.h>
#include <shutdown.h>

#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 55

Menu::Menu() {
    bounds.x = 400;
    bounds.y = 400;
    bounds.w = 300;
    bounds.h = 400;
    min_bounds.w = bounds.w;
    min_bounds.h = bounds.h;
    active_darken_numerator = DARK_MODE_WINDOW_DEFAULT_DARKEN_NUMERATOR;
    active_darken_denominator = DARK_MODE_WINDOW_DEFAULT_DARKEN_DENOMINATOR;
    strncpy(window_name, "Menu", WINDOW_NAME_MAX);
}

kret_t Menu::DrawInnerContent(i32 hover_x, i32 hover_y, bool is_top) {
    rect_t button_rect = {
        .x = bounds.x + ((bounds.w / 2) - (BUTTON_WIDTH / 2)),
        .y = bounds.y + 150,
        .w = BUTTON_WIDTH,
        .h = BUTTON_HEIGHT
    };

    rect_t button_rect2 = {
        .x = bounds.x + ((bounds.w / 2) - (BUTTON_WIDTH / 2)),
        .y = bounds.y + 225,
        .w = BUTTON_WIDTH,
        .h = BUTTON_HEIGHT
    };

    oxide_draw_hover_button(button_rect, hover_x, hover_y, "Crash Kernel", window_font);

    oxide_draw_hover_button(button_rect2, hover_x, hover_y, "Shutdown", window_font);

    oxide_draw_string(&font_ultralight, "Devbox", bounds.x, bounds.y + 30);

#ifdef STRESS_TEST_BOX_RENDERER
    // Make sure that large out-of-bounds draw calls don't panic the kernel
    rect_t out_of_bounds = {
        .x = SCREEN_W / 2 + bounds.x,
        .y = SCREEN_H / 2 + bounds.y,
        .w = 10000,
        .h = 10000 
    };
    oxide_draw_box(out_of_bounds, 0x41414141);
#endif // STRESS_TEST_BOX_RENDERER

    return ALL_GOOD;
}

kret_t Menu::OnClick(i32 click_x, i32 click_y, bool pressed) {
    rect_t button_rect = {
        .x = bounds.x + ((bounds.w / 2) - (BUTTON_WIDTH / 2)),
        .y = bounds.y + 150,
        .w = BUTTON_WIDTH,
        .h = BUTTON_HEIGHT
    };
    rect_t button_rect2 = {
        .x = bounds.x + ((bounds.w / 2) - (BUTTON_WIDTH / 2)),
        .y = bounds.y + 225,
        .w = BUTTON_WIDTH,
        .h = BUTTON_HEIGHT
    };
    if (is_coord_in_box(click_x, click_y, button_rect) && pressed) {
        u64 *invalid_ptr = (u64 *)0xDEADC0DE;
        *invalid_ptr = 0x4242424242424242;
    }
    if (is_coord_in_box(click_x, click_y, button_rect2) && pressed) {
        shutdown();
    }

    return ALL_GOOD;
}
