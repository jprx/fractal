#ifndef RASPI4B_UART_H
#define RASPI4B_UART_H

#include <io/serial.h>

class RaspiUART4B : public SerialPort {
public:
    RaspiUART4B(usize);
    virtual void Initialize() override;
    virtual void SendChar(u8) override;
    virtual u8 RecvChar() override;
    virtual bool HasData() override;
};

#endif // RASPI4B_UART_H
