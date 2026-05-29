#include <fractal.h>
#include <syscall.h>
#include <task.h>
#include <copy_task.h>
#include <filesys/fileio.h>

i64 sys_mkdir(virt_t path_in) {
    char copybuf[MAX_FILENAME_LEN];
    if (ALL_GOOD != copy_from_task(current_task(), &copybuf, path_in, MAX_FILENAME_LEN)) {
        return -1;
    }
    copybuf[MAX_FILENAME_LEN-1] = '\x00';
    return do_sys_mkdir(current_task(), (char *)&copybuf);
}

i64 do_sys_mkdir(task_t *t, char *path_in) {
    return filesys_mkdir(path_in, t->t_cwd);
}

i64 sys_chdir(virt_t path_in) {
    char copybuf[MAX_FILENAME_LEN];
    if (ALL_GOOD != copy_from_task(current_task(), &copybuf, path_in, MAX_FILENAME_LEN)) {
        return -1;
    }
    copybuf[MAX_FILENAME_LEN-1] = '\x00';
    return do_sys_chdir(current_task(), (char *)&copybuf);
}

i64 do_sys_chdir(task_t *t, char *path_in) {
    void *handle = filesys_open_relative(path_in, t->t_cwd);
    if (!handle) return -1;
    if (!filesys_is_dir(handle)) return -2;
    t->t_cwd = handle;
    return 0;
}

i64 do_sys_fchdir(task_t *t, u32 fd) {
    if (fd > NUM_FDS) return -1;
    if (!filesys_is_dir(t->t_fds[fd].resource)) return -2;
    t->t_cwd = t->t_fds[fd].resource;
    return 0;
}

i64 sys_fchdir(u32 fd) {
    return do_sys_fchdir(current_task(), fd);
}
