#include <syscall.h>
#include <task.h>

i64 do_sys_fork(task_t *caller) {
    task_t *child = NULL;

    if (current_task() != caller) panic("tried to fork a task that isn't the current calling task");
    child = fork_task(current_task(), current_thread());
    set_return_value(&child->t_threads[INIT_THREAD]->t_init_regs, FORK_PARENT_RETURNVAL);
    schedule_task(child);
    return child->t_pid;
}

i64 sys_fork() {
    return do_sys_fork(current_task());
}
