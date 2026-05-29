.global fractal_start
.section .text.start

// start_default.s: start for all non-Apple platforms
// Apple Silicon is kinda weird, so we use a separate file to deal with it

#ifndef BOARD_APPLE

#include <arch/arm_asm_defines.h>
#include <arch/arm_asm_macros.h>

.global fractal_start
.section .text.start

fractal_start:
    // Make sure interrupts are off
    msr daifset, 0xF

    // Need to switch EL2 -> EL1 on RPi
    // Check if we are in EL1 already
    mrs x0, CurrentEL
    cmp x0, 4
    beq fractal_start_el1

fractal_switch_el2_to_el1:
    msr sctlr_el1, xzr
    msr hcr_el2, xzr

    // HCR_EL2.RW needs to be 1 for AARCH64 mode in EL1
    // (See PacmanOS)
    mrs x0, hcr_el2
    bic x0, x0, #1
    orr x0, x0, #(1 << 31)
    msr hcr_el2, x0
    adr x0, fractal_start_el1
    msr elr_el2, x0

    // I and F set, EL1h was prev mode
    // Should this be 0xC5 instead of 0x65?
    MOV64 x0, 0x65
    msr spsr_el2, x0
    eret

    // Hopefully we never get here:
    b .

fractal_start_el1:
    // Make sure interrupts are off
    msr daifset, 0xF

    // Setup bringup stack to use the main boot stack,
    // just not in the higher half address space (eg.
    // subtracting KERNEL_LINK_ADDRESS)
    mov x0, 1
    msr SPSel, x0
    LEA64 fp, _bringup_stack_bottom
    mov sp, fp

configure_regs:
    adr x0, exception_table
    msr vbar_el1, x0
    isb

    /* Enable userspace access to a bunch of fun things */
    mrs x0, sctlr_el1
    orr x0, x0, SCTLR_EL1_UCI
    orr x0, x0, SCTLR_EL1_UCT
    orr x0, x0, SCTLR_EL1_DZE
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
    // Non-Apple architectures aren't relocatable (because
    // they don't need to be)- so we can hardcode the
    // bootstrap page tables (plus assume 1GB pages exist)
    adr x0, default_page_table
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
jump_to_start64:
    adrp x0, _fractal_start64_lma
    mov x1, KERNEL_LINK_ADDRESS
    add x0, x1, x0
    br x0

wfi_forever:
    b wfi_forever

.align 16
exception_table:
    .rept 8192
    b .
    .endr

# /* Configure 4K paging */
# /* T0SZ = 17 */
# /* T1SZ = 17 */
# /* TG0 = 10 (16KB) */
# /* TG1 = 01 (16KB) */

/* 4k paging, identity map first 512GB all over address space */
/* So that each 512GB virtual memory region maps to the first 512GB of physical memory */
/* Recall the kernel is loaded at 1GB (aka the second GB in phys mem) */
/* !!! YOU NEED TO SET ACCESS FLAG (bit 10) TO 1, otherwise every access will fault !!! */
.align 12
default_page_table:
    .rept 2048
    .quad (l2_page_table + 3 + 0x400)
    .endr

// Each entry has AP 00 (no access at EL0, RWX at EL1)
// Lower bits of 01 means block descriptor (aka stop here)
// Annoyingly ARM and X86 order the page tables in reverse numeric orders
// 1GB = (1 << 30)
// Or this with 1 to mark as present / table descriptor
/*
for i in range(512):
    print(hex((i << 30) | 1))
*/

