#include <regs.h>
#include <out.h>
#include <arch/arm_asm_defines.h>
#include <task.h>

void dump_regs(regs_t *r) {
    if (!r) return;
    printf("x0:  0x%X    ", r->x0);
    printf("x1:  0x%X    ", r->x1);
    printf("x2:  0x%X    ", r->x2);
    printf("x3:  0x%X\n", r->x3);
    printf("x4:  0x%X    ", r->x4);
    printf("x5:  0x%X    ", r->x5);
    printf("x6:  0x%X    ", r->x6);
    printf("x7:  0x%X\n", r->x7);
    printf("x8:  0x%X    ", r->x8);
    printf("x9:  0x%X    ", r->x9);
    printf("x10: 0x%X    ", r->x10);
    printf("x11: 0x%X\n", r->x11);
    printf("x12: 0x%X    ", r->x12);
    printf("x13: 0x%X    ", r->x13);
    printf("x14: 0x%X    ", r->x14);
    printf("x15: 0x%X\n", r->x15);
    printf("x16: 0x%X    ", r->x16);
    printf("x17: 0x%X    ", r->x17);
    printf("x18: 0x%X    ", r->x18);
    printf("x19: 0x%X\n", r->x19);
    printf("x20: 0x%X    ", r->x20);
    printf("x21: 0x%X    ", r->x21);
    printf("x22: 0x%X    ", r->x22);
    printf("x23: 0x%X\n", r->x23);
    printf("x24: 0x%X    ", r->x24);
    printf("x25: 0x%X    ", r->x25);
    printf("x26: 0x%X    ", r->x26);
    printf("x27: 0x%X\n", r->x27);
    printf("x28: 0x%X    ", r->x28);
    printf("x29: 0x%X    ", r->x29);
    printf("x30: 0x%X\n", r->x30);
    printf("ELR: 0x%X    ", r->elr);
    printf("SP0: 0x%X    ", r->sp_el0);
    printf("SP1: 0x%X\n", r->sp_el1);
    printf("FAR: 0x%X    ", r->far);
    printf("ESR: 0x%X    ", r->esr);
    printf("SPSR:0x%X\n", r->spsr);
}

void set_return_value(regs_t *r, u64 v) {
    if (!r) panic("Tried to set the return value for NULL regs");
    r->x0 = v;
}

void set_init_regs(regs_t *regs, ThreadKind task_kind, bool interrupts_on) {
    if (!regs) panic("Tried to set init task regs for a NULL reg pointer");
    switch (task_kind) {
        case KERNEL_OUTER_THREAD:
        case KERNEL_INNER_THREAD:
        // sp_el0 should be unused for kernel tasks, so set it to strange value
        regs->sp_el0   = 0x4242424242424242;
        regs->spsr     = interrupts_on ? KERNEL_SPSR : KERNEL_SPSR_NO_INTERRUPTS;
        break;

        case USER_THREAD:
        regs->spsr     = interrupts_on ? USER_SPSR : USER_SPSR_NO_INTERRUPTS;
        break;
    }
}

void set_pc(regs_t *regs, u64 pc) {
    regs->elr = pc;
}

u64 get_pc(regs_t *regs) {
    return regs->elr;
}

u64 get_interrupt_sp(void *t, regs_t *regs) {
    switch (((thread_t *)t)->t_kind) {
        case USER_THREAD:
        return regs->sp_el1;
        break;

        case KERNEL_INNER_THREAD:
        case KERNEL_OUTER_THREAD:
        return 0x4545454545454545ull;
        break;
    }

    return 0;
}

void set_interrupt_sp(void *t, regs_t *regs, u64 new_sp) {
    switch (((thread_t *)t)->t_kind) {
        case USER_THREAD:
        if (!IS_KERNEL_VA(new_sp)) panic("set_interrupt_sp: not a kernel VA (0x%X)", new_sp);
        regs->sp_el1 = new_sp;
        break;
    }
}

u64 get_runtime_sp(void *t, regs_t *regs) {
    switch (((thread_t *)t)->t_kind) {
        case USER_THREAD:
        return regs->sp_el0;
        break;

        case KERNEL_INNER_THREAD:
        case KERNEL_OUTER_THREAD:
        return regs->sp_el1;
        break;
    }

    return 0;
}

void set_runtime_sp(void *t, regs_t *regs, u64 new_sp) {
    switch (((thread_t *)t)->t_kind) {
        case USER_THREAD:
        regs->sp_el0 = new_sp;
        break;

        case KERNEL_INNER_THREAD:
        case KERNEL_OUTER_THREAD:
        regs->sp_el1 = new_sp;
        break;
    }
}

void set_task_arg(regs_t *regs, usize idx, u64 val) {
    if(!regs) return;
    switch(idx) {
        case 0: regs->x0 = val; break;
        case 1: regs->x1 = val; break;
        case 2: regs->x2 = val; break;
        default: panic("Tried to set ARM argument %d (we don't have a def for that index)", idx); break;
    }
}

void disable_interrupts(regs_t *r) {
    if (!r) return;
    r->spsr |= (SPSR_FIQ_MASK | SPSR_IRQ_MASK);
}

void enable_interrupts(regs_t *r) {
    if (!r) return;
    r->spsr &= ~(SPSR_FIQ_MASK | SPSR_IRQ_MASK);
}
