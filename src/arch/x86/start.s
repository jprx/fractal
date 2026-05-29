.intel_syntax noprefix
.global fractal_start
.section .text.start
#include <arch/x86_asm_defines.h>

// #define QUIET_BOOTLOADER

.code32

fractal_start:
    // ebx holds the multiboot info struct here
    lea esp, bringup_stack_bottom
#ifndef QUIET_BOOTLOADER
    lea edi, fractal_boot_str
    call bringup_puts
#else
    // Fix the serial console after SeaBIOS messed it up
    lea edi, serial_fixup_str
    call bringup_puts
#endif // !QUIET_BOOTLOADER

activate_long_mode:
    // Enable PAE
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    mov ecx, offset l4_page_table
    mov cr3, ecx

    // Enable long mode (this is not the same as activating it) using EFER
    // See AMD64 System Programmer's Manual Volume 2 page 470
    mov ecx, 0xC0000080
    rdmsr
    bts eax, 8
    wrmsr

    // Enable paging- this activates long mode
    mov eax, cr0
    bts eax, 31
    mov cr0, eax
    jmp .setup_segments

// We are now in IA-32e mode (long mode but 32 bit flavor)
// Need to configure 64 bit GDT and setup segment registers accordingly to get to 64-bit long mode
.setup_segments:
#ifndef QUIET_BOOTLOADER
    lea edi, jump_to_long_mode_str
    call bringup_puts
#endif // !QUIET_BOOTLOADER
    lgdt [gdt_ptr]
    mov ax, 0x18
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    // Need to use "ljmp segment, offset pointer" for far jumps
    // https://github.com/rust-lang/rust/issues/84676
    ljmp 0x0008, offset long_mode_entry

// puts(const char *str)
// put the string to call in edi
bringup_puts:
    push eax
    push ecx
    push edx
    mov dx, SERIAL_OUT_PORT
    mov ecx, edi
bringup_puts_loop:
    mov al, [ecx]
    cmp al, 0
    je bringup_puts_done
    outb dx, al
    lea ecx, [ecx+1]
    jmp bringup_puts_loop
bringup_puts_done:
    pop edx
    pop ecx
    pop eax
    ret

// Infinintely print 'A' to serial
infloop_serial:
    mov al, 'A'
    mov dx, SERIAL_OUT_PORT
infloop_serial_loop:
    outb dx, al
    lea ecx, [ecx+1]
    jmp infloop_serial_loop

.code64
.global long_mode_entry
.extern _fractal_start64_vma
long_mode_entry:
    // Set TCR to 0 (all interrupts allowed)
    mov rax, 0
    mov cr8, rax

    // Enable Legacy SSE (SSE 4.2, no AVX)
    mov rax, cr4
    or rax, CR4_OSFXSR
    mov cr4, rax

    // Calculate virtual address of fractal_start64 and jump to it
    // x86.ld exports _fractal_start64_lma which is the load address
    // of fractal_start64 (the beginning of the .text.start64 section).
    // We have copied the first 1GB of memory across every 1GB page in memory
    // so as long as the kernel is loaded in the first GB (it is, because of KERNEL_LOAD_ADDR)
    // we can jump to any copy of it anywhere. Let's use the one the kernel was linked at-
    // add KERNEL_LINK_ADDRESS to the load address to get it.
    lea rax, _fractal_start64_lma
    mov rcx, KERNEL_LINK_ADDRESS
    add rax, rcx
    jmp rax

    // Should never get here
    jmp .

// Emitting this byte will fixup newlines, because SeaBIOS
// messes up line endings (CRLF behavior). This resets it.
// See https://www2.ccs.neu.edu/research/gpc/VonaUtils/vona/terminal/vtansi.htm
serial_fixup_str:
    .string "\r\n\033[?7h"

fractal_boot_str:
    .string "\r\nFractal Bootloader\r\n"

jump_to_long_mode_str:
    .string "Jumping to long mode...\r\n"

bringup_stack_top:
    .fill 256, 8, 0
bringup_stack_bottom:
    .fill 1, 8, 0

// Default GDT
.align 8
gdt64:
// Null entry (offset 0)
    .quad 0

// Kernel CS (offset 0x08)
    .quad 0x0020980000000000 // L | P | DPL=0

