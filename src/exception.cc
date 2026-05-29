#include <fractal.h>
#include <exception.h>
#include <task.h>
#include <out.h>
#include <syscall.h>
#include <lib/mem.h>
#include <fthread.h>

#define LOG_EXCEPTIONS
#define QUIT_USERTASKS_ON_EXCEPTION 1

extern "C" void Exception(regs_t *r, exception_kind kind, u64 reason) {

    // If fthread can handle this exception, let it
    // Otherwise we need to handle it
    if (fthread_exception(r)) return;

#ifdef LOG_EXCEPTIONS
    printf("Exception %d:0x%X\r\n", kind, reason);
    if (current_thread()) printf("task %s\n", current_thread()->t_task->t_name);
    dump_regs(r);
#endif // LOG_EXCEPTIONS

#ifdef QUIT_USERTASKS_ON_EXCEPTION
    if (NULL != current_thread() && (USER_THREAD == current_thread()->t_kind || (KERNEL_OUTER_THREAD == current_thread()->t_kind)) && current_thread()->t_preempt_count < 2) {
        do_sys_write(current_task(), STDIO_ERR, (u8 *)PROCESS_KILL_MESSAGE, strlen(PROCESS_KILL_MESSAGE));
        quit_task(current_thread()->t_task, -1);
        panic("[exception] We should never get here (in Exception)");
        return;
    }
#endif // QUIT_USERTASKS_ON_EXCEPTION

    switch(kind) {
        case EXC_PAGE_FAULT:
            panic("Page Fault (0x%X)", reason);
        break;

        case EXC_ILLEGAL_INSTRUCTION:
            panic("Illegal Instruction");
        break;

        case EXC_UNKNOWN:
        default:
            panic("Unknown Exception %d", kind);
        break;
    }
}
