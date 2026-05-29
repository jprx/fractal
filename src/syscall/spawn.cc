#include <fractal.h>
#include <filesys/fileio.h>
#include <arch.h>
#include <cpu.h>
#include <lib/dlmalloc.h>
#include <virt_mem.h>
#include <syscall.h>
#include <task.h>
#include <out.h>
#include <copy_task.h>
#include <wait.h>

/*
 * libc view of argument / environment variable passing:
 * 
 * ┌────────────────────┐            ┌────────────────────┐          ┌─┬─┬─┬─┬─┬─┬─┬─┐
 * │        argv        │───────────▶│        arg0        │─────────▶│s│t│r│i│n│g│ │0│
 * ├────────────────────┤            ├────────────────────┤          └─┴─┴─┴─┴─┴─┴─┴─┘
 * │        envp        │─────┐      │        arg1        │────┐     ┌─┬─┬─┬─┬─┬─┬─┬─┐
 * └────────────────────┘     │      ├────────────────────┤    └────▶│s│t│r│i│n│g│ │1│
 *                            │      │        arg2        │────┐     └─┴─┴─┴─┴─┴─┴─┴─┘
 *                            │      ├────────────────────┤    │     ┌─┬─┬─┬─┬─┬─┬─┬─┐
 *                            │      │        ...         │    └────▶│s│t│r│i│n│g│ │2│
 *                            │      ├────────────────────┤          └─┴─┴─┴─┴─┴─┴─┴─┘
 *                            │      │        NULL        │───▶X
 *                            │      └────────────────────┘
 *                            │      ┌────────────────────┐          ┌───────────────┐
 *                            └─────▶│        env0        │─────────▶│  "VAR=VAL1"   │
 *                                   ├────────────────────┤          └───────────────┘
 *                                   │        env1        │────┐     ┌───────────────┐
 *                                   ├────────────────────┤    └────▶│  "VAR=VAL2"   │
 *                                   │        env2        │────┐     └───────────────┘
 *                                   ├────────────────────┤    │     ┌───────────────┐
 *                                   │        ...         │    └────▶│  "VAR=VAL3"   │
 *                                   ├────────────────────┤          └───────────────┘
 *                                   │        NULL        │───▶X
 *                                   └────────────────────┘
 */

kret_t copyout_args(thread_t *t, char **argv) {
    usize num_valid_args = 0;
    u64 sp_top = 0;

    // Add an extra entry that is always NULL to mark end of args
    u64 argv_ptr_array_user[TASK_MAX_NUM_ARGS+1] = { 0 };

    // If no arguments, or first argument is NULL, there were no args
    if (NULL == argv || NULL == argv[0]) {
        return ALL_GOOD;
    }

    // There is at least one argument passed in
    sp_top = get_runtime_sp(t, &t->t_init_regs);
    while(num_valid_args < TASK_MAX_NUM_ARGS) {
        argv_ptr_array_user[num_valid_args] = 0;
        usize arglen = 0;
        if (NULL == argv[num_valid_args]) break;

        arglen = strlen(argv[num_valid_args]) + 1; // Copy NULL terminator
        if (argv[num_valid_args][arglen-1] != '\x00') panic("Tried to launch a task with non-null terminated argument");

        sp_top -= arglen;
        sp_top = ALIGN_16(sp_top);
        if (ALL_GOOD != copy_to_task(
            t->t_task,
            sp_top,
            argv[num_valid_args],
            arglen
        )) {
            return OH_NO;
        }

        argv_ptr_array_user[num_valid_args] = sp_top;
        num_valid_args++;
    }

    // Copy argv pointers plus an extra NULL slot at the end
    // (argv_ptr_array_user has an extra zero-initted slot, so we can
    // copy to user 1 extra slot than the number of valid args with no problem)
    sp_top -= sizeof(argv_ptr_array_user[0]) * (num_valid_args+1);
    sp_top = ALIGN_16(sp_top);
    if (ALL_GOOD != copy_to_task(
        t->t_task,
        sp_top,
        &argv_ptr_array_user,
        sizeof(argv_ptr_array_user[0]) * (num_valid_args+1)
    )) {
        return OH_NO;
    }

    set_runtime_sp(t, &t->t_init_regs, sp_top);
    return ALL_GOOD;
}

