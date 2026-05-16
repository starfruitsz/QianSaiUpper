# CTLIB — STM32 LCD 抽象层

## 项目概述

基于 C++23 的 STM32 LCD 驱动抽象层，目标芯片 STM32F407ZG，LCD 控制器 ST7789（1.54 寸 240x240）。
CRTP + header-only 架构，使用 CubeMX 生成的 HAL 工程。

## 文件结构

```
CTLIB/
├── inc/
│   ├── ICommunication.hpp   # CRTP 传输基类（Buffer + Print + Flush）
│   ├── SPITransport.hpp     # SPI 传输层（WriteCommand/Cs/Dc 自管理）
│   ├── UartTransport.hpp    # UART 传输层（printf 调试输出）
│   ├── Fonts.hpp            # 字体结构 + 默认 12x24 ASCII 字库
│   ├── ILCD.hpp             # LCD 抽象基类（全部绘制算法 + 背光接口）
│   └── LCD_ST7789.hpp       # ST7789 驱动子类（寄存器 + 背光实现）
└── AGENTS.md
```

## 架构

```
ICommunication<Impl, BufSz>  ← CRTP 传输基类（Buffer + Print + Flush）
        ↑                              ↑
SPITransport              UartTransport
  （CS/DC/WriteCommand 自管理）  （仅 WriteData8 + Print）
        ↑  (注入到 ILCD)
ILCD<Driver, Transport>  ← LCD 基类（绘制算法 + 背光接口 + 默认字体）
        ↑
LCD_ST7789<Transport>    ← 驱动子类（寄存器初始化 + 背光 + SetDirection/SetAddr）
```

## 类职责划分

### ICommunication（传输基类）
- `BufferPolicy<BufSz> mBuf` — 共享缓冲区，带 `data[]` 和 `pos` 游标
- `Init / WriteData8/16 / DelayMs` — 通用传输接口（CRTP）
- `Flush(sz)` — 将 `mBuf[0..sz)` 刷入硬件（子类实现：SPI burst / UART 空）
- `Print(fmt, args...)` — C++23 `std::print` 风格格式化输出，旧标准回退 `vsnprintf`

### SPITransport（SPI 子类）
- `WriteCommand / CsLow/CsHigh / DcCommand/DcData` — LCD 专用，**非 CRTP**，直接公开方法
- `ImplFlush` — 切换 16 位模式 → CS=0 → burst 发送 `mBuf` → CS=1 → 恢复 8 位

### UartTransport（UART 子类）
- `ImplWriteData8` — 单字节轮询 TX
- `ImplFlush` — 空（UART 已逐字节即时发送）
- 其余 LCD 方法为空 stub

### ILCD（LCD 基类）
- `BacklightOn/Off` — 背光控制 CRTP 接口 → 委托子类 `ImplBacklightOn/Off`
- `SetDirection / SetAddr` — 方向 + 地址窗口 CRTP 接口 → 委托子类 `ImplSetDirection / ImplSetAddr`
- `Init()` — 自动加载 `DefaultFont::value`（12x24 ASCII）
- 全部 2D 绘制算法（`DrawString / FillRect / DrawCircle / DrawImage` 等）

### LCD_ST7789（ST7789 驱动子类）
- `ImplInit` — SPI 初始化 + ST7789 寄存器序列 + 方向 + 清屏 + 开背光
- `ImplSetDirection` — 硬件 `0x36` 命令
- `ImplSetAddr` — `CsLow` + `0x2A/0x2B/0x2C` 地址窗口
- `ImplBacklightOn/Off` — PD13 BSRR
- `ImplDrawPoint` — 单像素点绘制

### Fonts.hpp（字库）
- `Font` 结构体 — 兼容官方 `pFONT`
- `makeFont<W,H,Bytes>` — `constexpr` 工厂函数
- `DefaultFont` — 编译期静态 12x24 ASCII 字体实例
- `Font12x24_Table[]` — 内嵌字模数据（LSB-first, PCtoLCD C51 格式）

## 关键实现细节

### SPI 传输
- CS=PD11, DC=PD12, SCK=PB3, MOSI=PB5, BL=PD13
- **DC/CS/BL 用 `GPIOD->BSRR` 直接寄存器操作**
- **SPI_DIRECTION_1LINE** + **SPI_1LINE_TX**（只发不收）
- `WriteBulk` 已移除，统一用 `Flush(sz)` → 子类读取 `mBuf.data`

### 字体渲染
- 位序 **LSB-first**（bit0=最左像素），匹配 PCtoLCD C51 取模格式
- 默认 12x24 字体自动加载，用户无需调用 `SetAsciiFont`
- `drawGlyphFromFont`：`base = charIdx * sizes`（每字符固定字节数）

### 命名风格
- 公开方法：大驼峰（`Init / DrawString / BacklightOn`）
- 成员变量：`m` 前缀（`mComm / mColor / mWidth`）
- CRTP 实现方法：`Impl` 前缀（`ImplInit / ImplFlush`）
- 回调成员：`Cb` 前缀（`CbInit / CbCsLow`）

## 编译标准

- **编译器**: `arm-none-eabi-g++`
- **标准**: `-std=c++23`
- **优化**: `-Og`
- **目标**: `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb`

## 使用示例

```cpp
#include "LCD_ST7789.hpp"
#include "UartTransport.hpp"

// PTD 区
using Transport = CTLIB::SPITransport;
using LcdType   = CTLIB::LCD_ST7789<Transport>;

// PV 区
extern SPI_HandleTypeDef hspi3;
extern UART_HandleTypeDef huart1;
Transport              gTransport(&hspi3);
LcdType                gLcd(gTransport);
CTLIB::UartTransport   gUart(&huart1);

// main() 中
gLcd.Init();                         // 自动加载默认 12x24 字体
gUart.Init();

// UART 调试输出（C++23 std::print 风格）
gUart.Print("System Boot OK\r\n");
gUart.Print("Clock: {} MHz, LCD: {}x{}\r\n", 168, 240, 240);

// LCD 绘制
gLcd.SetBackColor(CTLIB::Colors::Blue);
gLcd.SetColor(CTLIB::Colors::Red);
gLcd.Clear();
gLcd.DrawString(20, 20, "Hello STM32!");
gLcd.DrawNumber(20, 50, 12345, 6);
gLcd.FillRect(10, 80, 100, 50);

// 背光控制
gLcd.BacklightOn();
gLcd.BacklightOff();
```

## 外部依赖

- `Core/Src/lcd_image.c` + `Core/Inc/lcd_image.h`（图片取模数据）
- `Core/Src/stm32f4xx_hal_msp.c`（CubeMX 生成，含 HAL_SPI_MspInit）
- 官方例程 `ASCII_2412_Table` 字库（已内嵌至 Fonts.hpp）