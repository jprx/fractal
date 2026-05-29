#include <fractal.h>
#include <io/serial.h>
#include <io/serial/uart16550.h>

#define RECV_TIMEOUT 5

void UART16550::Initialize() {
    // Disable interrupts:
    WriteReg(UART16550_INTERRUPT_ENABLE_REG, 0x00);

    // Set DLAB to 1 so we can set the BAUD rate
    WriteReg(UART16550_LINE_CONTROL_REG, 0x80);

    // Set BAUD rate to 115200 (fastest)
    // Rate of 0x0001 (115200 / 1 = 115200 AKA full speed)
    WriteReg(UART16550_BAUD_RATE_LSB, 0x01);
    WriteReg(UART16550_BAUD_RATE_MSB, 0x00);

    // 8N1 (8 bits, no parity, 1 stop bit)- this is the default for UART
    WriteReg(UART16550_LINE_CONTROL_REG, 0x03);

    // Set port to normal mode
    WriteReg(UART16550_MODEM_CONTROL_REG, 0x0F);

    // Enable Received Data Available interrupt
    WriteReg(UART16550_INTERRUPT_ENABLE_REG, 0x01);
}

void UART16550::SendChar(u8 c) {
    while ((ReadReg(UART16550_LINE_STATUS_REG) & UART16550_LSR_SEND_READY_MASK) == 0) {}
    WriteReg(UART16550_DATA_REG, c);
}

u8 UART16550::RecvChar() {
    int counter = 0;
    while ((ReadReg(UART16550_LINE_STATUS_REG) & UART16550_LSR_RECV_READY_MASK) == 0) {
        if (counter++ > RECV_TIMEOUT) return 0;
    }
    return ReadReg(UART16550_DATA_REG);
}

bool UART16550::HasData() {
    return ReadReg((UART16550_LINE_STATUS_REG) & UART16550_LSR_RECV_READY_MASK) != 0;
}