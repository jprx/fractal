#include <cpu.h>
#include <arch.h>
#include <types.h>
#include <lib/mem.h>
#include <lib/dlmalloc.h>
#include <page_alloc.h>
#include <virt_mem.h>
#include <out.h>
#include <page.h>
#include <acpi.h>
#include <acpi/madt.h>
#include <acpi/mcfg.h>
#include <io/interrupt/lapic.h>
#include <io/interrupt/ioapic.h>
#include <io/interrupt/apic.h>
#include <arch/x86/idt.h>
#include <arch/x86/gdt.h>
#include <syscall.h>
#include <task.h>
#include <io/gpu.h>
#include <io/gpu/ramfb-gpu.h>
#include <oxide.h>
#include <io/hid/ps2-hid.h>

extern FRACTALCORE_CLASS global_cpu;
extern u64 global_multiboot_ptr;
extern FractalGPU *global_gpu;

#define I8259A_PIC1_CMD  ((0x20))
#define I8259A_PIC1_DATA ((0x21))
#define I8259A_PIC2_CMD  ((0xA0))
#define I8259A_PIC2_DATA ((0xA1))

kret_t FRACTALCORE_CLASS::SetPageTable(u64 *new_pt, u64 asid) {
    if (0 != asid) {
        panic("ASIDs not supported on x86_64");
    }

    asm volatile(
        "mov cr3, %0"
        :: "r"(KERN_V2P((usize)new_pt))
    );

    return ALL_GOOD;
}

u64 *FRACTALCORE_CLASS::GetPageTable(virt_t vaddr) {
    u64 out_pt;

    asm volatile(
        "mov %0, cr3\n"
        : "=r"(out_pt)
    );

    out_pt = KERN_P2V(out_pt);
    return (u64 *)out_pt;
}

void FRACTALCORE_CLASS::FlushTLB() {
    u64 tmp;
    asm volatile(
        "mov %0, cr3\n"
        "mov cr3, %0\n"
        : "=r"(tmp)
    );
}

void FRACTALCORE_CLASS::FlushICache() {
    // Don't ever need to do this on X86_64,
    // because the data and instruction caches
    // are coherent.
    return;
}

