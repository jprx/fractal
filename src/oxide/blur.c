#include <fractal.h>
#include <oxide.h>
#include <oxide/blur.h>

void blur(
    color_t *dst, color_t *src,
    rect_t bounds, u32 amount
) {
    // Box blur

    u64 neighborhood = 2*amount+1;

    // First, vertical blur
    // This can be slow if done for x then for y, so we use a large memory region
    // to hold per-column accumulators, and traverse for y then x
    i64 accumulator_r[bounds.w];
    i64 accumulator_g[bounds.w];
    i64 accumulator_b[bounds.w];
    for (usize x = 0; x < bounds.w; x++) {
        accumulator_r[x] = 0;
        accumulator_g[x] = 0;
        accumulator_b[x] = 0;
        for (usize y = 0; y < neighborhood; y++) {
            // Initialize accumulators to sum of all pixels up to amount
            usize idx = x + (y * bounds.w);
            accumulator_r[x] += src[idx].r;
            accumulator_g[x] += src[idx].g;
            accumulator_b[x] += src[idx].b;
        }
    }

    for (usize y = 0; y < bounds.h; y++) {
        for (usize x = 0; x < bounds.w; x++) {
            isize front_y = y + amount;
            isize back_y  = y - amount;
            usize front_idx = x + (front_y * bounds.w);
            usize back_idx  = x + (back_y * bounds.w);
            usize idx = x + (y * bounds.w);

            if (back_y >= 0 && front_y < bounds.h) {
                accumulator_r[x] += src[front_idx].r - src[back_idx].r;
                accumulator_g[x] += src[front_idx].g - src[back_idx].g;
                accumulator_b[x] += src[front_idx].b - src[back_idx].b;
            }

            i64 pack_r = (accumulator_r[x] / neighborhood);
            i64 pack_g = (accumulator_g[x] / neighborhood);
            i64 pack_b = (accumulator_b[x] / neighborhood);

            // Handle underflows
            if (accumulator_r[x] < 0) {
                pack_r = 0;
            }
            if (accumulator_g[x] < 0) {
                pack_g = 0;
            }
            if (accumulator_b[x] < 0) {
                pack_b = 0;
            }

            if (pack_r < CHANNEL_MIN) pack_r = CHANNEL_MIN;
            if (pack_g < CHANNEL_MIN) pack_g = CHANNEL_MIN;
            if (pack_b < CHANNEL_MIN) pack_b = CHANNEL_MIN;
            if (pack_r > CHANNEL_MAX) pack_r = CHANNEL_MAX;
            if (pack_g > CHANNEL_MAX) pack_g = CHANNEL_MAX;
            if (pack_b > CHANNEL_MAX) pack_b = CHANNEL_MAX;

            dst[idx].v = PACK_COLOR(
                pack_r,
                pack_g,
                pack_b
            );
        }
    }

    // Next, perform horizontal blur
    // Since dst is written to and read from, we must cache each row before blurring it
    for (usize y = 0; y < bounds.h; y++) {
        u64 accumulator_r = 0;
        u64 accumulator_g = 0;
        u64 accumulator_b = 0;
        for (usize x = 0; x < neighborhood; x++) {
            // Initialize accumulators to sum of all pixels up to amount
            usize idx = x + (y * bounds.w);
            accumulator_r += dst[idx].r;
            accumulator_g += dst[idx].g;
            accumulator_b += dst[idx].b;
        }
        // Cache old values we read from src in this:
        // During second pass we modify dst in-place, so only read from dst once
        color_t old_val_cache[bounds.w];
        for (usize i = 0; i < bounds.w; i++) {
            usize idx = i + (y * bounds.w);
            old_val_cache[i].v = dst[idx].v;
        }
        for (usize x = 0; x < bounds.w; x++) {
            isize front_x = x + amount;
            isize back_x = x - amount;
            usize front_idx = front_x + (y * bounds.w);
            usize back_idx  = back_x + (y * bounds.w);
            usize idx = x + (y * bounds.w);

            if (back_x >= 0 && front_x < bounds.w) {
                accumulator_r += old_val_cache[front_x].r - old_val_cache[back_x].r;
                accumulator_g += old_val_cache[front_x].g - old_val_cache[back_x].g;
                accumulator_b += old_val_cache[front_x].b - old_val_cache[back_x].b;
            }

            i64 pack_r = (accumulator_r / neighborhood);
            i64 pack_g = (accumulator_g / neighborhood);
            i64 pack_b = (accumulator_b / neighborhood);

            // Handle underflows
            if (accumulator_r < 0) {
                pack_r = 0;
            }
            if (accumulator_g < 0) {
                pack_g = 0;
            }
            if (accumulator_b < 0) {
                pack_b = 0;
            }

            if (pack_r < CHANNEL_MIN) pack_r = CHANNEL_MIN;
            if (pack_g < CHANNEL_MIN) pack_g = CHANNEL_MIN;
            if (pack_b < CHANNEL_MIN) pack_b = CHANNEL_MIN;
            if (pack_r > CHANNEL_MAX) pack_r = CHANNEL_MAX;
            if (pack_g > CHANNEL_MAX) pack_g = CHANNEL_MAX;
            if (pack_b > CHANNEL_MAX) pack_b = CHANNEL_MAX;

            dst[idx].v = PACK_COLOR(
                pack_r,
                pack_g,
                pack_b
            );
        }
    }
}