.align 12
l2_page_table:
.quad 0x401
.quad 0x40000401
.quad 0x80000401
.quad 0xc0000401
.quad 0x100000401
.quad 0x140000401
.quad 0x180000401
.quad 0x1c0000401
.quad 0x200000401
.quad 0x240000401
.quad 0x280000401
.quad 0x2c0000401
.quad 0x300000401
.quad 0x340000401
.quad 0x380000401
.quad 0x3c0000401
.quad 0x400000401
.quad 0x440000401
.quad 0x480000401
.quad 0x4c0000401
.quad 0x500000401
.quad 0x540000401
.quad 0x580000401
.quad 0x5c0000401
.quad 0x600000401
.quad 0x640000401
.quad 0x680000401
.quad 0x6c0000401
.quad 0x700000401
.quad 0x740000401
.quad 0x780000401
.quad 0x7c0000401
.quad 0x800000401
.quad 0x840000401
.quad 0x880000401
.quad 0x8c0000401
.quad 0x900000401
.quad 0x940000401
.quad 0x980000401
.quad 0x9c0000401
.quad 0xa00000401
.quad 0xa40000401
.quad 0xa80000401
.quad 0xac0000401
.quad 0xb00000401
.quad 0xb40000401
.quad 0xb80000401
.quad 0xbc0000401
.quad 0xc00000401
.quad 0xc40000401
.quad 0xc80000401
.quad 0xcc0000401
.quad 0xd00000401
.quad 0xd40000401
.quad 0xd80000401
.quad 0xdc0000401
.quad 0xe00000401
.quad 0xe40000401
.quad 0xe80000401
.quad 0xec0000401
.quad 0xf00000401
.quad 0xf40000401
.quad 0xf80000401
.quad 0xfc0000401
.quad 0x1000000401
.quad 0x1040000401
.quad 0x1080000401
.quad 0x10c0000401
.quad 0x1100000401
.quad 0x1140000401
.quad 0x1180000401
.quad 0x11c0000401
.quad 0x1200000401
.quad 0x1240000401
.quad 0x1280000401
.quad 0x12c0000401
.quad 0x1300000401
.quad 0x1340000401
.quad 0x1380000401
.quad 0x13c0000401
.quad 0x1400000401
.quad 0x1440000401
.quad 0x1480000401
.quad 0x14c0000401
.quad 0x1500000401
.quad 0x1540000401
.quad 0x1580000401
.quad 0x15c0000401
.quad 0x1600000401
.quad 0x1640000401
.quad 0x1680000401
.quad 0x16c0000401
.quad 0x1700000401
.quad 0x1740000401
.quad 0x1780000401
.quad 0x17c0000401
.quad 0x1800000401
.quad 0x1840000401
.quad 0x1880000401
.quad 0x18c0000401
.quad 0x1900000401
.quad 0x1940000401
.quad 0x1980000401
.quad 0x19c0000401
.quad 0x1a00000401
.quad 0x1a40000401
.quad 0x1a80000401
.quad 0x1ac0000401
.quad 0x1b00000401
.quad 0x1b40000401
.quad 0x1b80000401
.quad 0x1bc0000401
.quad 0x1c00000401
.quad 0x1c40000401
.quad 0x1c80000401
.quad 0x1cc0000401
.quad 0x1d00000401
.quad 0x1d40000401
.quad 0x1d80000401
.quad 0x1dc0000401
.quad 0x1e00000401
.quad 0x1e40000401
.quad 0x1e80000401
.quad 0x1ec0000401
.quad 0x1f00000401
.quad 0x1f40000401
.quad 0x1f80000401
.quad 0x1fc0000401
.quad 0x2000000401
.quad 0x2040000401
.quad 0x2080000401
.quad 0x20c0000401
.quad 0x2100000401
.quad 0x2140000401
.quad 0x2180000401
.quad 0x21c0000401
.quad 0x2200000401
.quad 0x2240000401
.quad 0x2280000401
.quad 0x22c0000401
.quad 0x2300000401
.quad 0x2340000401
.quad 0x2380000401
.quad 0x23c0000401
.quad 0x2400000401
.quad 0x2440000401
.quad 0x2480000401
.quad 0x24c0000401
.quad 0x2500000401
.quad 0x2540000401
.quad 0x2580000401
.quad 0x25c0000401
.quad 0x2600000401
.quad 0x2640000401
.quad 0x2680000401
.quad 0x26c0000401
.quad 0x2700000401
.quad 0x2740000401
.quad 0x2780000401
.quad 0x27c0000401
.quad 0x2800000401
.quad 0x2840000401
.quad 0x2880000401
.quad 0x28c0000401
.quad 0x2900000401
.quad 0x2940000401
.quad 0x2980000401
.quad 0x29c0000401
.quad 0x2a00000401
.quad 0x2a40000401
.quad 0x2a80000401
.quad 0x2ac0000401
.quad 0x2b00000401
.quad 0x2b40000401
.quad 0x2b80000401
.quad 0x2bc0000401
.quad 0x2c00000401
.quad 0x2c40000401
.quad 0x2c80000401
.quad 0x2cc0000401
.quad 0x2d00000401
.quad 0x2d40000401
.quad 0x2d80000401
.quad 0x2dc0000401
.quad 0x2e00000401
.quad 0x2e40000401
.quad 0x2e80000401
.quad 0x2ec0000401
.quad 0x2f00000401
.quad 0x2f40000401
.quad 0x2f80000401
.quad 0x2fc0000401
.quad 0x3000000401
.quad 0x3040000401
.quad 0x3080000401
.quad 0x30c0000401
.quad 0x3100000401
.quad 0x3140000401
.quad 0x3180000401
.quad 0x31c0000401
.quad 0x3200000401
.quad 0x3240000401
.quad 0x3280000401
.quad 0x32c0000401
.quad 0x3300000401
.quad 0x3340000401
.quad 0x3380000401
.quad 0x33c0000401
.quad 0x3400000401
.quad 0x3440000401
.quad 0x3480000401
.quad 0x34c0000401
.quad 0x3500000401
.quad 0x3540000401
.quad 0x3580000401
.quad 0x35c0000401
.quad 0x3600000401
.quad 0x3640000401
.quad 0x3680000401
.quad 0x36c0000401
.quad 0x3700000401
.quad 0x3740000401
.quad 0x3780000401
.quad 0x37c0000401
.quad 0x3800000401
.quad 0x3840000401
.quad 0x3880000401
.quad 0x38c0000401
.quad 0x3900000401
.quad 0x3940000401
.quad 0x3980000401
.quad 0x39c0000401
.quad 0x3a00000401
.quad 0x3a40000401
.quad 0x3a80000401
.quad 0x3ac0000401
.quad 0x3b00000401
.quad 0x3b40000401
.quad 0x3b80000401
.quad 0x3bc0000401
.quad 0x3c00000401
.quad 0x3c40000401
.quad 0x3c80000401
.quad 0x3cc0000401
.quad 0x3d00000401
.quad 0x3d40000401
.quad 0x3d80000401
.quad 0x3dc0000401
.quad 0x3e00000401
.quad 0x3e40000401
.quad 0x3e80000401
.quad 0x3ec0000401
.quad 0x3f00000401
.quad 0x3f40000401
.quad 0x3f80000401
.quad 0x3fc0000401
.quad 0x4000000401
.quad 0x4040000401
.quad 0x4080000401
.quad 0x40c0000401
.quad 0x4100000401
.quad 0x4140000401
.quad 0x4180000401
.quad 0x41c0000401
.quad 0x4200000401
.quad 0x4240000401
.quad 0x4280000401
.quad 0x42c0000401
.quad 0x4300000401
.quad 0x4340000401
.quad 0x4380000401
.quad 0x43c0000401
.quad 0x4400000401
.quad 0x4440000401
.quad 0x4480000401
.quad 0x44c0000401
.quad 0x4500000401
.quad 0x4540000401
.quad 0x4580000401
.quad 0x45c0000401
.quad 0x4600000401
.quad 0x4640000401
.quad 0x4680000401
.quad 0x46c0000401
.quad 0x4700000401
.quad 0x4740000401
.quad 0x4780000401
.quad 0x47c0000401
.quad 0x4800000401
.quad 0x4840000401
.quad 0x4880000401
.quad 0x48c0000401
.quad 0x4900000401
.quad 0x4940000401
.quad 0x4980000401
.quad 0x49c0000401
.quad 0x4a00000401
.quad 0x4a40000401
.quad 0x4a80000401
.quad 0x4ac0000401
.quad 0x4b00000401
.quad 0x4b40000401
.quad 0x4b80000401
.quad 0x4bc0000401
.quad 0x4c00000401
.quad 0x4c40000401
.quad 0x4c80000401
.quad 0x4cc0000401
.quad 0x4d00000401
.quad 0x4d40000401
.quad 0x4d80000401
.quad 0x4dc0000401
.quad 0x4e00000401
.quad 0x4e40000401
.quad 0x4e80000401
.quad 0x4ec0000401
.quad 0x4f00000401
.quad 0x4f40000401
.quad 0x4f80000401
.quad 0x4fc0000401
.quad 0x5000000401
.quad 0x5040000401
.quad 0x5080000401
.quad 0x50c0000401
.quad 0x5100000401
.quad 0x5140000401
.quad 0x5180000401
.quad 0x51c0000401
.quad 0x5200000401
.quad 0x5240000401
.quad 0x5280000401
.quad 0x52c0000401
.quad 0x5300000401
.quad 0x5340000401
.quad 0x5380000401
.quad 0x53c0000401
.quad 0x5400000401
.quad 0x5440000401
.quad 0x5480000401
.quad 0x54c0000401
.quad 0x5500000401
.quad 0x5540000401
.quad 0x5580000401
.quad 0x55c0000401
.quad 0x5600000401
.quad 0x5640000401
.quad 0x5680000401
.quad 0x56c0000401
.quad 0x5700000401
.quad 0x5740000401
.quad 0x5780000401
.quad 0x57c0000401
.quad 0x5800000401
.quad 0x5840000401
.quad 0x5880000401
.quad 0x58c0000401
.quad 0x5900000401
.quad 0x5940000401
.quad 0x5980000401
.quad 0x59c0000401
.quad 0x5a00000401
.quad 0x5a40000401
.quad 0x5a80000401
.quad 0x5ac0000401
.quad 0x5b00000401
.quad 0x5b40000401
.quad 0x5b80000401
.quad 0x5bc0000401
.quad 0x5c00000401
.quad 0x5c40000401
.quad 0x5c80000401
.quad 0x5cc0000401
.quad 0x5d00000401
.quad 0x5d40000401
.quad 0x5d80000401
.quad 0x5dc0000401
.quad 0x5e00000401
.quad 0x5e40000401
.quad 0x5e80000401
.quad 0x5ec0000401
.quad 0x5f00000401
.quad 0x5f40000401
.quad 0x5f80000401
.quad 0x5fc0000401
.quad 0x6000000401
.quad 0x6040000401
.quad 0x6080000401
.quad 0x60c0000401
.quad 0x6100000401
.quad 0x6140000401
.quad 0x6180000401
.quad 0x61c0000401
.quad 0x6200000401
.quad 0x6240000401
.quad 0x6280000401
.quad 0x62c0000401
.quad 0x6300000401
.quad 0x6340000401
.quad 0x6380000401
.quad 0x63c0000401
.quad 0x6400000401
.quad 0x6440000401
.quad 0x6480000401
.quad 0x64c0000401
.quad 0x6500000401
.quad 0x6540000401
.quad 0x6580000401
.quad 0x65c0000401
.quad 0x6600000401
.quad 0x6640000401
.quad 0x6680000401
.quad 0x66c0000401
.quad 0x6700000401
.quad 0x6740000401
.quad 0x6780000401
.quad 0x67c0000401
.quad 0x6800000401
.quad 0x6840000401
.quad 0x6880000401
.quad 0x68c0000401
.quad 0x6900000401
.quad 0x6940000401
.quad 0x6980000401
.quad 0x69c0000401
.quad 0x6a00000401
.quad 0x6a40000401
.quad 0x6a80000401
.quad 0x6ac0000401
.quad 0x6b00000401
.quad 0x6b40000401
.quad 0x6b80000401
.quad 0x6bc0000401
.quad 0x6c00000401
.quad 0x6c40000401
.quad 0x6c80000401
.quad 0x6cc0000401
.quad 0x6d00000401
.quad 0x6d40000401
.quad 0x6d80000401
.quad 0x6dc0000401
.quad 0x6e00000401
.quad 0x6e40000401
.quad 0x6e80000401
.quad 0x6ec0000401
.quad 0x6f00000401
.quad 0x6f40000401
.quad 0x6f80000401
.quad 0x6fc0000401
.quad 0x7000000401
.quad 0x7040000401
.quad 0x7080000401
.quad 0x70c0000401
.quad 0x7100000401
.quad 0x7140000401
.quad 0x7180000401
.quad 0x71c0000401
.quad 0x7200000401
.quad 0x7240000401
.quad 0x7280000401
.quad 0x72c0000401
.quad 0x7300000401
.quad 0x7340000401
.quad 0x7380000401
.quad 0x73c0000401
.quad 0x7400000401
.quad 0x7440000401
.quad 0x7480000401
.quad 0x74c0000401
.quad 0x7500000401
.quad 0x7540000401
.quad 0x7580000401
.quad 0x75c0000401
.quad 0x7600000401
.quad 0x7640000401
.quad 0x7680000401
.quad 0x76c0000401
.quad 0x7700000401
.quad 0x7740000401
.quad 0x7780000401
.quad 0x77c0000401
.quad 0x7800000401
.quad 0x7840000401
.quad 0x7880000401
.quad 0x78c0000401
.quad 0x7900000401
.quad 0x7940000401
.quad 0x7980000401
.quad 0x79c0000401
.quad 0x7a00000401
.quad 0x7a40000401
.quad 0x7a80000401
.quad 0x7ac0000401
.quad 0x7b00000401
.quad 0x7b40000401
.quad 0x7b80000401
.quad 0x7bc0000401
.quad 0x7c00000401
.quad 0x7c40000401
.quad 0x7c80000401
.quad 0x7cc0000401
.quad 0x7d00000401
.quad 0x7d40000401
.quad 0x7d80000401
.quad 0x7dc0000401
.quad 0x7e00000401
.quad 0x7e40000401
.quad 0x7e80000401
.quad 0x7ec0000401
.quad 0x7f00000401
.quad 0x7f40000401
.quad 0x7f80000401
.quad 0x7fc0000401

#endif // ! BOARD_APPLE
