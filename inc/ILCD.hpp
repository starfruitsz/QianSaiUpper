#ifndef ILCD_HPP
#define ILCD_HPP

#include "ICommunication.hpp"
#include "Fonts.hpp"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <type_traits>

namespace CTLIB
{

/* ============================================================ */
/* 显示方向枚举 */
/* ============================================================ */

enum class Direction : uint8_t
{
    Horizontal     = 0,  /* 横屏 */
    HorizontalFlip = 1,  /* 横屏，上下翻转 */
    Vertical       = 2,  /* 竖屏 */
    VerticalFlip   = 3  /* 竖屏，上下翻转 */
};

/* ============================================================ */
/* 数字显示填充模式 */
/* ============================================================ */

enum class NumFillMode : uint8_t
{
    FillZero  = 0,  /* 多余位填充 '0' */
    FillSpace = 1  /* 多余位填充空格 */
};

/* ============================================================ */
/* 编译期 RGB888 合成 */
/* ============================================================ */

constexpr uint32_t rgb888(uint8_t r, uint8_t g, uint8_t b) noexcept
{
    return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
}

/* ============================================================ */
/* 编译期 RGB888 → RGB565 转换 */
/* 若输入参数均为 constexpr，则在编译期完成计算 */
/* ============================================================ */

constexpr uint16_t rgb888To565(uint32_t c) noexcept
{
    return static_cast<uint16_t>(((c & 0xF80000u) >> 8) | ((c & 0x00FC00u) >> 5) | ((c & 0x0000F8u) >> 3));
}

/* ============================================================ */
/* 常用颜色常量（RGB888 格式，使用时自动转为 RGB565） */
/* ============================================================ */

namespace Colors
{
    constexpr uint32_t White       = 0xFFFFFF;
    constexpr uint32_t Black       = 0x000000;
    constexpr uint32_t Blue        = 0x0000FF;
    constexpr uint32_t Green       = 0x00FF00;
    constexpr uint32_t Red         = 0xFF0000;
    constexpr uint32_t Cyan        = 0x00FFFF;
    constexpr uint32_t Magenta     = 0xFF00FF;
    constexpr uint32_t Yellow      = 0xFFFF00;
    constexpr uint32_t Grey        = 0x2C2C2C;
    constexpr uint32_t LightBlue   = 0x8080FF;
    constexpr uint32_t LightGreen  = 0x80FF80;
    constexpr uint32_t LightRed    = 0xFF8080;
    constexpr uint32_t LightCyan   = 0x80FFFF;
    constexpr uint32_t LightMagenta = 0xFF80FF;
    constexpr uint32_t LightYellow = 0xFFFF80;
    constexpr uint32_t LightGrey   = 0xA3A3A3;
    constexpr uint32_t DarkBlue    = 0x000080;
    constexpr uint32_t DarkGreen   = 0x008000;
    constexpr uint32_t DarkRed     = 0x800000;
    constexpr uint32_t DarkCyan    = 0x008080;
    constexpr uint32_t DarkMagenta = 0x800080;
    constexpr uint32_t DarkYellow  = 0x808000;
    constexpr uint32_t DarkGrey    = 0x404040;
}

/* ============================================================ */
/* 缓冲区策略（模板参数，零成本定制缓冲区大小） */
/* ============================================================ */

/* CRTP 三个模板参数： */
/* Driver    — 具体驱动类（如 LCD_ST7789） */
/* Transport — 通信传输层（CRTP ICommunication 子类） */
/* ============================================================ */

template <typename Driver, typename Transport>
class ILCD
{
    static_assert(std::is_base_of_v<ICommunication<Transport, 1024>, Transport> || std::is_base_of_v<ICommunication<Transport, 256>, Transport>,
                  "Transport must derive from ICommunication<Transport>");

public:
    /* 构造：绑定通信层、设置屏幕尺寸 */
    explicit ILCD(Transport &comm, uint16_t w = 240, uint16_t h = 240)
        : mComm(comm), mWidth(w), mHeight(h)
    {
    }

    /* ---------- 驱动层入口（通过 CRTP 委托给子类）---------- */

    /* 屏幕初始化（含寄存器配置） */
    void Init()
    {
        static_cast<Driver*>(this)->ImplInit();
    }

    /* 绘制单个像素点 */
    void DrawPoint(uint16_t x, uint16_t y, uint32_t c888)
    {
        static_cast<Driver*>(this)->ImplDrawPoint(x, y, c888);
    }

