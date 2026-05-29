#include <fractal.h>
#include <oxide.h>
#include <arch.h>
#include <oxide/window.h>
#include <oxide/logo.h>
#include <io/mouse.h>
#include <version.h>
#include <lib/mem.h>
#include <oxide/compositor.h>
#include <oxide/font.h>

#define PREVIEW_SPACE_BASE_X 199
#define PREVIEW_SPACE_BASE_Y 718

#define PREVIEW_RELEASE_BASE_X 125
#define PREVIEW_RELEASE_BASE_Y 423

#define PREVIEW_MOUNTAIN_BASE_X 486
#define PREVIEW_MOUNTAIN_BASE_Y 393

#define PREVIEW_GREEN_BASE_X 1654
#define PREVIEW_GREEN_BASE_Y 148

#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 55

extern u32 bootscreen_mountain[SCREEN_MAX_W * SCREEN_MAX_H];
extern u32 bootscreen_release[SCREEN_MAX_W * SCREEN_MAX_H];
extern u32 bootscreen_green[SCREEN_MAX_W * SCREEN_MAX_H];
extern u32 *wallpaper;

extern Compositor global_compositor;

static font_t *preferences_font = &font_mono;

Settings::Settings() {
    bounds.x = WINDOW_INIT_X;
    bounds.y = WINDOW_INIT_Y;
    bounds.w = 3 * (150 + WINDOW_INNER_PADDING - 10);
    bounds.h = 425;
    min_bounds.w = bounds.w;
    min_bounds.h = bounds.h;
    active_darken_numerator = DARK_MODE_WINDOW_DEFAULT_DARKEN_NUMERATOR;
    active_darken_denominator = DARK_MODE_WINDOW_DEFAULT_DARKEN_DENOMINATOR;
    strncpy(window_name, "Preferences", WINDOW_NAME_MAX);
}

kret_t Settings::DrawInnerContent(i32 hover_x, i32 hover_y, bool is_top) {
    rect_t left_box = {
        .x = bounds.x + 2 * WINDOW_INNER_PADDING,
        .y = (i32)(bounds.y + WINDOW_INNER_PADDING + font_mono.char_max_height),
        .w = 100, .h = 100
    };
    rect_t middle_box = {
        .x = (bounds.x + 2 * WINDOW_INNER_PADDING + 1 * 150),
        .y = (i32)(bounds.y + WINDOW_INNER_PADDING + font_mono.char_max_height),
        .w = 100, .h = 100
    };
    rect_t right_box = {
        .x = (bounds.x + 2 * WINDOW_INNER_PADDING + 2 * 150),
        .y = (i32)(bounds.y + WINDOW_INNER_PADDING + font_mono.char_max_height),
        .w = 100, .h = 100
    };

    for (usize y = left_box.y; y < left_box.y + left_box.h; y++) {
        for (usize x = left_box.x; x < left_box.x + left_box.w; x++) {
            usize screen_idx = x + y * SCREEN_MAX_W;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            usize img_idx = ((x - left_box.x) + PREVIEW_RELEASE_BASE_X) + ((y - left_box.y) + PREVIEW_RELEASE_BASE_Y) * WALLPAPER_W;
            framebuffer[screen_idx] = bootscreen_release[img_idx];
        }
    }

    for (usize y = middle_box.y; y < middle_box.y + middle_box.h; y++) {
        for (usize x = middle_box.x; x < middle_box.x + middle_box.w; x++) {
            usize screen_idx = x + y * SCREEN_MAX_W;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            usize img_idx = ((x - middle_box.x) + PREVIEW_MOUNTAIN_BASE_X) + ((y - middle_box.y) + PREVIEW_MOUNTAIN_BASE_Y) * WALLPAPER_W;
            framebuffer[screen_idx] = bootscreen_mountain[img_idx];
        }
    }

    for (usize y = right_box.y; y < right_box.y + right_box.h; y++) {
        for (usize x = right_box.x; x < right_box.x + right_box.w; x++) {
            usize screen_idx = x + y * SCREEN_MAX_W;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) panic("framebuffer oob access prevented");
            usize img_idx = ((x - right_box.x) + PREVIEW_GREEN_BASE_X) + ((y - right_box.y) + PREVIEW_GREEN_BASE_Y) * WALLPAPER_W;
            framebuffer[screen_idx] = bootscreen_green[img_idx];
        }
    }

    oxide_draw_box(
        left_box,
        WINDOW_BORDER_COLOR
    );
    oxide_draw_box(
        middle_box,
        WINDOW_BORDER_COLOR
    );
    oxide_draw_box(
        right_box,
        WINDOW_BORDER_COLOR
    );

    if (is_coord_in_box(hover_x, hover_y, left_box)) {
        oxide_draw_box(
            left_box,
            BUTTON_COLOR_HOVER
        );
    }
    if (is_coord_in_box(hover_x, hover_y, middle_box)) {
        oxide_draw_box(
            middle_box,
            BUTTON_COLOR_HOVER
        );
    }
    if (is_coord_in_box(hover_x, hover_y, right_box)) {
        oxide_draw_box(
            right_box,
            BUTTON_COLOR_HOVER
        );
    }

    draw_string_centeredx(
        preferences_font,
        "Retro",
        left_box.x + left_box.w / 2,
        left_box.y + 115
    );

    draw_string_centeredx(
        preferences_font,
        "Starlight",
        middle_box.x + middle_box.w / 2,
        middle_box.y + 115
    );

    draw_string_centeredx(
        preferences_font,
        "Greenway",
        right_box.x + right_box.w / 2,
        right_box.y + 115
    );

    toggle_button = {
        .x = bounds.x + ((bounds.w / 2) - (BUTTON_WIDTH / 2)),
        .y = bounds.y + 330,
        .w = BUTTON_WIDTH,
        .h = BUTTON_HEIGHT
    };

    lowres = {
        .x = bounds.x + 2 * WINDOW_INNER_PADDING,
        .y = bounds.y + 230,
        .w = 12 * (i32)font_get_spacewidth(window_font),
        .h = BUTTON_HEIGHT
    };

    midres = {
        .x = (bounds.x + 2 * WINDOW_INNER_PADDING + 1 * 150),
        .y = bounds.y + 230,
        .w = 12 * (i32)font_get_spacewidth(window_font),
        .h = BUTTON_HEIGHT
    };

    hires = {
        .x = (bounds.x + 2 * WINDOW_INNER_PADDING + 2 * 150),
        .y = bounds.y + 230,
        .w = 12 * (i32)font_get_spacewidth(window_font),
        .h = BUTTON_HEIGHT
    };

    oxide_draw_hover_button(toggle_button, hover_x, hover_y, "Toggle Effects", window_font);
    oxide_draw_hover_button(lowres, hover_x, hover_y, "1600 x 900", window_font);
    oxide_draw_hover_button(midres, hover_x, hover_y, "1920 x 1080", window_font);
    oxide_draw_hover_button(hires,  hover_x, hover_y, "2560 x 1440", window_font);

    return ALL_GOOD;
}

