#include <io/serial.h>
#include <io/serial/pl011.h>

PL011::PL011(usize baseaddr_in) {
    this->baseaddr = baseaddr_in;
}

void PL011::Initialize() {
    volatile u8 *regs = (u8 *)this->baseaddr;

    // Enable interrupts:
    regs[PL011_INTERRUPT_REGISTER] = (1ULL << 4);
}

void PL011::SendChar(u8 c) {
    volatile u8 *regs = (u8 *)this->baseaddr;
    regs[PL011_DATA_REGISTER] = c;
}

u8 PL011::RecvChar() {
    volatile u8 *regs = (u8 *)this->baseaddr;
#if 0
    while((regs[PL011_FLAGS_REGISTER] & PL011_RECV_FIFO_EMPTY_MASK) == 0) {}
#endif // 0
    return regs[PL011_DATA_REGISTER];
}

bool PL011::HasData() {
    volatile u8 *regs = (u8 *)this->baseaddr;
    return (regs[PL011_FLAGS_REGISTER] & PL011_RECV_FIFO_EMPTY_MASK) == 0;
}
