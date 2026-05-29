#ifndef PCI_MSI_X_H
#define PCI_MSI_X_H

#include <fractal.h>
#include <io/pci.h>

BEGIN_C_HEADER

#define MSI_X_CAP_MSG_CONTROL_ENABLED 0x8000
#define MSI_X_CAP_BAR_MASK            0b0111

/// Bits [10,0] are the table size (up to max 2048 entries)
#define MSI_X_CAP_MSG_CONTROL_TABLE_SIZE_MASK 0x7FF

typedef struct __attribute__((packed)) {
    PCICapability header;

    // Bits 10-0: Table size
    // Bits 13-11: Reserved
    // Bit 14: Function mask
    // Bit 15: Enable
    // Table size is the number of entries in the MSI-X table - 1, and is read only
    u16 message_control;

    // BIR is bits 2-0 of this, so mask those out before getting the table offset
    u32 table_offset_and_bir;

    // Bits 2-0 are the BAR to find the "pending bit", the rest is the offset into that BAR
    u32 pending_bit_and_table;

    // According to C++-11 spec, POD (plain old data) structs
    // are allowed to have member-functions, so this is ok and
    // will not affect the layout of this struct in memory.
    void enable();
    usize table_offset();
    usize table_bar();
    usize num_entries();
} MSI_XCapability;

// A table entry in the MSI-X table pointed to by the MSI-X Capability
typedef struct __attribute__((packed)) {
    u32 message_addr_low;
    u32 message_addr_high;
    u32 message_data;

    // Set this to 0 for enable, 1 for disable:
    u32 vector;
} MSI_XEntry;

// A struct that refers to a device's MSI-X table that we can copy around
typedef struct __attribute__((packed)) {
    // Table's base physical address
    usize addr;

    // How many entries are in the table?
    usize num_entries;

    // The capability that produced this table (in the PCIe MMCFG space)
    MSI_XCapability *cap;
} MSI_XTable;

END_C_HEADER

#endif // PCI_MSI_X_H
