#ifndef ICOMMUNICATION_HPP
#define ICOMMUNICATION_HPP

#include <cstdint>
#include <cstdarg>
#include <cstdio>

#if __cpp_lib_print >= 202207L
#include <print>
#include <format>
#endif

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

    /* Read element at index (const) */
    const uint16_t& operator[](uint16_t idx) const { return mData[idx]; }

    /* Push value at current cursor, auto-increment */
    void Push(uint16_t val) { mData[mPos++] = val; }

    /* Get raw pointer to buffer start (for bulk ops like vsnprintf) */
    uint16_t* Ptr()       { return mData; }
    const uint16_t* Ptr() const { return mData; }

    /* Get pointer to current cursor position (for sequential write) */
    uint16_t* Data()       { return &mData[mPos]; }
    const uint16_t* Data() const { return &mData[mPos]; }

    /* Reset write cursor to zero */
    void Reset() { mPos = 0; }

    uint16_t  mPos = 0;              /* current write cursor */

private:
    uint16_t  mData[Size] = {};      /* C++11: brace init */
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
/*   WriteData8   - send 8-bit data                             */
/*   WriteData16  - send 16-bit data MSB-first                  */
/*   DelayMs      - blocking delay (HAL_Delay)                  */
/*   Flush(sz)   - flush mBuf[0..sz) to hardware (subclass impl) */
/*   Print        - formatted UART output (C++23 std::print)    */
/* ============================================================ */

template <typename Impl, uint16_t BufSz = 1024>
class ICommunication
{
public:
    static constexpr uint16_t kBufSize = BufSz;  /* C++11: constexpr */

    /* --- Hardware lifecycle --- */
    inline void Init()            { GetImpl().ImplInit(); }    /* one-time hw setup */

    /* --- Data transfer --- */
    inline void WriteData8(uint8_t d)   { GetImpl().ImplWriteData8(d); }    /* 1 byte */
    inline void WriteData16(uint16_t d) { GetImpl().ImplWriteData16(d); }   /* 2 bytes MSB */

    /* --- Utility --- */
    inline void DelayMs(uint32_t ms) { GetImpl().ImplDelayMs(ms); }  /* blocking delay */
    inline void Flush(uint16_t sz)   { GetImpl().ImplFlush(sz); mBuf.Reset(); }  /* flush sz elements then reset */
    inline void Flush()              { Flush(mBuf.mPos); }

    /* --- Buffer helpers --- */
    inline void Buff(uint16_t val)   { mBuf.Push(val); }  /* push one pixel */
    inline uint16_t BuffPos() const  { return mBuf.mPos; }             /* read cursor */

    BufferPolicy<BufSz>  mBuf;  /* C++11: shared buffer, accessible by ILCD */

    /* ============================================================ */
    /* Print - C++23 std::print style (zero-cost format + UART TX)  */
    /* Enabled when __cpp_lib_print >= 202207L (C++23 <print>)       */
    /* Falls back to vsnprintf on C++11..C++20                       */
    /* ============================================================ */

#if __cpp_lib_print >= 202207L  /* C++23: std::print available */

    /* C++23: type-safe format, no format string vulns */
    template <typename... Args>
    void Print(std::format_string<Args...> fmt, Args&&... args)
    {
        auto &buf = this->mBuf;
        uint16_t Pos = BuffPos();
        auto result = std::format_to_n(
            reinterpret_cast<char*>(buf.Data()),
            (kBufSize -  Pos) * 2,
            fmt,
            std::forward<Args>(args)...
        );
        uint16_t n = static_cast<uint16_t>((result.size + 1) / 2);  /* char count -> uint16_t count */
        Flush(n + Pos);
    }

    /* C++23: raw string overload (no formatting overhead) */
    void Print(const char *s)
    {
        while (*s)
        {
            WriteData8(static_cast<uint8_t>(*s++));
        }
    }

#else  /* C++11..C++20 fallback: vsnprintf */

    /* C++11: variadic printf via vsnprintf */
    void Print(const char *fmt, ...)
    {
        auto &buf = this->mBuf;
        uint16_t Pos = BuffPos();
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(reinterpret_cast<char*>(buf.Data()), (kBufSize -  Pos) * 2, fmt, args);
        va_end(args);

        if (len < 0) return;
        uint16_t n = static_cast<uint16_t>((len + 1) / 2);  /* char count -> uint16_t count */
        Flush(Pos + n);
    }

#endif  /* __cpp_lib_print */

protected:
    ICommunication() = default;    /* C++11: defaulted constructor */
    ~ICommunication() = default;   /* C++11: defaulted destructor */

private:
    Impl &GetImpl() { return static_cast<Impl&>(*this); }  /* C++11: static_cast, CRTP downcast */
};

}  /* namespace CTLIB */

#endif