// User CS (offset 0x10)
    .quad 0x0020F80000000000 // L | P | DPL=3

// Any DS (offset 0x18)
    .quad 0x0000920000000000 // P | W

// Empty selector for padding
    .quad 0

gdt64_end:
    .quad 0

gdt_ptr:
    .word (gdt64_end - gdt64 - 1)
    .quad gdt64

.align 4096
// Second-stage page table for use by C with proper device maps
.global secondstage_pagetable
secondstage_pagetable:
    .rept 512
    .quad 0
    .endr

// Default page table- map every 512GB to the first 512 GBs of physical space
.align 4096
l4_page_table:
    .rept 512
    .quad (l3_page_table + 3)
    .endr
    # .fill 512, 8, (l3_page_table + 3)

// Identity map the first 512 GB of the address space
.align 4096
l3_page_table:
    .quad 0x83
    .quad 0x40000083
    .quad 0x80000083
    .quad 0xc0000083
    .quad 0x100000083
    .quad 0x140000083
    .quad 0x180000083
    .quad 0x1c0000083
    .quad 0x200000083
    .quad 0x240000083
    .quad 0x280000083
    .quad 0x2c0000083
    .quad 0x300000083
    .quad 0x340000083
    .quad 0x380000083
    .quad 0x3c0000083
    .quad 0x400000083
    .quad 0x440000083
    .quad 0x480000083
    .quad 0x4c0000083
    .quad 0x500000083
    .quad 0x540000083
    .quad 0x580000083
    .quad 0x5c0000083
    .quad 0x600000083
    .quad 0x640000083
    .quad 0x680000083
    .quad 0x6c0000083
    .quad 0x700000083
    .quad 0x740000083
    .quad 0x780000083
    .quad 0x7c0000083
    .quad 0x800000083
    .quad 0x840000083
    .quad 0x880000083
    .quad 0x8c0000083
    .quad 0x900000083
    .quad 0x940000083
    .quad 0x980000083
    .quad 0x9c0000083
    .quad 0xa00000083
    .quad 0xa40000083
    .quad 0xa80000083
    .quad 0xac0000083
    .quad 0xb00000083
    .quad 0xb40000083
    .quad 0xb80000083
    .quad 0xbc0000083
    .quad 0xc00000083
    .quad 0xc40000083
    .quad 0xc80000083
    .quad 0xcc0000083
    .quad 0xd00000083
    .quad 0xd40000083
    .quad 0xd80000083
    .quad 0xdc0000083
    .quad 0xe00000083
    .quad 0xe40000083
    .quad 0xe80000083
    .quad 0xec0000083
    .quad 0xf00000083
    .quad 0xf40000083
    .quad 0xf80000083
    .quad 0xfc0000083
    .quad 0x1000000083
    .quad 0x1040000083
    .quad 0x1080000083
    .quad 0x10c0000083
    .quad 0x1100000083
    .quad 0x1140000083
    .quad 0x1180000083
    .quad 0x11c0000083
    .quad 0x1200000083
    .quad 0x1240000083
    .quad 0x1280000083
    .quad 0x12c0000083
    .quad 0x1300000083
    .quad 0x1340000083
    .quad 0x1380000083
    .quad 0x13c0000083
    .quad 0x1400000083
    .quad 0x1440000083
    .quad 0x1480000083
    .quad 0x14c0000083
    .quad 0x1500000083
    .quad 0x1540000083
    .quad 0x1580000083
    .quad 0x15c0000083
    .quad 0x1600000083
    .quad 0x1640000083
    .quad 0x1680000083
    .quad 0x16c0000083
    .quad 0x1700000083
    .quad 0x1740000083
    .quad 0x1780000083
    .quad 0x17c0000083
    .quad 0x1800000083
    .quad 0x1840000083
    .quad 0x1880000083
    .quad 0x18c0000083
    .quad 0x1900000083
    .quad 0x1940000083
    .quad 0x1980000083
    .quad 0x19c0000083
    .quad 0x1a00000083
    .quad 0x1a40000083
    .quad 0x1a80000083
    .quad 0x1ac0000083
    .quad 0x1b00000083
    .quad 0x1b40000083
    .quad 0x1b80000083
    .quad 0x1bc0000083
    .quad 0x1c00000083
    .quad 0x1c40000083
    .quad 0x1c80000083
    .quad 0x1cc0000083
    .quad 0x1d00000083
    .quad 0x1d40000083
    .quad 0x1d80000083
    .quad 0x1dc0000083
    .quad 0x1e00000083
    .quad 0x1e40000083
    .quad 0x1e80000083
    .quad 0x1ec0000083
    .quad 0x1f00000083
    .quad 0x1f40000083
    .quad 0x1f80000083
    .quad 0x1fc0000083
    .quad 0x2000000083
    .quad 0x2040000083
    .quad 0x2080000083
    .quad 0x20c0000083
    .quad 0x2100000083
    .quad 0x2140000083
    .quad 0x2180000083
    .quad 0x21c0000083
    .quad 0x2200000083
    .quad 0x2240000083
    .quad 0x2280000083
    .quad 0x22c0000083
    .quad 0x2300000083
    .quad 0x2340000083
    .quad 0x2380000083
    .quad 0x23c0000083
    .quad 0x2400000083
    .quad 0x2440000083
    .quad 0x2480000083
    .quad 0x24c0000083
    .quad 0x2500000083
    .quad 0x2540000083
    .quad 0x2580000083
    .quad 0x25c0000083
    .quad 0x2600000083
    .quad 0x2640000083
    .quad 0x2680000083
    .quad 0x26c0000083
    .quad 0x2700000083
    .quad 0x2740000083
    .quad 0x2780000083
    .quad 0x27c0000083
    .quad 0x2800000083
    .quad 0x2840000083
    .quad 0x2880000083
    .quad 0x28c0000083
    .quad 0x2900000083
    .quad 0x2940000083
    .quad 0x2980000083
    .quad 0x29c0000083
    .quad 0x2a00000083
    .quad 0x2a40000083
    .quad 0x2a80000083
    .quad 0x2ac0000083
    .quad 0x2b00000083
    .quad 0x2b40000083
    .quad 0x2b80000083
    .quad 0x2bc0000083
    .quad 0x2c00000083
    .quad 0x2c40000083
    .quad 0x2c80000083
    .quad 0x2cc0000083
    .quad 0x2d00000083
    .quad 0x2d40000083
    .quad 0x2d80000083
    .quad 0x2dc0000083
    .quad 0x2e00000083
    .quad 0x2e40000083
    .quad 0x2e80000083
    .quad 0x2ec0000083
    .quad 0x2f00000083
    .quad 0x2f40000083
    .quad 0x2f80000083
    .quad 0x2fc0000083
    .quad 0x3000000083
    .quad 0x3040000083
    .quad 0x3080000083
    .quad 0x30c0000083
    .quad 0x3100000083
    .quad 0x3140000083
    .quad 0x3180000083
    .quad 0x31c0000083
    .quad 0x3200000083
    .quad 0x3240000083
    .quad 0x3280000083
    .quad 0x32c0000083
    .quad 0x3300000083
    .quad 0x3340000083
    .quad 0x3380000083
    .quad 0x33c0000083
    .quad 0x3400000083
    .quad 0x3440000083
    .quad 0x3480000083
    .quad 0x34c0000083
    .quad 0x3500000083
    .quad 0x3540000083
    .quad 0x3580000083
    .quad 0x35c0000083
    .quad 0x3600000083
    .quad 0x3640000083
    .quad 0x3680000083
    .quad 0x36c0000083
    .quad 0x3700000083
    .quad 0x3740000083
    .quad 0x3780000083
    .quad 0x37c0000083
    .quad 0x3800000083
    .quad 0x3840000083
    .quad 0x3880000083
    .quad 0x38c0000083
    .quad 0x3900000083
    .quad 0x3940000083
    .quad 0x3980000083
    .quad 0x39c0000083
    .quad 0x3a00000083
    .quad 0x3a40000083
    .quad 0x3a80000083
    .quad 0x3ac0000083
    .quad 0x3b00000083
    .quad 0x3b40000083
    .quad 0x3b80000083
    .quad 0x3bc0000083
    .quad 0x3c00000083
    .quad 0x3c40000083
    .quad 0x3c80000083
    .quad 0x3cc0000083
    .quad 0x3d00000083
    .quad 0x3d40000083
    .quad 0x3d80000083
    .quad 0x3dc0000083
    .quad 0x3e00000083
    .quad 0x3e40000083
    .quad 0x3e80000083
    .quad 0x3ec0000083
    .quad 0x3f00000083
    .quad 0x3f40000083
    .quad 0x3f80000083
    .quad 0x3fc0000083
    .quad 0x4000000083
    .quad 0x4040000083
    .quad 0x4080000083
    .quad 0x40c0000083
    .quad 0x4100000083
    .quad 0x4140000083
    .quad 0x4180000083
    .quad 0x41c0000083
    .quad 0x4200000083
    .quad 0x4240000083
    .quad 0x4280000083
    .quad 0x42c0000083
    .quad 0x4300000083
    .quad 0x4340000083
    .quad 0x4380000083
    .quad 0x43c0000083
    .quad 0x4400000083
    .quad 0x4440000083
    .quad 0x4480000083
    .quad 0x44c0000083
    .quad 0x4500000083
    .quad 0x4540000083
    .quad 0x4580000083
    .quad 0x45c0000083
    .quad 0x4600000083
    .quad 0x4640000083
    .quad 0x4680000083
    .quad 0x46c0000083
    .quad 0x4700000083
    .quad 0x4740000083
    .quad 0x4780000083
    .quad 0x47c0000083
    .quad 0x4800000083
    .quad 0x4840000083
    .quad 0x4880000083
    .quad 0x48c0000083
    .quad 0x4900000083
    .quad 0x4940000083
    .quad 0x4980000083
    .quad 0x49c0000083
    .quad 0x4a00000083
    .quad 0x4a40000083
    .quad 0x4a80000083
    .quad 0x4ac0000083
    .quad 0x4b00000083
    .quad 0x4b40000083
    .quad 0x4b80000083
    .quad 0x4bc0000083
    .quad 0x4c00000083
    .quad 0x4c40000083
    .quad 0x4c80000083
    .quad 0x4cc0000083
    .quad 0x4d00000083
    .quad 0x4d40000083
    .quad 0x4d80000083
    .quad 0x4dc0000083
    .quad 0x4e00000083
    .quad 0x4e40000083
    .quad 0x4e80000083
    .quad 0x4ec0000083
    .quad 0x4f00000083
    .quad 0x4f40000083
    .quad 0x4f80000083
    .quad 0x4fc0000083
    .quad 0x5000000083
    .quad 0x5040000083
    .quad 0x5080000083
    .quad 0x50c0000083
    .quad 0x5100000083
    .quad 0x5140000083
    .quad 0x5180000083
    .quad 0x51c0000083
    .quad 0x5200000083
    .quad 0x5240000083
    .quad 0x5280000083
    .quad 0x52c0000083
    .quad 0x5300000083
    .quad 0x5340000083
    .quad 0x5380000083
    .quad 0x53c0000083
    .quad 0x5400000083
    .quad 0x5440000083
    .quad 0x5480000083
    .quad 0x54c0000083
    .quad 0x5500000083
    .quad 0x5540000083
    .quad 0x5580000083
    .quad 0x55c0000083
    .quad 0x5600000083
    .quad 0x5640000083
    .quad 0x5680000083
    .quad 0x56c0000083
    .quad 0x5700000083
    .quad 0x5740000083
    .quad 0x5780000083
    .quad 0x57c0000083
    .quad 0x5800000083
    .quad 0x5840000083
    .quad 0x5880000083
    .quad 0x58c0000083
    .quad 0x5900000083
    .quad 0x5940000083
    .quad 0x5980000083
    .quad 0x59c0000083
    .quad 0x5a00000083
    .quad 0x5a40000083
    .quad 0x5a80000083
    .quad 0x5ac0000083
    .quad 0x5b00000083
    .quad 0x5b40000083
    .quad 0x5b80000083
    .quad 0x5bc0000083
    .quad 0x5c00000083
    .quad 0x5c40000083
    .quad 0x5c80000083
    .quad 0x5cc0000083
    .quad 0x5d00000083
    .quad 0x5d40000083
    .quad 0x5d80000083
    .quad 0x5dc0000083
    .quad 0x5e00000083
    .quad 0x5e40000083
    .quad 0x5e80000083
    .quad 0x5ec0000083
    .quad 0x5f00000083
    .quad 0x5f40000083
    .quad 0x5f80000083
    .quad 0x5fc0000083
    .quad 0x6000000083
    .quad 0x6040000083
    .quad 0x6080000083
    .quad 0x60c0000083
    .quad 0x6100000083
    .quad 0x6140000083
    .quad 0x6180000083
    .quad 0x61c0000083
    .quad 0x6200000083
    .quad 0x6240000083
    .quad 0x6280000083
    .quad 0x62c0000083
    .quad 0x6300000083
    .quad 0x6340000083
    .quad 0x6380000083
    .quad 0x63c0000083
    .quad 0x6400000083
    .quad 0x6440000083
    .quad 0x6480000083
    .quad 0x64c0000083
    .quad 0x6500000083
    .quad 0x6540000083
    .quad 0x6580000083
    .quad 0x65c0000083
    .quad 0x6600000083
    .quad 0x6640000083
    .quad 0x6680000083
    .quad 0x66c0000083
    .quad 0x6700000083
    .quad 0x6740000083
    .quad 0x6780000083
    .quad 0x67c0000083
    .quad 0x6800000083
    .quad 0x6840000083
    .quad 0x6880000083
    .quad 0x68c0000083
    .quad 0x6900000083
    .quad 0x6940000083
    .quad 0x6980000083
    .quad 0x69c0000083
    .quad 0x6a00000083
    .quad 0x6a40000083
    .quad 0x6a80000083
    .quad 0x6ac0000083
    .quad 0x6b00000083
    .quad 0x6b40000083
    .quad 0x6b80000083
    .quad 0x6bc0000083
    .quad 0x6c00000083
    .quad 0x6c40000083
    .quad 0x6c80000083
    .quad 0x6cc0000083
    .quad 0x6d00000083
    .quad 0x6d40000083
    .quad 0x6d80000083
    .quad 0x6dc0000083
    .quad 0x6e00000083
    .quad 0x6e40000083
    .quad 0x6e80000083
    .quad 0x6ec0000083
    .quad 0x6f00000083
    .quad 0x6f40000083
    .quad 0x6f80000083
    .quad 0x6fc0000083
    .quad 0x7000000083
    .quad 0x7040000083
    .quad 0x7080000083
    .quad 0x70c0000083
    .quad 0x7100000083
    .quad 0x7140000083
    .quad 0x7180000083
    .quad 0x71c0000083
    .quad 0x7200000083
    .quad 0x7240000083
    .quad 0x7280000083
    .quad 0x72c0000083
    .quad 0x7300000083
    .quad 0x7340000083
    .quad 0x7380000083
    .quad 0x73c0000083
    .quad 0x7400000083
    .quad 0x7440000083
    .quad 0x7480000083
    .quad 0x74c0000083
    .quad 0x7500000083
    .quad 0x7540000083
    .quad 0x7580000083
    .quad 0x75c0000083
    .quad 0x7600000083
    .quad 0x7640000083
    .quad 0x7680000083
    .quad 0x76c0000083
    .quad 0x7700000083
    .quad 0x7740000083
    .quad 0x7780000083
    .quad 0x77c0000083
    .quad 0x7800000083
    .quad 0x7840000083
    .quad 0x7880000083
    .quad 0x78c0000083
    .quad 0x7900000083
    .quad 0x7940000083
    .quad 0x7980000083
    .quad 0x79c0000083
    .quad 0x7a00000083
    .quad 0x7a40000083
    .quad 0x7a80000083
    .quad 0x7ac0000083
    .quad 0x7b00000083
    .quad 0x7b40000083
    .quad 0x7b80000083
    .quad 0x7bc0000083
    .quad 0x7c00000083
    .quad 0x7c40000083
    .quad 0x7c80000083
    .quad 0x7cc0000083
    .quad 0x7d00000083
    .quad 0x7d40000083
    .quad 0x7d80000083
    .quad 0x7dc0000083
    .quad 0x7e00000083
    .quad 0x7e40000083
    .quad 0x7e80000083
    .quad 0x7ec0000083
    .quad 0x7f00000083
    .quad 0x7f40000083
    .quad 0x7f80000083
    .quad 0x7fc0000083

