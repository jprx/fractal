#include <fractal.h>
#include <oxide.h>
#include <io/virtio.h>
#include <io/gpu/virtio-gpu.h>
#include <io/pci.h>
#include <io/virtio/virtio-pci.h>

#define IDX_WRAP 65536

#define RESOURCE_ID_CURSOR 101

kret_t VirtioGPU::Negotiate() {
    kret_t rval;

    // Steps 1-6: Negotiate the feature bits
    rval = ResetBegin();
    if (ALL_GOOD != rval) return rval;

    // 7. Perform device-specific setup, including discovery of virtqueues for the device
    // Install the command queue

#ifdef VIRTIO_GPU_DEBUG_INIT
    printf("controlq (virt addr): 0x%X\r\n", controlq);
    printf("controlq->desc (virt addr): 0x%X\r\n", controlq->desc);
#endif // VIRTIO_GPU_DEBUG_INIT

    enable_virtqueue(header, VIRTIO_GPU_CONTROLQ_IDX, controlq);
    enable_virtqueue(header, VIRTIO_GPU_CUSRORQ_IDX, cursorq);

    __sync_synchronize();

    return ResetEnd();
}

kret_t VirtioGPU::Setup() {
    kret_t rval;
    struct virtio_gpu_ctrl_hdr get_info = {
        .type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO,
        .flags = 0,
        .fence_id = 0,
        .ctx_id = 0,
        .ring_idx = 0,
    };
    struct virtio_gpu_resp_display_info get_info_resp;

    rval = virtqueue_send_packet(controlq,
        &get_info,
        sizeof(get_info),
        &get_info_resp,
        sizeof(get_info_resp)
    );

    if (rval != ALL_GOOD) return rval;

    if (get_info_resp.hdr.type != VIRTIO_GPU_RESP_OK_DISPLAY_INFO) {
        printf("Failed to get display info\r\n");
        return OH_NO;
    }
#ifndef QUIET_BOOT
    printf("==== STEP 1 OK ====\r\n");
#endif // !QUIET_BOOT

    struct virtio_gpu_resource_create_2d create_resource = {
        .hdr = {
            .type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
        },
        .resource_id = 100,
        .format = VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM,
        .width = SCREEN_MAX_W,
        .height = SCREEN_MAX_H
    };

    struct virtio_gpu_ctrl_hdr generic_resp;

    rval = virtqueue_send_packet(controlq,
        &create_resource,
        sizeof(create_resource),
        &generic_resp,
        sizeof(generic_resp)
    );

    if (generic_resp.type != VIRTIO_GPU_RESP_OK_NODATA) return OH_NO;
    if (rval != ALL_GOOD) return rval;
#ifndef QUIET_BOOT
    printf("==== STEP 2 OK ====\r\n");
#endif // !QUIET_BOOT

    struct virtio_gpu_resource_attach_backing attach_backing = {
        .hdr = {
            .type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING
        },
        .resource_id = 100,
        .nr_entries = 1,
        .entries = {
            {
                .addr = KERN_V2P((usize)framebuffer),
                .length = SCREEN_MAX_W * SCREEN_MAX_H * 4,
                .padding = 0,
            }
        }
    };

    rval = virtqueue_send_packet(controlq,
        &attach_backing,
        sizeof(attach_backing),
        &generic_resp,
        sizeof(generic_resp)
    );

    if (generic_resp.type != VIRTIO_GPU_RESP_OK_NODATA) return OH_NO;
    if (rval != ALL_GOOD) return rval;
#ifndef QUIET_BOOT
    printf("==== STEP 3 OK ====\r\n");
#endif // !QUIET_BOOT

    struct virtio_gpu_set_scanout set_scanout = {
        .hdr = {
            .type = VIRTIO_GPU_CMD_SET_SCANOUT
        },
        .r = {
            .x = 0,
            .y = 0,
            .width = SCREEN_MAX_W,
            .height = SCREEN_MAX_H,
        },
        .scanout_id = 0,
        .resource_id = 100
    };

    rval = virtqueue_send_packet(controlq,
        &set_scanout,
        sizeof(set_scanout),
        &generic_resp,
        sizeof(generic_resp)
    );

    if (generic_resp.type != VIRTIO_GPU_RESP_OK_NODATA) return OH_NO;
    if (rval != ALL_GOOD) return rval;
#ifndef QUIET_BOOT
    printf("==== STEP 4 OK ====\r\n");
#endif // !QUIET_BOOT

    // Allocate cursor resource
    struct virtio_gpu_resource_create_2d create_cursor = {
        .hdr = {
            .type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
        },
        .resource_id = RESOURCE_ID_CURSOR,
        .format = VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM,
        .width = CURSOR_W,
        .height = CURSOR_H
    };

    rval = virtqueue_send_packet(controlq,
        &create_cursor,
        sizeof(create_cursor),
        &generic_resp,
        sizeof(generic_resp)
    );

    if (generic_resp.type != VIRTIO_GPU_RESP_OK_NODATA) return OH_NO;
    if (rval != ALL_GOOD) return rval;
#ifndef QUIET_BOOT
    printf("==== STEP 5 OK ====\r\n");
#endif // !QUIET_BOOT

    struct virtio_gpu_resource_attach_backing attach_cursor = {
        .hdr = {
            .type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING
        },
        .resource_id = RESOURCE_ID_CURSOR,
        .nr_entries = 1,
        .entries = {
            {
                .addr = KERN_V2P((usize)cursor),
                .length = CURSOR_W * CURSOR_H * 4,
                .padding = 0,
            }
        }
    };

    rval = virtqueue_send_packet(controlq,
        &attach_cursor,
        sizeof(attach_cursor),
        &generic_resp,
        sizeof(generic_resp)
    );

    if (generic_resp.type != VIRTIO_GPU_RESP_OK_NODATA) return OH_NO;
    if (rval != ALL_GOOD) return rval;
#ifndef QUIET_BOOT
    printf("==== STEP 6 OK ====\r\n");
#endif // !QUIET_BOOT

    struct virtio_gpu_transfer_to_host_2d transfer_cursor_req = {
        .hdr = {
            .type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D
        },
        .r = {
            .x = 0,
            .y = 0,
            .width = CURSOR_W,
            .height = CURSOR_H,
        },
        .offset = 0,
        .resource_id = RESOURCE_ID_CURSOR
    };

    rval = virtqueue_send_packet(controlq,
        &transfer_cursor_req,
        sizeof(transfer_cursor_req),
        &generic_resp,
        sizeof(generic_resp)
    );

    if (generic_resp.type != VIRTIO_GPU_RESP_OK_NODATA) return OH_NO;
    if (rval != ALL_GOOD) return rval;
#ifndef QUIET_BOOT
    printf("==== STEP 7 OK ====\r\n");
    printf("GPU initialized\r\n");
#endif // !QUIET_BOOT

    // Read ISR to suppress any interrupts
    u32 tmp = *(volatile u32 *)isr_config;

    return ALL_GOOD;
}

