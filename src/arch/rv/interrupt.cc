#include <types.h>
#include <out.h>
#include <regs.h>
#include <arch/riscv_asm_defines.h>
#include <interrupt.h>
#include <io/interrupt_source.h>
#include <cpu.h>
#include <arch.h>
#include <io/serial.h>
#include <task.h>
#include <io/interrupt/plic.h>
#include <io/pci.h>
#include <syscall.h>
#include <exception.h>

extern FRACTALCORE_CLASS global_cpu;

extern "C" void rv_handle_external_interrupt(InterruptID int_id) {

    switch (int_id) {
        case UART0_IRQ:
        UARTInterrupt();
        break;

        case PCI_INTA_IRQ:
        case PCI_INTB_IRQ:
        case PCI_INTC_IRQ:
        case PCI_INTD_IRQ:
        handle_pci_interrupt();
        break;

        default:
        panic("unknown external interrupt %d", int_id);
        break;
    }
}

extern "C" void rv_trap_interrupt(regs_t *r, u64 cause) {
    InterruptID int_id;
    switch(cause) {
        case SCAUSE_INT_SOFTWARE:
            // For now, the only thing that can trigger these is M-mode scheduling a timer interrupt
            global_cpu.TimerInterrupt();

            // Mark interrupt as having been handled:
            write_csr(sip, read_csr(sip) & ~(SIP_SSIP));
        break;

        case SCAUSE_INT_EXTERNAL:
            int_id = global_cpu.GetInterruptController()->ClaimInterrupt();
            global_cpu.GetInterruptController()->SendEOI(int_id);

            rv_handle_external_interrupt(int_id);

            // Mark interrupt as having been handled:
            write_csr(sip, read_csr(sip) & ~(SIP_SEIP));
        break;

        default:
            panic("Unknown interrupt 0x%X\r\n", cause);
        break;
    }
}

extern "C" void rv_trap_exception(regs_t *r, u64 cause) {
    switch(cause) {
        case SCAUSE_SYSCALL:
        case SCAUSE_SYSCALL_FROM_S:
            r->sepc += RISCV_INSTRUCTION_SIZE;
            Syscall(r->x10, r->x11, r->x12, r->x13, r->x14, r->x15);
        break;

        case SCAUSE_INST_PAGEFAULT:
        case SCAUSE_LOAD_PAGEFAULT:
        case SCAUSE_STORE_AMO_PAGEFAULT:
            Exception(r, EXC_PAGE_FAULT, r->stval);
        break;

        default:
            Exception(r, EXC_UNKNOWN, cause);
        break;
    }
}

extern "C" void rv_handle_trap(regs_t *r) {
    thread_enter_kernel(r);

    if (SCAUSE_IS_INTERRUPT(r->scause)) {
        rv_trap_interrupt(r, r->scause);
    } else {
        rv_trap_exception(r, r->scause);
    }

    thread_leave_kernel();

    if (0 == current_thread()->t_preempt_count) {
        // Base level of interrupt has been reached, restore trap vector
        // Note that it may have changed if we just took a sys_fractal syscall!
        if (USER_THREAD == current_thread()->t_kind) write_csr(stvec, (u64)&user_trap);
        else /* KERNEL_*_THREAD */                   write_csr(stvec, (u64)&kernel_trap);
    }
}
