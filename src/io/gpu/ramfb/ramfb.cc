#include <fractal.h>
#include <io/gpu.h>
#include <io/gpu/ramfb-gpu.h>
#include <oxide.h>
#include <io/mouse.h>

extern i32 mouse_x, mouse_y;

// Every frame we cache the old framebuffer contents under the cursor here
i32 cursor_cache_x, cursor_cache_y;
u32 cursor_cache[CURSOR_W * CURSOR_H];
bool cursor_cache_valid = false;

extern bool demo_mode;

static inline u32 FIXUP_COLOR_32(u32 in_color) {
    u32 r = in_color >> 16 & 0x0FF;
    u32 g = in_color >> 8 & 0x0FF;
    u32 b = in_color & 0x0FF;
    return ((
        (r) | (g << 8) | (b << 16)
    ));
}

// All coords in screen-space, returns true if (x,y)
// is in the (CURSOR_W by CURSOR_H) rect at (screen_mouse_x, screen_mouse_y)
static inline bool is_in_cursor(i32 x, i32 y, i32 screen_mouse_x, i32 screen_mouse_y) {
    return (x >= screen_mouse_x) && (x < screen_mouse_x + CURSOR_W) && (y >= screen_mouse_y) && (y < screen_mouse_y + CURSOR_H);
}

kret_t RamfbGPUDevice::Repaint() {
    i32 cur_mouse_x = mouse_x;
    i32 cur_mouse_y = mouse_y;
    i32 hover_x = MOUSE_TO_SCREEN_X(cur_mouse_x);
    i32 hover_y = MOUSE_TO_SCREEN_Y(cur_mouse_y);

    // Make sure that ramfb points to a write-combining region,
    // otherwise this is going to be SLOW.
    for (usize y = 0; y < this->screen_h; y++) {
        for (usize x = 0; x < this->screen_w; x++) {
            // Check if we are about to draw over the cursor, stop if so
            if (!demo_mode && is_in_cursor(x,y,hover_x,hover_y)) {
                if (U32_TO_A(cursor[(x-hover_x)+((y-hover_y)*CURSOR_W)]) >= CURSOR_ALPHA_THRESH) {
                    this->ramfb[x+(y*screen_w)] = cursor[(x-hover_x)+((y-hover_y)*CURSOR_W)];
                    continue;
                }
            }

            this->ramfb[x+(y*screen_w)] = FIXUP_COLOR_32(framebuffer[x+(y*SCREEN_MAX_W)]);
        }
    }

    SetMouse(cur_mouse_x, cur_mouse_y);
    return ALL_GOOD;
}

kret_t RamfbGPUDevice::SetMouse(i32 x, i32 y) {
    i32 hover_x = MOUSE_TO_SCREEN_X(x);
    i32 hover_y = MOUSE_TO_SCREEN_Y(y);

    if (demo_mode) return ALL_GOOD;
    return ALL_GOOD;

    // Restore cursor cache
    if (cursor_cache_valid) {
        for (usize y = 0; y < CURSOR_H; y++) {
            for (usize x = 0; x < CURSOR_W; x++) {
                usize screen_idx = (x + cursor_cache_x) + ((y + cursor_cache_y) * screen_w);
                if (x + cursor_cache_x >= screen_w) continue;
                if (y + cursor_cache_y >= screen_h) continue;
                if (U32_TO_A(cursor[x+y*CURSOR_W]) < CURSOR_ALPHA_THRESH) continue;
                if (screen_idx > FRAMEBUFFER_MAX_INDEX) continue;
                this->ramfb[screen_idx] = cursor_cache[x+y*CURSOR_W];
            }
        }
    }

    // Draw new cursor, record new cursor cache
    for (usize y = 0; y < CURSOR_H; y++) {
        for (usize x = 0; x < CURSOR_W; x++) {
            usize screen_idx = (x + hover_x) + ((y + hover_y) * screen_w);
            usize screen_idx_fb = (x + hover_x) + ((y + hover_y) * SCREEN_MAX_W);
            if (x + hover_x >= screen_w) continue;
            if (y + hover_y >= screen_h) continue;
            if (U32_TO_A(cursor[x+y*CURSOR_W]) < CURSOR_ALPHA_THRESH) continue;
            if (screen_idx > FRAMEBUFFER_MAX_INDEX) continue;
            if (screen_idx_fb > FRAMEBUFFER_MAX_INDEX) continue;
            cursor_cache[x+y*CURSOR_W] = FIXUP_COLOR_32(framebuffer[screen_idx_fb]);
            this->ramfb[screen_idx] = cursor[x+y*CURSOR_W];
        }
    }

    cursor_cache_x = hover_x;
    cursor_cache_y = hover_y;
    cursor_cache_valid = true;
    return ALL_GOOD;
}

kret_t RamfbGPUDevice::QueryScreensize(struct virtio_gpu_rect *outr) {
    outr->width = screen_w;
    outr->height = screen_h;
    return ALL_GOOD;
}
