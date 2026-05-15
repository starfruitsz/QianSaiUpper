#ifndef ICOMMUNICATION_HPP
#define ICOMMUNICATION_HPP

#include <cstdint>

namespace CTLIB {

class ICommunication {
public:
    virtual ~ICommunication() = default;

    virtual void init() = 0;

    virtual void writeCommand(uint8_t cmd) = 0;

    virtual void writeData8(uint8_t data) = 0;

    virtual void writeData16(uint16_t data) = 0;

    virtual void writeBulk(uint16_t *buffer, uint16_t size) = 0;

    virtual void csLow() = 0;

    virtual void csHigh() = 0;

    virtual void dcCommand() = 0;

    virtual void dcData() = 0;

    virtual void backlightOn() = 0;
    virtual void backlightOff() = 0;

    virtual void delayMs(uint32_t ms) = 0;
};

} // namespace CTLIB

#endif // ICOMMUNICATION_HPP
