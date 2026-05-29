#include <fractal.h>
#include <syscall.h>
#include <task.h>

i64 do_sys_fcntl(task_t *t, u32 fd, u64 cmd, u64 arg) {
#ifdef LOG_SYSCALLS_HIGHLEVEL
    printf("fcntl(%d, %d)\n", fd, cmd);
#endif // LOG_SYSCALLS_HIGHLEVEL

    switch(cmd) {
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        return do_sys_dup(t, fd);
        break;

        case F_GETFD:
        case F_GETFL:
        case F_SETFD:
        case F_SETFL:
        // We support none of these flags
        return 0;
        break;
    }

    return -1;
}

i64 sys_fcntl(u32 fd, u64 cmd, u64 arg) {
    return do_sys_fcntl(current_task(), fd, cmd, arg);
}
