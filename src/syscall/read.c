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

const char *taskstate_lut[] = {
    "Unknown",
    "Running",
    "Runnable",
    "Waiting",
    "Zombie"
};

i64 read_procfs(task_t *t, u32 fd, u8 *buf, usize sz) {
    if (!t) return -1;
    if (fd > NUM_FDS) return -2;

    if (t->t_fds[fd].offset != 0) return 0;

    usize bytes_read = 0;
    thread_t *thread_cursor = current_thread();
    do {
        bytes_read += snprintf((char *)&buf[bytes_read], "[%s] [%d] [%s] %s\r\n",
                                thread_cursor->t_kind == USER_THREAD ? "user" : "kern",
                                thread_cursor->t_task->t_pid,
                                taskstate_lut[thread_cursor->t_state],
                                thread_cursor->t_task->t_name);
        thread_cursor = thread_cursor->t_next;
    } while(thread_cursor != current_thread());

    if (bytes_read > sz) {
        panic("We overflowed the kernel buffer :( need to make snprintf aware of buffer sizes");
        bytes_read = sz;
    }

    t->t_fds[fd].offset += bytes_read;
    return bytes_read;
}

i64 do_sys_read(task_t *t, u32 fd, u8 *buf, usize sz) {
    usize bytes_read = 0;
    if (sz > SYS_READWRITE_LEN_MAX) sz = SYS_READWRITE_LEN_MAX;

    switch(t->t_fds[fd].type) {
        case FD_SERIAL:
        case FD_CONSOLE:
        // This will put the task to sleep!
        bytes_read = task_read_stdio(t, fd, buf, sz);
        break;

        case FD_FILE:
        bytes_read = filesys_read(t->t_fds[fd].resource, buf, sz, t->t_fds[fd].offset);
        t->t_fds[fd].offset += bytes_read;
        break;

        case FD_PROC:
        bytes_read = read_procfs(t, fd, buf, sz);
        break;

        case FD_PIPE:
        // This may put the task to sleep!
        bytes_read = pipe_read((pipe_t *)t->t_fds[fd].resource, t->t_fds[fd].attribute, buf, sz);
        break;
    }

    return bytes_read;
}

i64 sys_read(u32 fd, virt_t userbuf, usize sz) {
    if (fd > NUM_FDS) return -1;
    if (current_task()->t_fds[fd].type == FD_NONE) return -2;
    if (sz > SYS_READWRITE_LEN_MAX) sz = SYS_READWRITE_LEN_MAX;

    u8 *kernelbuf = (u8 *)dlmalloc(sz);
    if (!kernelbuf) return -3;

    i64 rval = do_sys_read(current_task(), fd, kernelbuf, sz);

    if (rval < 0) return rval;
    usize bytes_read = rval;

    if (ALL_GOOD != copy_to_task(current_task(), userbuf, kernelbuf, bytes_read)) {
        dlfree(kernelbuf);
        return -3;
    }

    dlfree(kernelbuf);
    return rval;
}
