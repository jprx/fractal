#ifndef APPLE_UART_H
#define APPLE_UART_H

#include <io/serial.h>

class AppleUART : public SerialPort {
public:
    AppleUART(usize);
    virtual void Initialize() override;
    virtual void SendChar(u8) override;
    virtual u8 RecvChar() override;
    virtual bool HasData() override;
};

#endif // APPLE_UART_H
