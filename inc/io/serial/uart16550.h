#ifndef UART16550_H
#define UART16550_H

#include <io/serial.h>

class UART16550 : public SerialPort {
public:
    virtual void Initialize() override;
    virtual void SendChar(u8) override;
    virtual u8 RecvChar() override;
    virtual bool HasData() override;
    virtual u8 ReadReg(u16) = 0;
    virtual void WriteReg(u16 reg, u8 val) = 0;
};

// Port-based IO for UART16550 (x86_64)
class UART16550Port : public UART16550 {
public:
    UART16550Port(usize baseaddr_in);
    virtual u8 ReadReg(u16) override;
    virtual void WriteReg(u16 reg, u8 val) override;
};

// Memory-mapped IO for UART16550 (riscv64)
class UART16550MMIO : public UART16550 {
public:
    UART16550MMIO(usize baseaddr_in);
    virtual u8 ReadReg(u16) override;
    virtual void WriteReg(u16 reg, u8 val) override;
};

#define UART16550_NUM_REGS 	                8

// DLAB = Divisor Latch Access Bit
// DLAB is the most significant bit of LCR (register 3)
// When DLAB = 0 these ports behave as data and interrupt enable, respectively
#define UART16550_DATA_REG                  0
#define UART16550_INTERRUPT_ENABLE_REG      1

// When DLAB = 1 these ports behave as BAUD rate
#define UART16550_BAUD_RATE_LSB             0
#define UART16550_BAUD_RATE_MSB             1

// These are independent of DLAB
#define UART16550_FIFO_CONTROL_REG          2
#define UART16550_LINE_CONTROL_REG          3
#define UART16550_MODEM_CONTROL_REG         4
#define UART16550_LINE_STATUS_REG           5
#define UART16550_MODEM_STATUS_REG          6
#define UART16550_SCRATCH_REG               7

// Masks for line status register
#define UART16550_LSR_RECV_READY_MASK 		0x01
#define UART16550_LSR_SEND_READY_MASK 		0x20

#endif // UART16550_H
