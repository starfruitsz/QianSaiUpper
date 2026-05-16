#ifndef SPITRANSPORT_HPP
#define SPITRANSPORT_HPP

#include "ICommunication.hpp"
#include <cstdint>
#include <functional>

namespace CTLIB
{

class SPITransport final : public ICommunication<SPITransport, 1024>
{
public:
    explicit SPITransport(SPI_HandleTypeDef *hspi) noexcept : mHspi(hspi)
    {
        CbCsLow   = [] { GPIOD->BSRR = (uint32_t)GPIO_PIN_11 << 16u; };
        CbCsHigh  = [] { GPIOD->BSRR = GPIO_PIN_11; };
        CbDcCmd   = [] { GPIOD->BSRR = (uint32_t)GPIO_PIN_12 << 16u; };
        CbDcDat   = [] { GPIOD->BSRR = GPIO_PIN_12; };
        CbDelayMs = [] (uint32_t ms) { HAL_Delay(ms); };

        CbInit = [this]
        {
            __HAL_RCC_SPI3_CLK_ENABLE();
            __HAL_RCC_GPIOB_CLK_ENABLE();
            __HAL_RCC_GPIOD_CLK_ENABLE();

            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_3;  g.Mode=GPIO_MODE_AF_PP;      g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_VERY_HIGH; g.Alternate=GPIO_AF6_SPI3; HAL_GPIO_Init(GPIOB,&g); }
            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_5;  g.Mode=GPIO_MODE_AF_PP;      g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_VERY_HIGH; g.Alternate=GPIO_AF6_SPI3; HAL_GPIO_Init(GPIOB,&g); }
            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_11; g.Mode=GPIO_MODE_OUTPUT_PP;  g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_HIGH;      HAL_GPIO_Init(GPIOD,&g); }
            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_12; g.Mode=GPIO_MODE_OUTPUT_PP;  g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_HIGH;      HAL_GPIO_Init(GPIOD,&g); }
            { GPIO_InitTypeDef g={}; g.Pin=GPIO_PIN_13; g.Mode=GPIO_MODE_OUTPUT_PP;  g.Pull=GPIO_NOPULL; g.Speed=GPIO_SPEED_FREQ_LOW;       HAL_GPIO_Init(GPIOD,&g); }

            CbDcDat(); CbCsHigh();
            GPIOD->BSRR = (uint32_t)GPIO_PIN_13 << 16u;  /* Ĭ�Ϲر��� */

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
    std::function<void()>         CbInit;
    std::function<void()>         CbCsLow;
    std::function<void()>         CbCsHigh;
    std::function<void()>         CbDcCmd;
    std::function<void()>         CbDcDat;
    std::function<void(uint32_t)> CbDelayMs;

    void ImplInit() { CbInit(); }

    /* LCD command byte (DC=0, then DC=1) */
    void WriteCommand(uint8_t cmd)
    {
        while (mHspi->Instance->SR & SPI_SR_BSY) {}
        CbDcCmd();
        mHspi->Instance->DR = cmd;
        while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
        while (mHspi->Instance->SR & SPI_SR_BSY) {}
        CbDcDat();
    }

    void ImplWriteData8(uint8_t data)
    {
        mHspi->Instance->DR = data;
        while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
    }

    void ImplWriteData16(uint16_t data)
    {
        mHspi->Instance->DR = static_cast<uint8_t>(data >> 8u);
        while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
        mHspi->Instance->DR = static_cast<uint8_t>(data);
        while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
    }

    

    /* --- Chip-select (active-low) --- */
    void CsLow()     { CbCsLow(); }        /* CS = 0 */
    void CsHigh()    { CbCsHigh(); }       /* CS = 1 */

    /* --- Data/Command pin --- */
    void DcCommand() { CbDcCmd(); }        /* DC = 0 */
    void DcData()    { CbDcDat(); }        /* DC = 1 */
    void ImplDelayMs(uint32_t ms) { CbDelayMs(ms); }

    /* === Buffer Flush - SPI burst write with CS management === */
    void ImplFlush(uint16_t sz)
    {
        CLEAR_BIT(mHspi->Instance->CR1, SPI_CR1_SPE);
        SET_BIT(mHspi->Instance->CR1, SPI_CR1_DFF);
        SET_BIT(mHspi->Instance->CR1, SPI_CR1_SPE);

        CbCsLow();
        for (uint16_t i = 0; i < sz; ++i)
        {
            mHspi->Instance->DR = this->mBuf.Ptr()[i];
            while (!(mHspi->Instance->SR & SPI_SR_TXE)) {}
        }
        while (mHspi->Instance->SR & SPI_SR_BSY) {}
        CbCsHigh();

        CLEAR_BIT(mHspi->Instance->CR1, SPI_CR1_SPE);
        CLEAR_BIT(mHspi->Instance->CR1, SPI_CR1_DFF);
        SET_BIT(mHspi->Instance->CR1, SPI_CR1_SPE);
    }

private:
    SPI_HandleTypeDef *mHspi;
};

}  /* namespace CTLIB */

#endif

