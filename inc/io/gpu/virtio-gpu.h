#ifndef VIRTIO_GPU_H
#define VIRTIO_GPU_H

#include <fractal.h>
#include <io/gpu.h>
#include <io/virtio.h>
#include <io/virtio/virtqueue.h>
#include <io/virtio/virtio-pci.h>
#include <io/gpu/virtio-gpu-defs.h>

BEGIN_C_HEADER

PCIDevice *InitVirtioGPU(PCIHeader_t, usize, usize);

class VirtioGPU : public VirtioPCI {
public:
    // Points to the PCI common configuration structure for this GPU
    volatile virtio_pci_common_cfg *header;
    volatile u64 *notify;

    volatile Virtqueue *controlq;
    volatile Virtqueue *cursorq;

    /*
     * Negotiate
     * Reset and configure this GPU device.
     */
    virtual kret_t Negotiate() override;

    /*
     * Setup
     * Now that the virtqueue has been created,
     * send initial packets to "turn on" the display.
     */
    virtual kret_t Setup() override;

    /*
     * SendPacket
     * src: The data structure to send to the GPU
     * src_len: Size of the structure
     * dst: The address of a data buffer to read back from
     * dst_len: Size of the destination buffer
     *
     * Assumes that this makes use of two virtqueue descriptors:
     *     1. We send a read-only descriptor to the device with src
     *     2. We read a write-only descriptor from the device into dst
     */
    kret_t SendPacket(void *src, usize src_len, void *dst, usize dst_len);

    /*
     * Repaint
     * Send the current framebuffer out to the GPU to paint the screen.
     */
    kret_t Repaint();

    /*
     * SetMouse
     * Configure the cursorqueue to draw the cursor at screen-space (x, y).
     */
    kret_t SetMouse(i32 x, i32 y);

    /*
     * QueryScreensize
     * Returns the current screen size of this attached GPU.
     * (Assumes only one scanout).
     */
    kret_t QueryScreensize(struct virtio_gpu_rect *outr);

    /*
     * VirtioGPU
     * Creates a VirtioGPU using a common PCI configuration header,
     * discovered by enumerating the PCI bus.
     *
     * This will allocate two virtqueues (so, 6 small pages).
     * One for device control and another for cursor updates.
     */
    VirtioGPU(volatile virtio_pci_common_cfg *header_in, volatile u64 *notify_in, void *dev_conf_in, void *isr_conf_in, volatile PCIeHeader *pci_header_in);

    /*
     * ~VirtioGPU
     * Cleans up all allocated virtqueues and frees this device.
     */
    ~VirtioGPU();
};

// VirtioGPUDevice is a wrapper around VirtioGPU, which is a PCI device
// This is a "more abstract" FractalGPU object, which can be treated as a display device
// irrespective of its backing bus implementation.
class VirtioGPUDevice : public FractalGPU {
public:
    VirtioGPU *gpu;

    VirtioGPUDevice(VirtioGPU *pci_dev) {
        gpu = pci_dev;
    }

    virtual kret_t Repaint() override {
        return this->gpu->Repaint();
    }

    virtual kret_t SetMouse(i32 x, i32 y) override {
        return this->gpu->SetMouse(x,y);
    }

    virtual kret_t QueryScreensize(struct virtio_gpu_rect *outr) override {
        return this->gpu->QueryScreensize(outr);
    }
};

END_C_HEADER

#endif // VIRTIO_GPU_H
