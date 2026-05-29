#include <fractal.h>
#include <types.h>
#include <io/serial.h>

#ifdef HVF_OPTIMIZATION_HACK
#pragma GCC push_options
#pragma GCC optimize("O0")
#endif // HVF_OPTIMIZATION_HACK

void SerialPort::WriteString(const char *s) {
    const char *cursor = s;
    while (*cursor) {
        this->SendChar(*cursor);
        cursor++;
    }
}

#ifdef HVF_OPTIMIZATION_HACK
#pragma GCC pop_options
#endif // HVF_OPTIMIZATION_HACK
