#include <fractal.h>
#include <syscall.h>
#include <task.h>
#include <lib/mem.h>
#include <lib/dlmalloc.h>
#include <copy_task.h>
#include <stdio.h>
#include <filesys/fileio.h>
#include <task.h>

i64 open_procfs(task_t *t) {
    for (usize i = 0; i < NUM_FDS; i++) {
        if (FD_NONE == t->t_fds[i].type) {
            t->t_fds[i].type = FD_PROC;
            t->t_fds[i].resource = NULL;
            t->t_fds[i].offset = 0;
            return i;
        }
    }
    return -5;
}

i64 do_sys_open(task_t *t, char *path_in, u64 flags) {
    return do_sys_openat(t, AT_FDCWD, path_in, flags);
}

i64 sys_open(virt_t path, u64 flags) {
    char copied_path[MAX_FILENAME_LEN];
    if (ALL_GOOD != copy_from_task(current_task(), copied_path, path, MAX_FILENAME_LEN)) return -1;
    copied_path[MAX_FILENAME_LEN-1] = '\x00';
    return do_sys_open(current_task(), copied_path, flags);
}

i64 do_sys_openat(task_t *t, u32 fd, char *path_in, u64 flags) {
    void *dir_to_use = NULL;
    if ((fd > NUM_FDS) && (fd != AT_FDCWD)) return -1;

    if (AT_FDCWD == fd) {
        dir_to_use = t->t_cwd;
    } else {
        dir_to_use = t->t_fds[fd].resource;
        if (t->t_fds[fd].type == FD_NONE) return -2;
    }

#ifdef OPENAT_REJECT_EMPTY_PATHS
    if (strequal("", path_in)) {
        return -3;
    }
#endif // OPENAT_REJECT_EMPTY_PATHS

    void *h = filesys_open_relative(path_in, dir_to_use);
    if (!h) {
        if (flags & O_CREAT) {
            h = filesys_create(path_in, dir_to_use, FILE_REGULAR);
        }
        if (!h) {
            return -3;
        }
    }

    for (usize i = 0; i < NUM_FDS; i++) {
        if (FD_NONE == t->t_fds[i].type) {
            t->t_fds[i].type = FD_FILE;
            t->t_fds[i].resource = h;
            t->t_fds[i].offset = 0;
            t->t_fds[i].attribute = filesys_open_inodenum_relative(path_in, dir_to_use);
#ifdef LOG_SYSCALLS_HIGHLEVEL
            printf("openat(%d, %s) = %d\n", fd, path_in, i);
#endif // LOG_SYSCALLS_HIGHLEVEL
            return i;
        }
    }

    return -4;
}

i64 sys_openat(u32 fd, virt_t user_path, u64 flags) {
    char copied_path[MAX_FILENAME_LEN];
    if ((fd > NUM_FDS) && (fd != AT_FDCWD)) return -1;
    if (ALL_GOOD != copy_from_task(current_task(), copied_path, user_path, MAX_FILENAME_LEN)) return -2;
    copied_path[MAX_FILENAME_LEN-1] = '\x00';
    return do_sys_openat(current_task(), fd, copied_path, flags);
}
