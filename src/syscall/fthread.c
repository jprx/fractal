#include <fractal.h>
#include <syscall.h>
#include <paging.h>
#include <virt_mem.h>
#include <task.h>
#include <regs.h>
#include <copy_task.h>

bool fthread_exception(void *regs_in) {
    // If pc points to the wrong map for this thread, relocate it and return true
    // Otherwise, return false
    regs_t *r = (regs_t *)regs_in;

    if (1 != current_thread()->t_preempt_count) {
        // Can't do anything if this isn't the root level of preemption
        // (preemption > 1 means the kernel took a trap while doing something on behalf of the task)
        return false;
    }

    if (!current_thread()->t_is_fthread) {
        return false;
    }

    if (r != current_thread()->t_regs) panic("current thread and exception handler disagree on where the regs are");

    switch(current_thread()->t_kind) {
        case USER_THREAD:
        if (IS_PTR_IN_FTHREAD_KERN_MAP(get_pc(r))) {
            panic("fixing up a user thread (this case actually can indeed happen)");
            set_pc(r, TO_FTHREAD_USER_MAP(get_pc(r)));
            return true;
        }
        break;

        case KERNEL_OUTER_THREAD:
        if (IS_PTR_IN_FTHREAD_USER_MAP(get_pc(r))) {
            set_pc(r, TO_FTHREAD_KERN_MAP(get_pc(r)));
            return true;
        }
        break;

        case KERNEL_INNER_THREAD:
        // Can't do anything at all about this case (it's a real exception)
        break;
    }

    return false;
}

i64 sys_fthread_create(virt_t u_thread, u64 attr, virt_t u_start, virt_t u_arg) {
    thread_t *new_thread = NULL;
    ThreadKind new_thread_kind = (ThreadKind)-1;
    virt_t new_thread_pc = (virt_t)0;
    bool new_thread_ints_on = true;

    // Setup defaults- need to opt into kernel / cooperative modes
    if (0 == (attr & (FTHREAD_USER_THREAD | FTHREAD_KERNEL_THREAD))) attr |= FTHREAD_USER_THREAD;
    if (0 == (attr & (FTHREAD_PREEMPTIVE | FTHREAD_COOPERATIVE))) attr |= FTHREAD_PREEMPTIVE;

    if (PHYS_ADDR_INVALID == PageWalk(current_task()->t_pagetable, u_start)) {
        return -1;
    }
    if (!IS_USER_VA(u_start)) {
        return -2;
    }

    if (0 != (attr & FTHREAD_USER_THREAD)) {
        new_thread_pc = TO_FTHREAD_USER_MAP(u_start);
        new_thread_kind = USER_THREAD;
    }

    if (0 != (attr & FTHREAD_KERNEL_THREAD)) {
        new_thread_pc = TO_FTHREAD_KERN_MAP(u_start);
        new_thread_kind = KERNEL_OUTER_THREAD;
    }

    if (0 != (attr & FTHREAD_PREEMPTIVE)) {
        new_thread_ints_on = true;
    }

    if (0 != (attr & FTHREAD_COOPERATIVE)) {
        new_thread_ints_on = false;
    }

    new_thread = create_thread_in_task(current_task(), new_thread_kind, new_thread_ints_on, new_thread_pc);
    if (!new_thread) panic("fthread_create: failed to create a new thread");
    new_thread->t_is_fthread = true;

#ifdef ARCH_HAS_GLOBAL_POINTER
    // New thread needs a global pointer, so copy it from the current thread
    set_gp(new_thread, &new_thread->t_init_regs, get_gp(current_thread(), current_thread()->t_regs));
#endif // ARCH_HAS_GLOBAL_POINTER

    schedule_thread(new_thread);
    copy_to_task(current_task(), u_thread, &new_thread->t_tid, sizeof(new_thread->t_tid));
    return 0;
}

i64 sys_fthread_join(tid_t tid, virt_t u_val_ptr) {
    if (tid > MAX_THREADS) return -1;
    if (!current_task()->t_threads[tid]) return 0;
    if (current_task()->t_threads[tid]->t_state == THREAD_ZOMBIE) return 0;
    sleep_on(current_task()->t_threads[tid]);
    return 0;
}

i64 sys_fthread_switch(tid_t tid) {
    if (tid > MAX_THREADS) return -1;
    if (!current_task()->t_threads[tid]) return -2;
    if (current_task()->t_threads[tid] == current_thread()) return 0; // it's us, return ok
    if (current_task()->t_threads[tid]->t_state != THREAD_RUNNABLE) return -3;
    switchto(current_task()->t_threads[tid]);
    return 0;
}

i64 sys_fthread_exit(virt_t u_val_ptr) {
    wakeup_resource(current_thread());
    quit_thread(current_thread());
    naptime();
    return 0;
}
