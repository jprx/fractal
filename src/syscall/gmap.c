#include <fractal.h>
#include <syscall.h>
#include <fmap.h>
#include <paging.h>
#include <task.h>
#include <copy_task.h>
#include <virt_mem.h>

// since this file needs to be C, we can't use C++ stuff like global_cpu.
// instead, we can use this hook to flush the TLB from the current CPU object.
extern void __global_flush_tlb();

// For singlethreaded programs:
// Primary gmap: matches main thread's privilege level
// Secondary gmap: opposite of main thread's privilege level

// For multithreaded programs:
// Primary gmap: always kernel
// Secondary gmap: always user

i64 _gmap_alloc(task_t *t, virt_t u_newpgptr);
i64 _gmap_carve(task_t *t, virt_t u_carveout);
i64 _gmap_ugetbase(task_t *t, virt_t u_baseptr);
i64 _gmap_kgetbase(task_t *t, virt_t u_baseptr);
i64 _gmap_reset(task_t *t);
i64 _gmap_unmap(task_t *t);

typedef i64 (*gmap_fn_t)(task_t *,u64);

void init_gmap(gigamap_t *g) {
    if (!g) return;
    g->g_is_mapped = false;
    g->g_l2_pagetable = NULL;
    g->g_l3_pagetable = NULL;
    g->g_map_va = 0;
    g->g_map_pa = 0;
    g->g_backing_va = 0;
    g->g_privs = KERNEL_PAGE;
}

gmap_fn_t gmap_dispatch_table[] = {
    [GIGAOPT_ALLOC]    = (gmap_fn_t)&_gmap_alloc,
    [GIGAOPT_CARVE]    = (gmap_fn_t)&_gmap_carve,
    [GIGAOPT_UGETBASE] = (gmap_fn_t)&_gmap_ugetbase,
    [GIGAOPT_RESET]    = (gmap_fn_t)&_gmap_reset,
    [GIGAOPT_UNMAP]    = (gmap_fn_t)&_gmap_unmap,
    [GIGAOPT_KGETBASE] = (gmap_fn_t)&_gmap_kgetbase,
};

i64 _gmap_alloc(task_t *t, virt_t u_newpgptr) {
    u64 target_va = PRIMARY_GMAP_START;
    PagePrivilege primary_priv = KERNEL_PAGE;
    PagePrivilege secondary_priv = USER_PAGE;
    usize num_threads = 0;

    if (!is_user_addr(u_newpgptr)) return -1;

    for (usize i = 0; i < MAX_THREADS; i++) {
        if (t->t_threads[i]) num_threads++;
    }

    if (0 == num_threads) panic("_gmap_alloc: task has no threads?");

    if (1 == num_threads) {
        if (!t->t_threads[0]) panic("_gmap_alloc: only running thread isn't the main thread");

        switch(t->t_threads[0]->t_kind) {
            case USER_THREAD:
                primary_priv = USER_PAGE;
                secondary_priv = KERNEL_PAGE;
                break;

            case KERNEL_OUTER_THREAD:
                primary_priv = KERNEL_PAGE;
                secondary_priv = USER_PAGE;
                break;

            case KERNEL_INNER_THREAD:
                panic("_gmap_alloc: inner threads cannot allocate gmaps");
                break;
        }
    }

    GigaMap(&t->t_primary_gmap, t->t_pagetable, target_va, PRIMARY_GMAP_BACKING_VA, primary_priv);
    GigaMap(&t->t_secondary_gmap, t->t_pagetable, target_va + TO_SECONDARY_GMAP, SECONDARY_GMAP_BACKING_VA, secondary_priv);
    copy_to_task(t, u_newpgptr, &target_va, sizeof(target_va));
    __global_flush_tlb();
    return 0;
}

i64 _gmap_carve(task_t *t, virt_t u_carveout) {
    if (!is_user_addr(u_carveout)) return -1;
    GigaMapCarveout(&t->t_primary_gmap, t->t_pagetable, u_carveout);
    GigaMapCarveout(&t->t_secondary_gmap, t->t_pagetable, u_carveout + TO_SECONDARY_GMAP);
    __global_flush_tlb();
    return 0;
}

// The terms "kernel" and "user" gmap are deprecated- now we call them "primary" and "secondary" instead.
// For multithreaded programs, the primary gmap is a kernel one, and the secondary gmap is a user one.
// For single threaded programs, the primary gmap matches whatever privilege that single thread is in, and the secondary gmap is the opposite.
// In the future, we should update these APIs to reflect that change, but for now I'll leave them with their legacy kernel/user names.
i64 _gmap_kgetbase(task_t *t, virt_t u_baseptr) {
    virt_t base_va = PRIMARY_GMAP_BACKING_VA;
    if (!is_user_addr(u_baseptr)) return -1;
    if (ALL_GOOD != GigaMapGetBase(&t->t_primary_gmap, t->t_pagetable, &base_va)) {
        return -2;
    }

    copy_to_task(t, u_baseptr, &base_va, sizeof(base_va));
    __global_flush_tlb();
    return 0;
}

i64 _gmap_ugetbase(task_t *t, virt_t u_baseptr) {
    virt_t base_va = SECONDARY_GMAP_BACKING_VA;
    if (!is_user_addr(u_baseptr)) return -1;
    if (ALL_GOOD != GigaMapGetBase(&t->t_secondary_gmap, t->t_pagetable, &base_va)) {
        return -2;
    }

    copy_to_task(t, u_baseptr, &base_va, sizeof(base_va));
    __global_flush_tlb();
    return 0;
}

i64 _gmap_reset(task_t *t) {
    GigaMapReset(&t->t_primary_gmap, t->t_pagetable);
    GigaMapReset(&t->t_secondary_gmap, t->t_pagetable);
    __global_flush_tlb();
    return 0;
}

i64 _gmap_unmap(task_t *t) {
    GigaMapUnmap(&t->t_primary_gmap, t->t_pagetable);
    GigaMapUnmap(&t->t_secondary_gmap, t->t_pagetable);
    __global_flush_tlb();
    return 0;
}

i64 do_sys_gmap(task_t *t, gigaopt_t opt, u64 arg) {
    switch (opt) {
        case GIGAOPT_ALLOC:
        case GIGAOPT_CARVE:
        case GIGAOPT_UGETBASE:
        case GIGAOPT_KGETBASE:
        case GIGAOPT_RESET:
        case GIGAOPT_UNMAP:
            return gmap_dispatch_table[opt](t,arg);
            break;

        default:
            return -1;
            break;
    }
}

i64 sys_gmap(gigaopt_t opt, u64 arg) {
    return do_sys_gmap(current_task(), opt, arg);
}
