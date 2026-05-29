#ifndef FRACTAL_ARM
#error "Including ARM crt0.S without being the FRACTAL_ARM target"
#endif // !FRACTAL_ARM

// PUSH(r)- push register r to the stack
.macro PUSH r
	add sp, sp, -8
	str \r, [sp]
.endm

// POP(r)- pop register from the stack
.macro POP r
	ldr \r, [sp]
	add sp, sp, 8
.endm

.section .text.startup
.global _start
_start:
    // Save argc, argv, and envp
    PUSH x0
    PUSH x1
    PUSH x2

    // Copy argv[0] into __progname pointer
    // (see: https://sourceware.org/binutils/docs-2.30/as/AArch64_002dRelocations.html)
    adrp x10, __progname
    add x10, x10, :lo12:__progname
    ldr x11, [x1]
    str x11, [x10]

    // Copy envp into environ, if provided
    cbz x2, skip_envp_cpy
    adrp x10, environ
    add x10, x10, :lo12:environ
    str x2, [x10]

    // Setup constructors
    bl __libc_init_array

    // Reset clobbered regs
    skip_envp_cpy:
    eor x10, x10, x10
    eor x11, x11, x11
    POP x2
    POP x1
    POP x0

    bl main
    mov x1, x0
    mov x0, #2
    svc #0
    b .

.global do_syscall
do_syscall:
    svc #0
    ret
