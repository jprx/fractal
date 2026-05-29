#include <oxide.h>
#include <oxide/blur.h>
#include <oxide/logo.h>
#include <oxide/compositor.h>
#include <oxide/window.h>
#include <oxide/console.h>
#include <io/mouse.h>
#include <io/hid/input.h>

 // Used only by top window / top draggable UI elements:
u32 foreground_blurbuf[SCREEN_MAX_W * SCREEN_MAX_H];

// Used by all inactive UI elements.
// Should only be recalculated if very back layer (background) changes:
u32 background_blurbuf[SCREEN_MAX_W * SCREEN_MAX_H];

extern Compositor global_compositor;

// (w/2) by (h/2)
u32 downscaled1[((SCREEN_MAX_W * SCREEN_MAX_W))/4];

// (w/4) by (h/4)
u32 downscaled2[((SCREEN_MAX_W * SCREEN_MAX_W))/16];

// (w/8) by (h/8)
u32 downscaled3[((SCREEN_MAX_W * SCREEN_MAX_W))/64];

usize frame_idx = 0;

// Are we in a keynote presentation demo mode?
bool demo_mode = false;
usize demo_image_idx = 0;
u32 demo_image_buf[WALLPAPER_W * WALLPAPER_H];

// This is load_asset but it doesn't panic if it fails
void _load_demo_asset(char *abspath, void *dest, usize sz) {
    void *h = filesys_open_absolute(abspath);
    if (!h) goto fail;
    if (filesys_filelen(h) < sz) goto fail;
    if (sz != filesys_read(h, (u8*)dest, sz, 0)) goto fail;
    return;

fail:
    demo_mode = false;
    return;
}

#define DEMO_NUM_IMAGES 12

extern "C" void oxide_draw_logo(usize imgx, usize imgy, isize scale);

Dock dock;
TopBar bar;

extern u32 bootscreen_mountain[SCREEN_MAX_W * SCREEN_MAX_H];
extern u32 bootscreen_release[SCREEN_MAX_W * SCREEN_MAX_H];
extern u32 bootscreen_green[SCREEN_MAX_W * SCREEN_MAX_H];
u32 *wallpaper = (u32 *)&bootscreen_release;

#define MAX_WINDOWS 8

Window *windows[MAX_WINDOWS] = {0};
Window *top_window = NULL;

rect_t small_rect = {.w=SCREEN_MAX_W/2,.h=SCREEN_MAX_H/2};
rect_t smaller_rect = {.w=SCREEN_MAX_W/4,.h=SCREEN_MAX_H/4};
rect_t smallest_rect = {.w=SCREEN_MAX_W/8,.h=SCREEN_MAX_H/8};
rect_t screen_rect = {.w=SCREEN_MAX_W,.h=SCREEN_MAX_H};

// Still working on light mode- right now it looks pretty bad to be honest
bool light_mode = false;

void Compositor::Initialize() {
    mouse_moved = false;
    dragging_a_window = false;
    resizing_a_window = false;
    blur_needs_repaint = true;
    drag_offset_x = 0;
    drag_offset_y = 0;
    hover_x = 0;
    hover_y = 0;
    dock.bounds = {
        .x = 0,
        .y = SCREEN_MAX_H - 2 * DOCK_HEIGHT,
        .w = SCREEN_MAX_W,
        .h = DOCK_HEIGHT
    };

    for (usize i = 0; i < MAX_WINDOWS; i++) {
        windows[i] = NULL;
    }

    windows[0] = new SystemWindow();
    windows[0]->bounds.x = (WALLPAPER_W/2 - windows[0]->bounds.w / 2);
    windows[0]->bounds.y = (WALLPAPER_H/2 - windows[0]->bounds.h / 2) - TOPBAR_HEIGHT;
}

void Compositor::LaunchWindow(usize idx) {
    bool have_space = false;
    usize num_windows_allocd = 0;
    isize first_free_idx = -1;

    for (usize i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i]) {
            have_space = true;
            first_free_idx = i;
            break;
        }
        num_windows_allocd++;
    }

    if (!have_space) {
        printf("Cannot allocate any more windows\r\n");
        return;
    }

    Window *new_window;
    switch(idx) {
        default:
        // Unknown app
        return;
        break;

        case 1:
        new_window = new Console();
        break;

        case 2:
        new_window = new SystemWindow();
        break;

        case 3:
        new_window = new Settings();
        break;

        case 0:
        demo_mode = true;
        demo_image_idx = 0;
        _load_demo_asset("/keynote/slide0.bin", demo_image_buf, sizeof(demo_image_buf));
        return;
        break;
    }
    new_window->bounds.x = (SCREEN_W/2 - new_window->bounds.w / 2) + 2 * WINDOW_INIT_X * num_windows_allocd;
    new_window->bounds.y = (SCREEN_H/2 - new_window->bounds.h / 2) - TOPBAR_HEIGHT + WINDOW_INIT_Y * num_windows_allocd;
    blur_needs_repaint = true;

    for (isize i = first_free_idx; i > 0; i--) {
        windows[i] = windows[i-1];
    }

    windows[0] = new_window;
    top_window = new_window;
}