kret_t VirtioGPU::SetMouse(i32 x, i32 y) {
    kret_t rval;
    if (x < 0) x = 0;
    if (x > SCREEN_W - 64) x = SCREEN_W-1;
    if (y < 0) y = 0;
    if (y > SCREEN_H - 64) y = SCREEN_H-1;

    // VIRTIO_GPU_CMD_MOVE_CURSOR
    // (only pos.x and pos.y are used, hotx and hoty are unused)
    struct virtio_gpu_update_cursor update_req = {
        .hdr = {.type = VIRTIO_GPU_CMD_UPDATE_CURSOR},
        .pos = {
            .scanout_id = 0,
            .x = (u32)x,
            .y = (u32)y,
        },
        .resource_id = RESOURCE_ID_CURSOR,
        .hot_x = (u32)0,
        .hot_y = (u32)0,
    };

    struct virtio_gpu_ctrl_hdr generic_resp;

    rval = virtqueue_send_packet(cursorq,
        &update_req,
        sizeof(update_req),
        &generic_resp,
        sizeof(generic_resp)
    );

    return ALL_GOOD;
}

kret_t VirtioGPU::Repaint() {
    kret_t rval;
    struct virtio_gpu_transfer_to_host_2d transfer_req = {
        .hdr = {
            .type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D
        },
        .r = {
            .x = 0,
            .y = 0,
            .width = SCREEN_W,
            .height = SCREEN_H,
        },
        .offset = 0,
        .resource_id = 100
    };

    struct virtio_gpu_ctrl_hdr generic_resp;

    rval = virtqueue_send_packet(controlq,
        &transfer_req,
        sizeof(transfer_req),
        &generic_resp,
        sizeof(generic_resp)
    );

    if (generic_resp.type != VIRTIO_GPU_RESP_OK_NODATA) {
        printf("Failed repainting screen (1)\r\n");
        return OH_NO;
    }
    if (rval != ALL_GOOD) return rval;

    struct virtio_gpu_set_scanout set_scanout = {
        .hdr = {
            .type = VIRTIO_GPU_CMD_SET_SCANOUT
        },
        .r = {
            .x = 0,
            .y = 0,
            .width = SCREEN_W,
            .height = SCREEN_H,
        },
        .scanout_id = 0,
        .resource_id = 100
    };

    rval = virtqueue_send_packet(controlq,
        &set_scanout,
        sizeof(set_scanout),
        &generic_resp,
        sizeof(generic_resp)
    );

    if (generic_resp.type != VIRTIO_GPU_RESP_OK_NODATA) return OH_NO;

    struct virtio_gpu_resource_flush flushreq = {
        .hdr = {.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH},
        .r = {.x = 0, .y = 0, .width = SCREEN_MAX_W, .height = SCREEN_MAX_H},
        .resource_id = 100
    };

    rval = virtqueue_send_packet(controlq,
        &flushreq,
        sizeof(flushreq),
        &generic_resp,
        sizeof(generic_resp)
    );

    if (generic_resp.type != VIRTIO_GPU_RESP_OK_NODATA) {
        printf("Failed repainting screen (2)\r\n");
        return OH_NO;
    }
    if (rval != ALL_GOOD) return rval;

    // Read ISR to suppress any interrupts
    u32 tmp = *(volatile u32 *)isr_config;

    return ALL_GOOD;
}

