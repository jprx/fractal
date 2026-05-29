#ifndef OXIDE_BLUR_H
#define OXIDE_BLUR_H

#include <fractal.h>
#include <oxide.h>

BEGIN_C_HEADER

/*
 * downsample_nearest_neighbor
 * Downsample an image using the nearest neighbor approach
 * (just pick a close pixel in source image and that's the output one)
 *
 * This scales the entirety of src into dst
 *
 * dst: destination image buffer
 * src: source image buffer
 * dst_rect: bounding box for dst
 * src_rect: bounding box for src
 */
void downsample_nearest_neighbor(
    u32 *dst, u32 *src,
    rect_t dst_rect, rect_t src_rect
);

void upsample_bilinear(
    u32 *dst, u32 *src,
    rect_t dst_rect, rect_t src_rect
);

void upscale_double(
    u32 *dst, u32 *src,
    rect_t dst_rect
);

/*
 * blur
 * dst: destination buffer
 * src: source buffer
 * bounds: bounding box for both buffers
 * amount: how much blur
 */
void blur(
    color_t *dst, color_t *src,
    rect_t bounds,
    u32 amount
);

void blur_strided(
    color_t *dst, color_t *src,
    rect_t bounds,
    u32 amount, u32 stride
);

END_C_HEADER

#endif // OXIDE_BLUR_H
