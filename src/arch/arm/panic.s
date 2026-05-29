.section .text
.global panic
.global test_panic
#include <arch/arm_asm_defines.h>
#include <arch/arm_asm_macros.h>

#define STACK_BYTES_ALLOCD (ARM_REGS_SIZE+8)

# panic(const char *reason, ...)
# This calls panic(saved_regs_t *r, const char *reason, ...) by
# saving regs to stack and swapping args around
panic:
	PUSH_ALL

	# Calculate original stack and store it, cloberring x7, which
	# is about to get overwritten, anyways
	add x7, sp, (STACK_BYTES_ALLOCD)
	str x7, [sp,ARM_REGS_SP_EL1]

	# Shuffle variadic args around
	mov x7, x6
	mov x6, x5
	mov x5, x4
	mov x4, x3
	mov x3, x2
	mov x2, x1
	mov x1, x0

	# New first arg is the saved regs
	mov x0, sp
	bl _panic_internal

