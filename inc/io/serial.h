#ifndef SERIAL_H
#define SERIAL_H

#include <types.h>

class SerialPort {
public:
    usize baseaddr;
    void WriteString (const char *s);
    virtual void Initialize() = 0;
    virtual void SendChar(u8 c) = 0;
    virtual u8 RecvChar() = 0;
    virtual bool HasData() = 0;
};

/*
 * DECLARE_SERIAL
 * Creates a new SerialPort subclass instance
 *
 * Inputs: `name` is the name of the new object
 * being created at the call site.
 *
 * We expect SERIAL_PORT to have been defined
 * in the processor family's arch.h file
 * (at inc/arch/$(ARCH).h) to be a valid SerialPort
 * subclass appropriate for this architecture.
 *
 * We also expect SERIAL_BASEADDR to be populated
 * with the address of this serial port for the
 * given platform / board being compiled for.
 * (For now, this is set in the same place as
 * SERIAL_PORT, but in the future may be moved
 * to some kind of board-specific path, eg. inc/board)
 *
 * The idea is for clients of the serial API to
 * be able to simply declare a new SerialPort object
 * anywhere they'd like without having to worry about
 * what the actual derived class is, or where it is in
 * memory. They just use DECLARE_SERIAL and cast it to
 * a SerialPort pointer.
 *
 * Example:
 * DECLARE_SERIAL(my_serial_port);
 * SerialPort *ser = (SerialPort *)&my_serial_port;
 * ser->WriteString("I don't care what kind of port this is\r\n");
 */
#define DECLARE_SERIAL(name) SERIAL_PORT name(SERIAL_BASEADDR);

// Emitting this byte will fixup newlines, because SeaBIOS
// messes up line endings (CRLF behavior). This resets it.
#define SERIAL_FIXUP_NEWLINES "\033[?7h"

// Control + C (eg. ^C)
#define SERIAL_INTC 0x03

#define UART_BACKSPACE_CHAR 0x7F

/*
 * UARTInterrupt
 * Called by this architecture's interrupt handler when the serial device
 * has data for us. This will retrieve characters from the global serial port
 * and store them in the global serial InputBuffer.
 */
void UARTInterrupt();

#endif // SERIAL_H
