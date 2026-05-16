#ifndef ICOMMUNICATION_HPP
#define ICOMMUNICATION_HPP

#include <cstdint>

namespace CTLIB
{

// 通信传输层 CRTP 接口

template <typename Impl>
class ICommunication
{
public:
    inline void init()            { impl().implInit(); }
    inline void writeCommand(uint8_t c) { impl().implWriteCommand(c); }
    inline void writeData8(uint8_t d)   { impl().implWriteData8(d); }
    inline void writeData16(uint16_t d) { impl().implWriteData16(d); }
    inline void writeBulk(uint16_t *b, uint16_t s) { impl().implWriteBulk(b, s); }
    inline void csLow()       { impl().implCsLow(); }
    inline void csHigh()      { impl().implCsHigh(); }
    inline void dcCommand()   { impl().implDcCommand(); }
    inline void dcData()      { impl().implDcData(); }
    inline void delayMs(uint32_t ms) { impl().implDelayMs(ms); }

protected:
    ICommunication() = default;
    ~ICommunication() = default;

private:
    Impl &impl() { return static_cast<Impl&>(*this); }
};

} // namespace CTLIB

#endif