// Common method to setup a task and copyout all the arguments
task_t *_spawn_task_create_task_internal(task_t *parent, char *binpath, char **argv, SpawnMode mode, bool should_free_argv) {
    bool interrupts_on = true;
    task_t *t = NULL;
    usize num_valid_args = 0;
    ThreadKind kind_to_spawn;

    if (!binpath) return NULL;

    if (mode & SPAWN_MODE_KERN_TASK) kind_to_spawn = KERNEL_OUTER_THREAD;
    else kind_to_spawn = USER_THREAD;

    interrupts_on = (mode & SPAWN_MODE_COOPERATIVE) != 0 ? false : true;
    t = create_task(binpath, interrupts_on, kind_to_spawn);
    if (!t) return NULL;

    if (mode & SPAWN_MODE_ABANDONED) t->t_parent = NULL;
    else t->t_parent = parent;

    if (NULL != parent) {
        // Whether or not we are abandoned, copy over fds from parent
        task_copy_fds(t, parent);
    }

    if (mode & SPAWN_MODE_IMMEDIATE) {
        // Nothing required for this
    } else if (NULL != parent) {
        if (current_task() != parent) {
            // If do_sys_spawn was called on behalf of a different task,
            // we don't support SPAWN_MODE_WAIT_FINISH. This is because
            // we can only put the current task to sleep.
            // If the parent is not cur_thread, it may be currently scheduled,
            // so we would need to put it to sleep somehow and pass the return value back.
            // SPAWN_MODE_WAIT_FINISH only works if cur_thread is parent.
            panic("Tried to launch a SPAWN_MODE_WAIT_FINISH task without being in the parent context");
        }
    }

    if (ALL_GOOD != copyout_args(t->t_threads[INIT_THREAD], argv)) {
        free_task(t);
        return NULL;
    }

    if (NULL == argv) {
        set_task_arg(&t->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ARGC, 0);
        set_task_arg(&t->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ARGV, 0);
    } else {
        for (num_valid_args = 0; num_valid_args < TASK_MAX_NUM_ARGS; num_valid_args++) {
            if (!argv[num_valid_args]) break;
        }
        set_task_arg(&t->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ARGC, num_valid_args);
        set_task_arg(&t->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ARGV, get_runtime_sp(t, &t->t_threads[INIT_THREAD]->t_init_regs));
    }

    // @TODO: env vars for fractal tasks

    if (should_free_argv) {
        for (usize i = 0; i < TASK_MAX_NUM_ARGS; i++) {
            if (argv[i] != NULL) {
                dlfree(argv[i]);
                argv[i] = NULL;
            }
        }
    }

    return t;
}

// Internal version of do_sys_spawn
// This adds the "should_free_argv" flag, which is set to false for the kernel-visible do_sys_spawn method
// This is because we assume kernel tasks using do_sys_spawn can manage their own memory.
// However, syscalls coming in from user tasks using sys_spawn have just allocated buffers for the arguments.
// We want to free these buffers right after copying them to userspace instead of waiting for this task to exit to free them.
// So, for sys_spawn, should_free_argv is set to true to immediately free all argument buffers.
i64 _do_sys_spawn_internal(task_t *parent, char *binpath, char **argv, SpawnMode mode, bool should_free_argv) {
    InterruptMode old_status = global_cpu.SetInterrupts(INTS_DISABLED);
    i64 rval = 0;
    thread_t *us = current_thread();
    i64 exit_code = 0;

    task_t *t = _spawn_task_create_task_internal(parent, binpath, argv, mode, should_free_argv);
    if (!t) {
        rval = -2;
        goto exit_fail;
    }

    // From here on, we cannot fail, as the task is scheduled
    // thread_t deletion belongs to the scheduler from now on.
    schedule_task(t);

    if (0 == (mode & SPAWN_MODE_IMMEDIATE)) {
        // SPAWN_MODE_WAIT_FINISH basically means "fork+execve+wait"
        // We reuse the wait mechanism here to wait for returns
        if (current_task() != parent) panic("spawn cannot wait if the parent isn't the current task");
        do_sys_wait(current_task(), &exit_code, 0);
        exit_code >>= WAIT_INFOBYTE_SHIFT;
    } else {
        // Just directly run the scheduler and let the new task be picked up
        naptime();
    }

    if (us != current_thread()) panic("cur_thread changed during do_sys_spawn_internal");

    // We're back!
    global_cpu.SetInterrupts(old_status);
    if (mode & SPAWN_MODE_IMMEDIATE) return 0;
    else return exit_code;

exit_fail:
    global_cpu.SetInterrupts(old_status);
    if (NULL != t) {
        free_task(t);
    }
    return rval;
}

