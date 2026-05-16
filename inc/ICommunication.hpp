#ifndef ICOMMUNICATION_HPP
#define ICOMMUNICATION_HPP

#include <cstdint>

namespace CTLIB
{

/* ============================================================ */
/* BufferPolicy — 传输层缓冲区 */
/* ============================================================ */

template <uint16_t Size = 1024>
struct BufferPolicy
{
    static constexpr uint16_t size = Size;  /* C++11: constexpr */
    uint16_t data[Size] = {};  /* C++11: brace init */
};

/* ============================================================ */
/* ICommunication — CRTP 通信传输层接口 */
/* 模板参数: Impl=子类, BufSz=缓冲区大小（默认1024） */
/* ============================================================ */

template <typename Impl, uint16_t BufSz = 1024>
class ICommunication
{
public:
    static constexpr uint16_t kBufSize = BufSz;  /* C++11: constexpr */

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

    /* === 子类实现 === */
    /* 将缓冲区数据刷入硬件，子类根据传输特性实现 */
    inline void Flush(uint16_t sz) { GetImpl().ImplFlush(sz); }

protected:
    ICommunication() = default;
    ~ICommunication() = default;

    BufferPolicy<BufSz>  mBuf;  /* C++11: member buffer, shared with sub-classes */

private:
    Impl &GetImpl() { return static_cast<Impl&>(*this); }  /* C++11: static_cast */
};

}  /* namespace CTLIB */

#endif