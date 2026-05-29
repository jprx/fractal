.global fractal_start
.global bringup_stack_bottom
.section .text.start

#include <arch/riscv_asm_defines.h>
#include <arch/riscv_asm_macros.h>

/* Core begins in machine mode */
fractal_start:
	la t0, machine_exception
	csrw mtvec, t0
	la t0, supervisor_invalid_trap
	csrw stvec, t0
	la sp, bringup_stack_bottom
	mv fp, sp
	la ra, loop_forever

disable_pmp:
	/* PMP needs to grant access to S mode in order for it to actually run */
	/* Ran into this by debugging Qemu on mret and breaking in handle_mret,
	 * saw exceptions being generated related to pmp */

	/* RWX enabled, A=NAPOT, L=0 (unlocked) */
	li t0, PMP_R_BIT | PMP_W_BIT | PMP_X_BIT | PMP_A_NAPOT
	csrw pmpcfg0, t0
	li t0, PMP_ADDR_ALL
	csrw pmpaddr0, t0

reroute_interrupts:
	// Reroute all interrupts to supervisor mode
	// by setting 1s to everything in mdeleg / mideleg
	li t0, MEDELEG_DISABLED
	csrw medeleg, t0
	li t0, MIDELEG_DISABLED
	csrw mideleg, t0
	call setup_m_mode_timer_interrupts

enable_paging:
	/* Sv48 mode */
	/* This becomes active when we switch to supervisor mode */
	la t0, default_page_table
	srli t0, t0, SATP_PPN_PAGE_SHIFT
	li t1, ((RV_PAGING_MODE_SV48 << RV_PAGING_MODE_SHIFT))
	or t0, t1, t0
	csrw satp, t0
	sfence.vma x0

switch_to_s_mode:
	la t0, enter_supervisor
	csrw mepc, t0
	csrr t0, mstatus
	li t1, RV_MODE_S << MSTATUS_MPP_SHIFT
	or t0, t0, t1
	li t1, RV_XLEN_64 << MSTATUS_UXLEN_SHIFT
	or t0, t0, t1
	li t1, RV_XLEN_64 << MSTATUS_SXLEN_SHIFT
	or t0, t0, t1
	csrw mstatus, t0
	mret

enter_supervisor:
	// Construct sstatus
	mv t0, x0
	// sstatus.UXL = XLEN 64:
	li t1, RV_XLEN_64 << SSTATUS_UXLEN_SHIFT
	or t0, t0, t1
	li t1, SSTATUS_SUM_ON
	or t0, t0, t1
	// sstatus.UBE = 0
	// sstatus.MXR = 0
	// sstatus.XS is read-only
	// sstatus.SD is read-only
	// sstatus.FS (floating-point state) = 0 (unused)
	// sstatus.VS (other extension state) = 0 (unused)
	// sstatus.SPP = 0
	// sstatus.SPIE = 0
	// sstatus.SIE = 0 (interrupts disabled)
	csrw sstatus, t0

	// Calculate base address of fractal_start64 and jump there
	la t0, _fractal_start64_lma
	li t1, KERNEL_LINK_ADDRESS
	add t0, t1, t0
	jr t0

// Unlucky, we hit a trap before fractal setup a trap vector
.align 4
supervisor_invalid_trap:
	j .

loop_forever:
	j loop_forever

// @TODO: Make this per-core
// We use this stack for bringup and for handling M-mode interrupts
bringup_stack_top:
    .fill 256, 8, 0
bringup_stack_bottom:
    .fill 1, 8, 0

.align 20
// Let's just use only 512GB Terapages for now
default_page_table:
	.rept 512
	.quad 0xCF
	.endr
