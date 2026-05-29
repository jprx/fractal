#include <cpu.h>
#include <arch/arm/cpu.h>
#include <arch.h>
#include <types.h>
#include <lib/mem.h>
#include <page_alloc.h>
#include <virt_mem.h>
#include <out.h>
#include <page.h>
#include <paging.h>

#define DAIF_FIQ_BIT ((1 << 6))
#define DAIF_IRQ_BIT ((1 << 7))

kret_t FRACTALCORE_CLASS::SetPageTable(u64 *new_pt, u64 asid) {
    u64 old_ttbr0 = read_msr(TTBR0_EL1);
    u64 new_ttbr0 = KERN_V2P((u64)new_pt);

    if (0 != (new_ttbr0 >> 48ull)) {
        panic("new_ttbr0 page table ASID != 0");
    }

#ifndef CONFIG_USE_ASID
    if (0 != asid) {
        panic("CONFIG_USE_ASID is off, but asid != 0");
    }

    if (0 != old_ttbr0 >> 48ull) {
        panic("CONFIG_USE_ASID is off, but TTBR0 has non-zero ASID");
    }
#endif // ! CONFIG_USE_ASID

    write_msr(TTBR0_EL1, new_ttbr0 | (asid << 48ull));

    if (old_ttbr0 != new_ttbr0) {
        FlushTLB();
    }
    return ALL_GOOD;
}

u64 *FRACTALCORE_CLASS::GetPageTable(virt_t vaddr) {
    u64 pt_out;

    if (is_user_addr(vaddr)) {
        asm volatile(
            "mrs %0, ttbr0_el1\n"
            "dsb ish\n"
            "isb\n"
            : "=r"(pt_out)
        );
    } else {
        asm volatile(
            "mrs %0, ttbr1_el1\n"
            "dsb ish\n"
            "isb\n"
            : "=r"(pt_out)
        );
    }

    pt_out = KERN_P2V(pt_out);
    return (u64 *)pt_out;
}

void FRACTALCORE_CLASS::FlushTLB() {
    // See ARMv8-A Address Translation Document (100940_0101_en) page 28
    asm volatile(
        "dsb ishst\n"
        "tlbi vmalle1\n"
        "dsb ish\n"
        "isb"
    );

    // Not technically strictly necessary, but flush the icache too to be safe.
    FlushICache();
}

void FRACTALCORE_CLASS::FlushICache() {
    // Invalidate all instruction caches to point of unification
    asm volatile ("ic iallu");
}

void FRACTALCORE_CLASS::InitializePaging() {

#ifdef BOARD_NO_1GB_PAGES
    // On Apple Silicon there are no giant pages, and there's
    // not a whole lot to map anyways.
    // So, we never change TTBR1 after arriving from bringup.
    // We still call MapKernel to reconfigure MAIR.
    ttbr1_table = (u64*)read_msr(TTBR1_EL1);
#else // BOARD_NO_1GB_PAGES
    ttbr1_table = AllocPageTable();
#endif // ! BOARD_NO_1GB_PAGES

    this->initialized_ttbr1 = false;
    MapKernel(ttbr1_table);
}

#ifndef BOARD_NO_1GB_PAGES

void FRACTALCORE_CLASS::MapKernel(u64 *pt) {
    if (this->initialized_ttbr1) return;

    // Map kernel as normal memory
    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        PageMap(
            this->ttbr1_table,
            KERNEL_LINK_ADDRESS + (i * ONE_GB),
            KERN_V2P(KERNEL_LINK_ADDRESS + (i * ONE_GB)),
            GIANT_PAGE,
            KERNEL_PAGE
        );
    }

    // Map devices as device memory
    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        PageMap(
            this->ttbr1_table,
            KERNEL_DEVICEMAP_BEGIN + (i * ONE_GB),
            KERN_DV2P(KERNEL_DEVICEMAP_BEGIN + (i * ONE_GB)),
            GIANT_PAGE,
            KERNEL_PAGE
        );

        SetPageMemoryKind(
            this->ttbr1_table,
            KERNEL_DEVICEMAP_BEGIN + (i * ONE_GB),
            MEMORY_UNCACHEABLE
        );
    }

    // Map framebuffer as write-combining memory
    for (usize i = 0; i < NUM_PT_ENTRIES; i++) {
        PageMap(
            this->ttbr1_table,
            KERNEL_WRITECOMB_BEGIN + (i * ONE_GB),
            KERN_WCV2P(KERNEL_WRITECOMB_BEGIN + (i * ONE_GB)),
            GIANT_PAGE,
            KERNEL_PAGE
        );

        SetPageMemoryKind(
            this->ttbr1_table,
            KERNEL_WRITECOMB_BEGIN + (i * ONE_GB),
            MEMORY_WRITECOMB
        );
    }

    write_msr(MAIR_EL1, MAIR_RUNTIME);

    this->initialized_ttbr1 = true;
}

