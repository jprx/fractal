#ifndef RISCV_INTERRUPT_H
#define RISCV_INTERRUPT_H

#include <types.h>

BEGIN_C_HEADER

#ifndef ARCH_RISCV
#error "RISC-V target not selected, but including interrupt.h for RISC-V"
#endif // !ARCH_RISCV

#include <io/interrupt/plic.h>

extern void user_trap(void);
extern void kernel_trap(void);

#define UART0_IRQ ((10))

#define PCI_INTA_IRQ ((32))
#define PCI_INTB_IRQ ((33))
#define PCI_INTC_IRQ ((34))
#define PCI_INTD_IRQ ((35))

#define SCAUSE_INTERRUPT_MASK  ((0x8000000000000000ull))
#define SCAUSE_IS_INTERRUPT(s) (((s & SCAUSE_INTERRUPT_MASK) != 0))
#define SCAUSE_INT_SOFTWARE    ((1 | SCAUSE_INTERRUPT_MASK))
#define SCAUSE_INT_EXTERNAL    ((9 | SCAUSE_INTERRUPT_MASK))

#define SCAUSE_INST_ADDR_MISALIGNED    0
#define SCAUSE_INST_ACCESS_FAULT       1
#define SCAUSE_ILLEGAL_INST            2
#define SCAUSE_BREAKPOINT              3
#define SCAUSE_LOAD_MISALIGNED         4
#define SCAUSE_LOAD_ACCESS_FAULT       5
#define SCAUSE_STORE_AMO_MISALIGNED    6
#define SCAUSE_STORE_AMO_ACCESS_FAULT  7
#define SCAUSE_SYSCALL                 8
#define SCAUSE_SYSCALL_FROM_S          9
#define SCAUSE_ENVCALL_FROM_M          11
#define SCAUSE_INST_PAGEFAULT          12
#define SCAUSE_LOAD_PAGEFAULT          13
#define SCAUSE_STORE_AMO_PAGEFAULT     15

END_C_HEADER

#endif // RISCV_INTERRUPT_H
