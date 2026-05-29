#include "fmap.h"
#include <fractal.h>
#include <task.h>
#include <out.h>
#include <virt_mem.h>
#include <page.h>
#include <lib/mem.h>
#include <page_alloc.h>
#include <lib/dlmalloc.h>
#include <arch.h>
#include <regs.h>
#include <filesys/fileio.h>
#include <oxide/console.h>
#include <paging.h>
#include <syscall.h>
#include <loader/elf.h>
#include <pipe.h>
#include <wait.h>
#include <pid.h>
#include <kasan.h>

// #define TASKSWITCH_DEBUG 1
// #define DEBUG_FREE_TASKS 1

extern FRACTALCORE_CLASS global_cpu;

thread_t *cur_thread = NULL;
task_t *kernel_task = NULL;

// Runtime-tunable that configures whether we log what the scheduler is doing
bool __scheduler_logging = false;

task_t *current_task() {
    if (cur_thread) return cur_thread->t_task;
    return NULL;
}

thread_t *current_thread() {
    return cur_thread;
}

const char *get_task_name(task_t *t) {
    if (!t) return "[null task]";
    return (const char*)t->t_name;
}

const char *get_thread_name(thread_t *t) {
    if (!t) return "[null thread]";
    return get_task_name(t->t_task);
}

thread_t *alloc_new_blank_thread(ThreadKind kind, bool interrupts_on) {
    // Allocate tasks with the page allocator to ensure they are aligned properly
    // This is needed eg. when saving / restoring vector registers, need 16 byte alignment.
    thread_t *t = (thread_t *)AllocPage(SMALL_PAGE);
    if (!t) return NULL;

    if (sizeof(*t) >= SMALL_PAGE_SIZE) {
        panic("thread_t structure (%d) is too large to fit into a small page", sizeof(*t));
    }

    memset(t, '\x00', sizeof(*t));
    for (usize i = 0; i < sizeof(regs_t) / sizeof(u64); i++) {
        ((u64 *)&(t->t_init_regs))[i] = 0;
    }

    t->t_run_stack_page = AllocPage(LARGE_PAGE);
    if (!t->t_run_stack_page) {
        FreePage((virt_t)t);
        return NULL;
    }

    if (kind == USER_THREAD) {
        t->t_int_stack_page = AllocPage(LARGE_PAGE);
        if (!t->t_int_stack_page) {
            FreePage((virt_t)t->t_run_stack_page);
            FreePage((virt_t)t);
            return NULL;
        }
    } else {
        t->t_int_stack_page = (virt_t)0;
    }

    t->t_regs = NULL;
    t->t_launched = false;
    t->t_kind = kind;
    t->t_state = THREAD_RUNNABLE;
    t->t_next = t;
    t->t_prev = t;
    t->t_has_sched_pref = false;
    t->t_saved_sp = (virt_t)0;
    t->t_preempt_count = 0;
    t->t_is_fthread = false;
    t->t_interrupts_on = interrupts_on;
    t->t_tid = -1;
    return t;
}

task_t *alloc_new_blank_task() {
    task_t *t = (task_t*)dlmalloc(sizeof(*t));
    if (!t) return NULL;
    memset(t, '\x00', sizeof(*t));
    t->t_pid = alloc_pid();
    t->t_pagetable = NULL;
    t->t_state = TASK_HEALTHY;
    t->t_cwd = filesys_root_resource();
    t->t_heap_top = (virt_t)NULL;
    t->t_fmap_top = (virt_t)DEFAULT_USER_PAGE_HEAP;
    t->t_parent = NULL;
    t->t_fmaplist = NULL;
    t->t_waitlist = NULL;
    t->t_owner = NULL;
    t->t_tracing_mode = false;
    init_gmap(&t->t_primary_gmap);
    init_gmap(&t->t_secondary_gmap);
    memset(&t->t_name, '\x00', TASKNAME_LEN);
    for (int i = 0; i < NUM_FDS; i++) t->t_fds[i].type = FD_NONE;
    for (int i = 0; i < MAX_THREADS; i++) t->t_threads[i] = NULL;
    return t;
}

// Call this when constructing a new thread and pairing it to a task
// Returns the thread ID of the new thread.
void assign_thread_to_task(task_t *parent_task, thread_t *child_thread) {
    if (KERNEL_INNER_THREAD == child_thread->t_kind && kernel_task != parent_task) {
        panic("tried to assign a kernel inner thread to a task that isn't the kernel_task");
    }
    if (kernel_task == parent_task && KERNEL_INNER_THREAD != child_thread->t_kind) {
        panic("tried to create a non-kernel inner thread inside kernel_task");
    }

    child_thread->t_task = parent_task;
    for (usize i = 0; i < MAX_THREADS; i++) {
        if (!parent_task->t_threads[i]) {
            parent_task->t_threads[i] = child_thread;
            child_thread->t_tid = i;
            return;
        }
    }
    panic("failed to find a free thread slot in task %s\n", parent_task->t_name);
}

