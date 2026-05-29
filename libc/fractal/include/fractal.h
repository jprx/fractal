#ifndef FRACTAL_H
#define FRACTAL_H

// Fractal userspace bindings
// Keep these in-sync with the kernel!

#include <stdint.h>
#include <stdbool.h>

uint64_t sys_fractal(uint64_t cmd, uint64_t arg);

// Part of libc.a (via syscalls.c in libc/fractal):
uint64_t handoff();

typedef enum {
    FRACTAL_SET_SCHEDULER_AFFINITY = 1,
    FRACTAL_SET_THREAD_KIND = 2,
    FRACTAL_GET_TASK_KIND = 3,
    FRACTAL_GET_KERNEL_BASE = 4,
    FRACTAL_SET_PREEMPTION_MODE = 5,
    FRACTAL_SET_SCHEDULER_LOGGING = 6,
    FRACTAL_SET_MEMORY_TRACING = 7,
} sysfractal_arg_t;

typedef enum {
    USER_THREAD,          // An ELF (user)
    KERNEL_OUTER_THREAD,  // An ELF (kernel)
    KERNEL_INNER_THREAD,  // A task in the kernel (ex: Oxide, Init)
} ThreadKind;

typedef enum {
    FRACTAL_SCHEDULER_PREEMPTIVE = 0,
    FRACTAL_SCHEDULER_COOPERATIVE = 1,
} scheduler_mode_t;

static inline void fractal_promote_kernel() {
    sys_fractal(
        FRACTAL_SET_THREAD_KIND,
        KERNEL_OUTER_THREAD
    );
}

static inline void fractal_demote_user() {
    sys_fractal(
        FRACTAL_SET_THREAD_KIND,
        USER_THREAD
    );
}

static inline char *fractal_get_task_flavor() {
    ThreadKind cur_kind = sys_fractal(
        FRACTAL_GET_TASK_KIND,
        0
    );

    switch (cur_kind) {
        case USER_THREAD: return "User"; break;
        case KERNEL_OUTER_THREAD: return "Kernel"; break;
        case KERNEL_INNER_THREAD: return "Kernel Inner (how is that possible?)"; break;
        default: return "Unknown"; break;
    }
}

static inline uint64_t fractal_get_kernel_base() {
    return sys_fractal(
        FRACTAL_GET_KERNEL_BASE,
        0
    );
}

static inline uint64_t fractal_set_preemption_mode(scheduler_mode_t arg) {
    return sys_fractal(
        FRACTAL_SET_PREEMPTION_MODE,
        (uint64_t)arg
    );
}

static inline uint64_t fractal_set_scheduler_affinity(uint64_t next_pid) {
    return sys_fractal(
        FRACTAL_SET_SCHEDULER_AFFINITY,
        (uint64_t)next_pid
    );
}

static inline uint64_t fractal_set_logging(bool log_on) {
    return sys_fractal(
        FRACTAL_SET_SCHEDULER_LOGGING,
        (uint64_t)log_on
    );
}

static inline uint64_t ftrace_enable() {
    return sys_fractal(FRACTAL_SET_MEMORY_TRACING, true);
}

static inline uint64_t ftrace_disable() {
    return sys_fractal(FRACTAL_SET_MEMORY_TRACING, false);
}

#endif // FRACTAL_H
