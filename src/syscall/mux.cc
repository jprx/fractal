// System call multiplexing- select, poll, etc.
#include <fractal.h>
#include <syscall.h>
#include <task.h>
#include <copy_task.h>
#include <stdio.h>
#include <oxide/console.h>
#include <tty.h>

#if NUM_FDS > 64
#error "sys_select assumes at most 64 file descriptors are present, but NUM_FDS is more than 64. Need to update sys_select"
#endif // NUM_FDS > 64

extern TTYBuffer global_serial_tty;

i64 sys_select(i32 nfds, virt_t u_readfds, virt_t u_writefds, virt_t u_errfds, virt_t u_timeout) {
    return do_sys_select(current_task(), nfds, u_readfds, u_writefds, u_errfds, u_timeout);
}

i64 sys_poll(virt_t u_fds, u32 nfds, i32 timeout) {
    return do_sys_poll(current_task(), u_fds, nfds, timeout);
}

// A minimal implementation of select(), just what's required to get vim to work.
i64 do_sys_select(task_t *t, i32 nfds, virt_t u_readfds, virt_t u_writefds, virt_t u_errfds, virt_t u_timeout) {
    TTYBuffer *tty_to_check = NULL;
    u64 readfds = 0;
    u64 writefds = 0;
    u64 errfds = 0;
    i64 num_detected = 0; // Return value is number of detected FD's

    if (!t) return -1;
    if (current_thread()->t_state != THREAD_RUNNING) {
        panic("Called select() on non-active task (name %s, state %d)", t->t_name, current_thread()->t_state);
    }

    if (0 != u_readfds) {
        if (ALL_GOOD != copy_from_task(t, &readfds, u_readfds, sizeof(readfds))) return -1;
    }

    if (0 != u_writefds) {
        if (ALL_GOOD != copy_from_task(t, &writefds, u_writefds, sizeof(writefds))) return -1;
    }

    if (0 != u_errfds) {
        if (ALL_GOOD != copy_from_task(t, &errfds, u_errfds, sizeof(errfds))) return -1;
    }

    if (0 != writefds) panic("Our select() doesn't support write fd's");

    // We only allow select() to work on stdio fd's
    for (usize i = 0; i < NUM_STDIO_FDS; i++) {
        if (FD_ISSET(i, (fd_set *)&readfds)) {
            if (t->t_fds[i].type == FD_NONE) continue;

            switch(t->t_fds[i].type) {
                case FD_SERIAL:
                tty_to_check = &global_serial_tty;
                break;

                case FD_CONSOLE:
                tty_to_check = &((Console *)(t->t_fds[i].resource))->tty;
                break;

                default:
                panic("Called select() on readfd of type %d (fd num %d) but we only support serial and console fd's", t->t_fds[i].type, i);
                break;
            }
        }
    }

    // Sanity check nothing but stdio fd's was requested
    for (usize i = NUM_STDIO_FDS; i < NUM_FDS; i++) {
        if (FD_ISSET(i, (fd_set *)&readfds)) {
            panic("Called select() on fd %d, which isn't a stdio fd", i);
        }
    }

    if (!tty_to_check) return -2;
    if (!tty_to_check->DataWaiting()) readfds = 0;
    else num_detected = 1;

    // We don't support writefds or errfds, so zero them all out before returning to user
    writefds = 0;
    errfds = 0;

    // Copy back the current bit vectors
    if (0 != u_readfds) {
        if (ALL_GOOD != copy_to_task(t, u_readfds, &readfds, sizeof(readfds))) return -1;
    }

    if (0 != u_writefds) {
        if (ALL_GOOD != copy_to_task(t, u_writefds, &writefds, sizeof(writefds))) return -1;
    }

    if (0 != u_errfds) {
        if (ALL_GOOD != copy_to_task(t, u_errfds, &errfds, sizeof(errfds))) return -1;
    }

    return num_detected;
}

i64 do_sys_poll(task_t *t, virt_t u_fds, u32 nfds, i32 timeout) {
    return -1;
}
