#ifndef ARM_ASM_H
#define ARM_ASM_H

#include <gen/arm_asm_offsets.h>

#define ARCH_HAS_FAST_MEMCPY
#define ARCH_HAS_FAST_MEMSET

#define read_msr(r) ({ u64 ___tmp_msr_val; \
    asm volatile ("mrs %0, " #r : "=r"(___tmp_msr_val)); \
    ___tmp_msr_val; })

#define write_msr(r,v) ({asm volatile("msr " #r ", %0" :: "r"(v));})

#define KERNEL_LINK_ADDRESS ((0xFFFF800000000000))
#define INITRD_EXPECTED_ADDRESS ((0xFFFF800048000000))

#define TIMESLICE_QUANTUM ((0x50000))

#define ARM_NUM_VECREGS ((32))
#define ARM_VECREG_SZ   ((16))

#define HCR_EL2_RW  ((1 << 31))
#define HCR_EL2_API ((1 << 41))
#define HCR_EL2_APK ((1 << 40))

#define SCTLR_EL1_UCI      ((1 << 26))
#define SCTLR_EL1_UCT      ((1 << 15))
#define SCTLR_EL1_DZE      ((1 << 14))
#define SCTLR_EL1_ICACHE   ((1 << 12))
#define SCTLR_EL1_DCACHE   ((1 << 2))
#define SCTLR_EL1_MMU      ((1 << 0))

/* 4K pages */
#define TCR_EL1_TG0        ((0b00 << 14ULL))
#define TCR_EL1_TG1        ((0b10 << 30ULL))
#define TCR_EL2_TG0        ((0b00 << 14ULL))

#define TCR_EL1_T0SZ       ((16 << 0ULL))
#define TCR_EL1_T1SZ       ((16 << 16ULL))
#define TCR_EL2_T0SZ       ((16 << 0ULL))

/* 52 bits (4PB) physical addresses - the max */
#define TCR_EL1_IPS        ((0b110 << 32ULL))

/* 48 bits */
#define TCR_EL2_PS ((0b101 << 16ULL))

#define TCR_EL1_HA         ((1 << 39ULL))
#define TCR_EL2_HA         ((1 << 21ULL))

// IRGN == Inner cacheability attribute
// ORGN == Outer cacheability attribute
// 0b00: Non-cacheable
// 0b01: Write-back
// 0b10: Write-Through
// 0b11: Write-back, no write-allocate
#define TCR_IRGN ((0b01))
#define TCR_ORGN ((0b01))
#define TCR_ORGN0 ((TCR_ORGN << 10ULL))
#define TCR_ORGN1 ((TCR_ORGN << 26ULL))
#define TCR_IRGN0 ((TCR_IRGN << 8ULL))
#define TCR_IRGN1 ((TCR_IRGN << 24ULL))

#define TCR_SH0 ((0b11 << 12ULL))
#define TCR_SH1 ((0b11 << 28ULL))

#define TCR_DEFAULT_EL1 ((TCR_EL1_TG0 | TCR_EL1_TG1 | TCR_EL1_T0SZ | TCR_EL1_T1SZ | TCR_EL1_IPS | TCR_EL1_HA | TCR_IRGN0 | TCR_ORGN0 | TCR_IRGN1 | TCR_ORGN1 | TCR_SH0 | TCR_SH1))

#define TCR_DEFAULT_EL2 ((TCR_EL2_TG0 | TCR_EL2_T0SZ | TCR_EL2_PS | TCR_EL2_HA))

// Write this into DAIFSET to disable interrupts, into DAIFCLEAR to enable interrupts
#define DAIFMASK_INTERRUPTS ((0x3))

// All interrupts enabled (FIQ/ IRQ):
#define USER_SPSR ((0))

// M = EL1 | 64 bit | SP_ELn (use sp_el1)
#define KERNEL_SPSR ((0b0101))

#define SPSR_FIQ_MASK ((1ull << 6))
#define SPSR_IRQ_MASK ((1ull << 7))
#define USER_SPSR_NO_INTERRUPTS ((SPSR_FIQ_MASK | SPSR_IRQ_MASK))
#define KERNEL_SPSR_NO_INTERRUPTS ((KERNEL_SPSR | SPSR_FIQ_MASK | SPSR_IRQ_MASK))

// During bringup, all memory is device memory nGnRnE
#define MAIR_BRINGUP 0x0000000000000000ULL

// During runtime (after initializing paging), we map all memory with MAIR index 0
// (writeback mode, normal memory), and devices use index 1 (device memory, nGnRnE)
#define MAIR_RUNTIME 0x00000000000000FFULL

#endif // ARM_ASM_H
