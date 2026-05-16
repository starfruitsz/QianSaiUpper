#ifndef UARTTRANSPORT_HPP
#define UARTTRANSPORT_HPP

#include "ICommunication.hpp"
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <functional>

namespace CTLIB
{

/* ============================================================ */
/* UartTransport — UART 传输层 */
/* 继承 ICommunication，提供串口打印能力 */
/* C++20: std::format / C++11: vsnprintf 可切换 */
/* ============================================================ */

class UartTransport final : public ICommunication<UartTransport>
{
public:
    static constexpr uint16_t kTxBufSize = 256;  /* C++11: constexpr */

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

    /* ===== 以下为 LCD 专用接口，UART 不使用但仍需实现 ===== */
    void ImplWriteCommand(uint8_t) {}
    void ImplWriteData16(uint16_t) {}
    void ImplWriteBulk(uint16_t *, uint16_t) {}
    void ImplCsLow()        {}
    void ImplCsHigh()       {}
    void ImplDcCommand()    {}
    void ImplDcData()       {}
    void ImplDelayMs(uint32_t ms) { CbDelayMs(ms); }

    /* ============================================================ */
    /* Printf — C++11 可变参数格式化打印 */
    /* 基于 vsnprintf，最大 kTxBufSize 字节 */
    /* ============================================================ */
    void Printf(const char *fmt, ...)
    {
        char buf[kTxBufSize];  /* C++11: stack-allocated fixed array */
        va_list args;
        va_start(args, fmt);   /* C++11: variadic arguments */
        int len = vsnprintf(buf, sizeof(buf), fmt, args);  /* C++11: vsnprintf */
        va_end(args);

        if (len < 0) return;
        uint16_t n = static_cast<uint16_t>(len < static_cast<int>(sizeof(buf)) ? len : sizeof(buf) - 1);
        for (uint16_t i = 0; i < n; ++i)
        {
            ImplWriteData8(static_cast<uint8_t>(buf[i]));
        }
    }

private:
    UART_HandleTypeDef *mHuart;
};

}  /* namespace CTLIB */

#endif