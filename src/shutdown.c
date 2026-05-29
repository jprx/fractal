#include <fractal.h>
#include <virt_mem.h>
#include <out.h>

#ifdef ARCH_RISCV
// See hw/misc/sifive_test.c
// There's a sifive test device here (VIRT_TEST):
#define SHUTDOWN_BASEADDR KERN_P2DV(0x100000)
#define SHUTDOWN_VAL 0x5555

void __attribute__((noreturn)) shutdown() {
    *(volatile u32 *)SHUTDOWN_BASEADDR = SHUTDOWN_VAL;
    panic("shutdown failed");
    // good byte
}

#endif // ARCH_RISCV

#ifdef ARCH_ARM
// PL061 GPIO pin 0 shuts the device down (SECURE_GPIO_POWEROFF)
#define PL061_GPIO_BASE KERN_P2DV(0x090b0000)

void __attribute__((noreturn)) shutdown() {
    // *(volatile u32 *)(PL061_GPIO_BASE + 0x400) = 0xFFFFFFFF;
    // *(volatile u32 *)PL061_GPIO_BASE = 0;
    // *(volatile u32 *)PL061_GPIO_BASE = 1;
    panic("shutdown unsupported");
}
#endif // ARCH_ARM

#ifdef ARCH_X86
void outw(u16 port, u16 val);
#define X86_SHUTDOWN_PORT 0x604
#define X86_SHUTDOWN_VAL  0x2000

// See tests/tcg/x86_64/system/boot.S:132
// "QEMU ACPI poweroff"
void __attribute__((noreturn)) shutdown() {
    outw(X86_SHUTDOWN_PORT, X86_SHUTDOWN_VAL);
    panic("shutdown failed");
}
#endif // ARCH_X86