void Compositor::SendKey(char c) {
    if (top_window) top_window->Keypress(c);

    if (demo_mode) {
        char demo_image_path[32];
        switch (c) {
            case ' ':
            demo_mode = false;
            break;

            case KEY_RIGHT:
            case ']':
            if (demo_image_idx >= DEMO_NUM_IMAGES - 1) {
                demo_mode = false;
            } else {
                demo_image_idx++;
                snprintf(demo_image_path, "/keynote/slide%d.bin", demo_image_idx);
                _load_demo_asset(demo_image_path, demo_image_buf, sizeof(demo_image_buf));
            }
            break;

            case KEY_LEFT:
            case '[':
            if (demo_image_idx != 0) demo_image_idx--;
            snprintf(demo_image_path, "/keynote/slide%d.bin", demo_image_idx);
            _load_demo_asset(demo_image_path, demo_image_buf, sizeof(demo_image_buf));
            break;
        }
    }
}

void Compositor::MouseMoved() {
    hover_x = MOUSE_TO_SCREEN_X(mouse_x);
    hover_y = MOUSE_TO_SCREEN_Y(mouse_y);
    mouse_moved = true;
}

void Compositor::Draw() {
    i32 hover_x, hover_y;
    hover_x = MOUSE_TO_SCREEN_X(mouse_x);
    hover_y = MOUSE_TO_SCREEN_Y(mouse_y);

    if (demo_mode) {
        char demo_idx_str[16];
        oxide_stretch_wallpaper(demo_image_buf, WALLPAPER_W, WALLPAPER_H);
        return;
    }

    for (Window *w : windows) {
        if (!w) continue;
        if (w->IsClosing()) {
            CloseWindow(w);
        }
    }

    mouse_moved = false;

    oxide_stretch_wallpaper(wallpaper, WALLPAPER_W, WALLPAPER_H);

    if (background_changed) {
        if (use_blur) {
            blur_strided(
                (color_t *)downscaled3, (color_t *)framebuffer,
                screen_rect, 80, 8
            );
            upscale_double(downscaled2, downscaled3, smaller_rect);
            upscale_double(downscaled1, downscaled2, small_rect);
            upscale_double(background_blurbuf, downscaled1, screen_rect);
        } else {
            if (light_mode) {
                memset(background_blurbuf, '\xE0', sizeof(background_blurbuf));
            } else {
                memset(background_blurbuf, '\x40', sizeof(background_blurbuf));
            }
        }
        background_changed = false;
    }
    if (background_will_change) {
        background_changed = true;
        background_will_change = false;
    }

    if (top_window && dragging_a_window) {
        i32 new_x = hover_x - drag_offset_x;
        i32 new_y = hover_y - drag_offset_y;
        if (new_x < 0) new_x = 0;
        if (new_x + top_window->bounds.w > SCREEN_W) new_x = SCREEN_W - top_window->bounds.w - 1;
        if (new_y < TOPBAR_HEIGHT) new_y = TOPBAR_HEIGHT;
        if (new_y + top_window->bounds.h > SCREEN_H) new_y = SCREEN_H - top_window->bounds.h - 1;
        top_window->bounds.x = new_x;
        top_window->bounds.y = new_y;
    }

    if (top_window && resizing_a_window) {
        i32 new_w = hover_x - resize_offset_w;
        i32 new_h = hover_y - resize_offset_h;
        top_window->Resize(new_w, new_h);
    }

    for (isize i = MAX_WINDOWS-1; i >= 0; i--) {
        if (windows[i] != NULL) {
            if (top_window == windows[i]) continue;
            windows[i]->Paint(hover_x, hover_y, false);
        }
    }

    if (will_repaint_blur) {
        if (use_blur) {
            blur_strided(
                (color_t *)downscaled3, (color_t *)framebuffer,
                screen_rect, 80, 8
            );
            upscale_double(downscaled2, downscaled3, smaller_rect);
            upscale_double(downscaled1, downscaled2, small_rect);
            upscale_double(foreground_blurbuf, downscaled1, screen_rect);
        } else {
            if (light_mode) {
                memset(foreground_blurbuf, '\xF0', sizeof(foreground_blurbuf));
            } else {
                memset(foreground_blurbuf, '\x00', sizeof(foreground_blurbuf));
            }
        }
        will_repaint_blur = false;
    }
    if (blur_needs_repaint) {
        // If we need to repaint, schedule repaint for after we finish this current frame-
        // that way we are blurring an accurate representation of the frame contents
        will_repaint_blur = true;
        blur_needs_repaint = false;
    }

    if (top_window) top_window->Paint(hover_x, hover_y, true);

    bar.Paint(hover_x,hover_y,true);
    dock.Paint(hover_x,hover_y,true);

    // Fix colors for cocoa display (macOS)
#if FIXUP_COLORS_COCOA
    for (usize y = 0; y < SCREEN_H; y++) {
        for (usize x = 0; x < SCREEN_W; x++) {
            usize screen_idx = y * SCREEN_MAX_W + x;
            u8 r = U32_TO_R(framebuffer[screen_idx]);
            u8 g = U32_TO_G(framebuffer[screen_idx]);
            u8 b = U32_TO_B(framebuffer[screen_idx]);
            framebuffer[screen_idx] = PACK_COLOR(b,g,r);
        }
    }
#endif // FIXUP_COLORS_COCOA

    // Should Oxide draw a cursor? Or let the GPU do it?
#ifdef DRAW_CURSOR_INTO_FRAMEBUFFER
    for (usize y = 0; y < CURSOR_H; y++) {
        for (usize x = 0; x < CURSOR_W; x++) {
            usize screen_idx = (x + hover_x) + ((y + hover_y) * SCREEN_MAX_W);
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) continue;
            if (U32_TO_A(cursor[x+y*CURSOR_W]) < CURSOR_ALPHA_THRESH) continue;
            framebuffer[screen_idx] = cursor[x+y*CURSOR_W];
        }
    }
