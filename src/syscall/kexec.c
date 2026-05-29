#include <fractal.h>
#include <syscall.h>
#include <task.h>

i64 do_sys_kexec(task_t *t, virt_t new_kernel_path) {
    return -1;
}

i64 sys_kexec(virt_t new_kernel_path) {
    return -1;
}
