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

    /* Access element at index, auto-advance cursor */
    uint16_t& operator[](uint16_t idx) { return mData[idx]; }
    const uint16_t& operator[](uint16_t idx) const { return mData[idx]; }

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
    inline void Flush(uint16_t sz)   { GetImpl().ImplFlush(sz); }    /* flush mBuf to hw */

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
        auto result = std::format_to_n(
            reinterpret_cast<char*>(&buf[0]),
            kBufSize * 2 - 1,
            fmt,
            std::forward<Args>(args)...
        );
        uint16_t n = static_cast<uint16_t>(result.size);
        for (uint16_t i = 0; i < n; ++i)
        {
            WriteData8(reinterpret_cast<uint8_t*>(&buf[0])[i]);
        }
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
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(reinterpret_cast<char*>(&buf[0]), kBufSize * 2, fmt, args);
        va_end(args);

        if (len < 0) return;
        uint16_t n = static_cast<uint16_t>(len < static_cast<int>(kBufSize * 2) ? len : kBufSize * 2 - 1);
        for (uint16_t i = 0; i < n; ++i)
        {
            WriteData8(reinterpret_cast<uint8_t*>(&buf[0])[i]);
        }
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