thread_t *create_thread_in_task(task_t *parent_task, ThreadKind kind, bool interrupts_on, virt_t task_pc) {
    thread_t *t = NULL;
    u64 *runtime_stack_page = NULL;
    u64 runtime_stack_va = 0;

    if (!parent_task) panic("create_thread_in_task: NULL parent task");

    t = alloc_new_blank_thread(kind, interrupts_on);
    set_init_regs(&t->t_init_regs, kind, interrupts_on);
    set_interrupt_sp(t, &t->t_init_regs, GET_STACK_BASE(t->t_int_stack_page));
    set_pc(&t->t_init_regs, task_pc);
    assign_thread_to_task(parent_task, t);

    // Give each thread a unique stack
    runtime_stack_va = DEFAULT_USER_STACK_PAGE | STACK_SHIFT_BY_TASK_ID(t->t_tid);

    if (IsPagePresent(parent_task->t_pagetable, runtime_stack_va)) {
        panic("create_thread_in_task: a stack already exists where we tried to put one");
    }

    if (0 == t->t_run_stack_page) panic("create_thread_in_task: alloc_new_blank_thread didn't create a runtime stack for us");

#ifdef TASK_DEBUG_STACK_MAPPING
    printf("Mapping stack at 0x%X for thread %d (kind: %s)\n", runtime_stack_va, t->t_tid, t->t_kind == USER_THREAD ? "user" : "kernel");
#endif // TASK_DEBUG_STACK_MAPPING

    switch(t->t_kind) {
        case USER_THREAD:
        case KERNEL_OUTER_THREAD:
        PageMap(
            parent_task->t_pagetable,
            runtime_stack_va,
            KERN_V2P((virt_t)(t->t_run_stack_page)),
            LARGE_PAGE,
            t->t_kind == USER_THREAD ? USER_PAGE : KERNEL_PAGE
        );

        set_runtime_sp(t, &t->t_init_regs, GET_STACK_BASE(runtime_stack_va));
        break;

        case KERNEL_INNER_THREAD:
        set_runtime_sp(t, &t->t_init_regs, GET_STACK_BASE(t->t_run_stack_page));
        break;
    }

    if (USER_THREAD == t->t_kind && !t->t_int_stack_page) {
        panic("create_thread_in_task: user thread is missing an interrupt stack page");
    }

    return t;
}

task_t *alloc_task(ThreadKind kind, bool interrupts_on, virt_t task_pc) {
    task_t *new_task = alloc_new_blank_task();
    if (!new_task) return NULL;

    new_task->t_pagetable = AllocPageTable();
    if (!new_task->t_pagetable) {
        panic("Failed to create page table for task");
        FreePage((virt_t)new_task);
        return NULL;
    }

    global_cpu.MapKernel(new_task->t_pagetable);
    create_thread_in_task(new_task, kind, interrupts_on, task_pc);
    return new_task;
}

// Helper method to load an ELF into a task
// Returns true if good, false otherwise
bool load_bin_into_task(task_t *t, char *binpath) {
    void *file_handle = filesys_open_relative(binpath, t->t_cwd);
    if (!file_handle) return false;
    return ALL_GOOD == load_elf(file_handle, t);
}

task_t *create_task(char *binpath, bool interrupts_on, ThreadKind kind) {
    InterruptMode old_mode = global_cpu.SetInterrupts(INTS_DISABLED);

    if (kind != USER_THREAD && kind != KERNEL_OUTER_THREAD) panic("create_task called with a task kind that doesn't support ELFs (%d)", kind);

    task_t *t = alloc_task(kind, interrupts_on, DEFAULT_USER_PC);
    if (!t) return NULL;
    strncpy(t->t_name, filesys_basename(binpath), TASKNAME_LEN);
    if (!load_bin_into_task(t, binpath)) {
        free_task(t);
        return NULL;
    }
    global_cpu.SetInterrupts(old_mode);
    return t;
}

/*
 * clone_thread_into_task
 * Clones an existing thread into a given task.
 *
 * Each task is configured to be an "unlaunched" task, where:
 *  - the init_regs hold the contents of the running thread's registers
 *  - the saved floatvecs are copied
 *  - the kind and interrupt mode are copied
 */
