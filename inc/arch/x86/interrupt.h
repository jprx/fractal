#ifndef X86_INTERRUPT_H
#define X86_INTERRUPT_H

#ifndef ARCH_X86
#error "X86 target not selected, but including interrupt.h for X86"
#endif // !ARCH_X86

#include <io/interrupt/apic.h>
#include <io/interrupt/lapic.h>
#include <io/interrupt/ioapic.h>
#include <arch/x86_asm_defines.h>
#include <fractal.h>

BEGIN_C_HEADER

#define UART0_IRQ_IOAPIC_EXTERNAL             ((4))
#define LEGACY_KEYBOARD_IRQ_IOAPIC_EXTERNAL   ((1))
#define LEGACY_MOUSE_IRQ_IOAPIC_EXTERNAL      ((12))

#define PCI_INTA_EXTERNAL 10
#define PCI_INTB_EXTERNAL 11

extern void uart0_irq(void);
extern void lapic_timer_irq(void);
extern void virtio_irq(void);

extern void illegalinst_irq(void);
extern void pagefault_irq(void);

END_C_HEADER

#endif // X86_INTERRUPT_H
