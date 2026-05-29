#include <fractal.h>
#include <syscall.h>
#include <virt_mem.h>
#include <paging.h>
#include <arch.h>
#include <cpu.h>

extern FRACTALCORE_CLASS global_cpu;

i64 do_sys_mprotect(task_t *t, virt_t u_addr, size_t len, u64 prot_flags) {
    if (PHYS_ADDR_INVALID == PageWalk(t->t_pagetable, u_addr)) return -1;
    if (!IS_USER_VA(u_addr)) return -2;

    if ((prot_flags & PROT_USER) != 0) {
        SetPageMemoryPermission(t->t_pagetable, u_addr, USER_PAGE);
    }

    if ((prot_flags & PROT_KERNEL) != 0) {
        SetPageMemoryPermission(t->t_pagetable, u_addr, KERNEL_PAGE);
    }

    global_cpu.FlushTLB();
    return 0;
}

i64 sys_mprotect(virt_t u_addr, size_t len, u64 prot_flags) {
    return do_sys_mprotect(current_task(), u_addr, len, prot_flags);
}