void clone_thread_into_task(task_t *dst_task, thread_t *src_thread) {
    thread_t *new_thread;
    phys_t new_runtime_stack_phys;
    virt_t new_runtime_stack_virt;

    if (src_thread->t_task == dst_task) panic("tried to clone a thread into a task it already exists in");

    new_thread = alloc_new_blank_thread((ThreadKind)src_thread->t_kind, src_thread->t_interrupts_on);

    if (!dst_task->t_pagetable) panic("clone_thread_into_task: destination task doesn't have a page table");

    // Sanity check the new thread
    if (!new_thread) {
        panic("failed to create a new thread during clone_thread_into_task");
    }
    if (new_thread->t_launched) {
        // Make sure launched is false to force a re-launch
        // via the slow path (reading from init_regs)
        panic("alloc_new_blank_thread returned a thread that has already been launched");
    }
    if (THREAD_RUNNABLE != new_thread->t_state) {
        panic("alloc_new_blank_thread didn't return a runnable thread");
    }
    if (new_thread->t_kind != src_thread->t_kind || new_thread->t_interrupts_on != src_thread->t_interrupts_on) {
        panic("clone_thread_into_task: new thread and source thread don't agree on kind or interrupt status");
    }

    // A new runtime stack is automatically created by alloc_new_blank_thread;
    // we need to remove it because we want to use a copy of the one that already exists
    // in the source thread.
    if (!new_thread->t_run_stack_page) {
        // First, confirm we actually have a runtime stack to get rid of
        panic("clone_thread_into_task: new thread doesn't have a runtime stack");
    }

    // Discard the automatically created runtime stack, and use the
    // page in the cloned pagetable where the current stack pointer points.
    FreePage(new_thread->t_run_stack_page);
    new_runtime_stack_phys = PageWalk(dst_task->t_pagetable, get_runtime_sp(src_thread, src_thread->t_regs));
    if (PHYS_ADDR_INVALID == new_runtime_stack_phys) {
        panic("clone_thread_into_task: couldn't locate the new runtime stack page");
    }

    new_runtime_stack_virt = KERN_P2V(ALIGN_LARGE_PAGE(new_runtime_stack_phys));
    if (new_runtime_stack_virt == src_thread->t_run_stack_page) {
        panic("clone_thread_into_task: source thread and destination task share a physical mapping for the runtime stack");
    }

    // printf("**** src_thread.runtime_stack: 0x%X\n**** new_thread.runtime_stack: 0x%X\n", src_thread->t_run_stack_page, new_runtime_stack_virt);
    new_thread->t_run_stack_page = new_runtime_stack_virt;

    assign_thread_to_task(dst_task, new_thread);
    if (new_thread->t_tid != INIT_THREAD || dst_task->t_threads[INIT_THREAD] != new_thread) {
        panic("clone_thread_into_task: new_thread isn't INIT_THREAD");
    }

    // Copy all state over. This overwrites the default init_regs values,
    // so we'll need to re-configure anything important after (eg. interrupt stack).
    memcpy(&new_thread->t_saved_fvs, &src_thread->t_saved_fvs, sizeof(new_thread->t_saved_fvs));
    memcpy((void *)&new_thread->t_init_regs, (void *)src_thread->t_regs, sizeof(new_thread->t_init_regs));

    // This only affects USER_THREAD threads- make sure we're set
    // to take interrupts from the new interrupt stack
    // (rather than the old thread's interrupt stack).
    set_interrupt_sp(new_thread, &new_thread->t_init_regs, GET_STACK_BASE(new_thread->t_int_stack_page));
}

/*
 * fork_task
 * Creates a new task which is an identical copy of a given task.
 */
task_t *fork_task(task_t *parent, thread_t *thread_to_fork) {
    task_t *new_task;
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);

    if (!parent) panic("forking a NULL task");
    if (parent == kernel_task) panic("tried to fork the kernel_task");
    if (thread_to_fork->t_task != parent) panic("tried to fork a thread that doesn't belong to the same task");
    if (current_thread() != thread_to_fork) panic("current_thread != thread_to_fork; this is probably actually ok but only consider this case if it ever actually happens");

    if (thread_to_fork->t_task->t_primary_gmap.g_is_mapped || thread_to_fork->t_task->t_secondary_gmap.g_is_mapped) {
        panic("fork_task: cannot fork a task with a gmap");
    }

    new_task = alloc_new_blank_task();
    if (!new_task) panic("failed to alloc a new task during fork");

    // Deep copy of parent's page table slowly (no copy-on-write)
    new_task->t_pagetable = AllocPageTable();
    if (!new_task->t_pagetable) {
        panic("failed to create page table for forking task");
        free_task(new_task);
        global_cpu.SetInterrupts(old_interrupts);
        return NULL;
    }
    CopyPagetable(new_task->t_pagetable, parent->t_pagetable);
    global_cpu.MapKernel(new_task->t_pagetable);

    memcpy(&new_task->t_name, &parent->t_name, TASKNAME_LEN);

    // @TODO: Make this copy by reference, not by value
    // Eg. so that when one task changes an FD, the
    // other sees it (the POSIX spec requires this)
    task_copy_fds(new_task, parent);
    new_task->t_cwd = parent->t_cwd;
    new_task->t_heap_top = parent->t_heap_top;
    new_task->t_fmap_top = parent->t_fmap_top;
    if (parent->t_fmaplist != NULL) panic("tried to fork a task that has allocated heap pages, need to add code to duplicate the fmap list");
    new_task->t_parent = parent;

    // Clone only the running thread that called fork
    clone_thread_into_task(new_task, thread_to_fork);
    global_cpu.SetInterrupts(old_interrupts);
    return new_task;
}

