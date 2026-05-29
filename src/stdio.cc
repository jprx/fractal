#include <fractal.h>
#include <task.h>
#include <oxide/console.h>
#include <stdio.h>
#include <task.h>
#include <syscall.h>
#include <tty.h>
#include <io/serial.h>

extern TTYBuffer global_serial_tty;

extern "C" void init_serial_stdio(task_t *t) {
    fd_t template_fd;

    if(!t) return;

    memset(&template_fd, '\x00', sizeof(template_fd));
    template_fd.type = FD_SERIAL;
    template_fd.resource = NULL;

    copy_fd(&t->t_fds[STDIO_IN], &template_fd);
    copy_fd(&t->t_fds[STDIO_OUT], &template_fd);
    copy_fd(&t->t_fds[STDIO_ERR], &template_fd);
}

extern "C" void init_console_stdio(task_t *t, void *c) {
    fd_t template_fd;

    if(!t) return;

    memset(&template_fd, '\x00', sizeof(template_fd));
    template_fd.type = FD_CONSOLE;
    template_fd.resource = (void *)c;

    copy_fd(&t->t_fds[STDIO_IN], &template_fd);
    copy_fd(&t->t_fds[STDIO_OUT], &template_fd);
    copy_fd(&t->t_fds[STDIO_ERR], &template_fd);
}

extern "C" usize task_read_stdio(task_t *t, u32 fd_num, u8 *buf, usize sz) {
    if (!t) return 0;
    if (!buf) panic("Tried to read with NULL buffer");
    if (sz > SYS_READWRITE_LEN_MAX) sz = SYS_READWRITE_LEN_MAX;
    if (current_task() != t) panic("Tried to call a blocking read on a remote process");

    switch(t->t_fds[fd_num].type) {
        case FD_SERIAL:
        return global_serial_tty.RecvData(t, buf, sz);
        break;

        case FD_CONSOLE:
        return ((Console *)(t->t_fds[fd_num].resource))->tty.RecvData(t, buf, sz);
        break;

        default:
        panic("Tried to call read stdio on a non-stdio file descriptor");
        break;
    }

    panic("task_read_stdio: unhandled case");
}

extern "C" void task_write_stdio(task_t *t, u32 fd_num, u8 *buf, usize sz) {
    if (!t) return;
    if (fd_num > NUM_FDS) return;
    if (sz > SYS_READWRITE_LEN_MAX) sz = SYS_READWRITE_LEN_MAX;

    switch(t->t_fds[fd_num].type) {
        case FD_SERIAL:
        for (usize i = 0; i < sz; i++) {
            printf("%c", buf[i]);
        }
        break;

        case FD_CONSOLE:
        ((Console *)(t->t_fds[fd_num].resource))->ReceiveData((char *)buf, sz);
        break;

        default:
        panic("Tried to call write stdio on a non-stdio file descriptor");
        break;
    }
}