kret_t FRACTALCORE_CLASS::InitializeInterrupts() {
    // For now, only search BIOS data area < 1M
    // Some systems may store this in the extended BIOS data area, which
    // requires parsing the multiboot headers.
    // Multiboot 2 gives the RSDP for free, but Multiboot 1 (required to boot
    // in Qemu with -kernel) doesn't tell us where it is.
    RSDPv1 *rsdp = FindRSDP(X86_ACPI_RSDP_SEARCH_BEGIN, X86_ACPI_RSDP_SEARCH_END);
    MADT *madt = NULL;
    LAPIC *lapic = NULL;
    IOAPIC *ioapic = NULL;
    MCFG *mcfg = NULL;
    usize num_cores = 0;

    // Disable i8259A PIC
    outb(I8259A_PIC1_DATA, 0xFF); // Mask all
    outb(I8259A_PIC2_DATA, 0xFF); // Mask all
    outb(I8259A_PIC1_CMD, 0x20); // EOI
    outb(I8259A_PIC2_CMD, 0x20); // EOI

    InitGDT();
    InitIDT();
    InitTSS();

    if (NULL == rsdp) {
        panic("Couldn't find RSDP");
    }

#ifndef QUIET_BOOT
    printf("RSDP: 0x%X\r\n", rsdp);
#endif // !QUIET_BOOT

    if (!VerifyRSDP(rsdp)) {
        panic("RSDP Checksum invalid");
    }

    RSDT *rsdt = (RSDT *)KERN_P2V((rsdp->rsdt_addr));
    if (NULL == rsdt) {
        panic("RSDT at 0x%X is invalid", rsdt);
    }
    if (!VerifyRSDT(rsdt)) {
        panic("RSDT at 0x%X has invalid checksum", rsdt);
    }

#ifndef QUIET_BOOT
    printf("RSDT: 0x%X\r\n", rsdt);
#endif // !QUIET_BOOT

    // printf("Found ACPI tables:\r\n");
    for (RSDTIterator it = RSDTIterator(rsdt); *it != 0; ++it) {
        ACPIHeader *h = (ACPIHeader *)(*it);
        // printf("0x%X: %c%c%c%c\r\n", *it, h->signature[0], h->signature[1], h->signature[2], h->signature[3]);

        if (*(u32 *)(&(h->signature)) == MADT_MAGIC) {
#ifndef QUIET_BOOT
            printf("[ACPI] Found %c%c%c%c table\r\n", h->signature[0], h->signature[1], h->signature[2], h->signature[3]);
#endif // !QUIET_BOOT
            madt = (MADT *)h;
        }

        if (*(u32 *)(&(h->signature)) == MCFG_MAGIC) {
#ifndef QUIET_BOOT
            printf("[ACPI] Found %c%c%c%c table\r\n", h->signature[0], h->signature[1], h->signature[2], h->signature[3]);
#endif // !QUIET_BOOT
            mcfg = (MCFG *)h;
        }
    }

    if (0 == madt) {
        panic("Could not locate multiple APIC descriptor table (MADT)");
    }

    if (!VerifyACPITable(&madt->header)) {
        panic("Invalid MADT");
    }

    lapic = (LAPIC *)KERN_P2DV(madt->lapic_addr);
    if ((usize)lapic != LAPIC_DEFAULT_ADDRESS) {
        printf("WARNING: MADT reported LAPIC at non-default address (0x%X, expected 0x%X)\r\n", lapic, LAPIC_DEFAULT_ADDRESS);
    }

#ifndef QUIET_BOOT
    printf("LAPIC:  0x%X\r\n", lapic);
#endif // !QUIET_BOOT

    ioapic = (IOAPIC *)FindIOAPIC(madt);
    if (!ioapic) {
        panic("No IOAPIC found");
    }

#ifndef QUIET_BOOT
    printf("IOAPIC: 0x%X\r\n", ioapic);
#endif // !QUIET_BOOT

#define CHECK_NUMBER_OF_DETECTED_X86_CORES 1
#ifdef CHECK_NUMBER_OF_DETECTED_X86_CORES
#ifndef QUIET_BOOT
    printf("MADT Size: %d bytes\n", madt->header.length);
#endif // !QUIET_BOOT
    for (MADTIterator it = MADTIterator(madt); *it != NULL; ++it) {
        if ((*it)->entry_kind == MADT_ENTRY_LAPIC) {
#ifndef QUIET_BOOT
            printf("Discovered core %d with LAPIC 0x%X\r\n", num_cores++, ((MADTEntryLAPIC *)(*it))->apic_id);
#endif // !QUIET_BOOT
        }

#ifndef QUIET_BOOT
        if ((*it)->entry_kind == MADT_ENTRY_IOAPIC_INT_OVERRIDE) {
            MADTEntryIOAPICOverride *override = (MADTEntryIOAPICOverride*)(*it);
            printf("\tIRQ %d -> %d\n", override->irq, override->global_sys_interrupt);
        }
#endif // !QUIET_BOOT
    }
#else
    // Just pretend there's only 1...
    // What we don't initialize can't hurt us, right?
    num_cores = 1;
#endif // CHECK_NUMBER_OF_DETECTED_X86_CORES

#ifndef QUIET_BOOT
    if (num_cores > 1) {
        printf("There are %d cores in this system\r\n", num_cores);
    } else {
        printf("There is 1 core in this system\r\n");
    }
#endif // !QUIET_BOOT

    global_cpu.apic.ioapic = ioapic;
    global_cpu.apic.lapic = lapic;

    ioapic->Setup();
    ioapic->SetIRQ(UART0_IRQ_IOAPIC_EXTERNAL, UART0_IRQ);
    ioapic->SetIRQ(LEGACY_KEYBOARD_IRQ_IOAPIC_EXTERNAL, LEGACY_KEYBOARD_IRQ);
    ioapic->SetIRQ(LEGACY_MOUSE_IRQ_IOAPIC_EXTERNAL, LEGACY_MOUSE_IRQ);
    ioapic->SetIRQ(PCI_INTA_EXTERNAL, VIRTIO_IRQ);
    ioapic->SetIRQ(PCI_INTB_EXTERNAL, VIRTIO_IRQ);
    ioapic->EnableIRQ(PCI_INTA_EXTERNAL);
    ioapic->EnableIRQ(PCI_INTB_EXTERNAL);
    ioapic->EnableIRQ(UART0_IRQ_IOAPIC_EXTERNAL);
    ioapic->EnableIRQ(LEGACY_KEYBOARD_IRQ_IOAPIC_EXTERNAL);
    ioapic->EnableIRQ(LEGACY_MOUSE_IRQ_IOAPIC_EXTERNAL);
    lapic->Enable();

    // Configure APIC timer
    lapic->EnableTimer();

    // Find PCI
#ifndef QUIET_BOOT
    printf("MCFG: 0x%X\r\n", mcfg);
#endif // !QUIET_BOOT
    for (MCFGIterator it = MCFGIterator(mcfg); *it != NULL; ++it) {
#ifndef QUIET_BOOT
        printf("MCFGEntry with PCI base address at 0x%X\r\n", (*it)->config_base);
#endif // !QUIET_BOOT
        pci_mmcfg_base = (*it)->config_base;
    }
#ifndef QUIET_BOOT
    printf("PCI is at 0x%X\r\n", pci_mmcfg_base);
#endif // !QUIET_BOOT

    return ALL_GOOD;
}

