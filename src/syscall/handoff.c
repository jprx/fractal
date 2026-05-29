#include <fractal.h>
#include <syscall.h>
#include <task.h>
#include <out.h>

u64 sys_handoff() {
    if (!current_thread()) panic("Someone called handoff, but no task even exists to hand off!");
    if (!current_thread()->t_next) panic("handoff() ran out of stuff to do");
    naptime();
    return ALL_GOOD;
}
