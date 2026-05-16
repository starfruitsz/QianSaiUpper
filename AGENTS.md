# CTLIB — STM32 LCD 抽象层

## 项目概述

基于 C++23 的 STM32 LCD 驱动抽象层，目标芯片 STM32F407ZG，LCD 控制器 ST7789（1.54 寸 240x240）。
CRTP + header-only 架构，使用 CubeMX 生成的 HAL 工程。

## 文件结构

```
CTLIB/
├── inc/
│   ├── ICommunication.hpp   # CRTP 传输基类（BufferPolicy + Buff/Flush/Print）
│   ├── SPITransport.hpp     # SPI 传输层（WriteCommand/Cs/Dc 自管理，ImplFlush SPI burst）
│   ├── UartTransport.hpp    # UART 传输层（ImplFlush 逐字节 TX）
│   ├── Fonts.hpp            # 字体结构 + 内嵌默认 12x24 ASCII 字库 + DefaultFont
│   ├── ILCD.hpp             # LCD 抽象基类（全部绘制算法 + 背光/方向/地址 CRTP 接口）
│   └── LCD_ST7789.hpp       # ST7789 驱动（寄存器序列 + ImplDrawPoint + 背光实现）
└── AGENTS.md
```

## 架构

```
ICommunication<Impl, BufSz>   ← CRTP 传输基类
  │  BufferPolicy mBuf: data[] + mPos + operator[]
  │  公开: Init / WriteData8/16 / Buff / BuffPos / Flush(sz) / Flush() / Print / DelayMs
  │  CRTP: ImplInit / ImplWriteData8/16 / ImplFlush / ImplDelayMs
  │
  ├── SPITransport (BufSz=1024)
  │     公开: WriteCommand / CsLow/High / DcCommand/Data（非 CRTP）
  │     ImplFlush: 16bit SPI burst → CS=0 → mBuf.Ptr()[0..sz) → CS=1
  │
  └── UartTransport (BufSz=256)
        ImplFlush: UART TX mBuf.Ptr() 逐字节

ILCD<Driver, Transport>
  │  注入 Transport &mComm
  │  公开: Init / SetColor / DrawString / FillRect / DrawImage / BacklightOn/Off …
  │  CRTP → Driver: ImplInit / ImplDrawPoint / ImplSetDirection / ImplSetAddr
  │  内部 Buff → mComm.Buff / Flush → mComm.Flush
  │
  └── LCD_ST7789<Transport>
        ImplInit: SPI + 寄存器序列 + 方向 + 清屏 + 背光
        ImplSetDirection: 0x36 命令
        ImplSetAddr: CsLow + 0x2A/0x2B/0x2C 地址窗口
        ImplBacklightOn/Off: PD13 BSRR
```

## BufferPolicy 访问契约

| 接口 | 说明 |
|---|---|
| `operator[](idx)` | 只读访问（const） |
| `Push(val)` | 写入 `mData[mPos++]`，仅填充无溢出检查 |
| `Ptr()` | 缓冲区首地址 `&mData[0]`（vsnprintf / Flush 读取） |
| `Data()` | 当前游标地址 `&mData[mPos]`（追加写入） |
| `Reset()` | 游标归零，`Flush()` 自动调用 |

## ICommunication 缓冲接口

| 接口 | 等价 | 说明 |
|---|---|---|
| `Buff(val)` | `mBuf.Push(val)` | 写入一个像素，**不自动 Flush** |
| `BuffPos()` | `mBuf.mPos` | 当前已缓冲元素数 |
| `Flush(sz)` | `ImplFlush(sz) + Reset()` | 发送后游标归零 |
| `Flush()` | `Flush(mBuf.mPos)` | 发送全部已缓冲元素 |
| `Print(fmt, ...)` | `format_to_n → Flush(n)` | C++23 std::print 风格 |

### Flush 调用规则

- **fillRectImpl**：`SetAddr` 一次 → 循环 `Buff` + `if (BuffPos() >= kBufSize) Flush()` → 末尾 `Flush()`
- **DrawImage / drawGlyphFromFont**：逐行 `Buff` → `if (BuffPos() >= limit \|\| lastRow) { SetAddr + Flush }`
- **Print**：`format_to_n` 写入 `mBuf` → `Flush(n)` 一次发送

> 重要：`Flush` 前必须已调用 `SetAddr` 设置正确的地址窗口，否则数据写到错误位置。

## 关键实现细节

### SPI 传输
- CS=PD11, DC=PD12, SCK=PB3, MOSI=PB5, BL=PD13
- **SPI_DIRECTION_1LINE** + **SPI_1LINE_TX**（只发不收）
- `ImplFlush`：切换 16 位模式 → CS=0 → burst → CS=1 → 恢复 8 位

### 字体渲染
- 位序 **LSB-first**（bit0=最左像素），匹配 PCtoLCD C51 格式
- 默认 12x24 字体自动加载（`DefaultFont::value`）
- `drawGlyphFromFont`：`base = charIdx * sizes`

### 命名风格
- 公开方法：大驼峰（`Init / DrawString / BacklightOn`）
- 成员变量：`m` 前缀（`mComm / mColor / mWidth`）
- CRTP 实现：`Impl` 前缀（`ImplInit / ImplFlush`）
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

using Transport = CTLIB::SPITransport;
using LcdType   = CTLIB::LCD_ST7789<Transport>;

extern SPI_HandleTypeDef hspi3;
extern UART_HandleTypeDef huart1;
Transport            gTransport(&hspi3);
LcdType              gLcd(gTransport);
CTLIB::UartTransport gUart(&huart1);

void main() {
  gLcd.Init();                        // 自动加载默认字体
  gUart.Init();

  gUart.Print("Boot OK\r\n");
  gUart.Print("LCD: {}x{}\r\n", 240, 240);

  gLcd.SetBackColor(CTLIB::Colors::Blue);
  gLcd.SetColor(CTLIB::Colors::Red);
  gLcd.Clear();
  gLcd.DrawString(20, 20, "Hello STM32!");
  gLcd.FillRect(10, 80, 100, 50);
}
```

## 外部依赖

- `Core/Src/lcd_image.c` + `Core/Inc/lcd_image.h`（图片取模数据）
- `Core/Src/stm32f4xx_hal_msp.c`（CubeMX 生成，含 HAL_SPI_MspInit）
- 官方例程 `ASCII_2412_Table` 字库（已内嵌至 Fonts.hpp）