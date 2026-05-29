#include <fractal.h>
#include <syscall.h>
#include <paging.h>
#include <copy_task.h>

i64 do_sys_xlate(task_t *t, u64 va, virt_t u_pa) {
    u64 pa = PageWalk(t->t_pagetable, va);
    copy_to_task(t, u_pa, &pa, sizeof(pa));
    return 0;
}

i64 sys_xlate(u64 va, virt_t u_pa) {
    return do_sys_xlate(current_task(), va, u_pa);
}
