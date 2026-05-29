#include <fractal.h>
#include <oxide.h>
#include <io/pci.h>
#include <io/virtio.h>
#include <io/pci/msi.h>
#include <io/pci/bar.h>
#include <io/gpu/virtio-gpu.h>
#include <io/hid/virtio-hid.h>
#include <io/virtio/virtio-pci.h>
#include <cpu.h>
#include <arch.h>
#include <out.h>

// #define DEBUG_VIRTIO_PCI_INIT 1

extern FRACTALCORE_CLASS global_cpu;

extern FractalGPU *global_gpu;
extern VirtioHID *global_keyboard;
extern VirtioHID *global_mouse;
extern VirtioHID *global_tablet;

// Global allocation space for BARs
// The start address is whatever Qemu sets VIRT_PCIE_MMIO to
// For X86_64, the chipset/ BIOS handles this, so we can leave it be
static usize _pci_mmio_start = PLATFORM_PCI_MMIO_BASE;
static usize _pci_mmio_bytes_used = 0;

PCIDevice *pci_devices[MAX_PCI_DEVICES];

/*
 * PCI Device Initialization
 * 1. Find the PCI memory-mapped expanded configuration space.
 *    This is a space where each device gets 0x1000 bytes to be configured.
 *    The top of each per-device page is its PCIeHeader (see io/pci.h).
 *    This region can be discovered via ACPI, device tree (DTB), or hard-coded.
 *
 * 2. Find your PCI device by scanning all devices on all busses.
 *    Each device has 8 functions, where each function gets its own config 0x1000 region.
 *    You treat each 0x1000 region within the expanded configuration space as a possible "device".
 *    Check the header of each device for what you are scanning for.
 *    If you find one with your VENDOR_ID:DEVICE_ID pair, you've got your device.
 *    You can construct the header from the configuration MMIO base with this formula:
 *         config_base + (bus << 20) | (device << 15) | (func << 12)
 *
 * 3. Learn what capabilities this device has.
 *    The header->CapabilitiesPtr field points to an offset FROM THE HEADER in the config space
 *    that is a linked list of capabilities. These capabilities are vendor-specific. Your driver
 *    should parse this list of capabilities to learn which base address registers are used for what.
 *
 * 4. Configure the base address registers.
 *    Each device has 6 base address registers (set within its header). These point to physical
 *    memory locations at which device configuration things can be read/ written. For some architectures
 *    you need to initialize these yourself to point somewhere within the PCI MMIO region (separate
 *    from the extended configuration space).
 *
 * 5. Interact with the device.
 *    After detecting capabilities + setting up BARs for each capability, you can now interact with the device
 *    using MMIO at each base address register's physical location.
 *
 * 6. Enable the bus master bit (Don't forget this step!)
 *    Things will not work unless the header configuration register is set correctly.
 *    I remember I lost a ton of time debugging because I didn't actually "turn on" PCI. Gotta flip that bit on.
 */

/**
 * Locates a specific PCI device geographically.
 *
 * That is, based on where it's "plugged in."
 *
 * # Arguments
 *
 * * `config_base` - The physical base address of the PCI Extended Configuration Region to scan.
 * * `bus` - The bus to scan on.
 * * `device` - The device ID to scan for.
 * * `func`- The desired device function to scan for.
 */
PCIHeader_t FindPCIGeo(virt_t config_base, usize bus, usize device, usize func) {
    // Offset within the MMIO Extended Configuration Space of this device's header:
    usize offset = (bus << 20) | (device << 15) | (func << 12);
    PCIeHeader *header = (PCIeHeader *)(KERN_P2V(config_base + offset));

    if (header->VendorID == PCI_VENDOR_NONE) {
        return NULL;
    }

    return header;
}

/**
 * Prints out every detected PCI device.
 *
 * # Arguments
 *
 * * `config_region_base` - The physical base address of the PCI Extended Configuration Region to scan.
 */