// Blur but only look at every stride'd pixel
void blur_strided(
    color_t *dst, color_t *src,
    rect_t bounds, u32 amount, u32 stride
) {
    // Box blur

    u64 neighborhood = (2*amount+1);

    // First, vertical blur
    // This can be slow if done for x then for y, so we use a large memory region
    // to hold per-column accumulators, and traverse for y then x
    i64 accumulator_r[bounds.w];
    i64 accumulator_g[bounds.w];
    i64 accumulator_b[bounds.w];
    for (usize x = 0; x < bounds.w; x+=stride) {
        accumulator_r[x] = 0;
        accumulator_g[x] = 0;
        accumulator_b[x] = 0;
        for (usize y = 0; y < neighborhood; y+=stride) {
            // Initialize accumulators to sum of all pixels up to amount
            usize idx = x + (y * bounds.w);
            accumulator_r[x] += src[idx].r;
            accumulator_g[x] += src[idx].g;
            accumulator_b[x] += src[idx].b;
        }
    }

    for (usize y = 0; y < bounds.h; y+=stride) {
        for (usize x = 0; x < bounds.w; x+=stride) {
            isize front_y = y + amount;
            isize back_y  = y - amount;
            usize front_idx = x + (front_y * bounds.w);
            usize back_idx  = x + (back_y * bounds.w);
            usize idx = x + (y * bounds.w);
            usize dst_idx = (x / stride) + ((y / stride) * (bounds.w/stride));

            if (back_y >= 0 && front_y < bounds.h) {
                accumulator_r[x] += src[front_idx].r - src[back_idx].r;
                accumulator_g[x] += src[front_idx].g - src[back_idx].g;
                accumulator_b[x] += src[front_idx].b - src[back_idx].b;
            }

            i64 pack_r = ((stride * accumulator_r[x]) / neighborhood);
            i64 pack_g = ((stride * accumulator_g[x]) / neighborhood);
            i64 pack_b = ((stride * accumulator_b[x]) / neighborhood);

            // Handle underflows
            if (accumulator_r[x] < 0) {
                pack_r = 0;
            }
            if (accumulator_g[x] < 0) {
                pack_g = 0;
            }
            if (accumulator_b[x] < 0) {
                pack_b = 0;
            }

            if (pack_r < CHANNEL_MIN) pack_r = CHANNEL_MIN;
            if (pack_g < CHANNEL_MIN) pack_g = CHANNEL_MIN;
            if (pack_b < CHANNEL_MIN) pack_b = CHANNEL_MIN;
            if (pack_r > CHANNEL_MAX) pack_r = CHANNEL_MAX;
            if (pack_g > CHANNEL_MAX) pack_g = CHANNEL_MAX;
            if (pack_b > CHANNEL_MAX) pack_b = CHANNEL_MAX;

            dst[dst_idx].v = PACK_COLOR(
                pack_r,
                pack_g,
                pack_b
            );
        }
    }

    // Next, perform horizontal blur
    // Since dst is written to and read from, we must cache each row before blurring it
    for (usize y = 0; y < bounds.h; y+=stride) {
        u64 accumulator_r = 0;
        u64 accumulator_g = 0;
        u64 accumulator_b = 0;
        for (usize x = 0; x < neighborhood; x+=stride) {
            // Initialize accumulators to sum of all pixels up to amount
            usize idx = (x / stride) + ((y / stride) * (bounds.w / stride));
            accumulator_r += dst[idx].r;
            accumulator_g += dst[idx].g;
            accumulator_b += dst[idx].b;
        }
        // Cache old values we read from src in this:
        // During second pass we modify dst in-place, so only read from dst once
        color_t old_val_cache[bounds.w];
        for (usize i = 0; i < bounds.w; i++) {
            usize idx = (i / stride) + ((y / stride) * (bounds.w / stride));
            old_val_cache[i].v = dst[idx].v;
        }
        for (usize x = 0; x < bounds.w; x+=stride) {
            isize front_x = x + amount;
            isize back_x = x - amount;
            usize front_idx = front_x + (y * bounds.w);
            usize back_idx  = back_x + (y * bounds.w);
            usize idx = x + (y * bounds.w);
            usize dst_idx = (x / stride) + ((y / stride) * (bounds.w / stride));

            if (back_x >= 0 && front_x < bounds.w) {
                accumulator_r += old_val_cache[front_x].r - old_val_cache[back_x].r;
                accumulator_g += old_val_cache[front_x].g - old_val_cache[back_x].g;
                accumulator_b += old_val_cache[front_x].b - old_val_cache[back_x].b;
            }

            i64 pack_r = ((stride * accumulator_r) / neighborhood);
            i64 pack_g = ((stride * accumulator_g) / neighborhood);
            i64 pack_b = ((stride * accumulator_b) / neighborhood);

            // Handle underflows
            if (accumulator_r < 0) {
                pack_r = 0;
            }
            if (accumulator_g < 0) {
                pack_g = 0;
            }
            if (accumulator_b < 0) {
                pack_b = 0;
            }

            if (pack_r < CHANNEL_MIN) pack_r = CHANNEL_MIN;
            if (pack_g < CHANNEL_MIN) pack_g = CHANNEL_MIN;
            if (pack_b < CHANNEL_MIN) pack_b = CHANNEL_MIN;
            if (pack_r > CHANNEL_MAX) pack_r = CHANNEL_MAX;
            if (pack_g > CHANNEL_MAX) pack_g = CHANNEL_MAX;
            if (pack_b > CHANNEL_MAX) pack_b = CHANNEL_MAX;

            dst[dst_idx].v = PACK_COLOR(
                pack_r,
                pack_g,
                pack_b
            );
        }
    }
}