kret_t Settings::OnClick(i32 mx, i32 my, bool pressed) {
    rect_t left_box = {
        .x = bounds.x + 2 * WINDOW_INNER_PADDING,
        .y = (i32)(bounds.y + WINDOW_INNER_PADDING + font_mono.char_max_height),
        .w = 100, .h = 100
    };
    rect_t middle_box = {
        .x = (bounds.x + 2 * WINDOW_INNER_PADDING + 1 * 150),
        .y = (i32)(bounds.y + WINDOW_INNER_PADDING + font_mono.char_max_height),
        .w = 100, .h = 100
    };
    rect_t right_box = {
        .x = (bounds.x + 2 * WINDOW_INNER_PADDING + 2 * 150),
        .y = (i32)(bounds.y + WINDOW_INNER_PADDING + font_mono.char_max_height),
        .w = 100, .h = 100
    };

    if (is_coord_in_box(mx, my, left_box)) {
        wallpaper = (u32 *)&bootscreen_release;
        global_compositor.blur_needs_repaint = true;
        global_compositor.background_will_change = true;
    }
    if (is_coord_in_box(mx, my, middle_box)) {
        wallpaper = (u32 *)&bootscreen_mountain;
        global_compositor.blur_needs_repaint = true;
        global_compositor.background_will_change = true;
    }
    if (is_coord_in_box(mx, my, right_box)) {
        wallpaper = (u32 *)&bootscreen_green;
        global_compositor.blur_needs_repaint = true;
        global_compositor.background_will_change = true;
    }

    if (is_coord_in_box(mx,my,toggle_button)) {
        global_compositor.use_blur = !global_compositor.use_blur;
        global_compositor.blur_needs_repaint = true;
        global_compositor.background_will_change = true;
    }

    if (is_coord_in_box(mx,my,lowres)) {
        oxide_should_adjust_resolution = true;
        next_screen_w = 1600;
        next_screen_h = 900;
        global_compositor.blur_needs_repaint = true;
        global_compositor.background_will_change = true;
    }

    if (is_coord_in_box(mx,my,midres)) {
        oxide_should_adjust_resolution = true;
        next_screen_w = 1920;
        next_screen_h = 1080;
        global_compositor.blur_needs_repaint = true;
        global_compositor.background_will_change = true;
    }

    if (is_coord_in_box(mx,my,hires)) {
        oxide_should_adjust_resolution = true;
        next_screen_w = SCREEN_MAX_W;
        next_screen_h = SCREEN_MAX_H;
        global_compositor.blur_needs_repaint = true;
        global_compositor.background_will_change = true;
    }

    return ALL_GOOD;
}
