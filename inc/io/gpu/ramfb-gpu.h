#ifndef RAMFB_GPU_H
#define RAMFB_GPU_H

#include <fractal.h>

BEGIN_C_HEADER

class RamfbGPUDevice : public FractalGPU {
public:
    // The display screen buffer:
    volatile u32 *ramfb;
    i32 screen_w, screen_h;

    RamfbGPUDevice(u32 *ramfb_in, i32 w_in, i32 h_in) {
        ramfb = ramfb_in;
        screen_w = w_in;
        screen_h = h_in;
    }

    virtual kret_t Repaint() override;
    virtual kret_t SetMouse(i32 x, i32 y);
    virtual kret_t QueryScreensize(struct virtio_gpu_rect *outr);
};

END_C_HEADER

#endif // RAMFB_GPU_H