kret_t VirtioGPU::QueryScreensize(struct virtio_gpu_rect *outr) {
    kret_t rval;
    struct virtio_gpu_ctrl_hdr get_info = {
        .type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO,
        .flags = 0,
        .fence_id = 0,
        .ctx_id = 0,
        .ring_idx = 0,
    };
    struct virtio_gpu_resp_display_info get_info_resp;

    rval = virtqueue_send_packet(controlq,
        &get_info,
        sizeof(get_info),
        &get_info_resp,
        sizeof(get_info_resp)
    );

    if (rval != ALL_GOOD) return rval;

    if (get_info_resp.hdr.type != VIRTIO_GPU_RESP_OK_DISPLAY_INFO) {
        printf("Failed to get display info\r\n");
        return OH_NO;
    }

    *outr = get_info_resp.pmodes[0].r;
    return ALL_GOOD;
}

VirtioGPU::VirtioGPU(volatile virtio_pci_common_cfg *h_in, volatile u64 *notify_in, void *dev_conf_in, void *isr_conf_in, volatile PCIeHeader *pci_header_in) : VirtioPCI(h_in, notify_in, dev_conf_in, isr_conf_in, pci_header_in) {
    if (!h_in) panic("Invalid virtio_pci_common_cfg header for GPU");
    header = h_in;
    notify = notify_in;
    device_config = dev_conf_in;
    isr_config = isr_conf_in;
    pci_header = pci_header_in;
    status_flags = 0;

    controlq = new Virtqueue((volatile u64*)notify);
    cursorq  = new Virtqueue((volatile u64*)notify);

    if (!controlq || !cursorq) {
        panic("Failed to allocate virtqueues for GPU");
    }
}

VirtioGPU::~VirtioGPU() {
    delete controlq;
    delete cursorq;
    controlq = NULL;
    cursorq  = NULL;
}
