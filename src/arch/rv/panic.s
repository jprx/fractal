.section .text
.global panic
.global test_panic
#include <arch/riscv_asm_defines.h>
#include <arch/riscv_asm_macros.h>

# panic(const char *reason, ...)
# This calls panic(saved_regs_t *r, const char *reason, ...) by
# saving regs to stack and swapping args around
panic:
	PUSH_ALL

	# Shuffle variadic args around
	mv a7, a6
	mv a6, a5
	mv a5, a4
	mv a4, a3
	mv a3, a2
	mv a2, a1
	mv a1, a0

	# New first arg is the saved regs
	mv a0, sp

	j _panic_internal
	# Does not return
	j .
