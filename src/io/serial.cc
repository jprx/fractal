#include <fractal.h>
#include <cpu.h>
#include <arch.h>
#include <tty.h>
#include <io/serial.h>

/*
 * serial.cc
 * The higher-level abstraction connecting file descriptors,
 * buffered input, and terminal devices to the low-level SerialPort.
 */

// This is the SerialPort device driver class
extern SERIAL_PORT global_serial_device;

// This is the buffer that tracks data coming in from the serial port,
// and flushes it out to processes when they call read() on a FD_SERIAL file descriptor.
TTYBuffer global_serial_tty;

void UARTInterrupt() {
    // This is called from an interrupt context, so be quick
    while (global_serial_device.HasData()) {
        char c = global_serial_device.RecvChar();

        if (UART_BACKSPACE_CHAR == c) c = '\b';

        // Real UART16550 loves to randomly say that data is available,
        // and then not actually return anything (causing timeouts so we read 0)
        if (0 == c) return;

        if (SERIAL_INTC == c) printf("^C");
        if ('\b' == c) {
            printf("\b");
            printf(" ");
            printf("\b");
        }

        global_serial_tty.SendData(c);

        // For us, cooked mode = echo on,
        // and raw mode = echo off. UNIX has these as
        // two separate things, but we don't care.
        if (TTY_MODE_COOKED == global_serial_tty.GetMode()) {
            if ('\r' == c) c = '\n';
            if ('\b' == c) return;
            printf("%c", c);
        }
    }
}