kret_t replace_task(task_t **t, thread_t *thread_to_replace, char *newimage) {
    task_t *old_task = *t;
    task_t *new_task = NULL;
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);
    if (current_task() != old_task) panic("tried to replace a task that isn't the current task");
    if (current_thread() != thread_to_replace) panic("tried to replace a thread that isn't the current one");

    new_task = create_task(newimage, thread_to_replace->t_interrupts_on, (ThreadKind)thread_to_replace->t_kind);
    if (!new_task) {
        global_cpu.SetInterrupts(old_interrupts);
        return THING_DOESNT_EXIST;
    }

    // Copy over fields from old task
    // @TODO: Make this copy by reference, not by value
    // Eg. so that when one task changes an FD, the
    // other sees it (the POSIX spec requires this)
    task_copy_fds(new_task, old_task);
    new_task->t_cwd = old_task->t_cwd;
    new_task->t_pid = old_task->t_pid; // This will drop a pid, that's ok
    new_task->t_parent = old_task->t_parent;

    schedule_task(new_task);
    *t = new_task;

    // Can't free the old task now, as we're still using its stack and page table!
    // First confirm that old_task is indeed part of the task list.
    // That way it'll be freed on the next reaper pass.
    thread_t *cursor = cur_thread;
    bool found_old_task_in_list = false;
    do {
        if (cursor->t_task == old_task) {
            found_old_task_in_list = true;
            break;
        }
        cursor = cursor->t_next;
    } while (cursor != cur_thread);
    if (!found_old_task_in_list) panic("replace_task: old_task isn't part of the scheduler list");
    old_task->t_state = TASK_ZOMBIE;

    global_cpu.SetInterrupts(old_interrupts);
    return ALL_GOOD;
}

usize num_running_threads(task_t *t) {
    usize n_threads = 0;
    for (usize i = 0; i < MAX_THREADS; i++) {
        if (t->t_threads[i] && t->t_threads[i]->t_state != THREAD_ZOMBIE) n_threads++;
    }
    return n_threads;
}

// Change a running thread's kind
kret_t set_thread_kind(thread_t *t, ThreadKind new_kind) {
    virt_t old_task_sp;
    if (!t) return THING_DOESNT_EXIST;
    if (!t->t_launched) panic("Tried to change a running thread's kind, except the task hasn't been launched yet!");
    if (KERNEL_INNER_THREAD == t->t_kind) panic("Tried to change a kernel inner task's kind (can't do that)");
    if (KERNEL_INNER_THREAD == new_kind) panic("Tried to change a task to a kernel inner task (can't do that)");
    if (NULL == t->t_regs) panic("Couldn't locate saved regs for a task we are trying to adjust the kind of");

    // Trivial case: no change
    if (t->t_kind == new_kind) return ALL_GOOD;

    old_task_sp = get_runtime_sp(t, t->t_regs);

    // We only support setting the thread mode of a running thread if there's only 1 thread
    // It's currently undefined what happens if you later spawn more threads after promoting or demoting
    if (num_running_threads(t->t_task) != 1) {
        panic("tried to set the thread kind of a multithreaded app- currently this is disallowed");
    }

    ReconfigurePagetablePrivileges(
        t->t_task->t_pagetable,
        new_kind == USER_THREAD ? USER_PAGE : KERNEL_PAGE
    );

    global_cpu.FlushTLB();

    switch(t->t_kind) {
        // Promotion case
        case USER_THREAD:
        if (KERNEL_OUTER_THREAD != new_kind) panic("Tried to promote user task to some unsupported task kind");
        break;

        case KERNEL_OUTER_THREAD:
        if (USER_THREAD != new_kind) panic("Tried to demote kernel outer task to some unsupported task kind");
        // Outer tasks were never given a proper user interrupt stack
        if ((virt_t)0 == t->t_int_stack_page) {
            t->t_int_stack_page = AllocPage(LARGE_PAGE);
            if (!t->t_int_stack_page) panic("set_thread_kind: failed to allocate a new interrupt stack when moving from user -> kernel outer");
        }
        break;
    }

    // Actually switch modes
    set_init_regs(
        t->t_regs,
        new_kind,
        t->t_interrupts_on
    );

    // Here we set both the runtime and interrupt stack pointers.
    // Consider the case of ARM64: There are two stack pointers.
    // When moving user -> kernel, we need to copy sp_el0 to sp_el1 (or vice versa).
    // Note that set_interrupt_sp is a NOP for kernel tasks on all architectures.
    // So we can call it in both cases and be ok.
    t->t_kind = new_kind;
    set_runtime_sp(t, t->t_regs, old_task_sp);
    set_interrupt_sp(t, t->t_regs, GET_STACK_BASE(t->t_int_stack_page));
    return ALL_GOOD;
}