    /* ---------- 属性设置 ---------- */

    /* 设置画笔颜色（RGB888 格式） */
    constexpr void SetColor(uint32_t c) noexcept
    {
        mColor = c;
    }

    /* 设置背景颜色（RGB888 格式） */
    constexpr void SetBackColor(uint32_t c) noexcept
    {
        mBackColor = c;
    }

    /* 设置数字填充模式 */
    constexpr void SetNumFillMode(NumFillMode m) noexcept
    {
        mNumFillMode = m;
    }

    /* 设置屏幕显示方向（CRTP 委托给子类） */
    void SetDirection(Direction dir) noexcept
    {
        mDisplayDir = dir;
        static_cast<Driver*>(this)->ImplSetDirection(dir);
    }

        /* 背光控制（CRTP 委托给子类实现） */
    void BacklightOn()  { static_cast<Driver*>(this)->ImplBacklightOn(); }
    void BacklightOff() { static_cast<Driver*>(this)->ImplBacklightOff(); }

/* ---------- 属性读取 ---------- */

    constexpr uint32_t Color()     const noexcept { return mColor; }
    constexpr uint32_t BackColor() const noexcept { return mBackColor; }
    constexpr uint16_t Width()     const noexcept { return mWidth; }
    constexpr uint16_t Height()    const noexcept { return mHeight; }

    /* ---------- 字体设置 ---------- */

    /* 设置 ASCII 字体 */
    constexpr void SetAsciiFont(const Font *f) noexcept
    {
        mAsciiFont = f;
    }

    /* 设置中文字体（同时包含 ASCII） */
    constexpr void SetTextFont(const Font *f) noexcept
    {
        mChFont = f;
    }

    /* ---------- 清屏 ---------- */

    /* 全屏填充背景色 */
    void Clear()
    {
        fillRectImpl(0, 0, mWidth, mHeight, mBackColor);
    }

    /* 局部矩形区域清屏 */
    void ClearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
    {
        fillRectImpl(x, y, w, h, BackColor);
    }

    /* ---------- ASCII 字符绘制 ---------- */

    /* 显示单个 ASCII 字符 */
    void DrawChar(uint16_t x, uint16_t y, uint8_t c)
    {
        if (!mAsciiFont || c < ' ' || c > '~')
        {
            return;
        }
        auto &f = *mAsciiFont;
        /* 根据 ASCII 码计算在字模表中的偏移 */
        drawGlyphFromFont(x, y, c - ' ', f);
    }

    /* 显示 ASCII 字符串（支持 \n 换行） */
    void DrawString(uint16_t x, uint16_t y, const char *s)
    {
        if (!s || !mAsciiFont)
        {
            return;
        }
        uint16_t cx = x;
        for (; *s; ++s)
        {
            if (*s == '\n')
            {
                cx = x;
                y += mAsciiFont->height;
            }
            else
            {
                DrawChar(cx, y, static_cast<uint8_t>(*s));
                cx += mAsciiFont->width;
            }
        }
    }

    /* ---------- 中文绘制 ---------- */

    /* 显示单个汉字（GB2312 双字节编码） */
    void DrawChinese(uint16_t x, uint16_t y, const char *text)
    {
        if (!text || !mChFont)
        {
            return;
        }
        const auto &f = *mChFont;
        uint8_t hi = static_cast<uint8_t>(text[0]);
        uint8_t lo = static_cast<uint8_t>(text[1]);
        /* 遍历字库二维表，匹配汉字索引 */
        for (uint16_t i = 0; i < f.tableRows; ++i)
        {
            uint16_t base = i * (f.sizes + 2);
            if (f.table[base + f.sizes] == hi && f.table[base + f.sizes + 1] == lo)
            {
                drawGlyphFromFont(x, y, i, f);
                return;
            }
        }
    }

    /* 显示中英文混排字符串 */
    void DrawText(uint16_t x, uint16_t y, const char *t)
    {
        if (!t || !mChFont)
        {
            return;
        }
        uint16_t cx = x;
        while (*t)
        {
            uint8_t c = static_cast<uint8_t>(*t);
            if (c >= 0xA1 && mChFont)
            {
                /* 高字节 >= 0xA1 判定为中文 */
                DrawChinese(cx, y, t);
                t += 2;
                cx += mChFont->width;
            }
            else if (c >= ' ' && c <= '~' && mAsciiFont)
            {
                DrawChar(cx, y, c);
                ++t;
                cx += mAsciiFont->width;
            }
            else
            {
                ++t;
            }
        }
    }

