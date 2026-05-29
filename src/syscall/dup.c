#include <fractal.h>
#include <syscall.h>
#include <task.h>

i64 do_sys_dup(task_t *t, u32 fd_from) {
    if (!t) return -1;
    if (fd_from > NUM_FDS) return -2;

    for (usize i = 0; i < NUM_FDS; i++) {
        if (t->t_fds[i].type == FD_NONE) {
            return do_sys_dup2(t, fd_from, i);
        }
    }

    return -1;
}

i64 sys_dup(u32 fd_from) {
    return do_sys_dup(current_task(), fd_from);
}