void ListPCIDevices(virt_t config_base) {
    printf("Scanning PCI Devices at 0x%X...\r\n", config_base);
    for (usize bus = 0; bus < PCI_NUM_BUSSES; bus++) {
        for (usize device = 0; device < PCI_NUM_DEVICES; device++) {
            usize func = 0;
            PCIHeader_t header = FindPCIGeo(config_base, bus, device, func);
            if (!header) continue;
            printf("Detected PCI Device 0x%X:0x%X at Bus %d, Device %d\r\n", header->VendorID, header->DeviceID, bus, device);
        }
    }
}

/**
 * Initialize every PCI device we know about.
 *
 * # Arguments
 *
 * * `config_region_base` - The physical base address of the PCI Extended Configuration Region to scan.
 */
void InitPCI(virt_t config_base) {
    usize cur_pci_dev_idx = 0;
    for (usize i = 0; i < MAX_PCI_DEVICES; i++) {
        pci_devices[i] = NULL;
    }

#ifndef QUIET_BOOT
    printf("Scanning PCI Devices at 0x%X...\r\n", config_base);
#endif // !QUIET_BOOT
    for (usize bus = 0; bus < PCI_NUM_BUSSES; bus++) {
        for (usize device = 0; device < PCI_NUM_DEVICES; device++) {
            usize func = 0;
            PCIHeader_t header = FindPCIGeo(config_base, bus, device, func);
            if (!header) continue;
            PCIDevice *new_dev = NULL;

            switch(header->VendorID) {
                case VIRTIO_PCI_VENDOR:
                switch(header->DeviceID) {
                    case VIRTIO_PCI_DEV_GPU:
                    new_dev = InitVirtioGPU(header, bus, device);
                    break;

                    case VIRTIO_PCI_DEV_INPUT:
                    new_dev = InitVirtioHID(header, bus, device);
                    break;
                }

                if (new_dev) {
                    new_dev->is_virtio = true;
                }
                break;
            }

            if (NULL != new_dev) {
                new_dev->bus_id = bus;
                new_dev->device_id = device;
                pci_devices[cur_pci_dev_idx++] = new_dev;
            }

            if (cur_pci_dev_idx >= MAX_PCI_DEVICES) {
                printf("Maximum number of PCI devices exceeded\r\n");
                return;
            }
        }
    }

    printf("==== Allocated PCI Devices ====\r\n");
    for (usize i = 0; i < MAX_PCI_DEVICES; i++) {
        if (pci_devices[i] != NULL) {
            printf("\t%d: %s\r\n",
                i,
                pci_devices[i]->pci_name
            );

            if (strequal(pci_devices[i]->pci_name, VIRTIO_GPU_DEVICE_NAME)) {
                if (NULL != global_gpu) panic("unsupported configuration: Multiple GPUs detected");
                global_gpu = new VirtioGPUDevice((VirtioGPU *)pci_devices[i]);
            }

            if (strequal(pci_devices[i]->pci_name, VIRTIO_KEYBOARD_DEVICE_NAME)) {
                global_keyboard = (VirtioHID *)pci_devices[i];
            }

            if (strequal(pci_devices[i]->pci_name, VIRTIO_MOUSE_DEVICE_NAME)) {
                global_mouse = (VirtioHID *)pci_devices[i];
            }

            if (strequal(pci_devices[i]->pci_name, VIRTIO_TABLET_DEVICE_NAME)) {
                global_tablet = (VirtioHID *)pci_devices[i];
            }

            if (!pci_devices[i]->is_virtio) {
                panic("Found a non-virtio PCI device in my device list. How did that get there?");
            }
        }
    }
}

/**
 * Locates a specific PCI device by its Vendor:Device ID pair.
 *
 * Always returns the first function (function = 0) and first device found.
 *
 * # Arguments
 *
 * * `config_region_base` - The physical base address of the PCI Extended Configuration Region to scan.
 * * `vendorid` - The vendor ID to look for.
 * * `devid` - The device ID to look for.
 */
