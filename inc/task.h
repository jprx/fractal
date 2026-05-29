#ifndef TASK_H
#define TASK_H

#include <fractal.h>
#include <pid.h>
#include <virt_mem.h>
#include <filesys/fileio.h>
#include <wait.h>
#include <fmap.h>
#include <fthread.h>

BEGIN_C_HEADER

#ifdef SINGLE_PROGRAM_MODE
# define INIT_TASK "/bin/utest"
# define INIT_TASK_NAME "utest"
#else // SINGLE_PROGRAM_MODE
# define INIT_TASK "/bin/dash"
# define INIT_TASK_NAME "dash"
#endif // ! SINGLE_PROGRAM_MODE

#define DEFAULT_TASK_WORKING_DIRECTORY "/"
#define TASKNAME_LEN 64
#define NUM_FDS 32

#define KERNEL_TASK_PID 0
#define KERNEL_TASK_NAME "kernel"

// How large can argv get?
#define TASK_MAX_NUM_ARGS 64
#define TASK_NUM_ENV_VARS 64

// Max length of an argument string
#define TASK_ARGLEN 512

// Argument indices that are passed along to main()
#define MAIN_ARG_ARGC 0
#define MAIN_ARG_ARGV 1
#define MAIN_ARG_ENVP 2

// Max number of threads per task
#define MAX_THREADS 16
#define INIT_THREAD 0

typedef enum {
    USER_THREAD,          // An ELF (user)
    KERNEL_OUTER_THREAD,  // An ELF (kernel)
    KERNEL_INNER_THREAD,  // A task in the kernel (ex: Oxide, Init)
} ThreadKind;

// What is this thread up to?
typedef enum {
    THREAD_RUNNING,       // Currently running
    THREAD_RUNNABLE,      // Waiting to be run
    THREAD_WAITING,       // Waiting for someone to call wakeup_resource
    THREAD_ZOMBIE = -1,   // A thread that has exited, but the associated task is still live
    THREAD_FREED = -2,    // Should never, ever see this
} ThreadState;

typedef enum {
    TASK_HEALTHY,         // Normal task with scheduable threads
    TASK_ZOMBIE,          // Task just died and needs to be reaped
    TASK_FREED = -1,      // Should never, ever see this
} TaskState;

#include <regs.h>

// File descriptor stuff
#define STDIO_IN  0
#define STDIO_OUT 1
#define STDIO_ERR 2

typedef enum {
    FD_NONE,
    FD_SERIAL,  // Serial port
    FD_CONSOLE, // Some GUI app
    FD_FILE,    // Something from the filesystem
    FD_PROC,    // Something for procfs
    FD_PIPE,    // One end of a pipe
} fd_type_t;

typedef struct {
    // The kind of file descriptor this is:
    fd_type_t type;

    // An opaque pointer to some associated resource:
    // This could be an inode, pipe, console, or something else
    void *resource;

    // An opaque attribute field that corresponds to the resource
    // Eg. could tell us which end of the pipe this is
    u64 attribute;

    // How far have we read into the fd:
    usize offset;
} fd_t;

struct task_t;
struct thread_t;

/*
 * Thread
 * A Thread is the minimal schedulable entity in the system.
 *
 * Need to allocate these with the page allocator instead of dlmalloc,
 * because the saved floatvec registers at the beginning of this struct
 * must be at least 16 byte aligned (ISA enforced).
 *
 * Place aligned / u64s earlier in the struct, and the smaller fields
 * later on. We want to make interoping with this between C and ASM easy,
 * so fully aligned u64s should come earlier on.
 */
typedef struct thread_t {
    floatvec_regs_t     t_saved_fvs;       // Saved floating point and vector regs for this ISA
    regs_t              t_init_regs;       // Initial register values (only used in first launch)
    regs_t             *t_regs;            // Pointer to saved regs on the interrupt stack
    u64                 t_launched;        // Has this thread been launched yet?
    u64                 t_kind;            // ThreadKind
    u64                 t_state;           // ThreadState
    struct thread_t    *t_next, *t_prev;   // Links to other threads in the scheduler.
    u64                 t_sched_pref;      // PID of the preferred successor.
    u64                 t_has_sched_pref;  // Does this thread have a preferred successor?
    virt_t              t_run_stack_page;  // Stack page used while the thread is running for ALL thread kinds.
    virt_t              t_int_stack_page;  // Kernel stack page used during interrupts ONLY for userspace threads.
    u64                 t_saved_sp;        // Saved kernel stack pointer during context switches
    u64                 t_preempt_count;   // Number of times we've entered the kernel while running this task
    u64                 t_interrupts_on;   // Are interrupts enabled? (bool)
    struct task_t      *t_task;            // Our corresponding task object (potentially shared by multiple threads)
    bool                t_is_fthread;      // Is this thread running as an fthread? (eg. created via fthread_create)
    void               *t_sleep_resource;  // If we are in the THREAD_WAITING state, this is what we're waiting for
    u64                 t_tid;             // Thread ID (relative to task's PID)
} thread_t;

/*
 * Task
 * A Task object represents a collection of Threads.
 */
