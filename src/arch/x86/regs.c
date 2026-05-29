#include <regs.h>
#include <out.h>
#include <arch/x86_asm_defines.h>
#include <arch/x86/selector.h>
#include <task.h>

void dump_regs(regs_t *r) {
    if (!r) return;
    printf("pc:  0x%X\r\n", r->rip);
    printf("rax: 0x%X    ", r->rax);
    printf("rbx: 0x%X    ", r->rbx);
    printf("rcx: 0x%X    ", r->rcx);
    printf("rdx: 0x%X\r\n", r->rdx);
    printf("rdi: 0x%X    ", r->rdi);
    printf("rsi: 0x%X    ", r->rsi);
    printf("rbp: 0x%X    ", r->rbp);
    printf("rsp: 0x%X\r\n", r->rsp);
    printf("r8:  0x%X    ", r->r8);
    printf("r9:  0x%X    ", r->r9);
    printf("r10: 0x%X    ", r->r10);
    printf("r11: 0x%X\r\n", r->r11);
    printf("r12: 0x%X    ", r->r12);
    printf("r13: 0x%X    ", r->r13);
    printf("r14: 0x%X    ", r->r14);
    printf("r15: 0x%X\r\n", r->r15);
    printf("err: 0x%X    ", r->error_code);
    printf("rip: 0x%X    ", r->rip);
    printf("cs:  0x%X    ", r->cs);
    printf("rfl: 0x%X\r\n", r->rflags);
    printf("rsp: 0x%X    ", r->rsp);
    printf("ss:  0x%X\r\n", r->ss);
    printf("cr2: 0x%X\r\n", r->cr2);
}

void set_return_value(regs_t *r, u64 v) {
    if (!r) panic("Tried to set the return value for NULL regs");
    r->rax = v;
}

void set_init_regs(regs_t *regs, ThreadKind task_kind, bool interrupts_on) {
    // On x86, we store the interrupt stack in tss.rsp0, so no need to set it in t_init_regs
    // However, other architectures need to put the interrupt stack into a reg, so that argument
    // exists for X86 as well
    if (!regs) panic("Tried to set init task regs for a NULL reg pointer");
    switch (task_kind) {
        case KERNEL_OUTER_THREAD:
        case KERNEL_INNER_THREAD:
        regs->ss       = KERNEL_DS;
        regs->rflags   = interrupts_on ? KERNEL_RFLAG_START : KERNEL_RFLAG_NO_INTERRUPTS;
        regs->cs       = KERNEL_CS;
        break;

        case USER_THREAD:
        regs->ss       = USER_DS;
        regs->rflags   = interrupts_on ? USER_RFLAG_START : USER_RFLAG_NO_INTERRUPTS;
        regs->cs       = USER_CS;
        break;
    }
}

void set_pc(regs_t *regs, u64 pc) {
    if(!regs) return;
    regs->rip = pc;
}

u64 get_pc(regs_t *regs) {
    if (!regs) return 0;
    return regs->rip;
}

u64 get_interrupt_sp(void *t, regs_t *regs) {
    panic("x86 doesn't have a dedicated interrupt stack register");
}

void set_interrupt_sp(void *t, regs_t *regs, u64 new_sp) {
    // For user tasks, this is tss.rsp0
    // For kernel tasks, we use the runtime stack
    return;
}

u64 get_runtime_sp(void *t, regs_t *regs) {
    return regs->rsp;
}

void set_runtime_sp(void *t, regs_t *regs, u64 new_sp) {
    regs->rsp = new_sp;
}

void set_task_arg(regs_t *regs, usize idx, u64 val) {
    if(!regs) return;
    switch(idx) {
        case 0: regs->rdi = val; break;
        case 1: regs->rsi = val; break;
        case 2: regs->rdx = val; break;
        default: panic("Tried to set X86 argument %d (we don't have a def for that index)", idx); break;
    }
}

void disable_interrupts(regs_t *r) {
    if (!r) return;
    r->rflags &= ~X86_RFLAGS_INTERRUPT_ENABLE_MASK;
}

void enable_interrupts(regs_t *r) {
    if (!r) return;
    r->rflags |= X86_RFLAGS_INTERRUPT_ENABLE_MASK;
}
