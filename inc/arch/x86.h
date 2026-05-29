#ifndef X86_H
#define X86_H

#include <types.h>

#include <arch/x86_asm_defines.h>
#include <arch/x86/selector.h>
#include <arch/x86/idt.h>
#include <arch/x86/gdt.h>
#include <arch/x86/tss.h>
#include <arch/x86/multiboot.h>
#include <io/interrupt/lapic.h>
#include <io/interrupt/ioapic.h>
#include <io/interrupt/apic.h>

BEGIN_C_HEADER
    void outb(u16 port, u8 val);
    void outw(u16 port, u16 val);
    u8 inb(u16 port);
    void write_msr(u32 msr_idx, u64 val);
    u64 read_msr(u32 msr_idx);
END_C_HEADER

#include <io/serial/uart16550.h>
#define SERIAL_PORT UART16550Port
#define SERIAL_BASEADDR ((0x3F8))

#include <arch/x86/cpu.h>
#define FRACTALCORE_CLASS FractalCoreX86
#define FRACTALPTE_CLASS PTEX86

#define EFER_MSR   0xC0000080
#define EFER_SCE   ((1 << 0))

#define STAR_MSR   0xC0000081
#define LSTAR_MSR  0xC0000082
#define CSTAR_MSR  0xC0000083
#define SFMASK_MSR 0xC0000084

#define STAR_SYSRET_SHIFT  48ull
#define STAR_SYSCALL_SHIFT 32ull

#define FSBASE_MSR       0xC0000100
#define GSBASE_MSR       0xC0000101
#define KERNELGSBASE_MSR 0xC0000102

extern "C" void _stack_bottom_lma();
#define KERNEL_STACK ((((usize)&_stack_bottom_lma)))

typedef enum mtrr_mem_type {
    MTRR_UNCACHEABLE = 0,
    MTRR_WRITECOMB = 1,
    MTRR_WRITETHRU = 4,
    MTRR_WRITEPROT = 5,
    MTRR_WRITEBACK = 6,
} mtrr_type_t;

#define MTRR_DEFRANGE_MSR_ENABLE_FIXED_RANGE ((BIT(10)))
#define MTRR_DEFRANGE_MSR_ENABLE   ((BIT(11)))

#define MTRR_DEFRANGE_VALUE ((MTRR_DEFRANGE_MSR_ENABLE | MTRR_DEFRANGE_MSR_ENABLE_FIXED_RANGE | MTRR_WRITEBACK))

#define MTRR_MSR_MTRRCAP ((0x00FE))

#define MTRR_MSR_MTRRDEFRANGE ((0x02FF))

#define MTRRCAP_NUM_MSR_MASK ((0x0FF))
#define MTRRCAP_WRITECOMB_SUPPORTED ((BIT(10)))

#define MTRR_PHYSMASKN_VALID_BIT ((BIT(11)))

#define MTRR_PHYSMASKN_FIRST ((0x0201))
#define MTRR_PHYSMASKN_INC   ((2))
#define MTRR_PHYSMASKN_LAST ((0x020F))
#define MTRR_MAX_VARWIDTH_REGS ((8))

/*
 * Page Attribute Table (PAT)
 *  - PA0: Write Back
 *  - PA1: Write Combining
 *  - Rest: Uncacheable
 *
 * We never touch the PAT bit in a PTE because it is
 * not always in the same spot (bit 7 for 4K page, and
 * bit 12 for 2MB/ 4MB pages). But bit 7 is also used
 * to mark 2MB pages as huge pages, so from a PTE alone
 * it is impossible to tell whether it is a large page
 * or a small page with the PAT bit set.
 *
 * For that reason, we re-encode the PAT from its default
 * initialization values (as seen in table 7-9 in AMD
 * System Programming Manual Vol 2) to our own definitions.
 *
 * This means we effectively "overload" the PCD and PWT
 * values as they are used for encoding which PAT entry
 * to use- they're always in the same spot so our PTE
 * class mechanism works with those better than the PAT bit.
 *
 * To set a PTE to use Write Back:
 *  - PAT = 0 (don't touch), PCD = 0, PWT = 0
 *
 * To set a PTE to use Write Combining:
 *  - PAT = 0 (don't touch), PCD = 0, PWT = 1
 *
 * To set a PTE to be Uncacheable:
 *  - PAT = Any, PCD = 1, PWD = Any
 */
#define PAT_ENTRY_WRITEBACK   ((6ull))
#define PAT_ENTRY_WRITECOMB   ((1ull))
#define PAT_ENTRY_UNCACHEABLE ((0ull))

#define PAT_REG_VAL (((PAT_ENTRY_WRITEBACK << 0ull) | (PAT_ENTRY_WRITECOMB << 8ull)))
#define PAT_REG_MSR ((0x0277))

#endif // X86_H
