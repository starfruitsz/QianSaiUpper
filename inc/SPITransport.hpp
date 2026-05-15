#ifndef SPITRANSPORT_HPP
#define SPITRANSPORT_HPP

#include "ICommunication.hpp"
#include <cstdint>
#include <type_traits>

// ============================================================
// HAL 前向声明（用户需在主程序中 #include "stm32f4xx_hal.h"）
// 此处的结构体桩确保本头文件可独立编译
// ============================================================

struct GPIO_InitTypeDef
{
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
    uint32_t Alternate;
};
struct SPI_InitTypeDef
{
    uint32_t Mode;
    uint32_t Direction;
    uint32_t DataSize;
    uint32_t CLKPolarity;
    uint32_t CLKPhase;
    uint32_t NSS;
    uint32_t BaudRatePrescaler;
    uint32_t FirstBit;
    uint32_t TIMode;
    uint32_t CRCCalculation;
    uint32_t CRCPolynomial;
};
struct SPI_HandleTypeDef
{
    SPI_TypeDef     *Instance;
    SPI_InitTypeDef  Init;
};
struct SPI_TypeDef
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
};
struct GPIO_TypeDef
{
    volatile uint32_t BSRR;
};

extern "C"
{
    void HAL_GPIO_Init(GPIO_TypeDef*, GPIO_InitTypeDef*);
    void HAL_GPIO_WritePin(GPIO_TypeDef*, uint16_t, uint32_t);
    void HAL_SPI_Init(SPI_HandleTypeDef*);
    void __HAL_RCC_SPI3_CLK_ENABLE();
    void __HAL_SPI_ENABLE(SPI_HandleTypeDef*);
    void SPI_1LINE_TX(SPI_HandleTypeDef*);
    void HAL_Delay(uint32_t);
}

namespace CTLIB
{

// ============================================================
// C++17 constexpr HAL 常量（零宏污染，命名空间作用域）
// ============================================================

namespace PinLevel
{
    static constexpr uint32_t Set   = 1u;
    static constexpr uint32_t Reset = 0u;
}
namespace GpioMode
{
    static constexpr uint32_t AfPp      = 2u;
    static constexpr uint32_t OutputPp  = 1u;
}
namespace GpioPull
{
    static constexpr uint32_t NoPull = 0u;
}
namespace GpioSpeed
{
    static constexpr uint32_t VeryHigh = 3u;
    static constexpr uint32_t High     = 2u;
    static constexpr uint32_t Low      = 1u;
}
namespace SpiMode
{
    static constexpr uint32_t Master = 1u;
}
namespace SpiDir
{
    static constexpr uint32_t OneLine = 0u;
}
namespace SpiDataSz
{
    static constexpr uint32_t Data8Bit = 0u;
}
namespace SpiClkPol
{
    static constexpr uint32_t Low = 0u;
}
namespace SpiClkPha
{
    static constexpr uint32_t Edge1 = 0u;
}
namespace SpiNss
{
    static constexpr uint32_t Soft = 1u;
}
namespace SpiBaud
{
    static constexpr uint32_t Div2 = 0u;
}
namespace SpiBitOrder
{
    static constexpr uint32_t Msb = 0u;
}
namespace SpiTi
{
    static constexpr uint32_t Disable = 0u;
}
namespace SpiCrc
{
    static constexpr uint32_t Disable = 0u;
}
namespace SpiAf
{
    static constexpr uint8_t  Af6Spi3 = 6u;
}

// ============================================================
// 编译期 GPIO 引脚描述（类型安全，无运行时查表）
// 用法: using PinCS = GpioPin<(GPIO_TypeDef*)0x40011400, 1 << 11>;
// ============================================================

template <auto PortAddr, auto PinMask>
struct GpioPin
{
    static constexpr auto port = static_cast<GPIO_TypeDef*>(PortAddr);
    static constexpr auto mask = static_cast<uint16_t>(PinMask);

