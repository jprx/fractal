#ifndef PCI_H
#define PCI_H

#include <fractal.h>

BEGIN_C_HEADER

/*
 * Methods for interacting with PCI devices on any architecture.
 * See: <https://wiki.osdev.org/PCI>
 *
 * And: <https://wiki.osdev.org/PCI_Express>
 */

/*
On x86_64/ ACPI compliant systems:
Multiboot 2 ACPIv1 (tag type 14): Locate RSDP

ACPI
+------+      +------+
| RSDP | ---> | RSDT |
+------+      +------+
              |&MCFG | ---> +------+
              +------+      | MCFG |
                            +------+      +-----------------+
                            | dev0 | ---> | Extended Config |
                            +------+      |    MMIO Space   |
                                          +-----------------+

On non-ACPI systems:
We just hardcode the MMIO space address per-board. In the
future we should try and parse the device tree DTBs though,
assuming whatever system we are running even has a DTB.

The extended config MMIO space is laid out as follows:

Device 0:
+------------------+
|    PCIe Header   |
+------------------+
| 4K Configuration |
|   Address Space  |
+------------------+
        ...

Device 1:
+------------------+
|    PCIe Header   |
+------------------+
| 4K Configuration |
|   Address Space  |
+------------------+
        ...

Each device's 4KiB configuration address space is
laid out consecutively in memory, separated by empty
space (all 0xFFFFFFFF).

A given device's offset within the global extended
MMIO configuration space is calculated as follows:

Offset(Bus, Device, Function) = (Bus << 20) | (Device << 15) | (Function << 12)

(See: https://en.wikipedia.org/wiki/PCI_configuration_space)
(See: OSDev Wiki)

(This was tested in Qemu using -serial mon:stdio, and using info pci to
confirm that the PCI devices are being detected at the correct geographical location).

There are 256 busses, 32 devices, and 8 functions.

You should confirm the number of busses by reading the MCFG (or DTB).
*/

// 4KiB per device
#define PCI_EXTENDED_CONFIG_SPACE 0x1000

// The Vendor ID of a non-present device
#define PCI_VENDOR_NONE 0xFFFF

// How many PCI devices can we support?
#define MAX_PCI_DEVICES 16

// Bit flags for the PCI Header Command register:
#define PCI_CONFIG_PORTIO_ENABLED        0x01
#define PCI_CONFIG_MMIO_ENABLED          0x02
#define PCI_CONFIG_BUS_MASTER_ENABLED    0x04

#define PCI_NUM_BARS 6

#define PCI_DEVICE_NAMELEN 128

// header.Status will have this bit set if an interrupt is pending
#define PCI_STATUS_INTERRUPT_PENDING ((1ull << 3))

// Set this to zero to enable interrupts
#define PCI_HEADER_COMMAND_INTERRUPTS_ENABLE_ACTIVE_LOW ((1ull << 10))

/*
 * Fields
 *     |
 *     +---> Region Type: 0 = MMIO flavor, 1 = Port IO flavor
 *     |
 *     +---> Locatable:
 *     |        |
 *     |        +---> 0 = Any 32 bit
 *     |        +---> 1 = < 1 MB
 *     |        +---> 2 = Any 64 bit
 *     |
 *     +---> Prefetchable: 0 = no, 1 = yes
 *
 *     For IO flavor BARs:
 *     |31            2|      1        |      0      |
 *     +---------------------------------------------+
 *     | Base Address  |   Reserved    | Region Type |
 *     +---------------------------------------------+
 *
 *     For MMIO flavor BARs:
 *     |31            4|       3      |2         1|      0      |
 *     +--------------------------------------------------------+
 *     | Base Address  | Prefetchable | Locatable | Region Type |
 *     +--------------------------------------------------------+
 *
 *     For 64 bit BARs, you can put them anywhere you want. They
 *     overflow and occupy two BAR slots (so there are actually only
 *     3 potential 64 bit BARs, not 6).
 *
 *     @TODO: Add support for full 64 bit BARs. (Don't need them for now).
 */
typedef enum {
    BAR_MMIO = 0,
    BAR_PORT = 1,
} bar_type_t;
typedef u32 bar_t;

