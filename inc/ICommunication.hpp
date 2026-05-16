#ifndef ICOMMUNICATION_HPP
#define ICOMMUNICATION_HPP

#include <cstdint>

namespace CTLIB
{

/* ============================================================ */
/* BufferPolicy - fixed-size pixel buffer (uint16_t) */
/* For SPI LCD: stores RGB565 pixels before bulk transfer */
/* For UART:     reused as char buffer for printf formatting */
/* ============================================================ */

template <uint16_t Size = 1024>
struct BufferPolicy
{
    static constexpr uint16_t size = Size;  /* C++11: constexpr */
    uint16_t data[Size] = {};               /* C++11: brace init */
};

/* ============================================================ */
/* ICommunication - CRTP base class for hardware transports */
/*                                                              */
/* Template params:                                             */
/*   Impl   = CRTP subclass (SPITransport / UartTransport)      */
/*   BufSz  = buffer size in uint16_t units (default 1024)      */
/*                                                              */
/* Public CRTP interface (all inline, zero-cost delegation):    */
/*                                                              */
/*   Init()       - hardware init (clocks, GPIO, peripheral)    */
/*   WriteCommand - send LCD command byte (DC=0, CS active)     */
/*   WriteData8   - send 8-bit data (DC=1)                      */
/*   WriteData16  - send 16-bit data MSB-first (2 x WriteData8) */
/*   WriteBulk    - burst-write uint16_t[] buffer (optimized)   */
/*   CsLow/CsHigh - chip-select control                         */
/*   DcCommand    - DC pin low (command mode)                   */
/*   DcData       - DC pin high (data mode)                     */
/*   DelayMs      - blocking delay (HAL_Delay)                  */
/*   Flush        - flush buffer to hardware (subclass impl)    */
/* ============================================================ */

template <typename Impl, uint16_t BufSz = 1024>
class ICommunication
{
public:
    static constexpr uint16_t kBufSize = BufSz;  /* C++11: constexpr */

    /* --- Hardware lifecycle --- */
    inline void Init()            { GetImpl().ImplInit(); }    /* one-time hw setup */

    /* --- LCD command/data (DC-controlled) --- */
    inline void WriteCommand(uint8_t c) { GetImpl().ImplWriteCommand(c); }  /* DC=0, send cmd */
    inline void WriteData8(uint8_t d)   { GetImpl().ImplWriteData8(d); }    /* DC=1, 1 byte */
    inline void WriteData16(uint16_t d) { GetImpl().ImplWriteData16(d); }   /* DC=1, 2 bytes MSB */
    inline void WriteBulk(uint16_t *b, uint16_t s) { GetImpl().ImplWriteBulk(b, s); }  /* burst write */

    /* --- Chip-select (active-low) --- */
    inline void CsLow()       { GetImpl().ImplCsLow(); }       /* CS = 0 */
    inline void CsHigh()      { GetImpl().ImplCsHigh(); }      /* CS = 1 */

    /* --- Data/Command pin --- */
    inline void DcCommand()   { GetImpl().ImplDcCommand(); }   /* DC = 0 */
    inline void DcData()      { GetImpl().ImplDcData(); }      /* DC = 1 */

    /* --- Utility --- */
    inline void DelayMs(uint32_t ms) { GetImpl().ImplDelayMs(ms); }  /* blocking delay */
    inline void Flush(uint16_t sz)   { GetImpl().ImplFlush(sz); }    /* flush mBuf to hw */

    BufferPolicy<BufSz>  mBuf;  /* C++11: shared buffer, accessible by ILCD */

protected:
    ICommunication() = default;    /* C++11: defaulted constructor */
    ~ICommunication() = default;   /* C++11: defaulted destructor */

private:
    Impl &GetImpl() { return static_cast<Impl&>(*this); }  /* C++11: static_cast, CRTP downcast */
};

}  /* namespace CTLIB */

#endif
