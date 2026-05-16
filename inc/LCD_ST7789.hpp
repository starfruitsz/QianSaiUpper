#ifndef LCD_ST7789_HPP
#define LCD_ST7789_HPP

#include "ILCD.hpp"
#include <initializer_list>

namespace CTLIB
{
/* ============================================================ */
/* LCD_ST7789 驱动层（CRTP 子类） */
/* 模板参数： */
/* Transport — SPI 传输层（CRTP ICommunication 子类） */
/* ============================================================ */

template <typename Transport>
class LCD_ST7789 final
    : public ILCD<LCD_ST7789<Transport>, Transport>
{
    using Base = ILCD<LCD_ST7789<Transport>, Transport>;  /* C++11: type alias */
    friend Base;

public:
    /* 构造：绑定传输层，指定屏幕宽高（默认 240×240） */
    explicit LCD_ST7789(Transport &comm, uint16_t w = 240, uint16_t h = 240)
        : Base(comm, w, h)
    {
    }

private:
    void ImplBacklightOn()  { GPIOD->BSRR = GPIO_PIN_13; }
    void ImplBacklightOff() { GPIOD->BSRR = (uint32_t)GPIO_PIN_13 << 16u; }
    void ImplSetDirection(Direction dir)
    {
        this->mComm.WriteCommand(0x36);
        if (dir == Direction::Vertical)        { this->mComm.WriteData8(0x00); }
        else if (dir == Direction::Horizontal)     { this->mComm.WriteData8(0x60); }
        else if (dir == Direction::HorizontalFlip) { this->mComm.WriteData8(0xA0); }
        else                                       { this->mComm.WriteData8(0xC0); }
    }

    void ImplSetAddr(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
    {
        this->mComm.CsLow();
        /* 列地址 */
        this->mComm.WriteCommand(0x2A);
        this->mComm.WriteData16(xs);
        this->mComm.WriteData16(xe);

        /* 行地址 */
        this->mComm.WriteCommand(0x2B);
        this->mComm.WriteData16(ys);
        this->mComm.WriteData16(ye);

        /* 写显存（CS=0 后保持，由调用方负责释放） */
        this->mComm.WriteCommand(0x2C);
    }
    /* ============================================================ */
    /* CRTP 入口 — 屏幕初始化 */
    /* 1. 初始化 SPI 外设 */
    /* 2. 发送 ST7789 寄存器配置序列 */
    /* 3. 设置默认方向、颜色、清屏、开背光 */
    /* ============================================================ */
    void ImplInit()
    {
        this->mComm.Init();
        this->mComm.DelayMs(10);  /* 等待 LCD 上电复位 */
        this->mComm.CsLow();  /* 使能片选 */
        this->writeRegisters();  /* 写入 ST7789 寄存器序列 */
        this->mComm.CsHigh();
        this->SetDirection(Direction::Vertical);
        this->SetBackColor(Colors::Black);
        this->SetColor(Colors::White);
        this->Clear();  /* 全屏清屏 */
        this->BacklightOn();  /* 开启背光 */
    }

    /* ============================================================ */
    /* CRTP 入口 — 绘制单个像素点 */
    /* ============================================================ */
    void ImplDrawPoint(uint16_t x, uint16_t y, uint32_t c888)
    {
        if (x >= this->mWidth || y >= this->mHeight)
        {
            return;
        }
        this->SetAddr(x, y, x, y);  /* CS=0，设地址窗口 */
        this->mComm.WriteData16(Base::c888To565(c888));  /* 写颜色 */
        this->mComm.CsHigh();  /* CS=1，释放片选 */
    }

    /* ============================================================ */
    /* ST7789 初始化寄存器配置表（constexpr 编译期常量） */
    /* ============================================================ */
    struct RegCfg
    {
        uint8_t cmd;  /* 寄存器命令 */
        std::initializer_list<uint8_t> data;  /* 命令参数列表 */
    };

    static constexpr RegCfg kRegs[] = {
        /* 显存访问控制：从上到下、从左到右、RGB 像素格式 */
        {0x36, {0x00}},
        /* 接口像素格式：16 位 RGB565 */
        {0x3A, {0x05}},
        /* 电源控制 1 */
        {0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}},
        /* VGH/VGL 栅极电压 */
        {0xB7, {0x35}},
        /* VCOM 电压 */
        {0xBB, {0x19}},
        /* LCM 控制 */
        {0xC0, {0x2C}},
        /* VDV/VRH 使能 */
        {0xC2, {0x01}},
        /* VRH 电压 */
        {0xC3, {0x12}},
        /* VDV 电压 */
        {0xC4, {0x20}},
        /* 帧率控制：默认 60Hz */
        {0xC6, {0x0F}},
        /* 电源控制 2 */
        {0xD0, {0xA4, 0xA1}},
        /* 正 Gamma 校正 */
        {0xE0, {0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23}},
        /* 负 Gamma 校正 */
        {0xE1, {0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23}},
        /* 开启反显（IPS 屏通常需要） */
        {0x21, {}},
        /* 退出休眠模式（之后需延时 120ms） */
        {0x11, {}},
        /* 开启显示 */
        {0x29, {}},
    };

    /* ============================================================ */
    /* 遍历寄存器表写入初始化序列 */
    /* 使用 C++17 结构化绑定拆解 {cmd, data} */
    /* ============================================================ */
    void writeRegisters()
    {
        for (const auto &[cmd, data] : kRegs)
        {
            this->mComm.WriteCommand(cmd);
            for (auto d : data)
            {
                this->mComm.WriteData8(d);
            }
            /* 退出休眠指令需要等待电源稳定 */
            if (cmd == 0x11)
            {
                this->mComm.DelayMs(120);
            }
        }
    }
};

}  /* namespace CTLIB */

#endif