void thread_enter_kernel(regs_t *r) {
    if (NULL != cur_thread) {
        if (!cur_thread->t_launched) {
            panic("Entering kernel from a thread, but the thread wasn't launched?");
        }
        if (cur_thread->t_preempt_count == 0) {
            cur_thread->t_regs = r;
            if (KERNEL_INNER_THREAD != cur_thread->t_kind) {
                if (cur_thread->t_task->t_tracing_mode) kasan_begin_trace();
                // Touch every saved fv register so that they are recorded during the trace
                // As assembly code won't be instrumented by KASAN
                thread_save_floatvec_regs(cur_thread);
            }
        }
        cur_thread->t_preempt_count++;
    }
}

void thread_leave_kernel() {
    if (NULL != cur_thread) {
        if (!cur_thread->t_launched) {
            panic("Leaving kernel to a task, but the task wasn't launched?");
        }
        if (1 == cur_thread->t_preempt_count) {
            cur_thread->t_regs = NULL;
            if (KERNEL_INNER_THREAD != cur_thread->t_kind) {
                thread_load_floatvec_regs(cur_thread);
                if (cur_thread->t_task->t_tracing_mode) {
                    kasan_end_trace();
                    kasan_dump_trace();
                }
            }
        }
        if (0 == cur_thread->t_preempt_count) {
            // This means we have called thread_leave_kernel more times
            // than thread_enter_kernel for this current task.
            panic("Preemption refcount mismatch");
        }

        cur_thread->t_preempt_count--;
    }
}

void copy_fd(fd_t *dst, fd_t *src) {
    if (!dst || !src) return;
    memcpy(dst, src, sizeof(*dst));

    if (dst->type == FD_PIPE) {
        pipe_inc_refcount((pipe_t *)dst->resource, (pipe_attribute_t)dst->attribute);
    }
}

// Let the child inherit all fds from parent
void task_copy_fds(task_t *dst, task_t *src) {
    for (usize i = 0; i < NUM_FDS; i++) {
        copy_fd(&dst->t_fds[i], &src->t_fds[i]);
    }
}

// Insert thread to list of threads
void schedule_thread(thread_t *t) {
	if (NULL == cur_thread) {
        // No task launched yet, so setup t to be ready to be set to cur_thread
        // only switchto can change cur_thread though, so it'll fix that when we get there
        t->t_next = t;
        t->t_prev = t;
        return;
	}

	t->t_next = cur_thread->t_next;
	t->t_prev = cur_thread;
	cur_thread->t_next->t_prev = t;
	cur_thread->t_next = t;
}

void schedule_task(task_t *t) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (t->t_threads[i]) schedule_thread(t->t_threads[i]);
    }
}

u64 *task_get_pagetable(task_t *t) {
    return t->t_pagetable;
}

u64 *thread_get_pagetable(thread_t *t) {
    return t->t_task->t_pagetable;
}

/*
 * reap_zombie_tasks
 * Scan for zombie tasks and free them.
 */
void reap_zombie_tasks() {
    thread_t *cursor = cur_thread;
    do {
        if (TASK_ZOMBIE == cursor->t_task->t_state) {
            if (cursor->t_task == current_task()) panic("reap_zombie_tasks: cur_thread is a zombie");
            free_task(cursor->t_task);

            // At this point, the only guarantee is that cur_thread
            // is a valid healthy thread
            cursor = cur_thread;
        }
        cursor = cursor->t_next;
    } while (cursor != cur_thread);
}

/*
 * find_runnable_thread
 * Scan the thread list for something that we can run.
 */
thread_t *find_runnable_thread_by_pid(pid_t target) {
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);
    thread_t *cursor;

    // Garbage collect any zombie tasks before
    // scanning for a new thread to schedule.
    reap_zombie_tasks();

    cursor = cur_thread->t_next;
    do {
        if (!cursor) panic("find_runnable_thread: NULL task in list");

        if (cursor->t_state == THREAD_RUNNABLE) {
            if (cursor->t_task->t_state != TASK_HEALTHY) {
                panic("found a runnable thread in a non-healthy task");
            }
            if (PID_ANY == target || cursor->t_task->t_pid == target) {
                global_cpu.SetInterrupts(old_interrupts);
                return cursor;
            }
        }

        if (cursor->t_state == THREAD_FREED ||
            cursor->t_task->t_state == TASK_ZOMBIE ||
            cursor->t_task->t_state == TASK_FREED) {
            panic("found a freed thread / (zombie, freed) task in the thread list");
        }

        cursor = cursor->t_next;
    } while (cursor != cur_thread);

    if (cur_thread->t_state != THREAD_RUNNING) {
        panic("Only remaining task is a zombie");
    }

    // Nothing left to schedule, so return the current thread back to itself
    global_cpu.SetInterrupts(old_interrupts);
    return cur_thread;
}

