# CTLIB — STM32 LCD 抽象层

## 项目概述

基于 C++20 的 STM32 LCD 驱动抽象层，目标芯片 STM32F407ZG，LCD 控制器 ST7789（1.54 寸 240x240）。
CRTP + header-only 架构，使用 CubeMX 生成的 HAL 工程。

## 文件结构

```
CTLIB/
├── inc/
│   ├── ICommunication.hpp   # CRTP 通信接口（SPI 协议方法 + CS/DC 控制）
│   ├── SPITransport.hpp     # SPI 传输层（非模板，BSRR 直接寄存器）
│   ├── Fonts.hpp            # 字体/字模数据结构
│   ├── ILCD.hpp             # LCD 抽象基类（绘制算法 + 背光接口）
│   └── LCD_ST7789.hpp       # ST7789 驱动子类（init + drawPoint + 背光实现）
└── AGENTS.md
```

## 架构

```
ICommunication<Impl>  ← CRTP 通信接口（SPI 传输 + CS/DC/延时）
        ↑
SPITransport          ← SPI 硬件层（非模板，硬编码 SCK/MOSI/CS/DC 引脚）
        ↑  (注入到 ILCD)
ILCD<Driver, Transport, BufferSz>  ← LCD 基类（绘制算法 + backlightOn/Off 接口）
        ↑
LCD_ST7789<Transport, BufferSz>    ← 驱动子类（寄存器初始化 + 背光实现）
```

**背光控制职责划分：**
- `ILCD` 基类声明 `backlightOn()` / `backlightOff()` 公开接口，通过 CRTP 委托到 `implBacklightOn/Off`
- `LCD_ST7789` 子类在 private 区实现 `implBacklightOn/Off`，直接操作 `GPIOD->BSRR`（PD13）
- 不同 LCD 驱动（如 ILI9341）可在自己的子类中实现不同的背光控制逻辑

## 关键实现细节（容易踩坑！）

### SPI 传输
- **DC/CS 用 `GPIOD->BSRR` 直接寄存器操作**，不能用 HAL_GPIO_WritePin
- **背光 BL 已从 SPITransport 移除**，由 LCD_ST7789 子类实现（`implBacklightOn/Off`）
- **SPI_DIRECTION_1LINE**（单线双向）+ **SPI_1LINE_TX**（只发不收）
- CubeMX 默认生成 `SPI_DIRECTION_2LINES`，需要覆盖
- 21MHz（APB1 42MHz / 2）

### 引脚
- SCK=PB3, MOSI=PB5（SPI3，硬编码在 SPITransport.hpp）
- CS=PD11, DC=PD12（硬编码在 SPITransport.hpp 的 GPIO 初始化和回调中）
- BL=PD13（由 LCD_ST7789 子类控制，不在 SPITransport 中）

### CS 片选控制
- `setAddr()`：CS=0 → 发命令(0x2A/0x2B/0x2C) → 保持 CS=0
- `writeBulk()`：内部自管理 CS（CS=0 → 发数据 → CS=1）
- `drawPoint()`：`setAddr` + `writeData16` + 手动 `csHigh()`

### 初始化顺序（main.cpp）
```
MX_GPIO_Init() → MX_SPI3_Init() → gLcd.init()
                                      ↓
                         SPITransport::implInit()
                           → 开时钟 + 配 GPIO（覆盖）
                           → SPI_DIRECTION_1LINE + 使能
                         LCD_ST7789::implInit()
                           → CS=0 → 寄存器序列 → CS=1
                           → setDirection → clear → backlightOn()
```

### 注意事项
1. `SPITransport` 目前是非模板普通类，硬编码引脚。模板化尝试过多次，在 -Og 下会导致屏幕不亮
2. 不能使用 `reinterpret_cast` + `constexpr`（C++ 禁止）
3. 不能使用模板参数传指针（HAL 宏展开为表达式，非编译期常量）
4. 如果屏幕不亮，先检查：LED 是否闪烁（确认 MCU 在跑）→ GPIO 是否有波形 → SPI 模式是否为 1LINE

## 编译标准

- **编译器**: `arm-none-eabi-g++`
- **标准**: `-std=c++20`
- **优化**: `-Og`
- **目标**: `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb`

## 使用示例

```cpp
#include "LCD_ST7789.hpp"

// PTD 区
using Transport = CTLIB::SPITransport;
using LcdType   = CTLIB::LCD_ST7789<Transport>;

// PV 区
extern SPI_HandleTypeDef hspi3;
Transport gTransport(&hspi3);
LcdType   gLcd(gTransport);

// main() 中
gLcd.init();

// 绘制
gLcd.setColor(0xFF0000);
gLcd.fillRect(10, 10, 100, 50);
gLcd.drawString(20, 80, "Hello");

// 背光控制（直接调用 LCD 基类接口）
gLcd.backlightOn();
gLcd.backlightOff();
```

## 外部依赖

- `Core/Src/lcd_image.c` + `Core/Inc/lcd_image.h`（图片取模数据，从官方例程复制）
- `Core/Src/stm32f4xx_hal_msp.c`（CubeMX 生成，含 HAL_SPI_MspInit）