    /* ---------- 数字绘制 ---------- */

    /* 显示整数（支持负数、指定宽度、零填充或空格填充） */
    void DrawNumber(uint16_t x, uint16_t y, int32_t num, uint8_t len)
    {
        if (!mAsciiFont)
        {
            return;
        }
        char buf[16] = {};
        if (mNumFillMode == NumFillMode::FillZero)
        {
            /* 右对齐，零填充 */
            bool neg = num < 0;
            auto absVal = static_cast<uint32_t>(neg ? -num : num);
            int idx = len - 1;
            do
            {
                buf[idx--] = '0' + (absVal % 10);
                absVal /= 10;
            }
            while (idx >= 0 && absVal > 0);
            while (idx >= 0)
            {
                buf[idx--] = '0';
            }
            if (neg && len > 0)
            {
                buf[0] = '-';
            }
        }
        else
        {
            /* 左对齐，空格填充 */
            snprintf(buf, sizeof(buf), "%*d", len, static_cast<int>(num));
        }
        DrawString(x, y, buf);
    }

    /* 显示小数（指定总宽度和小数位数） */
    void DrawDecimal(uint16_t x, uint16_t y, double v, uint8_t len, uint8_t decs)
    {
        if (!mAsciiFont)
        {
            return;
        }
        char buf[32] = {};
        snprintf(buf, sizeof(buf), "%*.*f", len, decs, v);
        DrawString(x, y, buf);
    }

    /* ---------- 2D 基本图元 ---------- */

    /* 水平线（快速填充一行） */
    void DrawLineH(uint16_t x, uint16_t y, uint16_t w)
    {
        fillRectImpl(x, y, w, 1, mColor);
    }

    /* 垂直线（快速填充一列） */
    void DrawLineV(uint16_t x, uint16_t y, uint16_t h)
    {
        fillRectImpl(x, y, 1, h, mColor);
    }

    /* 任意两点间画线（Bresenham 算法） */
    void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
    {
        int dx = static_cast<int>(x1) - static_cast<int>(x0);
        int dy = static_cast<int>(y1) - static_cast<int>(y0);
        int adx = dx > 0 ? dx : -dx;
        int ady = dy > 0 ? dy : -dy;

        if (adx > ady)
        {
            /* 斜率 |k| <= 1，沿 X 轴步进 */
            if (x0 > x1)
            {
                std::swap(x0, x1);
                std::swap(y0, y1);
                dx = -dx;
                dy = -dy;
            }
            int err = adx >> 1;
            int yi = (y1 > y0) ? 1 : -1;
            uint16_t y = y0;
            for (uint16_t x = x0; x <= x1; ++x)
            {
                DrawPoint(x, y, mColor);
                err -= ady;
                if (err < 0)
                {
                    y += yi;
                    err += adx;
                }
            }
        }
        else
        {
            /* 斜率 |k| > 1，沿 Y 轴步进 */
            if (y0 > y1)
            {
                std::swap(x0, x1);
                std::swap(y0, y1);
            }
            int err = ady >> 1;
            int xi = (x1 > x0) ? 1 : -1;
            uint16_t x = x0;
            for (uint16_t y = y0; y <= y1; ++y)
            {
                DrawPoint(x, y, mColor);
                err -= adx;
                if (err < 0)
                {
                    x += xi;
                    err += ady;
                }
            }
        }
    }

    /* 空心矩形 */
    void DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
    {
        DrawLineH(x, y, w);
        DrawLineH(x, y + h - 1, w);
        DrawLineV(x, y, h);
        DrawLineV(x + w - 1, y, h);
    }

    /* 空心圆（中点画圆算法） */
    void DrawCircle(uint16_t x, uint16_t y, uint16_t r)
    {
        int cx = 0;
        int cy = static_cast<int>(r);
        int D = 3 - 2 * static_cast<int>(r);
        while (cy >= cx)
        {
            /* 利用八对称性一次画八个点 */
            DrawPoint(x + cx, y + cy, mColor);
            DrawPoint(x - cx, y + cy, mColor);
            DrawPoint(x + cx, y - cy, mColor);
            DrawPoint(x - cx, y - cy, mColor);
            DrawPoint(x + cy, y + cx, mColor);
            DrawPoint(x - cy, y + cx, mColor);
            DrawPoint(x + cy, y - cx, mColor);
            DrawPoint(x - cy, y - cx, mColor);
            if (D < 0)
            {
                D += (cx << 2) + 6;
            }
            else
            {
                D += ((cx - cy) << 2) + 10;
                --cy;
            }
            ++cx;
        }
    }