typedef struct task_t {
    pid_t             t_pid;
    u64              *t_pagetable;
    void             *t_cwd;
    virt_t            t_heap_top;
    virt_t            t_fmap_top;
    fd_t              t_fds[NUM_FDS];
    char              t_name[TASKNAME_LEN];
    thread_t         *t_threads[MAX_THREADS];
    struct task_t    *t_parent;
    TaskState         t_state;
    void             *t_owner;     // An object with a separate lifetime that owns this task.
                                   // When the owner is being free'd, we need to quit this task.
                                   // Example: a GUI console.
    waitnotif_t      *t_waitlist;  // List of notifications of closed tasks, used by wait
                                   // It is a task's responsibilty to free these notifications,
                                   // either on quit or as consumed by wait
    fmap_alloc_t     *t_fmaplist;  // List of fmap allocations by this task
    gigamap_t         t_primary_gmap, t_secondary_gmap;
    bool              t_tracing_mode; // Are we tracing this task?
} task_t;

/*
 * fork_task
 * Creates a new task which is a copy of a given task.
 *
 * Only the thread specified by thread_to_fork is duplicated.
 * thread_to_fork should probably always be equal to current_thread().
 */
task_t *fork_task(task_t *parent, thread_t *thread_to_fork);

/*
 * replace_task
 * Replaces the task t by re-initializing it and loading newimage into it.
 * Essentially, this implements the POSIX exec system call.
 *
 * We assume that t is the current task, and we were called with interrupts off
 * in a system call context. We need to set this task up in such a way that
 * when the scheduler sees it, it will be launched like a fresh task with the
 * default initial registers (floating-point and integer), and for all intents
 * and purposes acts like a brand new task.
 *
 * The caller must *NOT* return to the system call return path that called this-
 * instead, after calling replace_task with interrupts disabled, you *MUST* call
 * naptime() to request the scheduler to pick something else up, and then relaunch
 * this task later.
 *
 * Used by the execve syscall.
 */
kret_t replace_task(task_t **t, thread_t *thread_to_replace, char *newimage);

// Bootstrapping kernel inner threads / tasks
void create_kernel_root_task();

// This is the only way to create a new kernel outer / user task:
task_t *create_task(char *binpath, bool interrupts_on, ThreadKind kind);

// This is the only way to create kernel outer / user threads in an existing task:
thread_t *create_thread_in_task(task_t *parent_task, ThreadKind kind, bool interrupts_on, virt_t task_pc);

// This is the only way to create a kernel inner thread:
thread_t *create_kernel_inner_thread(bool interrupts_on, virt_t task_pc);

void set_thread_name(thread_t *t, char *name);
void set_task_name(task_t *t, char *name);

kret_t set_thread_kind(thread_t *t, ThreadKind new_kind);
void free_task(task_t *t);
void free_thread(thread_t *t);
void copy_fd(fd_t *dst, fd_t *src);
void task_copy_fds(task_t *dst, task_t *src);
void assign_thread_to_task(task_t *parent_task, thread_t *child_thread);

// Register a task as belonging to a given resource r
void assign_owner_to_task(task_t *t, void *r);

// A resource on the system, which could be an owner of a number of tasks,
// is about to quit. All tasks that are owned by it should be reaped immediately.
void resource_is_quitting(void *r);

usize num_running_threads(task_t *t);

/*
 * thread_enter_kernel
 * Called by the interrupt handler when an exception / interrupt / syscall
 * condition is detected. This will handle the thread preemption refcount level
 * and if this is level zero, save the registers into the task struct.
 *
 * Assumes cur_thread is the currently executing thread.
 *
 * r: A pointer to the pushed registers on the interrupt stack frame.
 */
void thread_enter_kernel(regs_t *r);

/*
 * thread_leave_kernel
 * The opposite of thread_enter_kernel- decrements the reference count,
 * and clears the saved regs if we are exiting back to the task (rather than
 * a nested preemption).
 *
 * Should be called before returning from an interrupt / syscall / exception.
 */
void thread_leave_kernel();

// Methods related to saving / restoring float and vector registers:
// These are defined per-platform in arch/$(ARCH) somewhere
void thread_save_floatvec_regs(thread_t *t);
void thread_load_floatvec_regs(thread_t *t);

/*
 * switchout
 * Low-level thread switching code (per-arch).
 *
 * If the next thread has not yet been run, this will launch it.
 * Otherwise, this assumes the next thread is parked in a switchout call
 * with kernel registers pushed to its kernel stack.
 */
void switchout(thread_t *us, thread_t *them);

// Switch from cur_thread -> t
void switchto(thread_t *t);

void schedule_thread(thread_t *t); // Insert a thread to list of threads
void schedule_task(task_t *t); // Schedule all threads in this task

u64 *task_get_pagetable(task_t *t);
u64 *thread_get_pagetable(thread_t *t);

/*
 * quit_task
 * Sends the quit signal to a task.
 *
 * If current_thread() belongs to the target task, it will be freed
 * and the caller will not be scheduled again after this call!
 */
kret_t quit_task(task_t *t, u64 reason);

/*
 * quit_thread
 * Allow a thread to exit without quitting the task.
 */
kret_t quit_thread(thread_t *t);

/*
 * naptime
 * This calls the scheduler, possibly swapping us back.
 * Our process takes a little nap, and gets woken up later.
 */
void naptime();

/*
 * sleep_on
 * Puts cur_thread to sleep until this resource becomes available.
 *
 * This resource is marked as avilable by calling wakeup_resource.
 */
void sleep_on(void *resource);

/*
 * wakeup_resource
 * Wakes up all sleeping tasks that are waiting on
 * a resource to become available.
 */
void wakeup_resource(void *resource);

const char *get_task_name(task_t *t);
const char *get_thread_name(thread_t *t);

// the kernel task holds all inner kernel threads
extern task_t *kernel_task;

task_t *current_task();
thread_t *current_thread();

END_C_HEADER

#endif // TASK_H