#endif // DRAW_CURSOR_INTO_FRAMEBUFFER

#ifdef DRAW_CURSOR_AS_BLOCK
    // Draw cursor
    if (hover_x < 0) hover_x = 0;
    if (hover_y < 0) hover_y = 0;
    if (hover_x >= SCREEN_W) hover_x = SCREEN_W-1;
    if (hover_y >= SCREEN_H) hover_y = SCREEN_H-1;
    for (usize y = hover_y; y < hover_y + CURSOR_H; y++) {
        for (usize x = hover_x; x < hover_x + CURSOR_W; x++) {
            framebuffer[x+(y*SCREEN_MAX_W)] = 0xFFFFFF;
        }
    }
#endif // DRAW_CURSOR_AS_BLOCK

    frame_idx++;
}

void Compositor::MouseClick(bool pressed) {
    // Detect which window we are looking at
    bool clicked_on_a_window = false;
    isize next_top_window_idx = -1;
    Window *next_top_window = NULL;
    i32 mouse_x_screen = MOUSE_TO_SCREEN_X(mouse_x);
    i32 mouse_y_screen = MOUSE_TO_SCREEN_Y(mouse_y);

    if (!pressed) {
        dragging_a_window = false;
        resizing_a_window = false;
        return;
    }

    for (isize i = 0; i < MAX_WINDOWS; i++) {
        Window *w = windows[i];
        if (!w) continue;
        if (w->bounds.x < mouse_x_screen && w->bounds.x + w->bounds.w > mouse_x_screen && w->bounds.y < mouse_y_screen && w->bounds.y + w->bounds.h > mouse_y_screen) {
            if (w != top_window) {
                // Set top window here because interrupts may rely on it being accurate
                // Later we will sort all the windows
                top_window = w;
                next_top_window = w;
                next_top_window_idx = i;
                blur_needs_repaint = true;
            }

            clicked_on_a_window = true;
            drag_offset_x = mouse_x_screen - w->bounds.x;
            drag_offset_y = mouse_y_screen - w->bounds.y;
            resize_offset_w = mouse_x_screen - w->bounds.w;
            resize_offset_h = mouse_y_screen - w->bounds.h;

            rect_t bottomright_corner = {
                .x = w->bounds.x + w->bounds.w - WINDOW_BORDER_IMAGE_WIDTH,
                .y = w->bounds.y + w->bounds.h - WINDOW_BORDER_IMAGE_WIDTH,
                .w = WINDOW_BORDER_IMAGE_WIDTH,
                .h = WINDOW_BORDER_IMAGE_WIDTH,
            };
            if (is_coord_in_box(mouse_x_screen, mouse_y_screen, bottomright_corner)) {
                resizing_a_window = true;
            } else {
                dragging_a_window = true;
            }

            // This may schedule a window for deletion next frame
            w->MouseClick(mouse_x_screen, mouse_y_screen, pressed);
            break;
        }
    }

    // Reset top window before checking whether we clicked the dock,
    // as the dock may call LaunchWindow which will set top_window
    // to the newly created window
    if (!clicked_on_a_window) {
        top_window = NULL;
        dragging_a_window = false;
        resizing_a_window = false;
    }

    if (is_coord_in_box(mouse_x_screen, mouse_y_screen, dock.bounds)) {
        dock.MouseClick(mouse_x_screen, mouse_y_screen, pressed);
    }

    if (NULL != next_top_window) {
        if (next_top_window_idx < 0 || next_top_window_idx >= MAX_WINDOWS) panic("window index invalid");
        // Re-sort all windows
        for (isize i = next_top_window_idx; i > 0; i--) {
            windows[i] = windows[i-1];
        }
        windows[0] = next_top_window;
        top_window = next_top_window;
    }
}

void Compositor::CloseWindow(Window *w) {
    if (!w) return;
    for (usize i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == w) {
            windows[i] = NULL;
            delete w;
        }
    }
    if (top_window == w) top_window = NULL;
}

