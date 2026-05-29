#ifndef ARM_INTERRUPT_H
#define ARM_INTERRUPT_H

#include <types.h>

BEGIN_C_HEADER

#ifndef ARCH_ARM
#error "ARM target not selected, but including interrupt.h for ARM"
#endif // !ARCH_ARM

#ifdef ARM_BOARD_HAS_GIC
#include <io/interrupt/gic.h>
#endif // ARM_BOARD_HAS_GIC

#define VIRTUAL_TIMER_IRQ 27
#define GENERIC_TIMER_IRQ 30
#define HVF_TIMER_IRQ 1023

#define PCI_INTA_IRQ 35
#define PCI_INTB_IRQ 36
#define PCI_INTC_IRQ 37
#define PCI_INTD_IRQ 38

#define ESR_SYNDROME(esr)  ((esr & ((1ull << 25) - 1)))
#define ESR_REASON(esr) (((esr >> 26) & ((1 << 6) - 1)))

#define ESR_REASON_SVC64              0b010101
#define ESR_INST_ABORT_LOWLEVEL       0b100000
#define ESR_INST_ABORT_NOCHANGE       0b100001
#define ESR_DATA_ABORT_LOWLEVEL       0b100100
#define ESR_DATA_ABORT_NOCHANGE       0b100101

END_C_HEADER

#endif // ARM_INTERRUPT_H