PCIHeader_t FindPCI(virt_t config_base, u16 vendor_id, u16 dev_id) {
    for (usize bus = 0; bus < PCI_NUM_BUSSES; bus++) {
        for (usize device = 0; device < PCI_NUM_DEVICES; device++) {
            PCIHeader_t h = FindPCIGeo(config_base, bus, device, 0);
            if (!h) continue;

            if (h->VendorID == vendor_id && h->DeviceID == dev_id) {
                return h;
            }
        }
    }
    return NULL;
}

void DumpPCIHeader(volatile PCIeHeader *h) {
    if (!h) { printf("NULL PCIe Header\r\n"); }
    printf("\tVendor: 0x%X\r\n\tDevice: 0x%X\r\n", h->VendorID, h->DeviceID);
    printf("\tCommand: 0x%X\r\n\tStatus: 0x%X\r\n", h->Command, h->Status);
    for (usize i = 0; i < PCI_NUM_BARS; i++) {
        printf("\tBAR%d: 0x%X\r\n", i, h->bar[i]);
    }
}

void dump_virtio_capability(VirtIOPCICapability *cap) {
    if(!cap) printf("NULL VirtIO PCI Capability\r\n");
    switch (cap->header.cap_vndr) {
        case PCI_CAP_VENDOR_SPECIFIC:
        printf("Vendor Capability %s at BAR%d, Offset 0x%X, Length 0x%X\r\n", get_virtio_cap_name((VirtIOPCICapabilityType)cap->cfg_type), cap->bar, cap->offset, cap->length);
        break;
    }
}

void dump_msi_x_capability(MSI_XCapability *cap) {
    if(!cap) printf("NULL MSI-X Capability\r\n");
    printf("MSI-X Capability with %d vectors at BAR%d\r\n", cap->num_entries(), cap->table_bar());
}

/*
 * alloc_pci_bar_mem
 * Allocate space in the PCI MMIO region for this BAR.
 */
void alloc_pci_bar_mem(PCIHeader_t header, usize bar_idx) {
    if (NULL == header) return;
    if (bar_idx > PCI_NUM_BARS) return;

    volatile bar_t *bar = &header->bar[bar_idx];
    if (bar_get_addr(bar) != 0) {
        // This is ok, probably just multiple capabilities sharing a BAR
#ifdef ALLOC_PCI_BAR_MEM_DEBUG
        printf("Tried to allocate memory for a BAR that is already allocated: 0x%X\r\n", *bar);
#endif // ALLOC_PCI_BAR_MEM_DEBUG
        return;
    }

    usize bar_size = bar_get_size(bar);
#ifdef ALLOC_PCI_BAR_MEM_DEBUG
    printf("Allocating BAR %d bytes for BAR%d\r\n", bar_size, bar_idx);
#endif // ALLOC_PCI_BAR_MEM_DEBUG

    if (_pci_mmio_bytes_used + bar_size > PLATFORM_PCI_MMIO_SIZE) {
        panic("Out of memory allocating BARs (used %d bytes of %d)\r\n", _pci_mmio_bytes_used + bar_size, PLATFORM_PCI_MMIO_SIZE);
    }

    bar_set_base32(bar, _pci_mmio_start + _pci_mmio_bytes_used);
    _pci_mmio_bytes_used += bar_size;

    return;
}

