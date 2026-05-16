#ifndef UARTTRANSPORT_HPP
#define UARTTRANSPORT_HPP

#include "ICommunication.hpp"
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <print>\n#include <format>

namespace CTLIB
{

/* ============================================================ */
/* UartTransport — UART 传输层 */
/* 继承 ICommunication，提供串口打印能力 */
/* C++20: std::format / C++11: vsnprintf 可切换 */
/* ============================================================ */

class UartTransport final : public ICommunication<UartTransport, 256>  /* C++11: final */
{
public:

    explicit UartTransport(UART_HandleTypeDef *huart) noexcept : mHuart(huart)  /* C++11: noexcept, explicit */
    {
        CbDelayMs = [](uint32_t ms) { HAL_Delay(ms); };  /* C++11: lambda */

        CbInit = [this]
        {
            __HAL_RCC_USART1_CLK_ENABLE();
            __HAL_RCC_GPIOA_CLK_ENABLE();

            /* TX = PA9, RX = PA10（AF7） */
            {  /* C++11: brace init */
                GPIO_InitTypeDef g = {};
                g.Pin       = GPIO_PIN_9;
                g.Mode      = GPIO_MODE_AF_PP;
                g.Pull      = GPIO_NOPULL;
                g.Speed     = GPIO_SPEED_FREQ_HIGH;
                g.Alternate = GPIO_AF7_USART1;
                HAL_GPIO_Init(GPIOA, &g);
            }
            {
                GPIO_InitTypeDef g = {};
                g.Pin       = GPIO_PIN_10;
                g.Mode      = GPIO_MODE_AF_PP;
                g.Pull      = GPIO_NOPULL;
                g.Speed     = GPIO_SPEED_FREQ_HIGH;
                g.Alternate = GPIO_AF7_USART1;
                HAL_GPIO_Init(GPIOA, &g);
            }

            mHuart->Instance          = USART1;
            mHuart->Init.BaudRate     = 115200;
            mHuart->Init.WordLength   = UART_WORDLENGTH_8B;
            mHuart->Init.StopBits     = UART_STOPBITS_1;
            mHuart->Init.Parity       = UART_PARITY_NONE;
            mHuart->Init.Mode         = UART_MODE_TX_RX;
            mHuart->Init.HwFlowCtl    = UART_HWCONTROL_NONE;
            mHuart->Init.OverSampling = UART_OVERSAMPLING_16;
            HAL_UART_Init(mHuart);
        };
    }

    /* ===== 用户可替换的回调 ===== */
    std::function<void()>         CbInit;        /* C++11: std::function */
    std::function<void(uint32_t)> CbDelayMs;

    /* ===== CRTP 接口实现 ===== */

    void ImplInit() { CbInit(); }

    /* 发送单个字节（UART TX 轮询） */
    void ImplWriteData8(uint8_t data)
    {
        while (!(mHuart->Instance->SR & USART_SR_TXE)) {}  /* 等待发送缓冲区空 */
        mHuart->Instance->DR = data;
    }


    /* ============================================================ */
    /* Print - C++23 std::print style (zero-cost format + UART TX)  */
    /* Enabled when __cpp_lib_print >= 202207L (C++23 <print>)       */
    /* Falls back to Printf (vsnprintf) on C++11..C++20              */
    /* ============================================================ */

#if __cpp_lib_print >= 202207L  /* C++23: std::print available */

    /* C++23: std::print style - type-safe, no format string vulns */
    template <typename... Args>
    void Print(std::format_string<Args...> fmt, Args&&... args)
    {
        auto &buf = this->mBuf;  /* C++11: reuse base buffer */
        auto result = std::format_to_n(
            reinterpret_cast<char*>(buf.data),
            kBufSize * 2 - 1,
            fmt,
            std::forward<Args>(args)...
        );
        uint16_t n = static_cast<uint16_t>(result.size);
        for (uint16_t i = 0; i < n; ++i)
        {
            ImplWriteData8(reinterpret_cast<uint8_t*>(buf.data)[i]);
        }
        reinterpret_cast<char*>(buf.data)[n] = '\0';
    }

    /* C++23: single-argument overload (no formatting needed) */
    void Print(const char *s)
    {
        while (*s)
        {
            ImplWriteData8(static_cast<uint8_t>(*s++));
        }
    }

#else  /* C++11..C++20 fallback: vsnprintf */

    /* C++11: variadic printf via vsnprintf, max kBufSize*2 bytes */
    void Printf(const char *fmt, ...)
    {
        auto &buf = this->mBuf;
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(reinterpret_cast<char*>(buf.data), kBufSize * 2, fmt, args);
        va_end(args);

        if (len < 0) return;
        uint16_t n = static_cast<uint16_t>(len < static_cast<int>(kBufSize * 2) ? len : kBufSize * 2 - 1);
        for (uint16_t i = 0; i < n; ++i)
        {
            ImplWriteData8(reinterpret_cast<uint8_t*>(buf.data)[i]);
        }
    }

#endif  /* __cpp_lib_print */


private:
    UART_HandleTypeDef *mHuart;
};

}  /* namespace CTLIB */

#endif