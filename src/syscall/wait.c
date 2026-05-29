#include <syscall.h>
#include <copy_task.h>
#include <lib/dlmalloc.h>

// Use up a wait notification, removing it from the queue and freeing it
// Returns PID of the notif we consumed
i64 consume_waitnotif(i64 *exitcode, waitnotif_t *wnotif, waitnotif_t **prev_ptr) {
    i64 rval;
    if (NULL != exitcode) *exitcode = (WAIT_INFOBYTE_MASK & wnotif->exit_code) << WAIT_INFOBYTE_SHIFT;
    rval = wnotif->pid;
    if (0 == wnotif->pid) panic("Child exited but the exit PID is 0 which is the kernel, how is that possible?");
    *prev_ptr = wnotif->next;
    dlfree(wnotif);
    return rval;
}

// Scans the waitlist queue and returns the first notification present
// Assumes at least one notification exists- will panic if the wait list is empty!
i64 scan_waitnotif_queue(task_t *t, i64 *exitcode) {
    i64 rval;
    if (!t) return 0;
    if (!t->t_waitlist) panic("scan_waitnotif_queue tried to consume an empty waitlist");
    return consume_waitnotif(exitcode, t->t_waitlist, &t->t_waitlist);
}

// Scan the waitlist and consume a notification only if a notification with the correct pid exists
// Returns true if we found the PID, false otherwise.
// If a notif matching the PID we want is found, will consume and free it
// Successive calls to this method will succeed for the same PID if and only if multiple tasks sharing the
// same PID were launched and quit. It is atomic in the sense that if it returns true for a PID, it will never
// return true again for that same PID unless somehow another task with the same PID was launched and quit.
bool scan_waitnotif_queue_for_pid(task_t *t, i64 *exitcode, pid_t pid) {
    i64 rval;
    if (!t) return false;
    waitnotif_t *cursor = t->t_waitlist;
    waitnotif_t **prev_ptr = &t->t_waitlist;
    while (cursor) {
        if (cursor->pid == pid) {
            // We only return true/ false, so ignore return value from consume_waitnotif (the pid of the task that quit)
            // we already know the PID of the task that quit!
            consume_waitnotif(exitcode, cursor, prev_ptr);
            return true;
        }
        prev_ptr = &cursor->next;
        cursor = cursor->next;
    }
    return false;
}

i64 do_sys_wait(task_t *t, i64 *exitcode, u64 flags) {
    if (t != current_task()) panic("Tried to call wait() on a task that isn't the requestor- can't put a task that isn't in a syscall to sleep in a wait context");

    // Something is already in the waitlist for us
    if (t->t_waitlist) return scan_waitnotif_queue(t, exitcode);

    if (flags & WNOHANG) {
        if (exitcode) *exitcode = 0;
        return 0;
    }

    // When a child task quits, it will call wakeup_resource on our waitlist
    if (t->t_waitlist) panic("t_waitlist is non-zero before going to sleep");
    sleep_on(&t->t_waitlist);
    if (!t->t_waitlist) panic("wait returned, but no notifications are waiting for us");

    return scan_waitnotif_queue(t, exitcode);
}

i64 sys_wait(virt_t user_status_loc, u64 flags) {
    i64 exitcode = 0;
    i64 rval = do_sys_wait(current_task(), &exitcode, flags);
    if (ALL_GOOD != copy_to_task(current_task(), user_status_loc, &exitcode, sizeof(exitcode))) {
        printf("sys_wait failed to copyout the status code\n");
        return -1;
    }
    return rval;
}

i64 do_sys_waitpid(task_t *t, pid_t p, i64 *exitcode, u64 flags) {
    if (t != current_task()) panic("Tried to call waitpid() on a task that isn't the requestor- can't put a task that isn't in a syscall to sleep in a wait context");

    // Check waitlist to see if the task is already there
    if (t->t_waitlist && scan_waitnotif_queue_for_pid(t, exitcode, p)) {
        return p;
    }

    if (flags & WNOHANG) {
        if (exitcode) *exitcode = 0;
        return 0;
    }

    while (true) {
        sleep_on(&t->t_waitlist);

        if (t->t_waitlist && scan_waitnotif_queue_for_pid(t, exitcode, p)) {
            return p;
        }
    }
}

i64 sys_waitpid(pid_t p, virt_t user_status_loc, u64 flags) {
    i64 exitcode = 0;
    i64 rval = do_sys_waitpid(current_task(), p, &exitcode, flags);
    if (ALL_GOOD != copy_to_task(current_task(), user_status_loc, &exitcode, sizeof(exitcode))) {
        printf("sys_waitpid failed to copyout the status code\n");
        return -1;
    }
    return rval;
}

i64 do_sys_getpid(task_t *t) { return t->t_pid; }
i64 sys_getpid() { return do_sys_getpid(current_task()); }
