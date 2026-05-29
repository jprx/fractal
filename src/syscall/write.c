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

// Assumes buf is a kernel address that has been copyin'd,
// if it needed to be
i64 do_sys_write(task_t *t, u32 fd, u8 *buf, usize sz) {
    usize bytes_written = 0;
    if (!t) return -3;
    if (fd > NUM_FDS) return -4;
    if (sz > SYS_READWRITE_LEN_MAX) sz = SYS_READWRITE_LEN_MAX; // panic("write too large");

    switch(t->t_fds[fd].type) {
        case FD_SERIAL:
        case FD_CONSOLE:
        task_write_stdio(t, fd, buf, sz);
        bytes_written = sz;
        break;

        case FD_FILE:
        bytes_written = filesys_write(t->t_fds[fd].resource, buf, sz, t->t_fds[fd].offset);
        t->t_fds[fd].offset += bytes_written;
        break;

        case FD_PIPE:
        // This may put the task to sleep!
        bytes_written = pipe_write((pipe_t *)t->t_fds[fd].resource, t->t_fds[fd].attribute, buf, sz);
        break;
    }

    return bytes_written;
}

i64 sys_write(u32 fd, virt_t userbuf, usize sz) {
    if (fd > NUM_FDS) return -1;
    if (current_task()->t_fds[fd].type == FD_NONE) return -1;
    if (sz > SYS_READWRITE_LEN_MAX) sz = SYS_READWRITE_LEN_MAX;

    u8 *kernelbuf = (u8 *)dlmalloc(sz);
    if (!kernelbuf) return -3;
    if (ALL_GOOD != copy_from_task(current_task(), (void *)kernelbuf, (virt_t)userbuf, sz)) {
        dlfree(kernelbuf);
        return -4;
    }

    i64 rval = do_sys_write(current_task(), fd, kernelbuf, sz);

    dlfree(kernelbuf);
    return rval;
}
