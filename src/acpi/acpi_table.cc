#include <types.h>
#include <acpi.h>
#include <acpi/madt.h>
#include <acpi/mcfg.h>
#include <out.h>

RSDPv1 *FindRSDP(virt_t search_start, virt_t search_end) {
    for (usize i = search_start; i < search_end; i += sizeof(u64)) {
        if (RSDP_MAGIC == *(volatile u64*)i) {
            return (RSDPv1*)i;
        }
    }
    return NULL;
}

template <typename TableType>
bool VerifyTableChecksum(TableType *t) {
    u8 accumulator = 0;
    for (usize i = 0; i < sizeof(*t); i++) {
        accumulator += ((u8 *)t)[i];
    }
    return accumulator == 0;
}

bool VerifyRSDP(RSDPv1 *r) {
    if (RSDP_MAGIC != *(u64 *)r) {
        return false;
    }

    return VerifyTableChecksum<typeof(*r)>(r);
}

bool VerifyACPITable(ACPIHeader *a) {
    u8 accumulator = 0;
    for (usize i = 0; i < a->length; i++) {
        accumulator += ((u8 *)a)[i];
    }
    return accumulator == 0;
}

bool VerifyRSDT(RSDT *r) {
    if (RSDT_MAGIC != *(u32 *)r) {
        return false;
    }

    return VerifyACPITable(&r->header);
}

usize RSDTNumTables(RSDT *t) {
    return (t->header.length - sizeof(*t)) / sizeof(u32);
}

virt_t FindIOAPIC(MADT *t) {
    if (!t) return (virt_t)NULL;

    for (MADTIterator it = MADTIterator(t); *it != NULL; ++it) {
        if ((*it)->entry_kind == MADT_ENTRY_IOAPIC) {
            return KERN_P2DV(((MADTEntryIOAPIC *)(*it))->ioapic_addr);
        }
    }

    return (virt_t)NULL;
}

usize MCFG::get_num_devices() {
    return (this->header.length - sizeof(MCFG)) / (sizeof(MCFGEntry));
}
