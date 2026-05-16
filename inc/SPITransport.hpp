#ifndef SPITRANSPORT_HPP
#define SPITRANSPORT_HPP

#include "ICommunication.hpp"
#include <cstdint>
#include <functional>

namespace CTLIB
{

class SPITransport final : public ICommunication<SPITransport>
{
public:
    explicit SPITransport(SPI_HandleTypeDef *hspi) noexcept : mHspi(hspi)
    {
        cbCsLow   = [] { GPIOD->BSRR = (uint32_t)GPIO_PIN_11 << 16u; };
        cbCsHigh  = [] { GPIOD->BSRR = GPIO_PIN_11; };
        cbDcCmd   = [] { GPIOD->BSRR = (uint32_t)GPIO_PIN_12 << 16u; };
        cbDcDat   = [] { GPIOD->BSRR = GPIO_PIN_12; };
        cbDelayMs = [] (uint32_t ms) { HAL_Delay(ms); };

        cbInit = [this]
        {
            __HAL_RCC_SPI3_CLK_ENABLE();
            __HAL_RCC_GPIOB_CLK_ENABLE();
            __HAL_RCC_GPIOD_CLK_ENABLE();

            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_3;  g.Mode=GPIO_MODE_AF_PP;      g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_VERY_HIGH; g.Alternate=GPIO_AF6_SPI3; HAL_GPIO_Init(GPIOB,&g); }
            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_5;  g.Mode=GPIO_MODE_AF_PP;      g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_VERY_HIGH; g.Alternate=GPIO_AF6_SPI3; HAL_GPIO_Init(GPIOB,&g); }
            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_11; g.Mode=GPIO_MODE_OUTPUT_PP;  g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_HIGH;      HAL_GPIO_Init(GPIOD,&g); }
            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_12; g.Mode=GPIO_MODE_OUTPUT_PP;  g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_HIGH;      HAL_GPIO_Init(GPIOD,&g); }
            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_13; g.Mode=GPIO_MODE_OUTPUT_PP;  g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_LOW;       HAL_GPIO_Init(GPIOD,&g); }

            cbDcDat(); cbCsHigh();
            GPIOD->BSRR = (uint32_t)GPIO_PIN_13 << 16u;  // 默认关背光

            mHspi->Instance            = SPI3;
            mHspi->Init.Mode           = SPI_MODE_MASTER;
            mHspi->Init.Direction      = SPI_DIRECTION_1LINE;
            mHspi->Init.DataSize       = SPI_DATASIZE_8BIT;
            mHspi->Init.CLKPolarity    = SPI_POLARITY_LOW;
            mHspi->Init.CLKPhase       = SPI_PHASE_1EDGE;
            mHspi->Init.NSS            = SPI_NSS_SOFT;
            mHspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
            mHspi->Init.FirstBit       = SPI_FIRSTBIT_MSB;
            mHspi->Init.TIMode         = SPI_TIMODE_DISABLE;
            mHspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
            mHspi->Init.CRCPolynomial  = 0;
            HAL_SPI_Init(mHspi);
            __HAL_SPI_ENABLE(mHspi);
            SPI_1LINE_TX(mHspi);
        };
    }

    // ===== 用户可替换的回调 =====
    std::function<void()>         cbInit;
    std::function<void()>         cbCsLow;
    std::function<void()>         cbCsHigh;
    std::function<void()>         cbDcCmd;
    std::function<void()>         cbDcDat;
    std::function<void(uint32_t)> cbDelayMs;

    // ===== CRTP 接口 =====

    void implInit() { cbInit(); }

    void implWriteCommand(uint8_t cmd)
    {
        while (mHspi->Instance->SR & SPI_SR_BSY) {}
        cbDcCmd();
        mHspi->Instance->DR = cmd;
        while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
        while (mHspi->Instance->SR & SPI_SR_BSY) {}
        cbDcDat();
    }

    void implWriteData8(uint8_t data)
    {
        mHspi->Instance->DR = data;
        while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
    }

    void implWriteData16(uint16_t data)
    {
        mHspi->Instance->DR = static_cast<uint8_t>(data >> 8u);
        while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
        mHspi->Instance->DR = static_cast<uint8_t>(data);
        while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
    }

    void implWriteBulk(uint16_t *buf, uint16_t sz)
    {
        CLEAR_BIT(mHspi->Instance->CR1, SPI_CR1_SPE);
        SET_BIT(mHspi->Instance->CR1, SPI_CR1_DFF);
        SET_BIT(mHspi->Instance->CR1, SPI_CR1_SPE);

        cbCsLow();
        for (uint16_t i = 0; i < sz; ++i)
        {
            mHspi->Instance->DR = buf[i];
            while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
        }
        while (mHspi->Instance->SR & SPI_SR_BSY) {}
        cbCsHigh();

        CLEAR_BIT(mHspi->Instance->CR1, SPI_CR1_SPE);
        CLEAR_BIT(mHspi->Instance->CR1, SPI_CR1_DFF);
        SET_BIT(mHspi->Instance->CR1, SPI_CR1_SPE);
    }

    void implCsLow()        { cbCsLow(); }
    void implCsHigh()       { cbCsHigh(); }
    void implDcCommand()    { cbDcCmd(); }
    void implDcData()       { cbDcDat(); }
    void implDelayMs(uint32_t ms) { cbDelayMs(ms); }

private:
    SPI_HandleTypeDef *mHspi;
};

} // namespace CTLIB

#endif
