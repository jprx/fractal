.global fractal_start
.section .text.start

#ifdef BOARD_APPLE

#include <arch/arm_asm_defines.h>
#include <arch/arm_asm_macros.h>
#include <board/arm/apple/soc.h>

// SERIAL_PUTC(c)- print char 'c'
// Clobbers x21 and x22 (should be unused by other stuff during early bringup)
// ONLY TO BE USED DURING EMERGENCIES (eg. trap to EL2 exception handler)

.macro SERIAL_PUTC c
    MOV64 x21, APPLE_SERIAL_BASEADDR
    mov x22, \c
    str w22, [x21, APPLE_SERIAL_WRITE_PORT_OFFSET]
.endm

fractal_start:
    /* Disable interrupts */
    msr daifset, 0xF

    /* iBoot info */
    LEA64 x1, boot_args_ptr
    str x0, [x1]

    /* Switch to EL1, if we aren't already there */
    mrs x0, CurrentEL
    cmp x0, 4
    beq fractal_start_el1

fractal_switch_to_el1:
    /* Setup an exception table in EL2 before we leave,
     * so we can catch any exceptions that make their way up here */
    LEA64 x0, exception_el2
    msr vbar_el2, x0

    msr sctlr_el1, xzr

    /* HCR.RW = 1 (enable AARCH64 at EL1) */
    /* Seems like we can't turn off E2H nor RW on Apple boards */
    msr hcr_el2, xzr
    mrs x0, hcr_el2
    orr x0, x0, HCR_EL2_RW
    orr x0, x0, HCR_EL2_API
    orr x0, x0, HCR_EL2_APK
    msr hcr_el2, x0

    MOV64 x0, TCR_DEFAULT_EL2
    msr tcr_el2, x0

    MOV64 x0, MAIR_BRINGUP
    msr mair_el2, x0

    LEA64 x0, fractal_start_el1
    msr elr_el2, x0

    // I and F set, prev mode is EL1h
    MOV64 x0, 0x65
    msr spsr_el2, x0
    eret

    // Hopefully we never get here:
    b .

fractal_start_el1:
    // REALLY make sure interrupts are off
    msr daifset, 0xF

    // Setup bringup stack to use the main boot stack,
    // just not in the higher half address space (eg.
    // subtracting KERNEL_LINK_ADDRESS)
    mov x0, 1
    msr SPSel, x0
    LEA64 fp, _bringup_stack_bottom
    mov sp, fp

configure_regs:
    /* Enable userspace access to a bunch of fun things */
    mrs x0, sctlr_el1
    orr x0, x0, SCTLR_EL1_UCI
    orr x0, x0, SCTLR_EL1_UCT
    orr x0, x0, SCTLR_EL1_DZE
    /* Apple Silicon doesn't seem to care whether these bits are set */
    /* (I ran into icache bugs when running new programs and the V->P) */
    /* (mappings were unchanged. Solved with ic iallu) */
    /* But let's make sure they're set, anyways */
    orr x0, x0, SCTLR_EL1_ICACHE
    orr x0, x0, SCTLR_EL1_DCACHE
    msr sctlr_el1, x0

    /* Turn on NEON / floating point */
    /* https://developer.arm.com/documentation/ddi0601/2020-12/AArch64-Registers/CPACR-EL1--Architectural-Feature-Access-Control-Register?lang=en */
    mov x0, #(0x3 << 20)
    msr cpacr_el1, x0

init_bss:
    LEA64 x0, _bss_start
    LEA64 x1, _bss_end
init_bss_loop:
    cmp x0, x1
    beq init_bss_done
    str xzr, [x0], #0
    add x0, x0, #8
    b init_bss_loop
init_bss_done:

enable_paging:
    /* To support being fully relocatable,
     * we need to construct the page tables at runtime.
     * apple_board_mk_pagetable is a C function
     * that will be placed in .text.start, near us,
     * so we can call it before jumping to higher-half.
     */
    LEA64 x0, apple_bringup_pagetable
    LEA64 x1, fractal_start
    bl apple_board_mk_pagetable

    LEA64 x0, apple_bringup_pagetable
    msr ttbr0_el1, x0
    msr ttbr1_el1, x0

    MOV64 x0, MAIR_BRINGUP
    msr mair_el1, x0

    MOV64 x0, TCR_DEFAULT_EL1
    msr tcr_el1, x0
    isb

    mrs x0, sctlr_el1
    orr x0, x0, SCTLR_EL1_MMU
    msr sctlr_el1, x0

    /* Flush TLB */
    dsb ishst
    tlbi vmalle1
    dsb ish
    isb

    /* Paging is now enabled */
handle_relocs:
    /* We need to cleanup all relocations sometime before
     * we get to C++ land, so let's just do it now */
    LEA64 x0, __very_start_of_kernel_mem
    LEA64 x1, _relocs_begin
    LEA64 x2, _relocs_end
    bl arm_handle_relocs

jump_to_start64:
    LEA64 x0, _fractal_start64_lma
    mov x1, KERNEL_LINK_ADDRESS
    add x0, x1, x0
    LEA64 x27, boot_args_ptr
    ldr x27, [x27]
    LEA64 x26, fractal_start
    br x0

wfi_forever:
    b wfi_forever

.align 3
boot_args_ptr:
    .quad 0

.align 14
exception_el2:
    .rept 8192
    nop
    .endr
    SERIAL_PUTC 'E'
    SERIAL_PUTC 'L'
    SERIAL_PUTC '2'
    SERIAL_PUTC '\n'
    b .

.macro DECLARE_4K_PAGE
    .rept 512
    .quad 0
    .endr
.endm

.align 12
apple_bringup_pagetable:
    .rept 16
    DECLARE_4K_PAGE
    .endr

#endif // BOARD_APPLE