i64 do_sys_spawn(task_t *t, char *binpath, char **argv, SpawnMode mode) {
    // This is the spawn system call for kernel tasks.
    // It doesn't ask _do_sys_spawn_internal to clean up any argv arguments passed in by default,
    // as we assume kernel tasks manage their own memory.
    // User requests from sys_spawn should use _do_sys_spawn_internal(*,*,*,*,true) to cleanup
    // argument buffers immediately, instead of waiting for the child task to terminate.
    return _do_sys_spawn_internal(t, binpath, argv, mode, false);
}

extern "C" task_t *spawn_anon_task(char *binpath, char **argv, SpawnMode mode) {
    return _spawn_task_create_task_internal(NULL, binpath, argv, mode, false);
}

kret_t copyin_args(char **copied_args, virt_t user_argbase) {
    usize argc = 0;
    kret_t rval = ALL_GOOD;

    for (usize i = 0; i < TASK_MAX_NUM_ARGS; i++) {
        copied_args[i] = NULL;
    }

    if ((virt_t)NULL == user_argbase) return ALL_GOOD;

    while (argc < TASK_MAX_NUM_ARGS) {
        virt_t this_arg_ptr_val;
        if (ALL_GOOD != copy_from_task(current_task(),
                            &this_arg_ptr_val,
                            user_argbase + (argc * sizeof(usize)),
                            sizeof(this_arg_ptr_val))) {
            rval = OH_NO;
            goto fail_exit;
        }

        if (0 == this_arg_ptr_val) {
            break;
        }

        copied_args[argc] = (char *)dlmalloc(TASK_ARGLEN);
        if (!copied_args[argc]) {
            rval = OH_NO;
            goto fail_exit;
        }

        if (ALL_GOOD != copy_from_task(current_task(),
                            copied_args[argc],
                            this_arg_ptr_val,
                            TASK_ARGLEN)) {
            rval = OH_NO;
            goto fail_exit;
        }

        argc++;
    }

    return ALL_GOOD;

fail_exit:
    for (usize i = 0; i < TASK_MAX_NUM_ARGS; i++) {
        if (NULL != copied_args[i]) {
            dlfree(copied_args[i]);
            copied_args[i] = NULL;
        }
    }

    return OH_NO;
}

i64 sys_spawn(virt_t binpath_user, virt_t user_args, SpawnMode mode) {
    i64 rval = 0;
    char *copied_args[TASK_MAX_NUM_ARGS];
    char *binpath_copiedin = NULL;

    binpath_copiedin = (char *)dlmalloc(MAX_FILENAME_LEN);
    if (!binpath_copiedin) {
        rval = -1;
        goto quit;
    }

    binpath_copiedin[MAX_FILENAME_LEN-1] = '\x00';

    if (ALL_GOOD != copy_from_task(current_task(), binpath_copiedin, binpath_user, MAX_FILENAME_LEN)) {
        rval = -2;
        goto quit;
    }

    if (ALL_GOOD != copyin_args(copied_args, user_args)) {
        rval = -3;
        goto quit;
    }

    // @TODO: Env vars for Fractal threads?

    rval = _do_sys_spawn_internal(current_task(), binpath_copiedin, copied_args, mode, true);

    quit:
    if (binpath_copiedin != NULL) dlfree(binpath_copiedin);
    for (usize i = 0; i < TASK_MAX_NUM_ARGS; i++) {
        if (NULL != copied_args[i]) {
            dlfree(copied_args[i]);
        }
    }

    return rval;
}

