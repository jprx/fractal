#include <regs.h>
#include <cpu.h>
#include <arch.h>
#include <out.h>
#include <task.h>
#include <io/pci.h>
#include <syscall.h>
#include <exception.h>
#include <io/serial.h>

extern FRACTALCORE_CLASS global_cpu;

extern "C" void handle_arm_irq(regs_t *r) {
    u8 val;
    u32 tmp;
    thread_enter_kernel(r);
    InterruptID int_id = global_cpu.GetInterruptController()->ClaimInterrupt();
    global_cpu.GetInterruptController()->SendEOI(int_id);

    switch(int_id) {
        case UART0_IRQ:
            UARTInterrupt();
        break;

        case GENERIC_TIMER_IRQ:
        case VIRTUAL_TIMER_IRQ:
        case HVF_TIMER_IRQ:
            arm_generic_timer_schedule_interrupt();
            global_cpu.TimerInterrupt();
        break;

        case PCI_INTA_IRQ:
        case PCI_INTB_IRQ:
        case PCI_INTC_IRQ:
        case PCI_INTD_IRQ:
            handle_pci_interrupt();
        break;

        default:
            panic("Unknown interrupt: %d", int_id);
        break;
    }
    thread_leave_kernel();
    if (r->sp_el1 == 0) {
        panic("handle_arm_irq: Was about to return to a saved null stack");
    }
}

extern "C" void handle_arm_fiq(regs_t *r) {
    u8 val;
    u32 tmp;
    thread_enter_kernel(r);

    // The only FIQ we can ever take is on Apple Silicon,
    // when we receive a timer interrupt.
    arm_generic_timer_schedule_interrupt();
    global_cpu.TimerInterrupt();

    // We don't do UART interrupts on Apple Si, so
    // poll for UART characters every timer interrupt
    UARTInterrupt();

    thread_leave_kernel();
    if (r->sp_el1 == 0) {
        panic("handle_arm_fiq: Was about to return to a saved null stack");
    }
}

extern "C" void handle_arm_sync_exception(regs_t *r) {
    thread_enter_kernel(r);
    u64 esr = r->esr;
    u64 syscall_number;

    switch(ESR_REASON(esr)) {
        case ESR_REASON_SVC64:
            syscall_number = ESR_SYNDROME(r->esr);
            if (syscall_number != 0) {
                panic("ARM tried to use a nonzero syscall number- but the Fractal ABI has been updated to use (x0,x1,x2,x3) rather than (ESR,x0,x1,x2) for ARM syscalls");
            }
            Syscall(r->x0, r->x1, r->x2, r->x3, r->x4, r->x5);
        break;

        case ESR_INST_ABORT_LOWLEVEL:
        case ESR_INST_ABORT_NOCHANGE:
        case ESR_DATA_ABORT_LOWLEVEL:
        case ESR_DATA_ABORT_NOCHANGE:
            // Should probably be decoding ISS field. For now this is ok though.
            Exception(r, EXC_PAGE_FAULT, r->far);
        break;

        default:
            Exception(r, EXC_UNKNOWN, esr);
        break;
    }

    thread_leave_kernel();

    if (r->sp_el1 == 0) {
        panic("handle_arm_sync_exception: Was about to return to a saved null stack");
    }
}

extern "C" void handle_arm_serror(regs_t *r) {
    panic("serror");
}

extern "C" void handle_arm_unk_exception(regs_t *r) {
    panic("unk exception");
}
