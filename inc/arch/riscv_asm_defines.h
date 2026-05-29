#ifndef RV_ASM_H
#define RV_ASM_H

#include <gen/rv_asm_offsets.h>

#define ARCH_HAS_FAST_MEMCPY
#define ARCH_HAS_FAST_MEMSET

#define read_csr(r) ({ u64 ___tmp_csr_val; \
    asm volatile ("csrr %0, " #r : "=r"(___tmp_csr_val)); \
    ___tmp_csr_val; })

#define write_csr(r,v) ({asm volatile("csrw " #r ", %0" :: "rK"(v));})

#define KERNEL_LINK_ADDRESS ((0xFFFF800000000000))
#define INITRD_EXPECTED_ADDRESS ((0xFFFF8000A0000000))

#define TIMESLICE_QUANTUM ((100000))

#define RISCV_NUM_FPREGS ((32))
#define RISCV_FPREG_SZ   ((8))

#define CLINT_BASEADDR          ((0x2000000))
#define CLINT_MTIMECMP_OFFSET   ((0x4000))
#define CLINT_MTIME_OFFSET      ((0xBFF8))
#define CLINT_MTIME             ((CLINT_BASEADDR + CLINT_MTIME_OFFSET))
#define CLINT_MTIMECMP_BASE     ((CLINT_BASEADDR + CLINT_MTIMECMP_OFFSET))
#define CLINT_MTIMECMP_PERCORE_OFFSET ((8))
#define CLINT_MTIMECMP_PERCORE_SHIFT  ((3))

/* mstatus fields */
#define RV_MODE_M           ((0b11))
#define RV_MODE_S           ((0b01))
#define RV_MODE_U           ((0b00))
#define MSTATUS_MPP_SHIFT   ((11))
#define MSTATUS_UXLEN_SHIFT ((32ull))
#define MSTATUS_SXLEN_SHIFT ((34ull))

/* sstatus fields */
#define RV_XLEN_64           ((0b10ull))
#define SSTATUS_UXLEN_SHIFT  ((32))
#define SSTATUS_SIE          ((2))
#define SSTATUS_SPIE         ((1ull << 5))
#define SSTATUS_SPP_SHIFT    ((8))

#define SPP_UMODE            ((0))
#define SPP_SMODE            ((1))

#define SSTATUS_SUM_ON       ((1ull << 18))
#define SSTATUS_FLOAT_ENABLE ((1 << 13))

/* Physical memory protection (pmp) configuration bits */
#define PMP_R_BIT       ((0b001))
#define PMP_W_BIT       ((0b010))
#define PMP_X_BIT       ((0b100))
#define PMP_A_SHIFT     ((3))
#define PMP_A_OFF_VAL   ((0b00))
#define PMP_A_NAPOT_VAL ((0b11))
#define PMP_A_OFF       ((PMP_A_OFF_VAL << PMP_A_SHIFT))
#define PMP_A_NAPOT     ((PMP_A_NAPOT_VAL << PMP_A_SHIFT))

#define PMP_ADDR_ALL    ((-1))

#define SATP_PPN_PAGE_SHIFT ((12ull))

#define RV_PAGING_MODE_DISABLED ((0ull))
#define RV_PAGING_MODE_SV39     ((8ull))
#define RV_PAGING_MODE_SV48     ((9ull))
#define RV_PAGING_MODE_SV57     ((10ull))
#define RV_PAGING_MODE_SV64     ((11ull))
#define RV_PAGING_MODE_SHIFT    ((60ull))

#define RV_SATP_GET_PPN_MASK    (((1ull << 22ull) - 1))

/* Machine-mode interrupt delegation registers */
#define MEDELEG_DISABLED ((0xFFFFFFFFFFFFFFFF))
#define MIDELEG_DISABLED ((0xFFFFFFFFFFFFFFFF))

// There are 3 interrupt flavors known by the core:
// SEI == "(S-Mode) External Interrupt"
// STI == "(S-Mode) Timer Interrupt"
// SSI == "(S-Mode) Software Interrupt"

/* SIP */
/* |15   10|   9  |8   6|  5   |4   2|  1   | 0 |
 * +--------------------------------------------+
 * |   0   | SEIP |  0  | STIP |  0  | SSIP | 0 |
 * +--------------------------------------------+
 */

/* SIE */
/* |15   10|   9  |8   6|  5   |4   2|  1   | 0 |
 * +--------------------------------------------+
 * |   0   | SEIE |  0  | STIE |  0  | SSIE | 0 |
 * +--------------------------------------------+
 */

/* MIP */
/* |15   10|   9  |8   6|  5   |4   2|  1   | 0 |
 * +--------------------------------------------+
 * |   0   | SEIP |  0  | STIP |  0  | SSIP | 0 |
 * +--------------------------------------------+
 */

#define MIE_MTIE ((1ull << 7ull))
#define MIP_MTIP ((1ull << 7ull))
#define SIE_STIE ((1ull << 5ull))
#define SIP_STIP ((1ull << 5ull))
#define SIE_SSIE ((1ull << 1ull))
#define SIP_SSIP ((1ull << 1ull))
#define SIE_SEIE ((1ull << 9ull))
#define SIP_SEIP ((1ull << 9ull))
#define MCAUSE_MTIMER_INTERRUPT ((0x8000000000000007ull))

#define SIE_ENABLE_ALL ((SIE_SSIE | SIE_SEIE))
#define SIE_DISABLE_ALL ((0))

#define USER_SSTATUS (((RV_XLEN_64 << MSTATUS_UXLEN_SHIFT) | (SPP_UMODE << SSTATUS_SPP_SHIFT) | (SSTATUS_SPIE) | (SSTATUS_SUM_ON) | (SSTATUS_FLOAT_ENABLE)))
#define KERNEL_SSTATUS (((RV_XLEN_64 << MSTATUS_UXLEN_SHIFT) | (SPP_SMODE << SSTATUS_SPP_SHIFT) | (SSTATUS_SPIE) | (SSTATUS_SUM_ON) | (SSTATUS_FLOAT_ENABLE)))
#define KERNEL_SSTATUS_NO_INTERRUPTS (((RV_XLEN_64 << MSTATUS_UXLEN_SHIFT) | (SPP_SMODE << SSTATUS_SPP_SHIFT) | (SSTATUS_SUM_ON) | (SSTATUS_FLOAT_ENABLE)))

#endif // RV_ASM_H