    // 写高电平（直接操作 BSRR 寄存器）
    static void writeHigh()
    {
        port->BSRR = mask;
    }

    // 写低电平（BSRR 高 16 位对应复位）
    static void writeLow()
    {
        port->BSRR = static_cast<uint32_t>(mask) << 16u;
    }

    // 初始化为推挽输出
    static void initOutput(uint32_t speed = GpioSpeed::High)
    {
        GPIO_InitTypeDef g = {};
        g.Pin   = mask;
        g.Mode  = GpioMode::OutputPp;
        g.Pull  = GpioPull::NoPull;
        g.Speed = speed;
        HAL_GPIO_Init(port, &g);
    }

    // 初始化为复用功能（SPI）
    static void initAf(uint8_t af)
    {
        GPIO_InitTypeDef g = {};
        g.Pin       = mask;
        g.Mode      = GpioMode::AfPp;
        g.Pull      = GpioPull::NoPull;
        g.Speed     = GpioSpeed::VeryHigh;
        g.Alternate = af;
        HAL_GPIO_Init(port, &g);
    }
};

// ============================================================
// SPITransport：CRTP 实现 ICommunication 接口
// 所有引脚和 SPI 外设参数均为编译期模板参数（零运行时开销）
// 模板参数：
//   CSPin / DCPin / SckPin / SdaPin / BlPin — 各引脚 GpioPin 类型
//   SpiInstanceType — SPI 外设描述符
//   SpiAfNum        — SPI 复用功能编号（默认 AF6）
// ============================================================

template <
    typename CsPin,
    typename DcPin,
    typename SckPin,
    typename SdaPin,
    typename BlPin,
    typename SpiInstanceType,
    uint8_t SpiAfNum = SpiAf::Af6Spi3
>
class SPITransport final
    : public ICommunication<
        SPITransport<CsPin, DcPin, SckPin, SdaPin, BlPin,
                     SpiInstanceType, SpiAfNum>>
{
    using Self = SPITransport;

public:
    // 构造：绑定 HAL SPI 句柄
    explicit SPITransport(SPI_HandleTypeDef *hspi) noexcept
        : mHspi(hspi)
    {
    }

    // ============================================================
    // CRTP 接口实现
    // ============================================================

    // 初始化 SPI 外设和引脚
    void implInit()
    {
        mspInit();          // 初始化 GPIO 引脚和时钟
        peripheralInit();    // 配置 SPI 外设参数
    }

    // 写入 LCD 命令（DC=0 → 发命令 → DC=1）
    void implWriteCommand(uint8_t cmd)
    {
        spiWaitBusy();      // 等待 SPI 总线空闲
        DcPin::writeLow();  // DC=0 命令模式
        mHspi->Instance->DR = cmd;
        spiWaitTx();        // 等待发送完成
        spiWaitBusy();
        DcPin::writeHigh(); // DC=1 恢复数据模式
    }

    // 写入 8 位数据
    void implWriteData8(uint8_t data)
    {
        mHspi->Instance->DR = data;
        spiWaitTx();
    }

    // 写入 16 位数据（分两次 8 位发送）
    void implWriteData16(uint16_t data)
    {
        mHspi->Instance->DR = static_cast<uint8_t>(data >> 8u);
        spiWaitTx();
        mHspi->Instance->DR = static_cast<uint8_t>(data);
        spiWaitTx();
    }

    // 批量写入像素数据（切换 16 位模式，CS 片选控制）
    void implWriteBulk(uint16_t *buf, uint16_t sz)
    {
        set16BitMode();
        CsPin::writeLow();
        for (uint16_t i = 0; i < sz; ++i)
        {
            mHspi->Instance->DR = buf[i];
            spiWaitTx();
        }
        spiWaitBusy();
        CsPin::writeHigh();
        set8BitMode();
    }

    // 片选控制
    void implCsLow()
    {
        CsPin::writeLow();
    }
    void implCsHigh()
    {
        CsPin::writeHigh();
    }

    // 数据/命令选择控制
    void implDcCommand()
    {
        DcPin::writeLow();
    }
    void implDcData()
    {
        DcPin::writeHigh();
    }

    // 背光控制
    void implBacklightOn()
    {
        BlPin::writeHigh();
    }
    void implBacklightOff()
    {
        BlPin::writeLow();
    }

    // 毫秒延时
    void implDelayMs(uint32_t ms)
    {
        HAL_Delay(ms);
    }

private:
    SPI_HandleTypeDef *mHspi;

    // ---------- SPI 寄存器内联辅助 ----------

    // 等待发送缓冲区空
    inline void spiWaitTx()
    {
        while (!(mHspi->Instance->SR & 0x0002u))
        {
        }
    }

    // 等待 SPI 总线空闲
    inline void spiWaitBusy()
    {
        while (mHspi->Instance->SR & 0x0080u)
        {
        }
    }

    // 切换为 16 位数据帧（批量写入像素用）
    inline void set16BitMode()
    {
        auto &cr1 = mHspi->Instance->CR1;
        cr1 &= ~(1u << 6u);   // 关 SPI
        cr1 |=  (1u << 11u);  // 16 位数据格式
        cr1 |=  (1u << 6u);   // 开 SPI
    }

    // 切换回 8 位数据帧（默认模式）
    inline void set8BitMode()
    {
        auto &cr1 = mHspi->Instance->CR1;
        cr1 &= ~(1u << 6u);   // 关 SPI
        cr1 &= ~(1u << 11u);  // 8 位数据格式
        cr1 |=  (1u << 6u);   // 开 SPI
    }

    // ---------- MSP 初始化：GPIO 时钟 + 引脚配置 ----------
    void mspInit()
    {
        __HAL_RCC_SPI3_CLK_ENABLE();  // 使能 SPI3 时钟
        SckPin::initAf(SpiAfNum);     // SCK  复用推挽
        SdaPin::initAf(SpiAfNum);     // MOSI 复用推挽
        CsPin::initOutput();          // CS   推挽输出
        DcPin::initOutput();          // DC   推挽输出
        BlPin::initOutput(GpioSpeed::Low); // 背光  推挽输出（低速）
        implDcData();                 // 默认 DC=1（数据模式）
        implCsHigh();                 // 默认 CS=1（未选中）
        implBacklightOff();           // 默认关闭背光
    }

    // ---------- SPI 外设配置 ----------
    void peripheralInit()
    {
        auto &init = mHspi->Init;
        mHspi->Instance        = SpiInstanceType::instance;
        init.Mode              = SpiMode::Master;
        init.Direction         = SpiDir::OneLine;
        init.DataSize          = SpiDataSz::Data8Bit;
        init.CLKPolarity       = SpiClkPol::Low;
        init.CLKPhase          = SpiClkPha::Edge1;
        init.Nss               = SpiNss::Soft;
        init.BaudRatePrescaler = SpiBaud::Div2;      // 42MHz/2 = 21MHz
        init.FirstBit          = SpiBitOrder::Msb;
        init.TiMode            = SpiTi::Disable;
        init.CrcCalculation    = SpiCrc::Disable;
        init.CrcPolynomial     = 0;
        HAL_SPI_Init(mHspi);
        __HAL_SPI_ENABLE(mHspi);
        SPI_1LINE_TX(mHspi);
    }
};

// ============================================================
// SPI 外设实例描述符（编译期常量）
// ============================================================

template <uintptr_t BaseAddr>
struct SpiInstanceDescr
{
    static constexpr auto instance = reinterpret_cast<SPI_TypeDef*>(BaseAddr);
};

// SPI3 基地址: 0x40003C00（STM32F4 系列）
using Spi3Instance = SpiInstanceDescr<0x40003C00u>;

} // namespace CTLIB

#endif