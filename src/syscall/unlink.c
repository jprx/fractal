#include <fractal.h>
#include <syscall.h>
#include <copy_task.h>

i64 do_sys_unlink(task_t *t, char *path) {
    return -1; // Disabled for now
}

i64 sys_unlink(virt_t user_path) {
    char copied_path[MAX_FILENAME_LEN];
    if (ALL_GOOD != copy_from_task(current_task(), copied_path, user_path, MAX_FILENAME_LEN)) return -1;
    copied_path[MAX_FILENAME_LEN-1] = '\x00';
    return do_sys_unlink(current_task(), copied_path);
}

i64 sys_rmdir(virt_t u_filename) {
    return sys_unlink(u_filename);
}

i64 do_sys_rmdir(task_t *t, char *path) {
    return do_sys_unlink(t, path);
}