/*
 * downsample_nearest_neighbor
 * Downsample an image using the nearest neighbor approach
 * (just pick a close pixel in source image and that's the output one)
 *
 * This scales the entirety of src into dst
 */
void downsample_nearest_neighbor(
    u32 *dst, u32 *src,
    rect_t dst_rect, rect_t src_rect
) {
    u32 scaled_width, scaled_height;

    scaled_width  = dst_rect.w;
    scaled_height = dst_rect.h;

    for (usize y = 0; y < scaled_height; y++) {
        for (usize x = 0; x < scaled_width; x++) {
            u32 src_x = (src_rect.w * x) / scaled_width;
            u32 src_y = (src_rect.h * y) / scaled_height;
            u32 dst_x = x;
            u32 dst_y = y;

            if (src_x > src_rect.w) continue;
            if (src_y > src_rect.h) continue;
            if (dst_y > dst_rect.h) continue;
            if (dst_x > dst_rect.w) continue;

            u32 src_idx = src_x + (src_y * src_rect.w);
            u32 dst_idx = dst_x + (dst_y * dst_rect.w);
            u32 col = src[src_idx];
            if (0 == COLOR_IGNORE_ALPHA(col)) continue;

            dst[dst_idx] = col;
        }
    }
}

// Upscale an image to twice its original value
// Faster version of upsample_bilinear, except instead of bilinear interpolation
// we copy exact pixels or merge pixels 50/50
// src_rect is assumed to have width dst_rect.w/2 and height dst_rect.h/2
void upscale_double(
    u32 *dst, u32 *src,
    rect_t dst_rect
) {
    for (usize y = 0; y < dst_rect.h; y++) {
        for (usize x = 0; x < dst_rect.w; x++) {
            usize dst_idx = x + y * dst_rect.w;
            usize src_idx = (x >> 1) + (y >> 1) * (dst_rect.w >> 1);
            if ((x % 2 == 0) && (y % 2 == 0)) {
                dst[dst_idx] = src[src_idx];
                continue;
            }
            if ((x == dst_rect.w - 1) || (y == dst_rect.h - 1)) {
                // Prevent overrunning the source buffer
                dst[dst_idx] = src[
                    (x >> 1) + (y >> 1) * (dst_rect.w >> 1)
                ];
                continue;
            }
            if (x % 2 == 0) {
                dst[dst_idx] = lerp_color(src[
                    (x >> 1) + (y >> 1) * (dst_rect.w >> 1)
                ], src[
                    (x >> 1) + ((y + 1) >> 1) * (dst_rect.w >> 1)
                ], LERP_HALF);
                continue;
            }
            if (y % 2 == 0) {
                dst[dst_idx] = lerp_color(src[
                    (x >> 1) + (y >> 1) * (dst_rect.w >> 1)
                ], src[
                    ((x + 1) >> 1) + (y >> 1) * (dst_rect.w >> 1)
                ], LERP_HALF);
                continue;
            }
            dst[dst_idx] = lerp_color(src[
                (x >> 1) + (y >> 1) * (dst_rect.w >> 1)
            ], src[
                ((x + 1) >> 1) + ((y + 1) >> 1) * (dst_rect.w >> 1)
            ], LERP_HALF);
        }
    }
}