/*
 * find_runnable_thread
 * Scan the thread list for something that we can run.
 */
thread_t *find_runnable_thread() {
    return find_runnable_thread_by_pid(PID_ANY);
}

kret_t quit_task(task_t *t, u64 reason) {
    waitnotif_t *new_notif = NULL;
    thread_t *cursor = NULL;
    thread_t *next_thread = NULL;
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);

#ifdef TASK_DEBUG_QUIT
    printf("quitting %s (%d)\n", t->t_name, t->t_pid);
#endif // TASK_DEBUG_QUIT

    if (t == kernel_task) {
        // We can never quit the kernel task
        // (This probably happened when we typed ^C)
        // Just ignore requests to quit the kernel task.
        return THING_DOESNT_EXIST;
    }

    if (t->t_parent) {
        new_notif = (waitnotif_t *)dlmalloc(sizeof(*t->t_parent->t_waitlist));
        new_notif->next = t->t_parent->t_waitlist;
        new_notif->pid = t->t_pid;
        new_notif->exit_code = reason;
        t->t_parent->t_waitlist = new_notif;
        wakeup_resource(&t->t_parent->t_waitlist);
    }

    // HACK: If we happen to have any FDs pointing to a console window,
    // notify the console window we are quitting.
    // If somehow a task closed all its stdio fds then this would orphan it.
    // Likewise if somehow a non-GUI console task gained a console fd
    // this would erroneously notify the console a task is quitting.
    if (t->t_owner) {
        for (usize i = 0; i < NUM_FDS; i++) {
            if (FD_CONSOLE == t->t_fds[i].type) {
                ((Console *)(t->t_owner))->AssocTaskQuitting(t, reason);
            }
        }
    }

    // Orphan all children of this task
    cursor = current_thread();
    do {
        if (t == cursor->t_task->t_parent) {
            cursor->t_task->t_parent = NULL;
        }
        cursor = cursor->t_next;
    } while (cursor != current_thread());

    // Find something new to schedule from a different task
    // This will infloop if there isn't anything- @TODO fix that
    cursor = current_thread();
    do {
        if (((THREAD_RUNNABLE == cursor->t_state) || (THREAD_RUNNING == cursor->t_state)) && t != cursor->t_task) {
            next_thread = cursor;
        }
        cursor = cursor->t_next;
    } while (cursor != current_thread() && !next_thread);

    if (!next_thread) panic("couldn't find something new to schedule after quit_task");
    if (next_thread->t_task == t) panic("next_thread is still part of the task that is quitting");

    // Mark this task for deletion
    t->t_state = TASK_ZOMBIE;

    // Schedule the next thread, maybe returning here if the task we quit
    // wasn't part of the current thread.
    if (next_thread != current_thread()) {
        // Don't re-enable interrupts, there is no current thread to take them!
        switchto(next_thread);
    }
    global_cpu.SetInterrupts(old_interrupts);
    return ALL_GOOD;
}

kret_t quit_thread(thread_t *t) {
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);
    // Deschedule this thread, it'll get reaped when the parent task quits
    t->t_state = THREAD_ZOMBIE;
    global_cpu.SetInterrupts(old_interrupts);
    return ALL_GOOD;
}

void free_thread(thread_t *t) {
    bool found_in_parent = false;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (t == t->t_task->t_threads[i]) {
            t->t_task->t_threads[i] = NULL;
            found_in_parent = true;
        }
    }
    if (!found_in_parent) panic("free_thread: couldn't find this thread in its task");

    // Can't delete the current thread, as we are relying on its stack and page table!
    if (cur_thread == t) panic("tried to free the current thread");

    if (t->t_task == kernel_task || t->t_kind == KERNEL_INNER_THREAD) {
        panic("tried to free a kernel inner thread (should be possible, but has not been tested yet)");
    }

    t->t_state = THREAD_FREED;
    if (t->t_kind == KERNEL_INNER_THREAD) {
        // Only kernel inner threads need to explicitly free the run stack page
        // @TODO: Verify this assumption when adding support for quitting kernel inner threads
        if (0 != t->t_run_stack_page) FreePage(t->t_run_stack_page);
    } else {
        // t_run_stack_page is freed when we free the task's page table
        // for outer kernel / user threads, so don't free it here.
    }
    if (0 != t->t_int_stack_page) FreePage(t->t_int_stack_page);
    t->t_prev->t_next = t->t_next;
    t->t_next->t_prev = t->t_prev;
    t->t_next = NULL;
    t->t_prev = NULL;
    t->t_task = NULL;
    FreePage((virt_t)t);
}

