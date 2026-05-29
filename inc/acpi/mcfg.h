#ifndef ACPI_MCFG_H
#define ACPI_MCFG_H

#include <fractal.h>
#include <acpi.h>

// See: https://wiki.osdev.org/PCI_Express

BEGIN_C_HEADER

// "MCFG":
#define MCFG_MAGIC 0x4746434D

typedef struct __attribute__((packed)) {
    ACPIHeader header;
    u64 reserved;

    // After this there are lots of MCFGEntry_t's
    usize get_num_devices();
} MCFG;

typedef struct __attribute__((packed)) {
    // Physical address of the extended configuration space mechanism
    u64 config_base;
    u16 segment_group_number;
    u8 start_bus_number;
    u8 end_bus_number;
    u32 reserved;
} MCFGEntry;

class MCFGIterator {
public:
    MCFGIterator(MCFG *table_in) : table(table_in), index(0), num_devices(table_in->get_num_devices()) {}

    MCFGIterator operator++() {
        index++;
        return *this;
    }

    MCFGEntry *operator*() {
        if (index >= num_devices) return NULL;
        return &((MCFGEntry *)((usize)table + sizeof(*table)))[index];
    }

    MCFG *table;
    usize index;
    usize num_devices;
};

END_C_HEADER

#endif // ACPI_MCFG_H
