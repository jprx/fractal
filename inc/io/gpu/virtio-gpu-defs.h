#ifndef VIRTIO_GPU_DEFS_H
#define VIRTIO_GPU_DEFS_H

#include <fractal.h>

BEGIN_C_HEADER

// Device feature bits
#define VIRTIO_GPU_F_VIRGL          ((1 << 0))
#define VIRTIO_GPU_F_EDID           ((1 << 1))
#define VIRTIO_GPU_F_RESOURCE_UUID  ((1 << 2))
#define VIRTIO_GPU_F_RESOURCE_BLOB  ((1 << 3))
#define VIRTIO_GPU_F_CONTEXT_INIT   ((1 << 4))

#define VIRTIO_GPU_EVENT_DISPLAY    ((1 << 0))

#define VIRTIO_GPU_CONTROLQ_IDX 0
#define VIRTIO_GPU_CUSRORQ_IDX  1

// This is what VIRTIO_PCI_CAP_DEVICE_CFG refers to (See VirtIO Spec 5.7.4)
typedef struct {
    u32 events_read;
    u32 events_clear;
    u32 num_scanouts;
    u32 num_capsets;
} virtio_gpu_config;

enum virtio_gpu_ctrl_type {
    /* 2d commands */
    VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
    VIRTIO_GPU_CMD_RESOURCE_UNREF,
    VIRTIO_GPU_CMD_SET_SCANOUT,
    VIRTIO_GPU_CMD_RESOURCE_FLUSH,
    VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
    VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
    VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
    VIRTIO_GPU_CMD_GET_CAPSET_INFO,
    VIRTIO_GPU_CMD_GET_CAPSET,
    VIRTIO_GPU_CMD_GET_EDID,
    VIRTIO_GPU_CMD_RESOURCE_ASSIGN_UUID,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_BLOB,
    VIRTIO_GPU_CMD_SET_SCANOUT_BLOB,

    /* 3d commands */
    VIRTIO_GPU_CMD_CTX_CREATE = 0x0200,
    VIRTIO_GPU_CMD_CTX_DESTROY,
    VIRTIO_GPU_CMD_CTX_ATTACH_RESOURCE,
    VIRTIO_GPU_CMD_CTX_DETACH_RESOURCE,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_3D,
    VIRTIO_GPU_CMD_TRANSFER_TO_HOST_3D,
    VIRTIO_GPU_CMD_TRANSFER_FROM_HOST_3D,
    VIRTIO_GPU_CMD_SUBMIT_3D,
    VIRTIO_GPU_CMD_RESOURCE_MAP_BLOB,
    VIRTIO_GPU_CMD_RESOURCE_UNMAP_BLOB,

    /* cursor commands */
    VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300,
    VIRTIO_GPU_CMD_MOVE_CURSOR,

    /* success responses */
    VIRTIO_GPU_RESP_OK_NODATA = 0x1100,
    VIRTIO_GPU_RESP_OK_DISPLAY_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET,
    VIRTIO_GPU_RESP_OK_EDID,
    VIRTIO_GPU_RESP_OK_RESOURCE_UUID,
    VIRTIO_GPU_RESP_OK_MAP_INFO,

    /* error responses */
    VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200,
    VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY,
    VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER,
};

#define VIRTIO_GPU_FLAG_FENCE          ((1 << 0))
#define VIRTIO_GPU_FLAG_INFO_RING_IDX  ((1 << 1))

// This is the main GPU request structure
struct virtio_gpu_ctrl_hdr {
    u32 type;
    u32 flags;
    u64 fence_id;
    u32 ctx_id;
    u8 ring_idx;
    u8 padding[3];
};

#define VIRTIO_GPU_MAX_SCANOUTS 16

struct virtio_gpu_rect {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
};

struct virtio_gpu_resp_display_info {
    struct virtio_gpu_ctrl_hdr hdr;
    struct virtio_gpu_display_one {
            struct virtio_gpu_rect r;
            u32 enabled;
            u32 flags;
    } pmodes[VIRTIO_GPU_MAX_SCANOUTS];
};

enum virtio_gpu_formats {
    VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM  = 1,
    VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM  = 2,
    VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM  = 3,
    VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM  = 4,

    VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM  = 67,
    VIRTIO_GPU_FORMAT_X8B8G8R8_UNORM  = 68,

    VIRTIO_GPU_FORMAT_A8B8G8R8_UNORM  = 121,
    VIRTIO_GPU_FORMAT_R8G8B8X8_UNORM  = 134,
};

struct virtio_gpu_resource_create_2d {
    struct virtio_gpu_ctrl_hdr hdr;
    u32 resource_id;
    u32 format;
    u32 width;
    u32 height;
};

struct virtio_gpu_mem_entry {
    u64 addr;
    u32 length;
    u32 padding;
};

struct virtio_gpu_resource_attach_backing {
    struct virtio_gpu_ctrl_hdr hdr;
    u32 resource_id;
    u32 nr_entries;
    // Can be any number of entries, for now I am just putting 1
    struct virtio_gpu_mem_entry entries[1];
};

struct virtio_gpu_set_scanout {
    struct virtio_gpu_ctrl_hdr hdr;
    struct virtio_gpu_rect r;
    u32 scanout_id;
    u32 resource_id;
};

struct virtio_gpu_transfer_to_host_2d {
    struct virtio_gpu_ctrl_hdr hdr;
    struct virtio_gpu_rect r;
    u64 offset;
    u32 resource_id;
    u32 padding;
};

struct virtio_gpu_resource_flush {
    struct virtio_gpu_ctrl_hdr hdr;
    struct virtio_gpu_rect r;
    u32 resource_id;
    u32 padding;
};

struct virtio_gpu_cursor_pos {
    u32 scanout_id;
    u32 x;
    u32 y;
    u32 padding;
};

struct virtio_gpu_update_cursor {
    struct virtio_gpu_ctrl_hdr hdr;
    struct virtio_gpu_cursor_pos pos;
    u32 resource_id;
    u32 hot_x;
    u32 hot_y;
    u32 padding;
};

END_C_HEADER

#endif // VIRTIO_GPU_DEFS_H