    /* 空心椭圆 */
    void DrawEllipse(int x, int y, int r1, int r2)
    {
        int cx = 0, cy = r2;
        int r1sq = r1 * r1, r2sq = r2 * r2;
        int d = r2sq + r1sq / 4 - r1sq * r2;

        /* 区域 1：X 方向增长快 */
        while (r2sq * cx <= r1sq * cy)
        {
            DrawPoint(x + cx, y + cy, mColor);
            DrawPoint(x - cx, y + cy, mColor);
            DrawPoint(x + cx, y - cy, mColor);
            DrawPoint(x - cx, y - cy, mColor);
            if (d < 0)
            {
                d += r2sq * (2 * cx + 3);
            }
            else
            {
                d += r2sq * (2 * cx + 3) + r1sq * (-2 * cy + 2);
                --cy;
            }
            ++cx;
        }

        /* 区域 2：Y 方向递减 */
        d = static_cast<int>(r2sq * (cx + 0.5) * (cx + 0.5) + r1sq * (cy - 1) * (cy - 1) - r1sq * r2sq);
        while (cy >= 0)
        {
            DrawPoint(x + cx, y + cy, mColor);
            DrawPoint(x - cx, y + cy, mColor);
            DrawPoint(x + cx, y - cy, mColor);
            DrawPoint(x - cx, y - cy, mColor);
            if (d > 0)
            {
                d += r1sq * (-2 * cy + 3);
            }
            else
            {
                d += r2sq * (2 * cx + 2) + r1sq * (-2 * cy + 3);
                ++cx;
            }
            --cy;
        }
    }

    /* ---------- 区域填充 ---------- */

    /* 实心矩形 */
    void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
    {
        fillRectImpl(x, y, w, h, mColor);
    }

    /* 实心圆（水平线填充法） */
    void FillCircle(uint16_t x, uint16_t y, uint16_t r)
    {
        int cx = 0, cy = static_cast<int>(r);
        int D = 3 - 2 * static_cast<int>(r);
        while (cy >= cx)
        {
            /* 用四条水平线填充圆的带状区域 */
            DrawLineH(x - cy, y - cx, 2 * cy);
            DrawLineH(x - cy, y + cx, 2 * cy);
            DrawLineH(x - cx, y - cy, 2 * cx);
            DrawLineH(x - cx, y + cy, 2 * cx);
            if (D < 0)
            {
                D += (cx << 2) + 6;
            }
            else
            {
                D += ((cx - cy) << 2) + 10;
                --cy;
            }
            ++cx;
        }
    }

    /* ---------- 单色图片绘制 ---------- */

    void DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *p)
    {
        if (!p)
        {
            return;
        }
        /* 计算缓冲区一次能容纳多少行图片数据 */
        uint16_t bh = (Transport::kBufSize / 2) / w;
        if (bh == 0)
        {
            bh = 1;
        }
        uint16_t ya = y, bc = 0;
        uint16_t row = 0;
        for (; row < h; ++row)
        {
            /* 逐字节解析位图，bit=1 用画笔色，bit=0 用背景色 */
            for (uint16_t col = 0; col < (w + 7) / 8; ++col)
            {
                uint8_t byte = p[row * ((w + 7) / 8) + col];
                for (uint8_t bit = 0; bit < 8; ++bit)
                {
                    if (col * 8 + bit >= w)
                    {
                        break;
                    }
                    mComm.mBuf.data[bc++] = (byte & (1u << bit)) ? c888To565(mColor) : c888To565(mBackColor);
                }
            }
            /* 缓冲区满或到最后一行：批量写入显存 */
            if (bc >= bh * w)
            {
                SetAddr(x, ya, x + w - 1, ya + bh - 1);
                mComm.WriteBulk(mComm.mBuf.data, w * bh);
                ya += bh;
                bc = 0;
            }
        }
        /* 剩余不足一行 */
        if (bc > 0)
        {
            SetAddr(x, ya, x + w - 1, row + y);
            mComm.WriteBulk(mComm.mBuf.data, bc);
        }
    }

protected:
    Transport              &mComm;  /* 通信传输层引用 */
    uint16_t  mWidth;  /* 屏幕像素宽度 */
    uint16_t  mHeight;  /* 屏幕像素高度 */
    uint32_t  mColor      = Colors::White;  /* 当前画笔颜色 */
    uint32_t  mBackColor  = Colors::Black;  /* 当前背景颜色 */
    Direction mDisplayDir = Direction::Vertical;  /* 当前显示方向 */
    NumFillMode mNumFillMode = NumFillMode::FillZero;  /* 数字填充模式 */
    uint8_t   mXOffset    = 0;  /* X 坐标偏移（设显示方向时使用） */
    uint8_t   mYOffset    = 0;  /* Y 坐标偏移（设显示方向时使用） */
    const Font *mAsciiFont = nullptr;  /* 当前 ASCII 字体 */
    const Font *mChFont    = nullptr;  /* 当前中文字体 */

