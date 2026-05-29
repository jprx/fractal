#include <fractal.h>
#include <syscall.h>
#include <termios.h>
#include <copy_task.h>
#include <oxide.h>
#include <oxide/console.h>
#include <tty.h>

extern TTYBuffer global_serial_tty;

struct termios default_terminal = {
    .c_iflag = TTYDEF_IFLAG,
    .c_oflag = TTYDEF_OFLAG,
    .c_cflag = TTYDEF_CFLAG,
    .c_lflag = TTYDEF_LFLAG,
    .c_line = 0,
    .c_cc = { 0 },
    .c_ispeed = TTYDEF_SPEED,
    .c_ospeed = TTYDEF_SPEED,
};

i64 sys_tcgetattr(i32 fd, virt_t u_termios_p) {
    return do_sys_tcgetattr(current_task(), fd, u_termios_p);
}

i64 sys_tcsetattr(i32 fd, i32 optional_actions, virt_t u_termios_p) {
    return do_sys_tcsetattr(current_task(), fd, optional_actions, u_termios_p);
}

i64 do_sys_tcgetattr(task_t *t, i32 fd, virt_t u_termios_p) {
    if (fd > NUM_FDS) return -1;
    // @TODO: Instead of always just copying the defaults, actually remember what serial / this console window set.
    if (ALL_GOOD != copy_to_task(t, u_termios_p, &default_terminal, sizeof(default_terminal))) return -2;
    return 0;
}

i64 do_sys_tcsetattr(task_t *t, i32 fd, i32 optional_actions, virt_t u_termios_p) {
    struct termios req;
    if (fd > NUM_FDS) return -1;

    if (ALL_GOOD != copy_from_task(t, &req, u_termios_p, sizeof(req))) return -2;

    switch(t->t_fds[fd].type) {
        case FD_SERIAL:
        if (0 == (req.c_lflag & ICANON)) {
            // Raw (non-canonical) mode requested
            global_serial_tty.SetMode(TTY_MODE_RAW);
        } else {
            // Cooked (canonical) mode requested
            global_serial_tty.SetMode(TTY_MODE_COOKED);
        }
        break;

        case FD_CONSOLE:
        if (0 == (req.c_lflag & ICANON)) {
            ((Console *)(t->t_fds[fd].resource))->tty.SetMode(TTY_MODE_RAW);
        } else {
            ((Console *)(t->t_fds[fd].resource))->tty.SetMode(TTY_MODE_COOKED);
        }
        break;

        default:
        panic("Called tcgetattr on non-terminal FD");
        break;
    }

    return 0;
}