// WARNING: This function is super slow, do not use it
void upsample_bilinear(
    u32 *dst, u32 *src,
    rect_t dst_rect, rect_t src_rect
) {
    u32 scaled_width, scaled_height;

    scaled_width  = dst_rect.w;
    scaled_height = dst_rect.h;

    for (usize y = 0; y < scaled_height; y++) {
        for (usize x = 0; x < scaled_width; x++) {
            // Calculate dst pixel based on 4 elements of src

            // Low = floor(x * (source width / dest width))
            // AKA how far are we into the image, width wise
            u32 low_x = floor_div(x * src_rect.w, dst_rect.w);
            u32 low_y = floor_div(y * src_rect.h, dst_rect.h);
            u32 hi_x = low_x + 1;
            u32 hi_y = low_y + 1;

            u32 val11 = src[low_x + (low_y * src_rect.w)];
            u32 val12 = src[low_x + (hi_y * src_rect.w)];
            u32 val21 = src[hi_x + (low_y * src_rect.w)];
            u32 val22 = src[hi_x + (hi_y * src_rect.w)];

            u32 how_close_x_is_to_low_x = (hi_x - x) / (hi_x - low_x);
            u32 how_close_x_is_to_high_x = (x - low_x) / (hi_x - low_x);

            // Lerp each axis
            u32 lerped_y1 = lerp_color(val11, val21, how_close_x_is_to_low_x);
            u32 lerped_y2 = lerp_color(val12, val22, how_close_x_is_to_high_x);

            u32 how_close_y_is_to_high_y = (y - low_y) / (hi_y - low_y);

            // Lerp one more time
            u32 val = lerp_color(lerped_y1, lerped_y2, how_close_y_is_to_high_y);

            usize dst_idx = x + (y * dst_rect.w);
            dst[dst_idx] = val;
        }
    }
}
