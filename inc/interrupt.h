#ifndef INTERRUPT_H
#define INTERRUPT_H

// Cross-platform generic interrupt reasons
typedef enum interrupt_kind_t {
    INT_TIMER,
    INT_UART,
} InterruptKind;

typedef enum interrupt_mode_t {
    INTS_DISABLED,
    INTS_ENABLED,
} InterruptMode;

typedef u64 InterruptID;

#ifdef ARCH_X86
#include <arch/x86/interrupt.h>
#endif // ARCH_X86

#ifdef ARCH_ARM
#include <arch/arm/interrupt.h>
#endif // ARCH_ARM

#ifdef ARCH_RISCV
#include <arch/rv/interrupt.h>
#endif // ARCH_RISCV

#endif // INTERRUPT_H