template <typename T>
T *InitVirtioPCI(PCIHeader_t dev, const char *dev_name) {
    // We use these detected PCI capabilities:
    VirtIOPCICapability *common_cfg, *notify_cfg, *device_cfg, *isr_cfg;

#ifdef VIRTIO_USE_MSI_X
    MSI_XCapability *msi_x;
#endif // VIRTIO_USE_MSI_X

    // To find the addresses of these structures in the PCI MMIO space:
    virtio_pci_common_cfg *config_hdr;
    u64 *notify_addr;
    void *device_conf_addr;
    void *isr_conf_addr;
    // @TODO: MSI-X table for MSI-X interrupts (but these are X86_64 only?)

    common_cfg = NULL;
    notify_cfg = NULL;
    device_cfg = NULL;
    isr_cfg = NULL;

#ifdef VIRTIO_USE_MSI_X
    msi_x = NULL;
#endif // VIRTIO_USE_MSI_X

#ifdef DEBUG_VIRTIO_PCI_INIT
    DumpPCIHeader(dev);
#endif // DEBUG_VIRTIO_PCI_INIT

#ifdef DEBUG_VIRTIO_PCI_INIT
    printf("Initializing %s command register\r\n", dev_name);
#endif // DEBUG_VIRTIO_PCI_INIT

    dev->Command |= PCI_CONFIG_BUS_MASTER_ENABLED;
    dev->Command |= PCI_CONFIG_MMIO_ENABLED;
    dev->Command |= PCI_CONFIG_BUS_MASTER_ENABLED;

#ifdef DEBUG_VIRTIO_PCI_INIT
    printf("Scanning PCI Capabilities List\r\n");
#endif // DEBUG_VIRTIO_PCI_INIT

    // Want to find the common configuration capability, notification capability, and MSI-X capability
    for (PCICapabilityIterator it = PCICapabilityIterator(dev); *it != NULL; ++it) {
        PCICapability *cap = *it;

        if (PCI_CAP_VENDOR_SPECIFIC == cap->cap_vndr) {
            VirtIOPCICapability *virt_cap = (VirtIOPCICapability *)cap;
#ifdef DEBUG_VIRTIO_PCI_INIT
            dump_virtio_capability(virt_cap);
#endif // DEBUG_VIRTIO_PCI_INIT

            switch(virt_cap->cfg_type) {
                case VIRTIO_PCI_CAP_COMMON_CFG:
                common_cfg = virt_cap;
                break;

                case VIRTIO_PCI_CAP_NOTIFY_CFG:
                notify_cfg = virt_cap;
                break;

                case VIRTIO_PCI_CAP_DEVICE_CFG:
                device_cfg = virt_cap;
                break;

                case VIRTIO_PCI_CAP_ISR_CFG:
                isr_cfg = virt_cap;
                break;
            }
        }

#ifdef VIRTIO_USE_MSI_X
        if (PCI_CAP_MSI_X == cap->cap_vndr) {
            msi_x = (MSI_XCapability *)cap;
            dump_msi_x_capability(msi_x);
        }
#endif // VIRTIO_USE_MSI_X
    }

    if (!common_cfg || !notify_cfg || !device_cfg || !isr_cfg) {
        printf("%s initialization failed to discover all required capabilities.\r\n", dev_name);
        return NULL;
    }

#ifdef VIRTIO_USE_MSI_X
    if (!msi_x) {
        printf("WARNING: No MSI-X table for this device. Continuing but things may go wrong\r\n");
    }
#endif // VIRTIO_USE_MSI_X

#ifdef DEBUG_VIRTIO_PCI_INIT
    printf("Common Configuration BAR: BAR%d\r\n", common_cfg->bar);
    printf("Notification BAR: BAR%d\r\n", notify_cfg->bar);
    printf("Device Configuration BAR: BAR%d\r\n", device_cfg->bar);
#endif // DEBUG_VIRTIO_PCI_INIT

    alloc_pci_bar_mem(dev, common_cfg->bar);
    alloc_pci_bar_mem(dev, notify_cfg->bar);
    alloc_pci_bar_mem(dev, device_cfg->bar);
    alloc_pci_bar_mem(dev, isr_cfg->bar);

    // Figure out interrupts
#ifdef DEBUG_VIRTIO_PCI_INIT
    printf("PCI Header: 0x%X\r\n", dev);
    printf("\t==> InterruptLine: %d\r\n\t==> InterruptPin: %d\r\n", dev->InterruptLine, dev->InterruptPin);
#endif // DEBUG_VIRTIO_PCI_INIT
    dev->Command &= ~(PCI_HEADER_COMMAND_INTERRUPTS_ENABLE_ACTIVE_LOW);

#ifdef VIRTIO_USE_MSI_X
    alloc_pci_bar_mem(dev, msi_x->table_bar());
#endif // VIRTIO_USE_MSI_X

#ifdef DEBUG_VIRTIO_PCI_INIT
    DumpPCIHeader(dev);
#endif // DEBUG_VIRTIO_PCI_INIT

    // These are physical addresses
    config_hdr = (virtio_pci_common_cfg *)(bar_get_addr(&dev->bar[common_cfg->bar]) + common_cfg->offset);
    notify_addr = (u64 *)(bar_get_addr(&dev->bar[notify_cfg->bar]) + notify_cfg->offset);
    device_conf_addr = (void *)(bar_get_addr(&dev->bar[device_cfg->bar]) + device_cfg->offset);
    isr_conf_addr = (void *)(bar_get_addr(&dev->bar[isr_cfg->bar]) + isr_cfg->offset);

    T *newdev = new T(
        (volatile virtio_pci_common_cfg *)(KERN_P2V((usize)config_hdr)),
        (volatile u64*)(KERN_P2V((usize)notify_addr)),
        (void *)(KERN_P2V((usize)device_conf_addr)),
        (void *)(KERN_P2V((usize)isr_conf_addr)),
        dev
    );

    if (!newdev) {
        printf("Failed to allocate a new %s\r\n", dev_name);
        return NULL;
    }

#ifdef DEBUG_VIRTIO_PCI_INIT
    printf("New %s allocated successfully\r\n", dev_name);
#endif // DEBUG_VIRTIO_PCI_INIT

    return newdev;
}

