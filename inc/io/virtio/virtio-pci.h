#ifndef VIRTIO_PCI_H
#define VIRTIO_PCI_H

#include <fractal.h>
#include <io/pci.h>

#define VIRTIO_PCI_VENDOR                0x1AF4
#define VIRTIO_PCI_DEV_GPU               0x1050
#define VIRTIO_PCI_DEV_INPUT             0x1052

// From VirtIO Specification Section 4.1.4: VirtIO Structure PCI Capabilities
// The header->CapabilitiesPtr pointer points to the first one of these.
typedef struct __attribute__((packed)) {
    PCICapability header;

    // Length and type of this capability
    u8 cap_len; // Unused by virtio- they are all the same length
    u8 cfg_type;

    // Which BAR (0 through 5) to find this capability?
    u8 bar;
    u8 id;
    u8 padding[2];

    // Offset inside the given BAR
    u32 offset;

    // Length within the BAR (in bytes)
    u32 length;
} VirtIOPCICapability;

typedef enum {
    VIRTIO_PCI_CAP_COMMON_CFG = 1,
    VIRTIO_PCI_CAP_NOTIFY_CFG = 2,
    VIRTIO_PCI_CAP_ISR_CFG = 3,
    VIRTIO_PCI_CAP_DEVICE_CFG = 4,
    VIRTIO_PCI_CAP_PCI_CFG = 5, // Do not use this, it is deprecated and slow apparently
} VirtIOPCICapabilityType;

#define CAPABILITY_TO_NAME(v) case v: return #v

static inline const char *get_virtio_cap_name(VirtIOPCICapabilityType t) {
    switch(t) {
        CAPABILITY_TO_NAME(VIRTIO_PCI_CAP_COMMON_CFG);
        CAPABILITY_TO_NAME(VIRTIO_PCI_CAP_NOTIFY_CFG);
        CAPABILITY_TO_NAME(VIRTIO_PCI_CAP_ISR_CFG);
        CAPABILITY_TO_NAME(VIRTIO_PCI_CAP_DEVICE_CFG);
        CAPABILITY_TO_NAME(VIRTIO_PCI_CAP_PCI_CFG);
        default: return "[Unknown Capability]";
    }
}

#undef CAPABILITY_TO_NAME

typedef struct {
    /* About the whole device */
    volatile u32 device_feature_select;
    volatile u32 device_feature;
    volatile u32 driver_feature_select;
    volatile u32 driver_feature;
    volatile u16 config_msix_vector;
    volatile u16 num_queues;
    volatile u8 device_status;
    volatile u8 config_generation;

    /* About a specific virtqueue. */
    volatile u16 queue_select;
    volatile u16 queue_size; // In number of descriptor table entries (not bytes!)
    volatile u16 queue_msix_vector;
    volatile u16 queue_enable;
    volatile u16 queue_notify_off;

    // Physical address of virtqueue descriptor area/ descriptor table
    volatile u64 queue_desc;

    // Physical address of virtqueue available ring/ driver space
    volatile u64 queue_driver;

    // Physical address of virtqueue used ring/ device space
    volatile u64 queue_device;
    volatile u16 queue_notify_data;
    volatile u16 queue_reset;
} virtio_pci_common_cfg;

/*
 * VirtioPCI
 * Superclass that all Virtio devices using the PCI bus should inherit from.
 */
class VirtioPCI : public PCIDevice {
public:
    // Points to the PCI common configuration structure for this device
    // Pointed to by VIRTIO_PCI_CAP_COMMON_CFG
    volatile virtio_pci_common_cfg *header;

    // Points to the notification port address for this device
    // Pointed to by VIRTIO_PCI_CAP_NOTIFY_CFG
    volatile u64 *notify;

    // The device configuration region (within PCIe MMIO)
    // Pointed to by VIRTIO_PCI_CAP_DEVICE_CFG
    void *device_config;

    // The device ISR region (within PCIe MMIO)
    // Pointed to by VIRTIO_PCI_CAP_ISR_CFG
    void *isr_config;

    // Status flags must be saved between ResetBegin() and ResetEnd()
    u8 status_flags;

    /*
     * ResetBegin
     * Reset this device (via device_status field of common pci cfg).
     * This performs steps 1-6 of the Virtio 3.1.1 Driver Requirements:
     * Device Initialization procedure.
     *
     * Your device should call this, setup its virtqueues, then call
     * ResetEnd() to complete device initialization.
     */
    kret_t ResetBegin();

    /*
     * ResetEnd
     * After setting up all virtqueues (step 7, the device-specific
     * configuration step), this performs step 8 of the device
     * initialization procedure.
     *
     * After this, no changes to the configuration state can be made
     * without reseting the device again.
     */
    kret_t ResetEnd();

    virtual kret_t Negotiate() = 0;
    virtual kret_t Setup() = 0;

    /*
     * VirtioHID
     * Creates a VirtioPCI using a common PCI configuration header,
     * discovered by enumerating the PCI bus.
     *
     * This will allocate all required virtqueues but
     * not initialize the device just yet.
     */
    VirtioPCI(volatile virtio_pci_common_cfg *header_in, volatile u64 *notify_in, void *dev_conf_in, void *isr_conf_in, volatile PCIeHeader *pci_header_in) : header(header_in), notify(notify_in), device_config(dev_conf_in), isr_config(isr_conf_in), status_flags(0) {}

    /*
     * ~VirtioPCI
     * Cleans up all allocated virtqueues and frees this device.
     */
    ~VirtioPCI();
};

/**
 * InitVirtioPCI
 * Given a PCIHeader_t (pointer to a PCIeHeader in the extended configuration space),
 * initialize it as a VirtIO device by:
 *   1. Discovering all capabilities (common, notify, and device), as well as MSI-X table
 *   2. Calling the constructor for T, a class that must inherit from VirtioPCI
 *
 * T: This must be a class that extends VirtioPCI.
 * dev: This must be a valid PCIe Header.
 * dev_name: The human-readable string name of the device to initialize.
 *
 * This allows us to initialize multiple devices of the same kind, storing
 * per-device configuration information in its VitioPCI object.
 */
template <typename T>
T *InitVirtioPCI(PCIHeader_t dev, const char *dev_name);

/*
 * enable_virtqueue
 * Attach a Virtqueue to this PCI configuration header
 * at virtqueue index `queue_idx`.
 */
void enable_virtqueue(
    volatile virtio_pci_common_cfg *h,
    usize queue_idx,
    volatile Virtqueue *vq
);

/*
 * Sends a virtqueue message
 */
kret_t virtqueue_send_packet (volatile Virtqueue *vq, void *src, usize src_len, void *dst, usize dst_len);

#endif // VIRTIO_PCI_H

