#include <fractal.h>
#include <syscall.h>

i64 do_sys_sysconf(task_t *t, u64 name) {
    return -1;
}

i64 sys_sysconf(u64 name) {
    return do_sys_sysconf(current_task(), name);
}
