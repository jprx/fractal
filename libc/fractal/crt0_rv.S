#ifndef FRACTAL_RISCV
#error "Including RISCV crt0.S without being the FRACTAL_RISCV target"
#endif // !FRACTAL_RISCV

// PUSH(r)- push register r to the stack
.macro PUSH r
	addi sp, sp, -8
	sd \r, 0(sp)
.endm

// POP(r)- pop register from the stack
.macro POP r
	ld \r, 0(sp)
	addi sp, sp, 8
.endm

.section .text.startup
.global _start
_start:
    // Save argc, argv, and envp
    PUSH a0
    PUSH a1
    PUSH a2

    // Initialize gp using norelax, like libgloss.
    // This is required, if you just do la gp, __global_pointer$
    // the compiler will emit "mv gp, gp" instead
    // see: https://five-embeddev.com/quickref/global_pointer.html
.option push
.option norelax
1:auipc gp, %pcrel_hi(__global_pointer$)
  addi  gp, gp, %pcrel_lo(1b)
.option pop

    // Copy argv[0] into __progname pointer
    la t0, __progname
    ld t1, 0(a1)
    sd t1, 0(t0)

    // Copy envp into environ, if provided
    beq a2, zero, skip_envp_cpy
    la t0, environ
    sd a2, 0(t0)

    // Setup constructors
    call __libc_init_array

    // Reset clobbered regs
    skip_envp_cpy:
    xor t0, t0, t0
    xor t1, t1, t1
    POP a2
    POP a1
    POP a0

    call main
    mv a1, a0
    li a0, 0x02
    j do_syscall

.global do_syscall
do_syscall:
    ecall
    ret
