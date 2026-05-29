#include <regs.h>
#include <out.h>
#include <task.h>
#include <arch/riscv_asm_defines.h>

void dump_regs(regs_t *r) {
    if (!r) return;
    printf("x1:  0x%X    ", r->x1);
    printf("x2:  0x%X    ", r->x2);
    printf("x3:  0x%X    ", r->x3);
    printf("x4:  0x%X\n", r->x4);
    printf("x5:  0x%X    ", r->x5);
    printf("x6:  0x%X    ", r->x6);
    printf("x7:  0x%X    ", r->x7);
    printf("x8:  0x%X\n", r->x8);
    printf("x9:  0x%X    ", r->x9);
    printf("x10: 0x%X    ", r->x10);
    printf("x11: 0x%X    ", r->x11);
    printf("x12: 0x%X\n", r->x12);
    printf("x13: 0x%X    ", r->x13);
    printf("x14: 0x%X    ", r->x14);
    printf("x15: 0x%X    ", r->x15);
    printf("x16: 0x%X\n", r->x16);
    printf("x17: 0x%X    ", r->x17);
    printf("x18: 0x%X    ", r->x18);
    printf("x19: 0x%X    ", r->x19);
    printf("x20: 0x%X\n", r->x20);
    printf("x21: 0x%X    ", r->x21);
    printf("x22: 0x%X    ", r->x22);
    printf("x23: 0x%X    ", r->x23);
    printf("x24: 0x%X\n", r->x24);
    printf("x25: 0x%X    ", r->x25);
    printf("x26: 0x%X    ", r->x26);
    printf("x27: 0x%X    ", r->x27);
    printf("x28: 0x%X\n", r->x28);
    printf("x29: 0x%X    ", r->x29);
    printf("x30: 0x%X    ", r->x30);
    printf("x31: 0x%X    \n", r->x31);
    printf("sepc:     0x%X\n", r->sepc);
    printf("sstatus:  0x%X\n", r->sstatus);
    printf("sscratch: 0x%X\n", r->sscratch);
    printf("scause:   0x%X\n", r->scause);
    printf("stval:    0x%X\n", r->stval);
}

void set_return_value(regs_t *r, u64 v) {
    if (!r) panic("Tried to set the return value for NULL regs");
    r->x10 = v;
}

void set_init_regs(regs_t *regs, ThreadKind task_kind, bool interrupts_on) {
    if (!regs) panic("Tried to set init task regs for a NULL reg pointer");
    switch (task_kind) {
        case KERNEL_OUTER_THREAD:
        case KERNEL_INNER_THREAD:
        regs->sstatus  = interrupts_on ? KERNEL_SSTATUS : KERNEL_SSTATUS_NO_INTERRUPTS;
        regs->sscratch = 0x4343434343434343;
        regs->sie      = interrupts_on ? SIE_ENABLE_ALL : SIE_DISABLE_ALL;
        break;

        case USER_THREAD:
        regs->sstatus  = USER_SSTATUS;
        // regs->sscratch will be set later in set_interrupt_sp
        regs->sie      = interrupts_on ? SIE_ENABLE_ALL : SIE_DISABLE_ALL;
        break;
    }
}

void set_pc(regs_t *regs, u64 pc) {
    regs->sepc = pc;
}

u64 get_pc(regs_t *regs) {
    return regs->sepc;
}

u64 get_interrupt_sp(void *t, regs_t *regs) {
    panic("Unimplemented");
}

void set_interrupt_sp(void *t, regs_t *regs, u64 new_sp) {
    switch (((thread_t*)t)->t_kind) {
        case USER_THREAD:
        if (!IS_KERNEL_VA(new_sp)) panic("set_interrupt_sp: not a kernel VA (0x%X)", new_sp);
        regs->sscratch = new_sp;
        break;
    }
}

u64 get_runtime_sp(void *t, regs_t *regs) {
    return regs->x2;
}

void set_runtime_sp(void *t, regs_t *regs, u64 new_sp) {
    regs->x2 = new_sp;
}

u64 get_gp(void *the_thread, regs_t *regs) {
    return regs->x3;
}

void set_gp(void *the_thread, regs_t *regs, u64 new_gp) {
    regs->x3 = new_gp;
}

void set_task_arg(regs_t *regs, usize idx, u64 val) {
    if(!regs) return;
    switch(idx) {
        case 0: regs->x10 = val; break;
        case 1: regs->x11 = val; break;
        case 2: regs->x12 = val; break;
        default: panic("Tried to set RISCV argument %d (we don't have a def for that index)", idx); break;
    }
}

void disable_interrupts(regs_t *r) {
    if (!r) return;
    r->sie = SIE_DISABLE_ALL;
}

void enable_interrupts(regs_t *r) {
    if (!r) return;
    r->sie = SIE_ENABLE_ALL;
}