    /* ============================================================ */
    /* 受保护的辅助方法（子类可调用） */
    /* ============================================================ */

    /* 编译期 RGB888 → RGB565 */
    static constexpr uint16_t c888To565(uint32_t c) noexcept
    {
        return rgb888To565(c);
    }

    /* 设置 GRAM 地址窗口（CRTP 委托给子类） */
    void SetAddr(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) noexcept
    {
        xs += mXOffset;
        ys += mYOffset;
        xe += mXOffset;
        ye += mYOffset;
        static_cast<Driver*>(this)->ImplSetAddr(xs, ys, xe, ye);
    }

    /* 通用字形渲染引擎（ASCII 和中文共用） */
    /* charIdx: ASCII 为 (c-' ')，中文为字库表索引 */
    void drawGlyphFromFont(uint16_t x, uint16_t y, uint16_t charIdx, const Font &f)
    {
        uint16_t bh = (Transport::kBufSize / 2) / f.width;
        if (bh == 0)
        {
            bh = 1;
        }
        uint16_t ya = y, bc = 0;
        for (uint16_t row = 0; row < f.height; ++row)
        {
            for (uint16_t col = 0; col < f.width; ++col)
            {
                /* 中文字库: charIdx * (sizes+2) 跳转到对应字模；ASCII: charIdx = (c-' ')*sizes */
                uint16_t base = (f.tableRows > 0) ? charIdx * (f.sizes + 2) : charIdx * f.sizes;  /* C++11: conditional */
                uint16_t byteIdx = base + row * ((f.width + 7) / 8) + (col / 8);
                uint8_t  bitPos  = 7 - (col % 8);
                /* bit=1 用画笔色，bit=0 用背景色 */
                mComm.mBuf.data[bc++] = (f.table[byteIdx] & (1u << bitPos))
                                    ? c888To565(mColor)
                                    : c888To565(mBackColor);
            }
            /* 缓冲区满：批量写入显存 */
            if (bc >= bh * f.width)
            {
                SetAddr(x, ya, x + f.width - 1, ya + bh - 1);
                mComm.WriteBulk(mComm.mBuf.data, f.width * bh);
                ya += bh;
                bc = 0;
            }
        }
        /* 剩余不足缓冲区一整行 */
        if (bc > 0)
        {
            SetAddr(x, ya, x + f.width - 1, y + f.height - 1);
            mComm.WriteBulk(mComm.mBuf.data, bc);
        }
    }

    /* 矩形区域填充（被 clear/clearRect/fillRect/drawLineH/drawLineV 共用） */
    void fillRectImpl(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color888)
    {
        if (x + w > mWidth || y + h > mHeight)
        {
            return;
        }
        SetAddr(x, y, x + w - 1, y + h - 1);
        uint16_t c = c888To565(color888);
        uint16_t total = w * h;
        for (uint16_t i = 0; i < total; ++i)
        {
            mComm.mBuf.data[i % Transport::kBufSize] = c;
            if ((i + 1) % Transport::kBufSize == 0 || i == total - 1)
            {
                uint16_t chunk = ((i + 1) % Transport::kBufSize == 0) ? Transport::kBufSize : ((i % Transport::kBufSize) + 1);
                mComm.WriteBulk(mComm.mBuf.data, chunk);
            }
        }
    }
};

}  /* namespace CTLIB */

#endif

