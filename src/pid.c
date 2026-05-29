#include <fractal.h>
#include <task.h>

static u64 process_id_counter = 0;
pid_t alloc_pid() {
    pid_t rval = process_id_counter;
    if (rval == PID_ANY) panic("alloc_pid: ran out of PIDs");
    process_id_counter++;
    return rval;
}
