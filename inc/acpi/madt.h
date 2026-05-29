#ifndef MADT_H
#define MADT_H

#include <types.h>
#include <out.h>

BEGIN_C_HEADER

// "APIC":
#define MADT_MAGIC 0x43495041

typedef struct __attribute__((packed)) MADT_t {
    ACPIHeader header;
    u32 lapic_addr;
    u32 flags;
} MADT;

typedef struct __attribute__((packed)) MADTEntry_t {
    u8 entry_kind;
    u8 entry_length;

    // Rest of contents goes here, size depends on type
} MADTEntry;

typedef struct __attribute__((packed)) MADTEntryLAPIC_t {
    MADTEntry header;
    u8 acpi_processor_id;
    u8 apic_id;
    u32 flags;
} MADTEntryLAPIC;

typedef struct MADTEntryIOAPICOverride_t {
    MADTEntry header;
    u8 bus;
    u8 irq;
    u32 global_sys_interrupt;
    u16 flags;
} MADTEntryIOAPICOverride;

typedef struct MADTEntryIOAPIC_t {
    MADTEntry header;
    u8 ioapic_id;
    u8 _reserved;
    u32 ioapic_addr;
    u32 global_system_interrupt_base;
} MADTEntryIOAPIC;

#define MADT_ENTRY_LAPIC                0
#define MADT_ENTRY_IOAPIC               1
#define MADT_ENTRY_IOAPIC_INT_OVERRIDE  2
#define MADT_ENTRY_IOAPIC_NMI_SOURCE    3
#define MADT_ENTRY_LAPIC_NMI            4
#define MADT_ENTRY_LAPIC_OVERRIDE       5
#define MADT_ENTRY_LOCAL_X2APIC         9

class MADTIterator {
public:
    MADTIterator(MADT *table_in) : table(table_in), offset(sizeof(MADT)) {}

    MADTIterator operator++() {
        MADTEntry *cur_entry = (MADTEntry *)(((usize)table) + offset);
        offset += cur_entry->entry_length;
        return *this;
    }

    MADTEntry *operator*() {
        MADTEntry *cur_entry = (MADTEntry *)(((usize)table) + offset);

        // Make sure current entry is actually in the ACPI tables
        if (offset >= table->header.length) {
            return 0;
        }

        if ((offset + cur_entry->entry_length - 1) > table->header.length) {
            return 0;
        }

        return (MADTEntry *)KERN_P2V((usize)cur_entry);
    }

    MADT *table;
    usize offset;
};

/*
 * FindIOAPIC
 * Returns the first IOAPIC reported in a given MADT,
 * NULL if none exist.
 *
 * Multiple IOAPICs could exist, we just return the first one.
 */
virt_t FindIOAPIC(MADT *t);

END_C_HEADER

#endif // MADT_H