kret_t FRACTALCORE_CLASS::EnableInterrupt(InterruptKind i) {
    return ALL_GOOD;
}

InterruptMode FRACTALCORE_CLASS::SetInterrupts(InterruptMode i) {
    u64 old_rflags;
    asm volatile(
        "pushfq\n"
        "pop %0"
        : "=r"(old_rflags)
    );
    switch(i) {
        case INTS_ENABLED:
        asm volatile ("sti");
        break;

        case INTS_DISABLED:
        asm volatile("cli");
        break;
    }
    return (old_rflags & X86_RFLAGS_INTERRUPT_ENABLE_MASK) == 0 ? INTS_DISABLED : INTS_ENABLED;
}

InterruptSource *FRACTALCORE_CLASS::GetInterruptController() {
    return &apic;
}

kret_t FRACTALCORE_CLASS::InitializeSyscalls() {
    // Gotta actually "turn these instructions on"
    write_msr(EFER_MSR, read_msr(EFER_MSR) | EFER_SCE);

    u64 sysret_cs = USER_CS_COMPAT;
    u64 syscall_cs = KERNEL_CS;

    if (gdt[SELECTOR_TO_GDT_INDEX(sysret_cs + 16)] != GDT_ENTRY_CODE_USER || gdt[SELECTOR_TO_GDT_INDEX(syscall_cs)] != GDT_ENTRY_CODE_KERNEL) {
        // sysret:  cs is loaded with whatever we write + 16
        // syscall: cs is loaded directly with whatever we write
        printf("syscall/ sysret selectors seem strange (is that sufficient alliteration?) (%X and %X)\r\n", SELECTOR_TO_GDT_INDEX(sysret_cs), SELECTOR_TO_GDT_INDEX(syscall_cs));
        printf("%X:%X\r\n%X:%X\r\n", gdt[SELECTOR_TO_GDT_INDEX(sysret_cs + 16)], GDT_ENTRY_CODE_USER, gdt[SELECTOR_TO_GDT_INDEX(syscall_cs)], GDT_ENTRY_CODE_KERNEL);
    }

    if (IS_USER_SELECTOR(syscall_cs) || !IS_USER_SELECTOR(sysret_cs)) {
        panic("Invalid RPLs in the STAR MSR; this should be ok in theory, but fix this regardless");
    }

    write_msr(LSTAR_MSR, (usize)&syscall_entry);
    write_msr(CSTAR_MSR, (usize)&syscall_compat_entry);
    write_msr(STAR_MSR, (sysret_cs << STAR_SYSRET_SHIFT) | (syscall_cs << STAR_SYSCALL_SHIFT));

    // Clear interrupts on syscall instruction
    write_msr(SFMASK_MSR, X86_RFLAGS_INTERRUPT_ENABLE_MASK);

    // Setup per-cpu data area
    // For now both hidden & user visible value are our per-cpu data area
    // In the future, we will have a separate value visible to userspace
    write_msr(GSBASE_MSR, (usize)per_cpu_data_area);
    write_msr(KERNELGSBASE_MSR, (usize)per_cpu_data_area);

    return ALL_GOOD;
}

usize FRACTALCORE_CLASS::GetPCIMMCFG() {
    return pci_mmcfg_base;
}

void *FRACTALCORE_CLASS::FindInitrd() {
    // start64 populates the multiboot info pointer before jumping to fractal main
    // if start.s or start64.s ever clobbers ebx/rbx, we will not be able to detect the multiboot struct
    // We assume only 1 multiboot module- the initrd
    multiboot_info_t *info = (multiboot_info_t *)(KERN_P2V(global_multiboot_ptr));

    if (0 == info->mods_count) {
        printf("Multiboot Info: 0x%X\r\n", global_multiboot_ptr);
        printf("mods_count: %d\r\n", info->mods_count);
        panic("initrd not found in multiboot header");
    }
    if (1 != info->mods_count) {
        printf("Multiboot Info: 0x%X\r\n", global_multiboot_ptr);
        printf("mods_count: %d\r\n", info->mods_count);
        panic("More than 1 multiboot module detected- we expect this to just be the initrd and nothing else");
    }

    multiboot_mod_t *mod = (multiboot_mod_t *)(KERN_P2V(info->mods_addr));

    return (void *)(KERN_P2V(mod->mod_start));
}

