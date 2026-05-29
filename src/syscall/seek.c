#include <fractal.h>
#include <syscall.h>
#include <task.h>
#include <lib/mem.h>
#include <lib/dlmalloc.h>
#include <copy_task.h>
#include <stdio.h>
#include <filesys/fileio.h>
#include <task.h>

i64 do_sys_lseek(task_t *t, u32 fd, i32 offset, int whence) {
    if (fd > NUM_FDS) return -1;
    if (t->t_fds[fd].type == FD_NONE) return -2;
    if (t->t_fds[fd].type == FD_CONSOLE || t->t_fds[fd].type == FD_SERIAL) return -3;

    switch(whence) {
        case LSEEK_WHENCE_SET:
        if ((i32)offset < 0) return -1;
        else t->t_fds[fd].offset = offset;
        break;

        case LSEEK_WHENCE_CUR:
        if ((i32)t->t_fds[fd].offset + offset < 0) return -1;
        else t->t_fds[fd].offset += offset;
        break;

        default:
        printf("Warning: Unsupported lseek command (%d)\r\n", whence);
        return -1;
        break;
    }

    return t->t_fds[fd].offset;
}

i64 sys_lseek(u32 fd, i32 offset, int whence) {
    return do_sys_lseek(current_task(), fd, offset, whence);
}
