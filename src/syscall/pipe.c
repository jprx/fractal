#include <fractal.h>
#include <syscall.h>
#include <copy_task.h>
#include <pipe.h>
#include <lib/dlmalloc.h>

// #define DEBUG_PIPES

pipe_t *make_pipe() {
    pipe_t *p = (pipe_t *)dlmalloc(sizeof(*p));
    if (!p) return NULL;

    p->buffer = (u8 *)AllocPage(SMALL_PAGE);
    if (!p->buffer) {
        dlfree(p);
        return NULL;
    }

    p->write_idx = 0;
    p->read_idx = 0;
    p->write_open = true;
    p->read_open = true;
    p->read_refcount = 1;
    p->write_refcount = 1;
    return p;
}

void free_pipe(pipe_t *p) {
    if (!p) return;
    if (p->read_open || p->write_open) panic("Tried to free an open pipe");
    if (NULL != p->buffer) {
        FreePage((virt_t)p->buffer);
        p->buffer = NULL;
    }
    dlfree(p);
}

// Call this if you are copying a pipe fd around
void pipe_inc_refcount(pipe_t *pi, pipe_attribute_t attrib) {
    if (!pi) return;
    // Only increment refcounts if a given end is open,
    // this way we don't accidentally reopen ends of the pipe
    switch (attrib) {
        case PIPE_READ:
        if (pi->read_open) pi->read_refcount++;
        break;

        case PIPE_WRITE:
        if (pi->write_open) pi->write_refcount++;
        break;
    }
}

i64 pipe_read(pipe_t *pi, pipe_attribute_t attrib, u8 *buf, usize sz) {
    usize bytes_copied = 0;
    if (!pi) return 0;
    if (attrib != PIPE_READ) return 0;
#ifdef DEBUG_PIPES
    printf("[pipe_read]\n", buf);
#endif // DEBUG_PIPES

    // If nothing available, wait until some data is written
    while (pi->write_idx == pi->read_idx && pi->write_open) {
#ifdef DEBUG_PIPES
        printf("[pipe_read] going to sleep\n");
#endif // DEBUG_PIPES
        sleep_on(&pi->read_idx);
#ifdef DEBUG_PIPES
        printf("[pipe_read] waking up\n");
#endif // DEBUG_PIPES
    }

    while (bytes_copied < sz) {
        // Check if we've gotten too far ahead of the read end
        if (pi->read_idx >= pi->write_idx) break;

        buf[bytes_copied++] = pi->buffer[pi->read_idx++ % PIPE_BUFLEN];
    }

    // Wakeup any writers
    wakeup_resource(&pi->write_idx);
#ifdef DEBUG_PIPES
    printf("[pipe_read] got %d bytes\n", bytes_copied);
#endif // DEBUG_PIPES
    return bytes_copied;
}

i64 pipe_write(pipe_t *pi, pipe_attribute_t attrib, u8 *buf, usize sz) {
    usize bytes_copied = 0;
    if (!pi) return 0;
    if (attrib != PIPE_WRITE) return 0;
#ifdef DEBUG_PIPES
    printf("[pipe_write]: %s\n", buf);
#endif // DEBUG_PIPES

    while (bytes_copied < sz) {
        // For every byte copied, check whether the other end of the pipe has closed
        // @TODO: Check if task was killed, stop trying to send data if so
        if (!pi->read_open) return -1;

        // Check if we've gotten too far ahead of the read end
        if (pi->write_idx >= pi->read_idx + PIPE_BUFLEN) {
            // Wakeup reader before going to sleep
            wakeup_resource(&pi->read_idx);
#ifdef DEBUG_PIPES
            printf("[pipe_write] going to sleep\n");
#endif // DEBUG_PIPES
            sleep_on(&pi->write_idx);
#ifdef DEBUG_PIPES
            printf("[pipe_write] waking up\n");
#endif // DEBUG_PIPES
        } else {
            pi->buffer[pi->write_idx++ % PIPE_BUFLEN] = buf[bytes_copied++];
        }
    }

    // Wakeup any readers
    wakeup_resource(&pi->read_idx);
    return bytes_copied;
}

void pipe_close(pipe_t *pi, pipe_attribute_t attrib) {
#ifdef DEBUG_PIPES
    printf("pipe_close (%d, %d)\n", pi->read_refcount, pi->write_refcount);
#endif // DEBUG_PIPES
    switch(attrib) {
        case PIPE_READ:
        pi->read_refcount--;
        if (pi->read_refcount != 0) return;
        pi->read_open = false;
        wakeup_resource(&pi->write_idx);
        break;

        case PIPE_WRITE:
        pi->write_refcount--;
        if (pi->write_refcount != 0) return;
        pi->write_open = false;
        wakeup_resource(&pi->read_idx);
        break;

        default:
        panic("pipe_close called with unknown pipe attribute");
        break;
    }

    if (!pi->read_open && !pi->write_open) {
#ifdef DEBUG_PIPES
        printf("Freeing pipe\n");
#endif // DEBUG_PIPES
        free_pipe(pi);
    }
}

i64 do_sys_pipe(task_t *t, i32 filedes[2]) {
    pipe_t *p = NULL;
    filedes[0] = -1;
    filedes[1] = -1;

    for (usize i = 0; i < NUM_FDS; i++) {
        if (FD_NONE == t->t_fds[i].type) {
            filedes[0] = i;
            break;
        }
    }

    for (usize i = filedes[0] + 1; i < NUM_FDS; i++) {
        if (FD_NONE == t->t_fds[i].type) {
            filedes[1] = i;
            break;
        }
    }

    if (filedes[0] == -1 || filedes[1] == -1) {
        return -ENFILE;
    }

    p = make_pipe();

    for (usize i = 0; i < 2; i++) {
        t->t_fds[filedes[i]].type = FD_PIPE;
        t->t_fds[filedes[i]].resource = p;
        t->t_fds[filedes[i]].offset = 0;
    }
    t->t_fds[filedes[0]].attribute = PIPE_READ;
    t->t_fds[filedes[1]].attribute = PIPE_WRITE;

    return 0;
}

i64 sys_pipe(virt_t user_fds) {
    i64 rval = 0;
    i32 filedes[2];

    rval = do_sys_pipe(current_task(), filedes);
    if (0 != rval) return -1;

    if (ALL_GOOD != copy_to_task(current_task(), user_fds, filedes, sizeof(filedes))) {
        return -2;
    }

    return 0;
}
