#include <fractal.h>
#include <io/virtio.h>
#include <io/virtio/virtqueue.h>
#include <io/virtio/virtio-pci.h>

kret_t VirtioPCI::ResetBegin() {
    u64 dev_features_low, dev_features_high;
    status_flags = 0;
    dev_features_low = 0;
    dev_features_high = 0;

    // See 3.1.1 Driver Requirements: Device Initialization
    // 1. Reset the device:
    header->device_status = 0;
    __sync_synchronize();

    // 2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device.
    status_flags |= VIRTIO_DEVICE_STATUS_ACK;
    __sync_synchronize();
    header->device_status = status_flags;
    __sync_synchronize();

    // 3.Set the DRIVER status bit: the guest OS knows how to drive the device.
    status_flags |= VIRTIO_DEVICE_STATUS_DRIVER;
    __sync_synchronize();
    header->device_status = status_flags;
    __sync_synchronize();

    // 4. Read feature bits and tell device what we support.
    __sync_synchronize();
    header->device_feature_select = 0;
    __sync_synchronize();
    header->driver_feature_select = 1;
    __sync_synchronize();
    // Let's say we support them all except for VIRTIO_F_EVENT_IDX
    dev_features_low = *((volatile u32 *)(&header->device_feature));
    __sync_synchronize();

    dev_features_low &= ~(1ull << VIRTIO_F_EVENT_IDX);

#ifdef VIRTIO_PCI_DEBUG
    printf("VirtioPCI device supports bits 31 -> 0:  0x%X\r\n", dev_features_low);
#endif // VIRTIO_PCI_DEBUG

    __sync_synchronize();
    header->device_feature_select = 1;
    __sync_synchronize();
    header->driver_feature_select = 1;
    __sync_synchronize();
    dev_features_high = 0;
    dev_features_high = *((volatile u32 *)(&header->device_feature));
    __sync_synchronize();

#ifdef VIRTIO_PCI_DEBUG
    printf("VirtioPCI device supports bits 64 -> 32: 0x%X\r\n", dev_features_high);
#endif // VIRTIO_PCI_DEBUG

    header->driver_feature = dev_features_high;
    __sync_synchronize();

    // 5. Set the FEATURES_OK status bit
    status_flags |= VIRTIO_DEVICE_STATUS_FEATURES_OK;
    header->device_status = status_flags;

    // 6. Re-read device status to ensure the FEATURES_OK bit is still set
    u8 new_status = header->device_status;
    if (new_status != status_flags) {
        printf("Uh oh- GPU didn't like our feature request\r\n");
        return OH_NO;
    }

    return ALL_GOOD;
}

kret_t VirtioPCI::ResetEnd() {
    // 8. Set the DRIVER_OK status bit.
    status_flags |= VIRTIO_DEVICE_STATUS_DRIVER_OK;
    header->device_status = status_flags;

    u8 final_status = header->device_status;
    if ((final_status & VIRTIO_DEVICE_STATUS_DRIVER_OK) == 0) {
        printf("Failed to initialize GPU\r\n");
        return OH_NO;
    }

    return ALL_GOOD;
}

VirtioPCI::~VirtioPCI() {}
