#include <fractal.h>
#include <syscall.h>
#include <task.h>

i64 do_sys_dup2(task_t *t, u32 fd_from, u32 fd_to) {
    if (!t) return -1;
    if (fd_from > NUM_FDS) return -2;
    if (fd_to > NUM_FDS) return -3;

    do_sys_close(t, fd_to);
    copy_fd(&t->t_fds[fd_to], &t->t_fds[fd_from]);
    return fd_to;
}

i64 sys_dup2(u32 fd_from, u32 fd_to) {
    return do_sys_dup2(current_task(), fd_from, fd_to);
}
