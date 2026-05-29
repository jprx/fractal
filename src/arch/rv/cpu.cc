#include <types.h>
#include <arch.h>
#include <cpu.h>
#include <virt_mem.h>
#include <page_alloc.h>
#include <arch/riscv_asm_defines.h>
#include <page.h>
#include <interrupt.h>
#include <lib/mem.h>
#include <out.h>

kret_t FRACTALCORE_CLASS::SetPageTable(u64 *new_pt, u64 asid) {
    if (0 != asid) {
        panic("ASIDs not supported on riscv64");
    }

    asm volatile(
        "sfence.vma zero, zero\n"
        "csrw satp, %0\n"
        "sfence.vma zero, zero\n"
        :: "r"((KERN_V2P(((usize)new_pt)) >> SATP_PPN_PAGE_SHIFT) | (RV_PAGING_MODE_SV48 << RV_PAGING_MODE_SHIFT))
    );

    return ALL_GOOD;
}

u64 *FRACTALCORE_CLASS::GetPageTable(virt_t vaddr) {
    u64 out_pt;

    asm volatile(
        "sfence.vma zero, zero\n"
        "csrr %0, satp\n"
        "sfence.vma zero, zero\n"
        : "=r"(out_pt)
    );

    out_pt = KERN_P2V((RV_SATP_GET_PPN_MASK & out_pt) << SATP_PPN_PAGE_SHIFT);
    return (u64 *)out_pt;
}

void FRACTALCORE_CLASS::FlushTLB() {
    u64 tmp;
    asm volatile(
        "sfence.vma zero, zero\n"
        "csrr %0, satp\n"
        "csrw satp, %0\n"
        "sfence.vma zero, zero\n"
        : "=r"(tmp)
    );
}

void FRACTALCORE_CLASS::FlushICache() {
    // @TODO: instruction cache flushes on risc-v
    // (Only needed if we go bare metal)
    return;
}

kret_t FRACTALCORE_CLASS::InitializeInterrupts() {
    write_csr(stvec, (u64)&kernel_trap);
    plic.Init();

    return ALL_GOOD;
}

kret_t FRACTALCORE_CLASS::EnableInterrupt(InterruptKind i) {
    switch(i) {
        case INT_TIMER:
        // We receive timer interrupts through the software interrupt port
        // Rather, M-mode receives machine mode timer interrupts, and then
        // issues S-mode software interrupts later
        write_csr(sie, read_csr(sie) | SIE_SSIE);
        break;

        case INT_UART:
        // UART comes from PLIC, which is serviced by the external interrupt port
        write_csr(sie, read_csr(sie) | SIE_SEIE);
        break;
    }

    return ALL_GOOD;
}

InterruptMode FRACTALCORE_CLASS::SetInterrupts(InterruptMode i) {
    InterruptMode prev_interrupts = ((read_csr(sstatus) & SSTATUS_SIE) != 0) ? INTS_ENABLED : INTS_DISABLED;
    switch (i) {
        case INTS_ENABLED:
        write_csr(sstatus, read_csr(sstatus) | SSTATUS_SIE);
        break;

        case INTS_DISABLED:
        write_csr(sstatus, read_csr(sstatus) & ~SSTATUS_SIE);
        break;
    }
    return prev_interrupts;
}

InterruptSource *FRACTALCORE_CLASS::GetInterruptController() {
    return &this->plic;
}

kret_t FRACTALCORE_CLASS::InitializeSyscalls() {
    return ALL_GOOD;
}

usize FRACTALCORE_CLASS::GetPCIMMCFG() {
    return QEMU_RISCV_VIRT_PCI_MMCFG_BASE;
}

void *FRACTALCORE_CLASS::FindInitrd() {
    // @TODO: Parse FDT and discover this organically,
    // as even Qemu's virt target may rebase this depending on
    // how large the device's memory is and the kernel size
    return (void *)RISCV_INITRD_BASE;
}
