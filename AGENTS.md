# CTLIB — STM32 LCD 抽象层

## 项目概述

基于 C++20 的 STM32 LCD 驱动抽象层，目标芯片 STM32F407ZG，LCD 控制器 ST7789。
采用 CRTP 模板 + constexpr + header-only 架构，编译期零虚函数开销。

## 文件结构

```
CTLIB/
├── inc/
│   ├── ICommunication.hpp   # 通信传输层 CRTP 接口（纯模板，无虚函数）
│   ├── SPITransport.hpp     # SPI 传输层实现（编译期引脚配置）
│   ├── Fonts.hpp            # 字体/字模数据结构
│   ├── ILCD.hpp             # LCD 抽象基类模板（含全部绘制算法）
│   └── LCD_ST7789.hpp       # ST7789 驱动子类（仅 init + drawPoint）
├── src/                     # 暂无源文件（全 header-only）
└── AGENTS.md                # 本文件
```

## 架构层次

```
ICommunication<Impl>  ← CRTP 通信接口（零开销静态分发）
        ↑
SPITransport<CsPin, DcPin, SckPin, SdaPin, BlPin, SpiInst>  ← SPI 硬件层
        ↑  (注入)
ILCD<Driver, Transport, BufferSz>  ← LCD 基类（颜色/字体/2D/填充/图片）
        ↑
LCD_ST7789<Transport, BufferSz>    ← 驱动子类（寄存器序列 + drawPoint）
```

## 命名规范

- **驼峰命名**：`setColor()` `mWidth` `drawLineH()`
- **花括号另起一行**：函数/类/命名空间 `{` 独占一行
- **中文注释**：每个方法/逻辑块有 `====` 分隔的中文说明
- **命名空间**：所有代码在 `CTLIB::` 下

## C++17 / C++20 特性

| 特性 | 说明 |
|------|------|
| CRTP | `ICommunication<Impl>` 编译期静态分发 |
| `constexpr` | `rgb888To565()` 编译期颜色转换 |
| `static constexpr` 命名空间常量 | 替代 `#define`，无宏污染 |
| 结构化绑定 | `for (const auto &[cmd, data] : kRegs)` |
| `template<auto>` | `GpioPin<PortAddr, PinMask>` 编译期引脚 |
| `if constexpr` | 留待后续优化使用 |
| `consteval` | C++20 立即函数 |
| `std::span` | C++20 视图，替代裸指针+长度 |
| `concepts` | C++20 模板约束，替代 `static_assert` |
| 三路比较 `<=>` | C++20 默认运算符生成 |
| `constexpr` 容器 | C++20 `std::vector`/`std::string` 可用于编译期 |

## 使用示例

```cpp
#include "LCD_ST7789.hpp"

// 1. 编译期引脚定义
using PinCS  = CTLIB::GpioPin<(GPIO_TypeDef*)0x40011400, 1 << 11>;
using PinDC  = CTLIB::GpioPin<(GPIO_TypeDef*)0x40011400, 1 << 12>;
using PinSCK = CTLIB::GpioPin<(GPIO_TypeDef*)0x40020400, 1 << 3>;
using PinSDA = CTLIB::GpioPin<(GPIO_TypeDef*)0x40020400, 1 << 5>;
using PinBL  = CTLIB::GpioPin<(GPIO_TypeDef*)0x40011400, 1 << 13>;

// 2. 传输层
using Transport = CTLIB::SPITransport<PinCS, PinDC, PinSCK, PinSDA, PinBL, CTLIB::Spi3Instance>;

SPI_HandleTypeDef hspi;
Transport transport(&hspi);

// 3. LCD 驱动层
CTLIB::LCD_ST7789<Transport> lcd(transport);
lcd.init();
lcd.drawString(10, 10, "Hello!");
```

## 编译标准

- **编译器**: `arm-none-eabi-g++`
- **标准**: `-std=c++20`
- **目标**: `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb`

## 注意事项

1. 使用前必须在主文件中 `#include "stm32f4xx_hal.h"`（提供 HAL 函数实现）
2. 字体数据（`Font` 结构体）需用户自行提供字模 `.c` 文件，与原 `lcd_fonts.h` 的 `pFONT` 兼容
3. 缓冲区大小可通过模板参数 `BufferSz` 调整（默认 1024）
4. 添加新 LCD 驱动只需继承 `ILCD`，实现 `implInit()` 和 `implDrawPoint()`
