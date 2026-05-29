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

i64 do_sys_stat(task_t *t, u32 fd, struct stat *buf) {
    if (fd > NUM_FDS) return -1;
#ifdef LOG_SYSCALLS_HIGHLEVEL
    printf("stat(%d)\n", fd);
#endif // LOG_SYSCALLS_HIGHLEVEL

    switch (t->t_fds[fd].type) {
        case FD_NONE:
        return 0;
        break;

        case FD_FILE:
        buf->st_dev = 0;
        buf->st_ino = t->t_fds[fd].attribute;
        buf->st_mode = ACCESSPERMS | (filesys_is_file(t->t_fds[fd].resource) ? STAT_MODE_REG : STAT_MODE_DIR);
        buf->st_size = filesys_filelen(t->t_fds[fd].resource);
        return 0;
        break;

        case FD_CONSOLE:
        case FD_SERIAL:
        buf->st_dev = 1;
        buf->st_mode = ACCESSPERMS | STAT_MODE_CHAR;
        buf->st_size = SYS_READWRITE_LEN_MAX;
        return 0;
        break;

        case FD_PIPE:
        buf->st_dev = 2;
        buf->st_size = PIPE_BUFLEN;
        buf->st_mode = ACCESSPERMS  | STAT_MODE_FIFO;
        return 0;
        break;

        default:
        return -1;
        break;
    }

    return 0;
}

i64 sys_stat(u32 fd, virt_t userbuf) {
    struct stat kern_statbuf = {0};
    i64 rval = 0;
    if (fd > NUM_FDS) return -1;

    rval = do_sys_stat(current_task(), fd, &kern_statbuf);

    if (0 != rval) return rval;

    if (ALL_GOOD != copy_to_task(
        current_task(),
        userbuf,
        &kern_statbuf,
        sizeof(kern_statbuf))) {
        return -2;
    }

    return 0;
}

i64 do_sys_fstatat(task_t *t, u32 fd, char *path, struct stat *buf, u32 flags) {
    void *dir_to_use = NULL;
    if ((fd > NUM_FDS) && (fd != AT_FDCWD)) return -1;

#ifdef LOG_SYSCALLS_HIGHLEVEL
    printf("fstatat(%d, %s)\n", fd, path);
#endif // LOG_SYSCALLS_HIGHLEVEL

    if (AT_FDCWD == fd) {
        dir_to_use = t->t_cwd;
    } else {
        dir_to_use = t->t_fds[fd].resource;
        if (t->t_fds[fd].type == FD_NONE) return -2;
    }

    void *handle = filesys_open_relative(path, dir_to_use);
    if (!handle) return -3;

    buf->st_ino = filesys_open_inodenum_relative(path, dir_to_use);
    buf->st_mode = ACCESSPERMS | (filesys_is_file(handle) ? STAT_MODE_REG : STAT_MODE_DIR);
    buf->st_size = filesys_filelen(handle);

    filesys_close(handle);

    return 0;
}

i64 sys_fstatat(u32 fd, virt_t user_path, virt_t user_statbuf, u32 flags) {
    struct stat kern_statbuf = {0};
    char kern_binpath[MAX_FILENAME_LEN];
    i64 rval = 0;
    if ((fd > NUM_FDS) && (fd != AT_FDCWD)) return -1;

    if (ALL_GOOD != copy_from_task(
        current_task(),
        kern_binpath,
        user_path,
        MAX_FILENAME_LEN
    )) {
        return -2;
    }

    kern_binpath[MAX_FILENAME_LEN-1] = '\x00';

    rval = do_sys_fstatat(current_task(), fd, kern_binpath, &kern_statbuf, flags);

    if (0 != rval) return rval;

    if (ALL_GOOD != copy_to_task(
        current_task(),
        user_statbuf,
        &kern_statbuf,
        sizeof(kern_statbuf))) {
        return -3;
    }

    return 0;
}