#else // ! BOARD_NO_1GB_PAGES

void FRACTALCORE_CLASS::MapKernel(u64 *pt) {
    if (this->initialized_ttbr1) return;

    // For now, just keep TTBR1 as-is (using the bringup page tables)
    write_msr(MAIR_EL1, MAIR_RUNTIME);

    this->initialized_ttbr1 = true;
}

#endif // BOARD_NO_1GB_PAGES

// Doesn't need to do anything on ARM- the kernel is never
// mapped into user page tables in the first place
void FRACTALCORE_CLASS::UnmapKernel(u64 *pt) {}

kret_t FRACTALCORE_CLASS::InitializeInterrupts() {
#ifdef ARM_BOARD_HAS_GIC
    // Trap table already set by start64.s
    gic.Init();
#endif // ARM_BOARD_HAS_GIC

    // Setup timer interrupts
    arm_generic_timer_enable_interrupts();
    arm_generic_timer_schedule_interrupt();

    return ALL_GOOD;
}

kret_t FRACTALCORE_CLASS::EnableInterrupt(InterruptKind i) {
    // All interrupts enabled by default
    return ALL_GOOD;
}

InterruptMode FRACTALCORE_CLASS::SetInterrupts(InterruptMode i) {
    // Read pstate from CPSR to learn IF from DAIF
    u64 daif;
    InterruptMode prev;
    asm volatile(
        "mrs %0, daif\n"
        : "=r"(daif)
    );

    if ((daif & DAIF_IRQ_BIT) || (daif & DAIF_FIQ_BIT)) {
        prev = INTS_DISABLED;
    } else {
        prev = INTS_ENABLED;
    }

    switch (i) {
        case INTS_ENABLED:
            // Clear IF in the (DAIF) bits, this ENABLES IRQs and FIQs
            asm volatile(
                "msr daifclr, 0x3"
            );
        break;

        case INTS_DISABLED:
            // Set IF in the (DAIF) bits, this DISABLES IRQs and FIQs
            asm volatile(
                "msr daifset, 0x3"
            );
        break;
    }

    return prev;
}

InterruptSource *FRACTALCORE_CLASS::GetInterruptController() {
#ifdef ARM_BOARD_HAS_GIC
    return &this->gic;
#else // ARM_BOARD_HAS_GIC
    return NULL;
#endif // !ARM_BOARD_HAS_GIC
}

kret_t FRACTALCORE_CLASS::InitializeSyscalls() {
    return ALL_GOOD;
}

usize FRACTALCORE_CLASS::GetPCIMMCFG() {
    if (0 == PLATFORM_PCI_MMCFG_BASE) panic("Tried to load PCI drivers on a platform that does not support PCI");
    return PLATFORM_PCI_MMCFG_BASE;
}

void *FRACTALCORE_CLASS::FindInitrd() {

#ifdef BOARD_HAS_CUSTOM_INITRD_FINDER
    return (void *)__board_find_initrd__();
#else // BOARD_HAS_CUSTOM_INITRD_FINDER
    // @TODO: Parse FDT and discover this organically,
    // as even Qemu's virt target may rebase this depending on
    // how large the device's memory is and the kernel size
    return (void *)ARM_INITRD_BASE;
#endif // !BOARD_HAS_CUSTOM_INITRD_FINDER
}
