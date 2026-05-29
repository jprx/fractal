#ifndef WAIT_H
#define WAIT_H

#include <fractal.h>

BEGIN_C_HEADER

#define WAIT_INFOBYTE_SHIFT 8
#define WAIT_INFOBYTE_MASK 0x0FF

typedef struct waitnotif_t {
    struct waitnotif_t *next;
    u64 pid;
    u64 exit_code;
} waitnotif_t;

struct task_t;

// Scans the waitlist queue and returns the first notification present
// Assumes at least one notification exists- will panic if the wait list is empty!
i64 scan_waitnotif_queue(struct task_t *t, i64 *exitcode);

// Scan the waitlist and consume a notification only if a notification with the correct pid exists
// Returns true if we found the PID, false otherwise.
bool scan_waitnotif_queue_for_pid(struct task_t *t, i64 *exitcode, pid_t pid);

// Use up a wait notification, removing it from the queue and freeing it
// Returns PID of the notif we consumed
i64 consume_waitnotif(i64 *exitcode, waitnotif_t *wnotif, waitnotif_t **prev_ptr);

END_C_HEADER

#endif // WAIT_H