typedef struct __attribute__((packed)) {
    u16 VendorID;
    u16 DeviceID;
    /*
Command Register:
   15-11     10        9       8      7       6        5   4   3       2              1           0
 +------------------------------------------------------------------------------------------------------+
 | Res | Int Disable | 0 | SERR# En | 0 | Parity Err | 0 | 0 | 0 | Bus Master | Memory Space | IO Space |
 +------------------------------------------------------------------------------------------------------+

To enable BARs you need to ensure Memory Space and/ or IO Space are enabled!
    */
    u16 Command;
    u16 Status;
    u8 RevisionID;
    u8 ProgIF;
    u8 Subclass;
    u8 ClassCode;
    u8 CacheLineSize;
    u8 LatencyTimer;
    u8 HeaderType;
    u8 BIST;

    bar_t bar[PCI_NUM_BARS];
    u32 CardbusCISPointer;

    u16 SubsystemVendorID;
    u16 SubsystemID;

    u32 ExpansionROMBAR;
    u8 CapabilitiesPtr; // Points to a linked list of PCI Capabilities,
                        // which are vendor-specific.
    u8 _reserved0;
    u16 _reserved1;
    u32 _reserved2;

    u8 InterruptLine;
    u8 InterruptPin;
    u8 MinGrant;
    u8 MaxLatency;
} PCIeHeader;

#define PCI_CAP_VENDOR_SPECIFIC   0x09
#define PCI_CAP_MSI_X             0x11

typedef struct __attribute__((packed)) {
    // This needs to be 0x09 (vendor specific) for it to be a virtio capability
    u8 cap_vndr;

    // Absolute offset into configuration space of the next capability structure
    u8 cap_next;
} PCICapability;

class PCICapabilityIterator {
public:
    PCICapabilityIterator(volatile PCIeHeader *h) : header(h), cur_offset(h->CapabilitiesPtr) {}

    PCICapabilityIterator operator++() {
        PCICapability *cap = (PCICapability  *)(((usize)header) + cur_offset);
        cur_offset = cap->cap_next;
        return *this;
    }

    PCICapability *operator*() {
        if (cur_offset == 0) return NULL;
        return (PCICapability  *)(((usize)header) + cur_offset);
    }

    volatile PCIeHeader *header;
    usize cur_offset;
};

typedef volatile PCIeHeader *PCIHeader_t;

/**
 * PCIDevice
 * A base class representing an attached PCI device on the system.
 * All PCI devices should inherit from this.
 */
class PCIDevice {
public:
    usize bus_id;
    usize device_id;
    volatile PCIeHeader *pci_header;
    char pci_name[PCI_DEVICE_NAMELEN];

    // Called whenever this device may have had an interrupt
    // Need to read pci_header to check, and if true, then service
    // Service means call the implementation-defined HandleInterrupt method.
    virtual kret_t InterruptReceived();

    // Base classes should fill this in with whatever they need to do
    virtual kret_t HandleInterrupt();

    // Is this a virtio PCI device?
    // For now, the answer is always yes.
    bool is_virtio;
};

PCIHeader_t FindPCI(virt_t config_base, u16 vendor_id, u16 dev_id);
PCIHeader_t FindPCIGeo(virt_t config_base, usize bus, usize device, usize func);
void ListPCIDevices(virt_t config_base);
void InitPCI(virt_t config_base);

#define PCI_NUM_BUSSES 256
#define PCI_NUM_DEVICES 32
#define PCI_NUM_FUNCTIONS 8

// Qemu's PCI MMIO base for each platform
// This is where we allocate BAR memory
// On X86 this is done for us by SeaBIOS/ chipset magic
// See qemu/hw/$(arch)/virt.c, look for VIRT_PCIE_MMIO
#ifdef ARCH_X86
#define PLATFORM_PCI_MMIO_BASE 0
#define PLATFORM_PCI_MMIO_SIZE 0
#endif // ARCH_X86
#ifdef ARCH_ARM
#define PLATFORM_PCI_MMIO_BASE 0x10000000
#define PLATFORM_PCI_MMIO_SIZE 0x2eff0000
#endif // ARCH_ARM
#ifdef ARCH_RISCV
#define PLATFORM_PCI_MMIO_BASE 0x40000000
#define PLATFORM_PCI_MMIO_SIZE 0x40000000
#endif // ARCH_RISCV
#ifndef PLATFORM_PCI_MMIO_BASE
#error "Your platform did not configure the PCI MMIO region base"
#endif // PLATFORM_PCI_MMIO_BASE

// Each board/ cpu tuple should export PLATFORM_PCI_MMCFG_BASE
// that points to the mmcfg base address for that system.
// If PCI is not supported on that platform, set it to this value:
#define PLATFORM_PCI_MMCFG_BASE_NULL ((0))

// List of allocated PCI devices:
extern PCIDevice *pci_devices[MAX_PCI_DEVICES];

/*
 * handle_pci_interrupt
 * Called from platform-specific interrupt handler code
 * anytime any of the legacy PCI interrupt lines is detected.
 *
 * We would be using MSI-X, if not for SOMEONE (ahem riscv)
 * not supporting those.
 *
 * No way to tell which device triggered the interrupt, so
 * check the interrupt pending field of every PCI device
 * and service the one(s) needed.
 */
void handle_pci_interrupt();

END_C_HEADER

#endif // PCI_H
