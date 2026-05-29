
#include <types.h>
#include <arch/riscv_asm_defines.h>

/*
 * Configure the CLINT and various CSRs to take timer interrupts
 *
 * This is part of early bringup and happens before the jump to higher-half kernel code.
 * Thus we put this function in the special .text.start_c section, just after .text.start.
 *
 * In the future it would be cool to move more of the bringup code out of ASM and into C like this.
 */
void __attribute__((section(".text.start_c"))) setup_m_mode_timer_interrupts() {
    u64 my_id = read_csr(mhartid);

    u64 *mtime = (u64 *)(CLINT_MTIME);
    u64 *mtimecmp = (u64 *)(CLINT_MTIMECMP_BASE + (my_id * CLINT_MTIMECMP_PERCORE_OFFSET));

    *mtimecmp = *mtime + TIMESLICE_QUANTUM;

    write_csr(mie, read_csr(mie) | MIE_MTIE);
}
