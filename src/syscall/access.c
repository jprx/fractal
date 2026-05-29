#include <fractal.h>
#include <syscall.h>
#include <copy_task.h>
#include <filesys/fileio.h>

i64 do_sys_access(task_t *t, char *path, u64 mode) {
    return filesys_exists(path, t->t_cwd) ? 0 : -1;
}

i64 sys_access(virt_t user_path, u64 mode) {
    char copied_path[MAX_FILENAME_LEN];
    if (ALL_GOOD != copy_from_task(current_task(), copied_path, user_path, MAX_FILENAME_LEN)) return -1;
    copied_path[MAX_FILENAME_LEN-1] = '\x00';
    return do_sys_access(current_task(), copied_path, mode);
}
