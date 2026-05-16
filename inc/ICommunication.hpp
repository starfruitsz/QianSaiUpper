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
    inline void Init()            { GetImpl().ImplInit(); }
    inline void WriteCommand(uint8_t c) { GetImpl().ImplWriteCommand(c); }
    inline void WriteData8(uint8_t d)   { GetImpl().ImplWriteData8(d); }
    inline void WriteData16(uint16_t d) { GetImpl().ImplWriteData16(d); }
    inline void WriteBulk(uint16_t *b, uint16_t s) { GetImpl().ImplWriteBulk(b, s); }
    inline void CsLow()       { GetImpl().ImplCsLow(); }
    inline void CsHigh()      { GetImpl().ImplCsHigh(); }
    inline void DcCommand()   { GetImpl().ImplDcCommand(); }
    inline void DcData()      { GetImpl().ImplDcData(); }
    inline void DelayMs(uint32_t ms) { GetImpl().ImplDelayMs(ms); }

protected:
    ICommunication() = default;
    ~ICommunication() = default;

private:
    Impl &GetImpl() { return static_cast<Impl&>(*this); }
};

} // namespace CTLIB

#endif
