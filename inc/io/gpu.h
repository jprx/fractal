#ifndef FRACTAL_GPU_H
#define FRACTAL_GPU_H

#include <fractal.h>
#include <io/gpu/virtio-gpu-defs.h>

BEGIN_C_HEADER

// Some of these interfaces reuse virtio GPU structs, as
// this is essentially a lightweight wrapper over a VirtioGPU
// plus shims to support simple RAM framebuffer backends (eg. for real hardware)

class FractalGPU {
public:
    /*
     * Repaint
     * Send the current global framebuffer out to the GPU to paint the screen.
     */
    virtual kret_t Repaint() = 0;

    /*
     * SetMouse
     * Configure the cursorqueue to draw the cursor at screen-space (x, y).
     */
    virtual kret_t SetMouse(i32 x, i32 y) = 0;

    /*
     * QueryScreensize
     * Returns the current screen size of this attached GPU.
     * (Assumes only one screen).
     */
    virtual kret_t QueryScreensize(struct virtio_gpu_rect *outr) = 0;
};

END_C_HEADER

#endif // FRACTAL_GPU_H
