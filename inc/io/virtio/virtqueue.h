#ifndef VIRTIO_VIRTQUEUE_H
#define VIRTIO_VIRTQUEUE_H

#include <fractal.h>

BEGIN_C_HEADER

#define VIRTQUEUE_NUM_DESCRIPTORS 8

#define VIRTQ_USED_F_NO_NOTIFY          1
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1

#define VIRTIO_F_EVENT_IDX 29

typedef struct {
    u64 addr;
    u32 len;

// Flags for the VirtqueueDescriptor.flags field:
#define VIRTQ_DESC_F_NEXT      1

// If this is set, the buffer is device write-only
// Otherwise, it is device read-only:
#define VIRTQ_DESC_F_WRITE     2
#define VIRTQ_DESC_F_READ      0

#define VIRTQ_DESC_F_INDIRECT  4

    u16 flags;
    u16 next;
} virtq_desc;

typedef struct {
    u16 flags;
    volatile u16 idx;
    u16 ring[VIRTQUEUE_NUM_DESCRIPTORS];
} virtq_avail;

typedef struct {
    u32 id;
    u32 len;
} virtq_used_elem;

typedef struct {
    u16 flags;
    u16 idx;
    virtq_used_elem ring[VIRTQUEUE_NUM_DESCRIPTORS];
} virtq_used;

class Virtqueue {
public:
    volatile virtq_desc *desc;
    volatile virtq_avail *avail;
    volatile virtq_used *used;

    // Write to this address to send notifications:
    volatile u64 *notify;

    Virtqueue(volatile u64 *notify_addr);
    ~Virtqueue();
};

#define VIRTIO_VQ_SEND_TIMEOUT 100000000

END_C_HEADER

#endif // VIRTIO_VIRTQUEUE_H
