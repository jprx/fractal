#include <fractal.h>
#include <io/virtio.h>
#include <io/virtio/virtqueue.h>
#include <page_alloc.h>
#include <lib/dlmalloc.h>
#include <io/virtio/virtio-pci.h>

// #define VIRTIO_VQ_DEBUG_PACKETS 1

kret_t virtqueue_send_packet(volatile Virtqueue *vq, void *src, usize src_len, void *dst, usize dst_len) {
    if (!vq) return THING_DOESNT_EXIST;

    static usize packet_index = 0;
#ifdef VIRTIO_VQ_DEBUG_PACKETS
    printf("==== PROCESSING PACKET %d ====\r\n", packet_index++);
#endif // VIRTIO_VQ_DEBUG_PACKETS
    memset(dst, '\x00', dst_len);

    // Our request (chained with next descriptor):
    usize head_base = (vq->avail->idx) % VIRTQUEUE_NUM_DESCRIPTORS;
    usize head_next = head_base + 1;
    if (head_base == VIRTQUEUE_NUM_DESCRIPTORS - 1) {
#ifdef VIRTIO_VQ_DEBUG_PACKETS
        printf("Wrapping: %d -> %d\r\n", head_next, 0);
#endif // VIRTIO_VQ_DEBUG_PACKETS
        head_next = 0;
    }
#ifdef VIRTIO_VQ_DEBUG_PACKETS
    printf("Head is %d, Head Next is %d\r\n", head_base, head_next);
    printf("avail.idx is %d\r\n", vq->avail->idx);
#endif // VIRTIO_VQ_DEBUG_PACKETS
    vq->desc[head_base].addr = KERN_V2P((usize)src);
    vq->desc[head_base].flags = VIRTQ_DESC_F_READ | VIRTQ_DESC_F_NEXT;
    vq->desc[head_base].len = src_len;
    vq->desc[head_base].next = head_next;

    // The dev's response:
    vq->desc[head_next].addr = KERN_V2P((usize)dst);
    vq->desc[head_next].flags = VIRTQ_DESC_F_WRITE;
    vq->desc[head_next].len = dst_len;
    vq->desc[head_next].next = 0;

    u16 orig_used_idx = vq->used->idx;

    // Fill in available ring:
    vq->avail->ring[head_base] = head_base;
#ifdef VIRTIO_VQ_DEBUG_PACKETS
    printf("Prev idx: %d\r\n", vq->avail->idx);
#endif // VIRTIO_VQ_DEBUG_PACKETS
    vq->avail->idx++;
#ifdef VIRTIO_VQ_DEBUG_PACKETS
    printf("Next idx: %d\r\n", vq->avail->idx);
#endif // VIRTIO_VQ_DEBUG_PACKETS

    // Notify (I believe you are supposed to write which virtqueue this is into this address)
    // But it seems that the device in Qemu 9.0.0 does not care.
    __sync_synchronize();
    *(vq->notify) = 1;
    __sync_synchronize();

#ifdef VIRTIO_VQ_DEBUG_PACKETS
    printf("Request issued, waiting for Virtqueue...\r\n");
#endif // VIRTIO_VQ_DEBUG_PACKETS

    usize timeout_counter = 0;
    while (vq->used->idx == orig_used_idx) {
        if (timeout_counter++ > VIRTIO_VQ_SEND_TIMEOUT) {
            return TIMEOUT;
        }
        __sync_synchronize();
    }

#ifdef VIRTIO_VQ_DEBUG_PACKETS
    printf("Packet Xfer OK (%d retries)\r\n", timeout_counter);
#endif // VIRTIO_VQ_DEBUG_PACKETS
    __sync_synchronize();
    return ALL_GOOD;
}

Virtqueue::Virtqueue(volatile u64 *notif_in) {
    desc  = (virtq_desc *)AllocPage(SMALL_PAGE);
    avail = (virtq_avail *)AllocPage(SMALL_PAGE);
    used  = (virtq_used *)AllocPage(SMALL_PAGE);
    notify = (volatile u64 *)notif_in;

    if (!desc || !avail || !used) {
        panic("Failed to allocate pages for a new Virtqueue");
    }

    memset((void *)desc, 0, SMALL_PAGE_SIZE);
    memset((void *)avail, 0, SMALL_PAGE_SIZE);
    memset((void *)used, 0, SMALL_PAGE_SIZE);
}

Virtqueue::~Virtqueue() {
    printf("Freeing Virtqueue (0x%X, 0x%X, 0x%X)\r\n", desc, avail, used);
    FreePage((virt_t)desc);
    FreePage((virt_t)avail);
    FreePage((virt_t)used);
}

void enable_virtqueue(
    volatile virtio_pci_common_cfg *h,
    usize queue_idx,
    volatile Virtqueue *vq
) {
    vq->used->flags = 0;
    h->queue_select = queue_idx;
    h->queue_desc = KERN_V2P((usize)vq->desc);
    h->queue_size = VIRTQUEUE_NUM_DESCRIPTORS;
    h->queue_driver = KERN_V2P((usize)vq->avail);
    h->queue_device = KERN_V2P((usize)vq->used);
    h->queue_enable = 1;
}
