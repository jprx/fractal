#ifndef PL011_H
#define PL011_H

#include <io/serial.h>

class PL011 : public SerialPort {
public:
    PL011(usize);
    virtual void Initialize() override;
    virtual void SendChar(u8) override;
    virtual u8 RecvChar() override;
    virtual bool HasData() override;
};

#define PL011_DATA_REGISTER ((0x00))
#define PL011_FLAGS_REGISTER ((0x018))
#define PL011_RECV_FIFO_EMPTY_MASK ((1 << 4))
#define PL011_XMIT_FIFO_EMPTY_MASK ((1 << 5))
#define PL011_INTERRUPT_REGISTER ((0x38))

#endif // PL011_H
