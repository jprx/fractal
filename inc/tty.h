#ifndef IOBUF_H
#define IOBUF_H

/*
 * IOBuf.h
 * Various classes to support input buffering for both serial and GUI terminals.
 */

#include <fractal.h>

#define TTY_BUFFER_LEN ((1024))

typedef enum {
    TTY_MODE_COOKED,
    TTY_MODE_RAW,
} tty_mode_t;

class TTYBuffer {
public:
    /*
     * SendData
     * New data is available for this resource.
     * Called from an interrupt context, this method
     * appends the new byte to the current buffer, and
     * if applicable, wakes up all tasks sleeping on this resource.
     */
    void SendData(char c);

    /*
     * RecvData
     * Consume all pending data from this buffer up to len bytes.
     * This will put the current task to sleep if no data is available yet.
     *
     * Returns the number of bytes read.
     */
    usize RecvData(task_t *t, u8 *buf, usize len);

    /*
     * DataWaiting
     * Returns true if data is ready to be received,
     * false otherwise.
     */
    bool DataWaiting();

    /*
     * SetMode
     * Changes the terminal mode between raw and cooked.
     */
    void SetMode(tty_mode_t mode);
    tty_mode_t GetMode();

    TTYBuffer();

private:
    bool data_ready;
    tty_mode_t mode;
    usize cursor;
    u8 dbuf[TTY_BUFFER_LEN];
};

#endif // IOBUF_H