kret_t FRACTALCORE_CLASS::DetectHardwareFramebuffer() {
    multiboot_info_t *info = (multiboot_info_t *)(KERN_P2V(global_multiboot_ptr));

    if ((info->flags & MULTIBOOT_FRAMEBUFFER_PRESENT_FLAG) == 0) {
        // Multiboot info says no framebuffer
#ifndef QUIET_BOOT
        printf("Multiboot- No framebuffer detected\n");
#endif // !QUIET_BOOT

        return ALL_GOOD;
    }

#ifndef QUIET_BOOT
    printf("Detected framebuffer at 0x%X\n", info->framebuffer_addr);
    printf("Width: %d\nHeight: %d\n", info->framebuffer_width, info->framebuffer_height);
    printf("BPP: %d\nPitch: %d\n", info->framebuffer_bpp, info->framebuffer_pitch);
#endif // !QUIET_BOOT

    if ((info->framebuffer_width > SCREEN_MAX_W) || (info->framebuffer_height > SCREEN_MAX_H)) {
        panic("Multiboot framebuffer is too large for Oxide");
    }

    if (NULL != global_gpu) panic("Detected a multiboot framebuffer, but a display device already exists");

    global_gpu = new RamfbGPUDevice(
        (u32 *)KERN_P2WCV(info->framebuffer_addr),
        info->framebuffer_width,
        info->framebuffer_height
    );

    if (MULTIBOOT_FRAMEBUFFER_TYPE_RGB != info->framebuffer_type) panic("Multiboot framebuffer isn't RGB");

    SCREEN_W = info->framebuffer_width;
    SCREEN_H = info->framebuffer_height;

    return ALL_GOOD;
}

kret_t FRACTALCORE_CLASS::DetectPlatformIO() {
#ifndef QUIET_BOOT
    printf("[pc] Detecting PlatformIO\r\n");
#endif // !QUIET_BOOT

    initialize_8042();
    return ALL_GOOD;
}

static inline volatile u64 read_cr0() {
    u64 out;
    asm volatile(
        "mov %0, cr0\n"
        : "=r"(out)
    );
    return out;
}

static inline volatile void write_cr0(u64 new_cr0) {
    asm volatile(
        "mov cr0, %0\n"
        :: "r"(new_cr0)
    );
}

// Disable variable-range MTRR at index idx
static inline void disable_mtrr(usize idx) {
    if (idx > MTRR_MAX_VARWIDTH_REGS) return;
    usize msr_idx = MTRR_PHYSMASKN_FIRST + (idx * MTRR_PHYSMASKN_INC);
    printf("MTRR Mask %d was 0x%X\r\n", idx, read_msr(msr_idx));
    write_msr(
        msr_idx,
        0
    );
}

static inline void dump_mtrr_regs() {
    printf("CR0:         0x%X\r\n", read_cr0());
    printf("MTRRcap:     0x%X\r\n", read_msr(MTRR_MSR_MTRRCAP));
    printf("MTRRdefType: 0x%X\r\n", read_msr(MTRR_MSR_MTRRDEFRANGE));

    for (usize i = 0; i < MTRR_MAX_VARWIDTH_REGS; i++) {
        printf("\tMTRRPhysMask %d: 0x%X\r\n", i, read_msr(MTRR_PHYSMASKN_FIRST + (i * MTRR_PHYSMASKN_INC)));
    }

    printf("PAT: 0x%X\r\n", read_msr(PAT_REG_MSR));
}

void FRACTALCORE_CLASS::InitializeCaching() {
    // LESSON LEARNED
    // Trust the bootloader. Don't modify the MTRRs we inherit from GRUB.
    // Instead, configure a PAT and it will override as appropriate.

    // Configure PAT
    write_msr(PAT_REG_MSR, PAT_REG_VAL);

#ifndef QUIET_BOOT
    printf("PAT reg: 0x%X\r\n", read_msr(PAT_REG_MSR));
    dump_mtrr_regs();
#endif // !QUIET_BOOT

}