void free_task(task_t *t) {
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);
    if (kernel_task == t) panic("tried to free the kernel task");
    for (int i = 0; i < MAX_THREADS; i++) {
        if (t->t_threads[i]) free_thread(t->t_threads[i]);
    }
    for (usize i = 0; i < NUM_FDS; i++) do_sys_close(t, i);
    if (t->t_primary_gmap.g_is_mapped) GigaMapUnmap(&t->t_primary_gmap, t->t_pagetable);
    if (t->t_secondary_gmap.g_is_mapped) GigaMapUnmap(&t->t_secondary_gmap, t->t_pagetable);
    if (t->t_pagetable) FreeUserPageTable(t->t_pagetable);
    strncpy(t->t_name, "[freed task]", TASKNAME_LEN);
    while (t->t_waitlist) consume_waitnotif(NULL, t->t_waitlist, &t->t_waitlist);
    cleanup_fmap_list(t);
    dlfree(t);
    global_cpu.SetInterrupts(old_interrupts);
}

void assign_owner_to_task(task_t *t, void *r) {
    t->t_owner = r;
}

void resource_is_quitting(void *r) {
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);
    thread_t *cursor = cur_thread->t_next;

    // Check all but cur_thread, as quit_task won't return if we
    // quit the currently running thread.
    do {
        if (!cursor) break;
        if (cursor == cur_thread) panic("resource_is_quitting: cursor should not be able to point to cur_thread");
        if (cursor->t_task->t_owner == r) {
            quit_task(cursor->t_task, 0);
        }
        cursor = cursor->t_next;
    } while (cursor != cur_thread);

    // Now, check cur_thread, never returning if it's a match.
    // I actually don't think this path is possible, as resource_is_quitting
    // should only be currently called from the Oxide kernel thread
    if (cur_thread->t_task->t_owner == r) {
        panic("resource_is_quitting: cur_thread is owned by r. This should be fine, but currently I believe this is impossible so I put a panic here to see if it ever actually happens; if you see this panic, you should verify that this code path works as intended.");
        quit_task(cur_thread->t_task, 0);
    }

    global_cpu.SetInterrupts(old_interrupts);
}

/*
 * switchto
 * Goodbye kernel, this takes us into a thread.
 *
 * This *really* must be called with a "sanitized" stack, eg.
 * the stack pointer should point to the kernel stack alias
 * for kernel tasks.
 *
 * This means kernel ELF tasks should have used ALIAS_STACK_IF_KERNEL_TASK
 * before getting here (eg. we entered via syscall or interrupt), user tasks
 * should be fine as the stack switching mechanism forces this, and in-kernel
 * tasks should be fine as they always use kernel stacks.
 */
extern "C" void switchto(thread_t *t) {
    phys_t next_pt = KERN_V2P((u64)t->t_task->t_pagetable);
    u64 next_asid = 0;

#ifdef CONFIG_USE_ASID
    next_asid = (t->t_task->t_pid << 4) | (t->t_tid);
    if (next_asid >= 512) panic("ASID too large");
#endif // CONFIG_USE_ASID

#ifdef ARCH_ARM
    if ((next_pt >> 48ull) != 0) {
        panic("Non-zero ASID in task's pagetable pointer (t_pagetable: 0x%X)", next_pt);
    }
#endif // ARCH_ARM

    if(!t) panic("Switching to NULL thread");

#ifdef VERBOSE_THREAD_LOGGING
    printf("Switching to thread %d (kind: %s) in task %s\n", t->t_tid, t->t_kind == USER_THREAD ? "USER" : "KERN", t->t_task->t_name);
#endif // VERBOSE_THREAD_LOGGING

    if (__scheduler_logging) {
        if (cur_thread) {
            printf("%s [%d] -> %s [%d]\r\n",
                cur_thread->t_task->t_name,
                cur_thread->t_task->t_pid,
                t->t_task->t_name,
                t->t_task->t_pid
            );
        } else {
            printf("null -> %s [%d]\r\n",
                t->t_task->t_name,
                t->t_task->t_pid
            );
        }
    }

    if (!IS_KERNEL_VA(get_current_sp())) {
        panic("Tried to switchout with non-kernel stack: 0x%X\n", get_current_sp());
    }

    // Clear interrupts, and remember what the state used to be
    // This enters a critical section
    // @TODO: Locking for multicore?
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);

    thread_t *prev = cur_thread;

    if (t->t_state != THREAD_RUNNABLE) {
        panic("Tried to switch to a non-runnable task");
    }

    if (t != prev) {
        global_cpu.SetPageTable((u64*)next_pt, next_asid);
    }

