#include <types.h>
#include <lib/mem.h>
#include <page_alloc.h>
#include <virt_mem.h>
#include <out.h>
#include <page.h>
#include <task.h>
#include <syscall.h>
#include <copy_task.h>
#include <filesys/fileio.h>
#include <paging.h>

typedef i64 (*SystemCall)(u64, u64, u64, u64, u64);

SystemCall sys_table[NUM_SYSCALLS] = {
    [SYS_HANDOFF ]  =  (SystemCall)&sys_handoff,
    [SYS_SPAWN   ]  =  (SystemCall)&sys_spawn,
    [SYS_QUIT    ]  =  (SystemCall)&sys_quit,
    [SYS_OPEN    ]  =  (SystemCall)&sys_open,
    [SYS_READ    ]  =  (SystemCall)&sys_read,
    [SYS_WRITE   ]  =  (SystemCall)&sys_write,
    [SYS_CLOSE   ]  =  (SystemCall)&sys_close,
    [SYS_STAT    ]  =  (SystemCall)&sys_stat,
    [SYS_LSEEK   ]  =  (SystemCall)&sys_lseek,
    [SYS_MKDIR   ]  =  (SystemCall)&sys_mkdir,
    [SYS_CHDIR   ]  =  (SystemCall)&sys_chdir,
    [SYS_FORK    ]  =  (SystemCall)&sys_fork,
    [SYS_EXECVE  ]  =  (SystemCall)&sys_execve,
    [SYS_WAIT    ]  =  (SystemCall)&sys_wait,
    [SYS_GETDENTS]  =  (SystemCall)&sys_getdents,
    [SYS_PIPE    ]  =  (SystemCall)&sys_pipe,
    [SYS_DUP     ]  =  (SystemCall)&sys_dup,
    [SYS_DUP2    ]  =  (SystemCall)&sys_dup2,
    [SYS_FCNTL   ]  =  (SystemCall)&sys_fcntl,
    [SYS_UNAME   ]  =  (SystemCall)&sys_uname,
    [SYS_FSTATAT ]  =  (SystemCall)&sys_fstatat,
    [SYS_OPENAT  ]  =  (SystemCall)&sys_openat,
    [SYS_FCHDIR  ]  =  (SystemCall)&sys_fchdir,
    [SYS_ACCESS  ]  =  (SystemCall)&sys_access,
    [SYS_SYSCONF ]  =  (SystemCall)&sys_sysconf,
    [SYS_UNLINK  ]  =  (SystemCall)&sys_unlink,
    [SYS_WAITPID ]  =  (SystemCall)&sys_waitpid,
    [SYS_GETPID  ]  =  (SystemCall)&sys_getpid,
    [SYS_KEXEC   ]  =  (SystemCall)&sys_kexec,
    [SYS_FRACTAL ]  =  (SystemCall)&sys_fractal,
    [SYS_SELECT  ]  =  (SystemCall)&sys_select,
    [SYS_POLL    ]  =  (SystemCall)&sys_poll,
    [SYS_TIOS_GET]  =  (SystemCall)&sys_tcgetattr,
    [SYS_TIOS_SET]  =  (SystemCall)&sys_tcsetattr,
    [SYS_RMDIR   ]  =  (SystemCall)&sys_rmdir,
    [SYS_SBRK    ]  =  (SystemCall)&sys_sbrk,
    [SYS_FT_CREAT]  =  (SystemCall)&sys_fthread_create,
    [SYS_FT_JOIN ]  =  (SystemCall)&sys_fthread_join,
    [SYS_FT_EXIT ]  =  (SystemCall)&sys_fthread_exit,
    [SYS_FT_SWTCH]  =  (SystemCall)&sys_fthread_switch,
    [SYS_MPROTECT]  =  (SystemCall)&sys_mprotect,
    [SYS_FMAP    ]  =  (SystemCall)&sys_fmap,
    [SYS_XLATE   ]  =  (SystemCall)&sys_xlate,
    [SYS_FUNMAP  ]  =  (SystemCall)&sys_funmap,
    [SYS_GIGAMAP ]  =  (SystemCall)&sys_gmap,
};

// Simple sanity checks to perform every syscall to detect bugs
void sanity_check_thread() {
    u64 thread_runtime_sp_va = 0;
    u64 thread_runtime_sp_pa = 0;

    // Make sure the stack is correct for this thread
    switch(current_thread()->t_kind) {
        case USER_THREAD:
        if (!addr_in_page(get_current_sp(), current_thread()->t_int_stack_page, LARGE_PAGE_SIZE)) {
            panic("sanity_check_thread: stack pointer (0x%X) isn't in t_int_stack_page", get_current_sp());
        }
        break;

        case KERNEL_OUTER_THREAD:
        case KERNEL_INNER_THREAD:
        if (!addr_in_page(get_current_sp(), current_thread()->t_run_stack_page, LARGE_PAGE_SIZE)) {
            panic("sanity_check_thread: stack pointer (0x%X) isn't in t_run_stack_page", get_current_sp());
        }
        thread_runtime_sp_va = get_runtime_sp(current_thread(), current_thread()->t_regs);
        thread_runtime_sp_pa = PageWalk(current_task()->t_pagetable, thread_runtime_sp_va);
        if (PHYS_ADDR_INVALID == thread_runtime_sp_pa ||
            !addr_in_page(KERN_P2V(thread_runtime_sp_pa), current_thread()->t_run_stack_page, LARGE_PAGE_SIZE)) {
            panic("sanity_check_thread: runtime stack pointer (0x%X) out of bounds", KERN_P2V(thread_runtime_sp_pa));
        }
        break;
    }

    // Confirm the state of the thread and its corresponding task
    if (current_thread()->t_state != THREAD_RUNNING) {
        panic("sanity_check_thread: thread isn't running (in state %d)", current_thread()->t_state);
    }
    if (current_task()->t_state != TASK_HEALTHY) {
        panic("sanity_check_thread: task isn't running (in state %d)", current_thread()->t_state);
    }
    if (current_thread()->t_task != current_task()) {
        panic("sanity_check_thread: current_thread() and current_task() disagree");
    }
}

void Syscall(u64 num, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) {
    u64 rval = (u64)ALL_GOOD;
    sanity_check_thread(); // While we're here, make sure things make sense
    if (num >= NUM_SYSCALLS) rval = IDK_WHAT_THAT_THING_IS;
    else rval = sys_table[num](arg0, arg1, arg2, arg3, arg4);
    set_return_value(current_thread()->t_regs, rval);
}
