#include <fractal.h>
#include <syscall.h>
#include <task.h>
#include <lib/mem.h>
#include <lib/dlmalloc.h>
#include <copy_task.h>
#include <stdio.h>
#include <filesys/fileio.h>
#include <task.h>
#include <pipe.h>

i64 do_sys_close(task_t *t, u32 fd) {
    if (!t) return -1;
    if (fd > NUM_FDS) return -2;

    if (t->t_fds[fd].type == FD_PIPE) {
        pipe_close((pipe_t *)t->t_fds[fd].resource, t->t_fds[fd].attribute);
    }

    t->t_fds[fd].type = FD_NONE;
    t->t_fds[fd].offset = 0;
    t->t_fds[fd].resource = NULL;
    t->t_fds[fd].attribute = 0;
    return 0;
}

i64 sys_close(u32 fd) {
    if (fd > NUM_FDS) return -1;
    return do_sys_close(current_task(), fd);
}
