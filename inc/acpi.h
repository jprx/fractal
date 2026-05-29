#ifndef ACPI_H
#define ACPI_H

#include <types.h>
#include <virt_mem.h>

BEGIN_C_HEADER

typedef struct __attribute__((packed)) ACPIHeader_t {
    u8 signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    u8 OEMID[6];
    u8 OEMTableID[8];
    u32 OEMRev;
    u32 CreatorID;
    u32 CreatorRevision;

	// All other tables exist right after this
} ACPIHeader;

typedef struct __attribute__((packed)) RSDPv1_t {
    u8 signature[8];
    u8 checksum;
    u8 OEMID[6];
    u8 revision;
    u32 rsdt_addr;
} RSDPv1;

typedef struct __attribute__((packed)) RSDT_t {
    ACPIHeader header;
} RSDT;

// "RSD PTR ":
#define RSDP_MAGIC 0x2052545020445352ull

// "RSDT":
#define RSDT_MAGIC 0x54445352ull

#define X86_ACPI_RSDP_SEARCH_BEGIN ((0x000E0000 | KERNEL_VIRT_MASK))
#define X86_ACPI_RSDP_SEARCH_END   ((0x000FFFFF | KERNEL_VIRT_MASK))

/*
 * FindRSDP
 * Find the root system description pointer,
 * by scanning through memory [search_start -> search_end]
 *
 * On X86_64 systems, this is in the extended BIOS data area,
 * or in the main BIOS data area < 1M.
 */
RSDPv1 *FindRSDP(virt_t search_start, virt_t search_end);

/*
 * Verify*
 * Check the signature and checksum of an ACPI table.
 */
bool VerifyRSDP(RSDPv1 *r);
bool VerifyRSDT(RSDT *r);

// Verify any generic ACPI table (via checksum)
// Can skip the signature magic check because presumably that will have
// been performed while iterating over the RSDT
bool VerifyACPITable(ACPIHeader *h);

usize RSDTNumTables(RSDT *);

class RSDTIterator {
public:
    RSDTIterator(RSDT *table_in) : table(table_in), index(0), num_tables(RSDTNumTables(table_in)) {}

    RSDTIterator operator++() {
        this->index++;
        return *this;
    }

    usize operator*() {
        if (index > num_tables) {
            return 0;
        }
        u32 next_table = *(u32 *)((usize)table + sizeof(*table) + (index * sizeof(u32)));
        if (0 == next_table) {
            return 0;
        }
        return KERN_P2V(next_table);
    }

    RSDT *table;
    usize index;
    usize num_tables;
};

END_C_HEADER

#endif // ACPI_H
