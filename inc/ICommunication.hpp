#ifndef ICOMMUNICATION_HPP
#define ICOMMUNICATION_HPP

#include <cstdint>

namespace CTLIB
{

// ͨ�Ŵ���� CRTP �ӿ�

template <typename Impl>
class ICommunication
{
public:
    inline void Init()            { Impl().ImplInit(); }
    inline void WriteCommand(uint8_t c) { Impl().ImplWriteCommand(c); }
    inline void WriteData8(uint8_t d)   { Impl().ImplWriteData8(d); }
    inline void WriteData16(uint16_t d) { Impl().ImplWriteData16(d); }
    inline void WriteBulk(uint16_t *b, uint16_t s) { Impl().ImplWriteBulk(b, s); }
    inline void CsLow()       { Impl().ImplCsLow(); }
    inline void CsHigh()      { Impl().ImplCsHigh(); }
    inline void DcCommand()   { Impl().ImplDcCommand(); }
    inline void DcData()      { Impl().ImplDcData(); }
    inline void DelayMs(uint32_t ms) { Impl().ImplDelayMs(ms); }

protected:
    ICommunication() = default;
    ~ICommunication() = default;

private:
    Impl &Impl() { return static_cast<Impl&>(*this); }
};

} // namespace CTLIB

#endif