PCIDevice *InitVirtioHID(PCIHeader_t dev, usize bus, usize device) {
#ifndef QUIET_BOOT
    printf("Detected Virtio Input Device at bus %d, device %d\r\n", bus, device);
#endif // !QUIET_BOOT
    VirtioHID *hid_dev = InitVirtioPCI<VirtioHID>(dev, "VirtioHID");
    if (!hid_dev) return NULL;

    strncpy(hid_dev->pci_name, "VirtioHID", PCI_DEVICE_NAMELEN);

    hid_dev->Negotiate();
    hid_dev->Setup();
    hid_dev->DetermineKind();

    return hid_dev;
}

PCIDevice *InitVirtioGPU(PCIHeader_t dev, usize bus, usize device) {
#ifndef QUIET_BOOT
    printf("Detected Virtio GPU at bus %d, device %d\r\n", bus, device);
#endif // !QUIET_BOOT
    VirtioGPU *vgpu = InitVirtioPCI<VirtioGPU>(dev, "VirtioGPU");
    if(!vgpu) return NULL;

    strncpy(vgpu->pci_name, "VirtioGPU", PCI_DEVICE_NAMELEN);

    if (ALL_GOOD != vgpu->Negotiate()) {
        delete vgpu;
        printf("Failed to negotiate with GPU\r\n");
        return NULL;
    }

    if (ALL_GOOD != vgpu->Setup()) {
        delete vgpu;
        printf("Failed to setup GPU\r\n");
        return NULL;
    }

    // Previously used to setup framebuffer,
    // but we can just leave it blank for now because
    // the compositor will take over and play an animation
    // momentarily

    if (ALL_GOOD != vgpu->Repaint()) {
        delete vgpu;
        printf("Failed to paint bootscreen\r\n");
        return NULL;
    }

    return vgpu;
}

extern "C" void handle_pci_interrupt() {
    for (usize i = 0; i < MAX_PCI_DEVICES; i++) {
        if (NULL != pci_devices[i]) {
            pci_devices[i]->InterruptReceived();

            // Really make sure all ISRs are serviced for virtio devices
            // (on arm64 this causes interrupt 37 to "get stuck" unless we do this)
            if (pci_devices[i]->is_virtio) {
                *(volatile u64 *)((VirtioPCI *)pci_devices[i])->isr_config;
            } else {
                panic("Found a non-virtio PCI device while servicing PCI interrupts. Unsupported");
            }
        }
    }
}

kret_t PCIDevice::InterruptReceived() {
    if (!pci_header) return THING_DOESNT_EXIST;

    if (pci_header->Status & PCI_STATUS_INTERRUPT_PENDING) {
        this->HandleInterrupt();
    }
    return ALL_GOOD;
}

kret_t PCIDevice::HandleInterrupt() {
    return ALL_GOOD;
}
