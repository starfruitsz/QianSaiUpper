#include "SPITransport.hpp"
#include "stm32f4xx_hal.h"

namespace CTLIB {

SPITransport::SPITransport(const PinConfig &pins, const SPIConfig &spi)
    : m_pins(pins), m_spi(spi)
{
}

SPITransport::~SPITransport()
{
}

void SPITransport::spiMspInit()
{
    GPIO_InitTypeDef gpio = {};

    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    gpio.Pin       = m_pins.sckPin;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(static_cast<GPIO_TypeDef*>(m_pins.sckPort), &gpio);

    gpio.Pin = m_pins.sdaPin;
    HAL_GPIO_Init(static_cast<GPIO_TypeDef*>(m_pins.sdaPort), &gpio);

    gpio.Pin   = m_pins.csPin;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = 0;
    HAL_GPIO_Init(static_cast<GPIO_TypeDef*>(m_pins.csPort), &gpio);

    gpio.Pin   = m_pins.blPin;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(static_cast<GPIO_TypeDef*>(m_pins.blPort), &gpio);

    gpio.Pin   = m_pins.dcPin;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(static_cast<GPIO_TypeDef*>(m_pins.dcPort), &gpio);

    dcData();
    csHigh();
    backlightOff();
}

void SPITransport::spiPeriphInit()
{
    auto *hspi = static_cast<SPI_HandleTypeDef*>(m_spi.instance);

    hspi->Instance               = SPI3;
    hspi->Init.Mode              = SPI_MODE_MASTER;
    hspi->Init.Direction         = SPI_DIRECTION_1LINE;
    hspi->Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi->Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi->Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi->Init.NSS               = SPI_NSS_SOFT;
    hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi->Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi->Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial     = 0;

    HAL_SPI_Init(hspi);
    __HAL_SPI_ENABLE(hspi);
    SPI_1LINE_TX(hspi);
}

void SPITransport::init()
{
    spiMspInit();
    spiPeriphInit();
}

void SPITransport::writeCommand(uint8_t cmd)
{
    auto *hspi = static_cast<SPI_HandleTypeDef*>(m_spi.instance);
    while ((hspi->Instance->SR & 0x0080) != RESET);
    dcCommand();
    hspi->Instance->DR = cmd;
    while ((hspi->Instance->SR & 0x0002) == 0);
    while ((hspi->Instance->SR & 0x0080) != RESET);
    dcData();
}

void SPITransport::writeData8(uint8_t data)
{
    auto *hspi = static_cast<SPI_HandleTypeDef*>(m_spi.instance);
    hspi->Instance->DR = data;
    while ((hspi->Instance->SR & 0x0002) == 0);
}

void SPITransport::writeData16(uint16_t data)
{
    auto *hspi = static_cast<SPI_HandleTypeDef*>(m_spi.instance);
    hspi->Instance->DR = static_cast<uint8_t>(data >> 8);
    while ((hspi->Instance->SR & 0x0002) == 0);
    hspi->Instance->DR = static_cast<uint8_t>(data);
    while ((hspi->Instance->SR & 0x0002) == 0);
}

void SPITransport::writeBulk(uint16_t *buffer, uint16_t size)
{
    auto *hspi = static_cast<SPI_HandleTypeDef*>(m_spi.instance);

    hspi->Instance->CR1 &= ~(1u << 6);
    hspi->Instance->CR1 |=  (1u << 11);
    hspi->Instance->CR1 |=  (1u << 6);

    csLow();
    for (uint16_t i = 0; i < size; i++) {
        hspi->Instance->DR = buffer[i];
        while ((hspi->Instance->SR & 0x0002) == 0);
    }
    while ((hspi->Instance->SR & 0x0080) != RESET);
    csHigh();

    hspi->Instance->CR1 &= ~(1u << 6);
    hspi->Instance->CR1 &= ~(1u << 11);
    hspi->Instance->CR1 |=  (1u << 6);
}

void SPITransport::csLow()
{
    HAL_GPIO_WritePin(static_cast<GPIO_TypeDef*>(m_pins.csPort),
                      m_pins.csPin, GPIO_PIN_RESET);
}

void SPITransport::csHigh()
{
    HAL_GPIO_WritePin(static_cast<GPIO_TypeDef*>(m_pins.csPort),
                      m_pins.csPin, GPIO_PIN_SET);
}

void SPITransport::dcCommand()
{
    HAL_GPIO_WritePin(static_cast<GPIO_TypeDef*>(m_pins.dcPort),
                      m_pins.dcPin, GPIO_PIN_RESET);
}

void SPITransport::dcData()
{
    HAL_GPIO_WritePin(static_cast<GPIO_TypeDef*>(m_pins.dcPort),
                      m_pins.dcPin, GPIO_PIN_SET);
}

void SPITransport::backlightOn()
{
    HAL_GPIO_WritePin(static_cast<GPIO_TypeDef*>(m_pins.blPort),
                      m_pins.blPin, GPIO_PIN_SET);
}

void SPITransport::backlightOff()
{
    HAL_GPIO_WritePin(static_cast<GPIO_TypeDef*>(m_pins.blPort),
                      m_pins.blPin, GPIO_PIN_RESET);
}

void SPITransport::delayMs(uint32_t ms)
{
    HAL_Delay(ms);
}

} // namespace CTLIB
