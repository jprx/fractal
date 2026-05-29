#include <fractal.h>
#include <syscall.h>
#include <task.h>
#include <regs.h>

extern u8 __very_start_of_kernel_mem;
extern bool __scheduler_logging;

i64 do_sys_fractal(task_t *t, u64 cmd, u64 arg) {
    if (current_thread()->t_task != t || current_task() != t) {
        panic("called do_sys_fractal on something other than the current task or thread");
    }

    switch((sysfractal_arg_t)cmd) {
        case FRACTAL_SET_SCHEDULER_AFFINITY:
        current_thread()->t_sched_pref = (pid_t)arg;
        current_thread()->t_has_sched_pref = true;
        return 0;
        break;

        case FRACTAL_SET_THREAD_KIND:
        return set_thread_kind(current_thread(), arg);
        break;

        case FRACTAL_GET_TASK_KIND:
        return current_thread()->t_kind;
        break;

        case FRACTAL_SET_PREEMPTION_MODE:
        switch ((scheduler_mode_t)arg) {
            case FRACTAL_SCHEDULER_PREEMPTIVE:
            current_thread()->t_interrupts_on = true;
            enable_interrupts(current_thread()->t_regs);
            break;

            case FRACTAL_SCHEDULER_COOPERATIVE:
            current_thread()->t_interrupts_on = false;
            disable_interrupts(current_thread()->t_regs);
            break;

            default: return -1; break;
        }
        break;

        case FRACTAL_GET_KERNEL_BASE:
        return KERN_P2V((u64)&__very_start_of_kernel_mem);
        break;

        case FRACTAL_SET_SCHEDULER_LOGGING:
        __scheduler_logging = (bool)arg;
        break;

        case FRACTAL_SET_MEMORY_TRACING:
        t->t_tracing_mode = (bool)arg;
        break;
    }

    return -1;
}

i64 sys_fractal(u64 cmd, u64 arg) {
    return do_sys_fractal(current_task(),cmd,arg);
}