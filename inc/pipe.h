#ifndef PIPE_H
#define PIPE_H

#include <fractal.h>

BEGIN_C_HEADER

#define PIPE_BUFLEN ((SMALL_PAGE_SIZE))

typedef struct pipe_t {
    // A page used for buffering data
    u8 *buffer;

    // Synchronization indices for each end of the pipe:
    usize write_idx;
    usize read_idx;

    // Whether or not each end of the pipe is open?
    // If both ends are closed, we can free the pipe
    bool write_open;
    bool read_open;

    // Each end of the pipe has its own refcount, as
    // Fractal file descriptors are not reference counted
    usize read_refcount;
    usize write_refcount;
} pipe_t;

typedef enum {
    PIPE_READ  = 1,
    PIPE_WRITE = 2,
} pipe_attribute_t;

i64 pipe_read(pipe_t *pi, pipe_attribute_t attrib, u8 *buf, usize sz);
i64 pipe_write(pipe_t *pi, pipe_attribute_t attrib, u8 *buf, usize sz);
void pipe_close(pipe_t *pi, pipe_attribute_t attrib);
void pipe_inc_refcount(pipe_t *pi, pipe_attribute_t attrib);

END_C_HEADER

#endif // PIPE_H