#ifdef ARCH_ARM
#ifdef CONFIG_USE_ASID
    write_msr(tpidrro_el0, next_asid);
    write_msr(tpidr_el1, next_asid);
    write_msr(tpidr_el0, next_asid);
    write_msr(contextidr_el1, next_asid);
#endif // CONFIG_USE_ASID
#endif // ARCH_ARM

#ifdef ARCH_X86
    tss.rsp0 = GET_STACK_BASE(t->t_int_stack_page);
    if (USER_THREAD == t->t_kind) {
        // tss.rsp0 is only used for user threads
        if (!IS_KERNEL_VA(tss.rsp0)) panic("tss.rsp0 is not a kernel VA (0x%X)", tss.rsp0);
    }
#endif // ARCH_X86

#ifdef ARCH_RISCV
    // Need to swap stvec depending on what kind of task we are about to schedule
    if (USER_THREAD == t->t_kind)   write_csr(stvec, (u64)&user_trap);
    else /* KERNEL_*_THREAD */      write_csr(stvec, (u64)&kernel_trap);
#endif // ARCH_RISCV

    t->t_state = THREAD_RUNNING;
    if (NULL != prev && t != prev && prev->t_state == THREAD_RUNNING) {
        prev->t_state = THREAD_RUNNABLE;
    }
    cur_thread = t;

#ifdef ARCH_ARM
#ifdef CONFIG_USE_ASID

{
    u64 actual_ttbr0 = read_msr(ttbr0_el1);
    if ((actual_ttbr0 >> 48ull) != next_asid) {
        panic("CONFIG_USE_ASID is on, but the actual ASID is incorrect (ttbr0: 0x%X, expected asid 0x%X)", actual_ttbr0, next_asid);
    }
}

#endif // CONFIG_USE_ASID
#endif // ARCH_ARM

    switchout(prev, cur_thread);

    // ... and we're back
    global_cpu.SetInterrupts(old_interrupts);
}

/*
 * naptime
 * A kernel function for cur_thread has requested we
 * switch to something else, and then eventually
 * switch back to them.
 */
void naptime() {
    thread_t *next = NULL;

    // Why are we trying to switch to another task when
    // we aren't even running a task?
    if (!cur_thread) return;

    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);

    if (cur_thread->t_has_sched_pref) {
        next = find_runnable_thread_by_pid(cur_thread->t_sched_pref);
    }

    if (NULL == next) {
        // Don't have an affinity set, or if we did, didn't find the target
        // Scan the list for anything else to run
        next = find_runnable_thread();
    }

    // Nothing to do
    if (next == cur_thread) {
        global_cpu.SetInterrupts(old_interrupts);
        return;
    }

    if (next->t_state != THREAD_RUNNABLE) panic("find_runnable_thread gave us a non-runnable task");
    switchto(next);
    global_cpu.SetInterrupts(old_interrupts);
}

void create_kernel_root_task() {
    kernel_task = alloc_new_blank_task();
    if (!kernel_task) panic("failed to create a kernel task");
    if (KERNEL_TASK_PID != kernel_task->t_pid) panic("kernel task pid is non-zero");

    for (usize i = 0; i < NUM_FDS; i++) {
        kernel_task->t_fds[i].type = FD_NONE;
    }

    kernel_task->t_pagetable = AllocPageTable();
    if (!kernel_task->t_pagetable) {
        panic("Failed to create a page table for the kernel task");
    }

    global_cpu.MapKernel(kernel_task->t_pagetable);
    strncpy(kernel_task->t_name, KERNEL_TASK_NAME, TASKNAME_LEN);
}

thread_t *create_kernel_inner_thread(bool interrupts_on, virt_t task_pc) {
    return create_thread_in_task(
        kernel_task,
        KERNEL_INNER_THREAD,
        interrupts_on,
        task_pc
    );
}

void sleep_on(void *resource) {
    if (!cur_thread) panic("Tried to sleep on a resource but no task exists");
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);

    cur_thread->t_sleep_resource = resource;
    cur_thread->t_state = THREAD_WAITING;

    naptime();

    cur_thread->t_sleep_resource = NULL;

    global_cpu.SetInterrupts(old_interrupts);
}

void wakeup_resource(void *resource) {
    InterruptMode old_interrupts = global_cpu.SetInterrupts(INTS_DISABLED);

    thread_t *cursor = cur_thread;
    do {
        if (NULL == cursor) {
            panic("wakeup_resource: NULL task in list");
        }
        if (THREAD_WAITING == cursor->t_state && resource == cursor->t_sleep_resource) {
            cursor->t_state = THREAD_RUNNABLE;
        }
        cursor = cursor->t_next;
    } while (cursor != cur_thread);

    global_cpu.SetInterrupts(old_interrupts);
}
