#include <out.h>
#include <cpu.h>
#include <arch.h>
#include <regs.h>
#include <task.h>
#include <io/pci.h>
#include <syscall.h>
#include <exception.h>
#include <io/serial.h>
#include <io/hid/ps2-hid.h>

extern FRACTALCORE_CLASS global_cpu;

extern "C" void handle_uart0_irq(regs_t *r) {
    global_cpu.GetInterruptController()->SendEOI(0);
    UARTInterrupt();
}

extern "C" void handle_lapic_timer_irq(regs_t *r) {
    global_cpu.GetInterruptController()->SendEOI(0);
    global_cpu.TimerInterrupt();
}

extern "C" __attribute__((noreturn)) void _panic_internal(regs_t *regs, char *reason, ...);
extern "C" void unknown_int(u32 idx, regs_t *r) {
    thread_enter_kernel(r);
    // dump_regs(r);
    _panic_internal(r, "Unhandled interrupt %d", idx);
    thread_leave_kernel();
}

extern "C" void unknown_exception(u32 idx, regs_t *r) {
    thread_enter_kernel(r);
    printf("Unknown exception: %d\r\n", idx);
    Exception(r, EXC_UNKNOWN, idx);
}

extern "C" void handle_syscall_wrapper(regs_t *r) {
    thread_enter_kernel(r);
    Syscall(r->rax, r->rdi, r->rsi, r->rdx, r->r10, r->r8);
    thread_leave_kernel();
}

extern "C" void handle_virtio_irq(regs_t *r) {
    global_cpu.GetInterruptController()->SendEOI(0);
    handle_pci_interrupt();
}

extern "C" void x86_interrupt(regs_t *r) {
    thread_enter_kernel(r);

#ifdef DEBUG_INTERRUPT_STACK_CORRUPTION
    printf("[int] num: %d (rip: 0x%X) in %s\n", r->interrupt_number, r->rip, current_thread()->name);
#endif // DEBUG_INTERRUPT_STACK_CORRUPTION

    switch(r->interrupt_number) {
        case IDT_ILLEGAL_INST_EXCEPTION:
        Exception(r, EXC_ILLEGAL_INSTRUCTION, 0);
        break;

        case IDT_PAGEFAULT_EXCEPTION:
        Exception(r, EXC_PAGE_FAULT, r->cr2);
        break;

        case UART0_IRQ:
        handle_uart0_irq(r);
        break;

        case VIRTIO_IRQ:
        handle_virtio_irq(r);
        break;

        case APIC_TIMER_IRQ:
        handle_lapic_timer_irq(r);
        break;

        // Send EOI *after* handling this interrupt
        // Otherwise we may get nested PS/2 interrupts and get confused
        case LEGACY_KEYBOARD_IRQ:
        ps2_keyboard_interrupt();
        global_cpu.GetInterruptController()->SendEOI(0);
        break;

        case LEGACY_MOUSE_IRQ:
        ps2_mouse_interrupt();
        global_cpu.GetInterruptController()->SendEOI(0);
        break;

        case X86_RESERVED_EXCEPTION:
        // For some reason the AMD server throws this during boot
        // So, uh, ignore it
        printf("[warning] Received a reserved exception\r\n");
        break;

        default:
        unknown_exception(r->interrupt_number, r);
        break;
    }

    thread_leave_kernel();
}
