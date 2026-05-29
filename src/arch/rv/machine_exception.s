.section .text.start
.global machine_exception

#include <arch/riscv_asm_defines.h>
#include <arch/riscv_asm_macros.h>

.align 4
machine_exception:
	// If this is a timer interrupt, pass it along to s-mode
	// Otherwise, "panic"
	// Use bringup stack (@TODO: make this per-core) for handling interrupt
	// This is written in ASM to reduce number of regs that need to be saved so it goes very fast (tm)
	// Also need to link this in .text.start so it is easy to find for physical memory locations
	// (eg. not "loaded" in the higher half address space)
	csrw mscratch, sp
	la sp, bringup_stack_bottom
	PUSH t0
	PUSH t1
	PUSH t2

	// Confirm we indeed received a timer interrupt
	csrr t0, mcause
	li t1, MCAUSE_MTIMER_INTERRUPT
	bne t0, t1, machine_invalid_interrupt

	// Schedule the next timer interrupt by setting mtime back to zero
	li t1, CLINT_MTIME
	sd x0, 0(t1)
	li t1, CLINT_MTIMECMP_BASE
	li t2, TIMESLICE_QUANTUM
	sd t2, 0(t1)

	// Set the supervisor timer bit to schedule an interrupt in supervisor mode
	// From testing (trying to write 0xFFFF) we can set sip to 0x2002- aka
	// set the external interrupt or software interrupt bit, but not the timer
	// interrupt bit (which would be bit 5)
	li t1, SIP_SSIP
	csrw sip, t1

	// Clear M-mode interrupt
	li t1, MIP_MTIP
	csrc mip, t1

	POP t2
	POP t1
	POP t0
	csrr sp, mscratch
	mret

machine_invalid_interrupt:
	// Something... horrible happened
	j .
