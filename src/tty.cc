#include <fractal.h>
#include <task.h>
#include <tty.h>
#include <io/serial.h>

/*
 * SendSigINT
 * Kills all running/ runnable tasks attached to this tty.
 * (For now, hardcoded to be serial).
 */
void SendSigINT() {
    // We're in an interrupt context, so don't need to lock the CPU
    // (interrupts are off already)
    thread_t *cursor = current_thread()->t_next;

    // Quit any non-current tasks first
    while (cursor != current_thread()) {
        if (FD_SERIAL == cursor->t_task->t_fds[STDIO_IN].type) {
            // @TODO: A better way of doing this
            // We'd like to also cancel tasks waiting on pipes (eg. in THREAD_WAITING),
            // but don't want to cancel tasks waiting on children (also in THREAD_WAITING now).
            if (THREAD_RUNNABLE == cursor->t_state ||
                THREAD_RUNNING == cursor->t_state) {
                quit_task(cursor->t_task, 0);
            }
        }
        cursor = cursor->t_next;
    }

    // And maybe quit the current one too
    // (this will cause us to not be rescheduled)
    if (FD_SERIAL == current_thread()->t_task->t_fds[STDIO_IN].type) {
        quit_task(current_thread()->t_task, 0);
    }
}

TTYBuffer::TTYBuffer() {
    this->cursor = 0;
    this->mode = TTY_MODE_COOKED;
    this->data_ready = false;
    memset(&this->dbuf, '\x00', sizeof(this->dbuf));
}

void TTYBuffer::SendData(char c) {
    if (cursor >= sizeof(this->dbuf)) panic("TTYBuffer overflow");

    if (TTY_MODE_COOKED == mode) {
        if ('\r' == c) c = '\n';
        if ('\b' == c) {
            if (cursor == 0) return;
            dbuf[--cursor] = '\x00';
            return;
        }

        if (SERIAL_INTC == c) {
            // If we're not in raw mode, we can interrupt tasks
            SendSigINT();
            return;
        }
    }

    if (cursor < sizeof(dbuf) - 1) {
        dbuf[cursor++] = c;
    }

    if ('\n' == c || TTY_MODE_RAW == mode) {
        // This line is ready to go, wakeup any sleeping readers
        // Readers are sleeping in TTYBuffer::RecvData, if they exist.
        data_ready = true;
        wakeup_resource(this);
    }
}

usize TTYBuffer::RecvData(task_t *t, u8 *buf, usize len) {
    if (current_task() != t) panic("tried to receive data for a task that isn't the current task");

    while (!DataWaiting()) {
        // We'll be woken up by SendData when there's some activity
        // Race conditions may cause us to be woken up, but another task may have gotten to the data first,
        // so that's why I use while(!DataWaiting()) in a loop here. Once we exit the loop we guarantee
        // that we both have A) been woken up, and B) there actually is some data to read.
        sleep_on(this);
    }

    if (!DataWaiting()) panic("TTYBuffer::RecvData woke up, but no data is ready for us");

    // @TODO: Only report 1 line at a time.
    // Right now, if there are 2 or more lines in the buffer when read() is called,
    // we just return everything at once. Instead, we should read 4095 chars and then
    // discard everything until the next newline, and only return 1 line at a time.
    // See: https://www.man7.org/linux/man-pages/man3/termios.3.html
    // Specifically the section on Canonical vs Non-Canonical Modes.

    data_ready = false;
    usize bytes_to_copy = min(len, cursor);
    memcpy(buf, dbuf, bytes_to_copy);
    cursor = 0;

    return bytes_to_copy;
}

bool TTYBuffer::DataWaiting() {
    return data_ready;
}

void TTYBuffer::SetMode(tty_mode_t mode_in) {
    mode = mode_in;
}

tty_mode_t TTYBuffer::GetMode() {
    return mode;
}