// Internal version of do_sys_exec
// Does the same thing as _do_sys_spawn_internal, but instead of making a second task,
// replaces the current task with the new task.
// Unconditionally frees argv and envp, if they are passed in!
// This is because the calling task ceases to exist after this call.
i64 _do_sys_exec_internal(task_t *replaceme, char *binpath, char **argv, char **envp) {
    InterruptMode old_interrupt_state = global_cpu.SetInterrupts(INTS_DISABLED);
    i64 rval = 0;
    kret_t status = ALL_GOOD;
    usize num_valid_args = 0;

    if (current_task() != replaceme) {
        panic("Tried to exec on a remote task- unsupported (but theoretically no reason this wouldn't work, at least until we get multiprocessor support)");
    }

    if (!binpath) {
        rval = -1;
        goto exit_fail;
    }

    // If this fails, we should be still able to return to the calling task
    status = replace_task(&replaceme, current_thread(), binpath);
    if (ALL_GOOD != status) {
        rval = -2;
        goto exit_fail;
    }

    if (current_task()->t_state != TASK_ZOMBIE) {
        // Confirm that replace_task correctly marked our current task as a zombie
        panic("_do_sys_exec_internal: we successfully replaced the current task, but somehow the stale current task is not a zombie");
    }

    if (replaceme->t_state != TASK_HEALTHY) {
        // Confirm that replace_task correctly modified replaceme to point to a new, healthy task
        panic("_do_sys_exec_internal: new task is not healthy");
    }

    if (ALL_GOOD != copyout_args(replaceme->t_threads[INIT_THREAD], argv)) {
        rval = -3;
        panic("copying out args failed");
        goto exit_fail;
    }

    if (NULL == argv) {
        set_task_arg(&replaceme->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ARGC, 0);
        set_task_arg(&replaceme->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ARGV, 0);
    } else {
        for (num_valid_args = 0; num_valid_args < TASK_MAX_NUM_ARGS; num_valid_args++) {
            if (!argv[num_valid_args]) break;
        }
        set_task_arg(&replaceme->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ARGC, num_valid_args);
        set_task_arg(&replaceme->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ARGV, get_runtime_sp(replaceme, &replaceme->t_threads[INIT_THREAD]->t_init_regs));
    }

    if (ALL_GOOD != copyout_args(replaceme->t_threads[INIT_THREAD], envp)) {
        rval = -4;
        panic("copying out envp failed");
        goto exit_fail;
    }

    if (NULL == envp) {
        set_task_arg(&replaceme->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ENVP, 0);
    } else {
        for (num_valid_args = 0; num_valid_args < TASK_MAX_NUM_ARGS; num_valid_args++) {
            if (!envp[num_valid_args]) break;
        }
        set_task_arg(&replaceme->t_threads[INIT_THREAD]->t_init_regs, MAIN_ARG_ENVP, get_runtime_sp(replaceme, &replaceme->t_threads[INIT_THREAD]->t_init_regs));
    }

    // Always free argv if it exists
    if (argv) {
        for (usize i = 0; i < TASK_MAX_NUM_ARGS; i++) {
            if (argv[i] != NULL) {
                dlfree(argv[i]);
                argv[i] = NULL;
            }
        }
    }

    // Same for envp
    if (envp) {
        for (usize i = 0; i < TASK_MAX_NUM_ARGS; i++) {
            if (envp[i] != NULL) {
                dlfree(envp[i]);
                envp[i] = NULL;
            }
        }
    }

    // Schedule a context switch rather than popping down the stack
    // and returning to where we came from, as this will unconditionally
    // restore user registers incorrectly and return to the wrong spot.
    // Rather, we need to set launched to false, and context switch away-
    // then, the scheduler will pick this task up as an unlaunched task,
    // and launch it the slow way / first init way like it's a new task.
    switchto(replaceme->t_threads[INIT_THREAD]);
    panic("_do_sys_exec_internal: We should never get here");

exit_fail:
    global_cpu.SetInterrupts(old_interrupt_state);
    return rval;
}

i64 do_sys_execve(task_t *t, char *path, char *argv[], char *envp[]) {
    return _do_sys_exec_internal(t, path, argv, envp);
}

i64 sys_execve(virt_t user_path, virt_t user_argv, virt_t user_envp) {
    i64 rval = 0;
    char *copied_args[TASK_MAX_NUM_ARGS];
    char *copied_envs[TASK_MAX_NUM_ARGS];
    char *binpath_copiedin = NULL;

    binpath_copiedin = (char *)dlmalloc(MAX_FILENAME_LEN);
    if (!binpath_copiedin) {
        rval = -1;
        goto quit;
    }

    if (ALL_GOOD != copy_from_task(current_task(), binpath_copiedin, user_path, MAX_FILENAME_LEN)) {
        rval = -2;
        goto quit;
    }

    binpath_copiedin[MAX_FILENAME_LEN-1] = '\x00';

    if (ALL_GOOD != copyin_args(copied_args, user_argv)) {
        rval = -3;
        goto quit;
    }

    if (ALL_GOOD != copyin_args(copied_envs, user_envp)) {
        rval = -4;
        goto quit;
    }

    rval = do_sys_execve(current_task(), binpath_copiedin, copied_args, copied_envs);

    quit:
    if (binpath_copiedin != NULL) dlfree(binpath_copiedin);
    for (usize i = 0; i < TASK_MAX_NUM_ARGS; i++) {
        if (NULL != copied_args[i]) {
            dlfree(copied_args[i]);
        }
    }

    return rval;
}
